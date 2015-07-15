// Implements then data logger component of the clock metrology project.
// Created by D & D Kirkby, Dec 2013

/***********************************************
	Requires
***********************************************/

var fs = require('fs');
var async = require('async');
var serial = require('serialport');
var winston_module = require('winston');

var sprintf = require('sprintf').sprintf;
var spawn = require('child_process').spawn;

var packet = require('./packet');
var average = require('./average');
var bins = require('./bins');
var connectToDB = require('./dbConnection').connectToDB;

/***********************************************
	Globals
***********************************************/

// For logging all binary data to a file
var binaryPacketsFile = fs.createWriteStream('binaryPackets',{ flags: 'a' });
var dataBuffer = null;
var lengthBuffer = new Buffer(2);

// Tracks the last seen data packet sequence number to enable sequencing errors to be detected.
var lastSamplesSinceBoot = 0;

// Track the time of the initial PPS and then the subsequent packets and the delta
var lastTime;
var lastDeltaTime;
var runningMicroseconds;
var runningUs = 0;
var flag = false;

// Remember the initial UTC-GPS offset
var utcgpsOffset = 17;

// Remember the last "timeSinceLastBootPacket" so we can reconstruct the crude period
var lastTimeSinceLastBootPacket = 0;

// Packet Sizes : change this when you want to modify the number of samples
var RAW_LENGTH = 2200;
var RAW_START = 52;
var MAX_PACKET_SIZE = RAW_START+RAW_LENGTH;

// Keep track of the dates we are waiting on a fit to process
// FIFO : push on, then pop off
var datesBeingProcessed = [];
var lastDatesLength = 0;

/***********************************************
	Logging with Winston
***********************************************/

// Log to file
var winston = new (winston_module.Logger)({
	transports: [
		new (winston_module.transports.Console)({ level: 'warn' }),
		// Note: logging to a subfolder does not create the subfolder for
		// you.... it needs to already exist.  Future WinstonJS pull request?
		// For now make sure node/winstonLogs/ exists
		new (winston_module.transports.DailyRotateFile)({
			filename: 'winstonLogs/ticktock.log',
			level: 'verbose',
			handleExceptions: true
		})
	]
});

/***********************************************
	Command Line Arguments
***********************************************/

// Use database for templating
var templateFile = "db";

// Parses command-line arguments.
var noSerial = false;
var noDatabase = false;
var debug = false;
var debugLevel2 = false;
var runningData = false;
var pythonFlags = ["--load-template", templateFile, "--spline-pad", "0.03"];
var service = false;
process.argv.forEach(function(val,index,array) {
	if(val == '--no-serial') noSerial = true;
	else if(val == '--no-database') noDatabase = true;
	else if(val == '--debug') {
		// Set all transports to debug level
		for(var key in winston.transports) {
			winston.transports[key].level = 'debug';
		}
		winston.debug("Done");
		debug = true;
	}
	else if(val == '--running-data') runningData = true;
	else if(val == '--service') service = true;
	else if(val == '--physical')  pythonFlags = ["--physical"];
});

/***********************************************
	Configure and Launch fit.py
***********************************************/

// Assumption: this command is being called with cwd /path/to/Gears/node

// Start process with data pipes
var fit = spawn("../fit/fit.py", pythonFlags, { cwd : "../fit", stdio : 'pipe'});
fit.alive = true;
// Send all output to node stdout (readable.pipe(writable))
// fit.stdout.pipe(process.stdout);

// Thank you to https://www.exratione.com/2013/05/die-child-process-die/ for the following snippets

// Helper function added to the child process to manage shutdown.
fit.onUnexpectedExit = function (code, signal) {
	winston.error("Child process terminated with code: " + code);
	// this.disconnect();
	// process.exit(1);
};
fit.on("exit", fit.onUnexpectedExit);

// A helper function to shut down the child.
fit.shutdown = function () {
	winston.info("Stopping Fit.py");
	fit.alive = false;
	// Get rid of the exit listener since this is a planned exit.
	this.removeListener("exit", this.onUnexpectedExit);
	this.kill("SIGTERM");
};
// The exit event shuts down the child.
process.once("exit", function () {
	winston.info("Logger Stopped");
	fit.shutdown();
});
// This is a somewhat ugly approach, but it has the advantage of working
// in conjunction with most of what third parties might choose to do with
// uncaughtException listeners, while preserving whatever the exception is.
process.once("uncaughtException", function (error) {
	// If this was the last of the listeners, then shut down the child and rethrow.
	// Our assumption here is that any other code listening for an uncaught
	// exception is going to do the sensible thing and call process.exit().
	if (process.listeners("uncaughtException").length === 0) {
		fit.shutdown();
		winston.warn("Throwing error: ", error);
		throw error;
	}
});

// Handle Exit Signals
process.once('SIGTERM', function(){
	winston.info("Stopping Logger " + __filename);
	process.exit(0);
});
process.once('SIGINT', function(){
	winston.info("Stopping Logger " + __filename);
	process.exit(0);
});

/***********************************************
	Connect to 1. Serial Port 2. Database
***********************************************/

winston.verbose(__filename + ' connecting to the database...');

async.parallel({
	// Opens a serial port connection to the TickTock device.
	port: function(callback) {
		if(noSerial) return callback(null,null);
		async.waterfall([
			// Finds the tty device name of the serial port to open.
			function(portCallback) {
				winston.verbose('Looking for the tty device...');
				serial.list(function(err,ports) {
					// Looks for the first port with 'FTDI' as the manufacturer
					async.detectSeries(ports,function(port,ttyCallback) {
						winston.debug('scanning port',port);
						ttyCallback(port.manufacturer == 'FTDI' || port.pnpId.indexOf('FTDI') > -1);
					},
					// Forwards the corresponding tty device name.
					function(firstFtdiPort) {
						if(firstFtdiPort === undefined) {
							// No suitable tty found
							portCallback(new Error('No FTDI tty port found'));
						}
						else {
							portCallback(null,firstFtdiPort.comName);
						}
					}
					);
				});
			},
			// Opens the serial port.
			function(ttyName,portCallback) {
				winston.verbose('Opening device %s...',ttyName);
				var port = new serial.SerialPort(ttyName, {
					baudrate: 78125,
					buffersize: 255,
					parser: serial.parsers.raw
				});
				port.on('open',function(err) {
					if(err) return portCallback(err);
					winston.verbose('Port open');
					portCallback(null,port);
				});
			}],
			// Propagates our device info to data logger.
			function(err,port) {
				if(!err) winston.info('serial port is ready');
				callback(err,port);
			}
		);
	},
	db: connectToDB },
	// Performs steps that require both an open serial port and database connection.
	// Note that either of config.port or config.db might be null if they are disabled
	// with command-line flags.
	function(err,config) {
		if(err) throw err;
		// Record our startup time.
		// config.startupTime = new Date();
		if(config.db && config.port) {
			// Logs TickTock packets from the serial port into the database.
			winston.debug('starting data logger');
			// Initializes our binary packet assembler to initially only accept a boot packet.
			// NB: the maximum and boot packet sizes are hardcoded here!
			var assembler = new packet.Assembler(0xFE,3,MAX_PACKET_SIZE,{0x00:66},0);
			// Initializes averagers
			var averagerCollection = new average.AveragerCollection(bins.stdBinSizes());
			// Flush serial port of any old data before accepting new data
			config.port.flush();
			// Handles incoming chunks of binary data from the device.
			config.port.on('data',function(data) {
				// Log binary data to a file
				dataBuffer = new Buffer(data);
				lengthBuffer.writeUInt16LE(data.length, 0);
				binaryPacketsFile.write(Buffer.concat([lengthBuffer,dataBuffer]));
				// Send to receive()
				receive(data,assembler,averagerCollection,config.db.bootPacketModel,config.db.dataPacketModel,
					config.db.gpsStatusModel,config.db.averageDataModel,config.db.rawDataModel);
			});

			// Handles incoming data packets from pipe to fit.py
			fit.stdout.on('data', function(data){
				storeRefinedPeriodAndAngle(data, config.db.dataPacketModel, averagerCollection);
			});
		}
	}
);

/***********************************************
	Incoming Data
***********************************************/

// Receives a new chunk of binary data from the serial port and returns the
// updated value of remaining that should be used for the next call.
function receive(data,assembler,averager,bootPacketModel,dataPacketModel,gpsStatusModel,averageDataModel,rawDataModel) {
	assembler.ingest(data,function(ptype,buf) {
		var p = null;
		var computerTimestamp = null;
		if(ptype === 0x00) {
			winston.verbose("Got Boot Packet!");
			// Prepares boot packet for storing to the database.
			// GPS Time
			utcgpsOffset				= buf.readFloatBE(56);
			var weekNumber			= buf.readUInt16BE(60);
			var timeOfWeek			= buf.readFloatBE(62);

			winston.debug("Week Number: " + weekNumber);
			winston.debug("Time of Week: " + timeOfWeek);
			winston.debug("UTC Offset: " + utcgpsOffset);

			var GPS_EPOCH_IN_MS = 315964800000;			// January 6, 1980 UTC
			var MS_PER_WEEK = 7*24*60*60*1000;
			// GPS time!!! 17 leap seconds ahead of UTC
			var timestamp = new Date(GPS_EPOCH_IN_MS + weekNumber*MS_PER_WEEK + timeOfWeek*1000);	
			winston.debug("Date: " + timestamp);

			// Truncate decimal to trim miliseconds
			lastTime = new Date(GPS_EPOCH_IN_MS + weekNumber*MS_PER_WEEK + Math.floor(timeOfWeek)*1000);
			winston.debug("PPS Time: " + lastTime);

			computerTimestamp = new Date();
			var predictedPPSTime = new Date(computerTimestamp.getTime()+utcgpsOffset*1000);
			winston.debug("Predicted PPS date: " + predictedPPSTime);

			var predictionError = Math.abs(predictedPPSTime.getTime() - lastTime.getTime());
			if(isNaN(predictedPPSTime.getTime()) || isNaN(lastTime.getTime()) || predictionError > 1000){
				throw new Error("Initial GPS time is not correct!");
			}

			// NB: the boot packet layout is hardcoded here!
			hash = '';
			for(var offset = 11; offset < 31; offset++) hash += sprintf("%02x",buf.readUInt8(offset));
			p = new bootPacketModel({
				'timestamp': timestamp,
				'computerTimestamp': computerTimestamp,
				'serialNumber': sprintf("%08x",buf.readUInt32LE(0)),
				'bmpSensorOk': buf.readUInt8(4),
				'gpsSerialOk': buf.readUInt8(5),
				'sensorBlockOK': buf.readUInt8(6),
				'commitTimestamp': new Date(buf.readUInt32LE(7)*1000),
				'commitHash': hash,
				'commitStatus': buf.readUInt8(31),
				'latitude': buf.readDoubleBE(32),
				'longitude': buf.readDoubleBE(40),
				'altitude': buf.readDoubleBE(48),
				'initialWeekNumber': weekNumber,
				'initialTimeOfWeek': timeOfWeek,
				'initialUTCOffset': utcgpsOffset
			});
			winston.debug("Latitude, Longitude: " + 180/Math.PI*buf.readDoubleBE(32) + ", " + 180/Math.PI*buf.readDoubleBE(40));
			winston.debug("Altitude: " + buf.readDoubleBE(48));
			winston.debug("Sanity Check: should be around 34m?");
			winston.debug("Serial OK: " + buf.readUInt8(5));
			// After seeing a boot packet, we accept data packets.
			// NB: the data packet size is hardcoded here!
			assembler.addPacketType(0x01,MAX_PACKET_SIZE);
			// Resets the last seen sequence number.
			lastDataSequenceNumber = 0;
		}
		else if(ptype == 0x01) {
			winston.verbose("Got Data!");

			// Prepare to recieve data
			var QUADRANT = 0xFF;						// 2^8 when we're running the ADC in 8 bit mode

			// Gets the raw data from the packet.raw field
			var raw = [];
			var rawFill = 0;
			var rawPhase = buf.readUInt16LE(38);
			// winston.info(rawPhase);
			var initialReadOffset = RAW_START;
			var initialReadOffsetWithPhase = initialReadOffset+(rawPhase);

			if(initialReadOffsetWithPhase >= MAX_PACKET_SIZE || initialReadOffsetWithPhase < 0){
				winston.warn("PROBLEMS!! Phase is " + initialReadOffsetWithPhase + " and max packet size is " + MAX_PACKET_SIZE);
				// Crash and log offending packet.
				winston.error(JSON.stringify(buf));
				// throw new Error("Bad Phase (possible corrupted packet)");
				// Shouldn't send next packet to fit.py so return
				return;
			}

			// Get least and most significant parts of 64 bit ADC samples since boot counter
			var lsn = buf.readUInt32LE(16);		// fast counting part (changes every second)
			var msn = buf.readUInt32LE(20);		// slow counting part (changes every ~4 days)

			// Use JavaScript's Number to store this 64 bit integer as a double (52 bits of integer precision = plenty)
			var samplesSinceBoot = lsn + msn*Math.pow(2,32);

			// Calculates the time since the last boot packet assuming 10MHz clock with prescaler set to 64.
			// NB: ADC Frequency hardcoded here
			var timeSince = samplesSinceBoot*128.0*13.0/10000.0;	// in ms

			// Create date object
			var date = new Date(lastTime.getTime() + timeSince);

			// Date sanity check
			var referenceDate = new Date();
			computerTimestamp = referenceDate;
			referenceDate = new Date(referenceDate.getTime() + utcgpsOffset*1000);
			var deltaTime = Math.abs(referenceDate.getTime() - date.getTime());
			var SYNC_THRESHOLD = 500;

			// Date logging
			winston.debug("Delta Time: " + deltaTime);
			//winston.debug("Date: " + date);
			//winston.debug("Reference Date: " + referenceDate);
			//winston.debug("Samples Since Boot: " + samplesSinceBoot);


			// Perform test
			if(deltaTime > SYNC_THRESHOLD && Math.abs(lastDeltaTime - deltaTime) > SYNC_THRESHOLD) {
				winston.error("Running GPS time out of sync!");
				winston.info("Running GPS time out of sync!");
				throw new Error("Running GPS time out of sync!");
			}
			deltaLastTime = deltaTime;

			// Store last buffer entry
			var lastReading = buf.readUInt8(initialReadOffsetWithPhase);
			// Value to add to each reading to expand from 8 bit to 10 bit values
			var addToRawReading = QUADRANT * 3;

			// Assumption: The first data point is in the upper quadrant (between 75% and 100% light level)
			// Reconstruct first part of raw IR waveform from circular buffer
			for(var readOffsetA = initialReadOffsetWithPhase; readOffsetA < MAX_PACKET_SIZE; readOffsetA++) {
				raw[rawFill] = buf.readUInt8(readOffsetA);

				// This point minus last point (large positive means this goes down a quadrant
				// large negative means this goes up a quadrant
				if(raw[rawFill] - lastReading > 150){
					// This goes down a quadrant
					addToRawReading -= QUADRANT;

				} else if(raw[rawFill] - lastReading < -150){
					// This goes up a quadrant
					addToRawReading += QUADRANT;
				}

				lastReading = raw[rawFill];

				raw[rawFill] += addToRawReading;
				rawFill = rawFill + 1;
			}
			// Reconstruct second part of raw IR waveform from circular buffer
			for(var readOffsetB = initialReadOffset; readOffsetB < initialReadOffsetWithPhase; readOffsetB++) {
				raw[rawFill] = buf.readUInt8(readOffsetB);

				// This point minus last point (large positive means this goes down a quadrant
				// large negative means this goes up a quadrant
				if(raw[rawFill] - lastReading > 150){
					// This goes down a quadrant
					addToRawReading -= QUADRANT;

				} else if(raw[rawFill] - lastReading < -150){
					// This goes up a quadrant
					addToRawReading += QUADRANT;
				}

				lastReading = raw[rawFill];

				raw[rawFill] += addToRawReading;
				rawFill = rawFill + 1;
			}

			// use nominal 1st order fit from sensor datasheet to calculate RH in %
			var humidity			= (buf.readUInt16LE(34)/65536.0 - 0.1515)/0.00636;
			var crudePeriod			= samplesSinceBoot - lastSamplesSinceBoot;
			var sequenceNumber		= buf.readInt32LE(0);
			var boardTemperature	= buf.readInt32LE(24)/160.0;
			var pressure			= buf.readInt32LE(28);
			var irLevel				= buf.readUInt16LE(36)/65536.0*5.0;				// convert IR level to volts
			// GPS status
			var gpsStatusValues = {
				recieverMode		: buf.readUInt8(40),
				discipliningMode	: buf.readUInt8(41),
				criticalAlarms		: buf.readUInt16BE(42),
				minorAlarms			: buf.readUInt16BE(44),
				gpsDecodingStatus	: buf.readUInt8(46),
				discipliningActivity: buf.readUInt8(47),
				clockOffset			: buf.readFloatBE(48)
			};

			var expectedValues = {
				recieverMode		: 0x07,
				discipliningMode	: 0,
				criticalAlarms		: 0,
				minorAlarms			: 0,
				gpsDecodingStatus	: 0,
				discipliningActivity: 0
			};

			var unexpectedValues = {
				"timestamp" : date,
				"clockOffset" : gpsStatusValues.clockOffset
			};

			for(var value in expectedValues){
				if(gpsStatusValues[value] != expectedValues[value]){
					unexpectedValues[value] = gpsStatusValues[value];
					// winston.debug(expectedValues[value] + " != " + gpsStatusValues[value]);
				} else {
					// winston.debug(expectedValues[value] + " = " + gpsStatusValues[value]);
				}
			}

			// winston.debug(unexpectedValues);

			gpsStatusModel.create(unexpectedValues);

			//winston.debug("Clock Offset: " + gpsStatusValues.clockOffset + " ppb");

			// Calculates the thermistor resistance in ohms assuming 100uA current source.
			var rtherm = buf.readUInt16LE(32)/65536.0*5.0/100e-6;
			// Calculates the corresponding temperature in degC using a Steinhart-Hart model.
			var logr = Math.log(rtherm);
			var ttherm = 1.0/(0.000878844 + 0.000231913*logr + 7.70349e-8*logr*logr*logr) - 273.15;
			// Prepares data packet for storing to the database.
			// NB: the data packet layout is hardcoded here!
			dataPacketData = {
				'timestamp': date,
				'samplesSinceBoot': samplesSinceBoot,
				'computerTimestamp': computerTimestamp,
				'sequenceNumber': sequenceNumber,
				'boardTemperature': boardTemperature,
				'pressure': pressure,
				'blockTemperature': ttherm,
				'humidity': humidity,
				'irLevel': irLevel
			};
			if(sequenceNumber == 1) {
				dataPacketData['initialCrudePeriod'] = crudePeriod;
			} else {
				dataPacketData['crudePeriod'] = crudePeriod;
			}
			p = new dataPacketModel(dataPacketData);

			// Save Raw Data in separate collection if sequence number is multiple of 100 or is next (i.e. 100 and 101, 200,201)
			rawData = {
				'expiryTimestamp'	: (((sequenceNumber%100) === 0 || (((sequenceNumber-1)%100) === 0)) ? null : date),
				'timestamp'			: date,
				'sequenceNumber'	: sequenceNumber,
				'raw'				: raw
			};
			rawDataModel.collection.insert(rawData, function onInsert(err, docs) {
				if (err) throw err;
				// Raw Saved!
			});

			// Input averageable fields to averager object
			averager.input({
				'timestamp': date,
				'crudePeriod': ((sequenceNumber == 1) ? null : crudePeriod),
				'boardTemperature': boardTemperature,
				'pressure': pressure,
				'blockTemperature': ttherm,
				'humidity': humidity,
				'clockOffset' : gpsStatusValues.clockOffset
			}, function (data){
				// To be called when the average reaches its period
				averageDataModel.create(data,function(err,data){
					if(err) throw err;
					// Saved!
				});
			});

			//winston.info(raw);
			// Checks for a packet sequence error.
			if(p.sequenceNumber != lastDataSequenceNumber+1) {
				winston.warn('Got packet #%d when expecting packet #%d',
					p.sequenceNumber,lastDataSequenceNumber+1);
				winston.warn(p);
				// Crash and log offending packet.
				winston.error(JSON.stringify(buf));
				// throw new Error("Packets coming in out of order!");
			}
			lastDataSequenceNumber = p.sequenceNumber;
			lastSamplesSinceBoot = samplesSinceBoot;
			// Never save the first packet since there is usually a startup glitch
			// where the serial port sees a second boot packet arriving during the first
			// data packet that still needs to be debugged...
			// EDIT 8/22/14: I think this issue has been resolved?
			if(lastDataSequenceNumber == 1) {
				// winston.debug("Time: " + timeSince);
			} else{
				// Write to first entry in file
				if(runningData) fs.appendFileSync('runningData.dat', samplesSinceBoot + '\n');
				// Push most recent date to the top of the FIFO stack
				datesBeingProcessed.push(date);
				// Write crude period to pipe
				if(fit.alive) fit.stdin.write(samplesSinceBoot + '\n');

				// Iterate through samples writing them to the fit pipe
				for(var i = 0; i < RAW_LENGTH; i++){
					if(fit.alive) fit.stdin.write(raw[i] + '\n');
					if(runningData) fs.appendFileSync('runningData.dat', raw[i] + '\n');
				}
			}
		}
		else {
			winston.warn('Got unexpected packet type',ptype);
			return;
		}

		p.save(function(err,p) {
			if(err) winston.warn('Error saving data packet',p);
		});
	});
}

/***********************************************
	Process fit.py output
***********************************************/

// Write refined period and swing arc angle to database
// Format : period angle
function storeRefinedPeriodAndAngle(periodAngleHeight, dataPacketModel, averager) {
	// Catch Exception
	if(periodAngleHeight.toString().substring(0,10) == "Exception:")
	{
		winston.error("Fit Exception!");
		winston.error(periodAngleHeight.toString());
		return;
	}

	// Pop least recent date off FIFO stack
	var storeDate = datesBeingProcessed.pop();
	if(datesBeingProcessed.length !== 0 && lastDatesLength !== datesBeingProcessed.length){
		// Only log length on a change
		winston.warn("Length :" + datesBeingProcessed.length);
		// Clobber old file
		var datesBeingProcessedFile = fs.createWriteStream('datesBeingProcessed',{ flags: 'w' });
		datesBeingProcessed.forEach(function(element){
			// Write actual dates to a file
			datesBeingProcessedFile.write(element + '\n');
		});
	}

	lastDatesLength = datesBeingProcessed.length;
	var badPeriod,badAngle,badHeight = false;
	var data = {};
	var period = Number(periodAngleHeight.toString().split(" ")[0].toString());
	var angle = Number(periodAngleHeight.toString().split(" ")[1].toString());
	var height = Number(periodAngleHeight.toString().split(" ")[2].toString());
	if(period <= 0 || period >= 2.5 || isNaN(period)){
		winston.warn("Bad Period: " + period);
		badPeriod = true;
	} else {
		winston.verbose("Period: " + period);
		data["refinedPeriod"] = period;
	}
	if(angle <= 0 || isNaN(angle)){
		winston.warn("Bad Angle: " + angle);
		badAngle = true;
	} else {
		winston.verbose("Angle: " + angle);
		data["angle"] = angle;
	}
	if(height <= 0 || isNaN(height)){
		winston.warn("Bad Height: " + height);
		badHeight = true;
	} else {
		winston.verbose("Height: " + height);
		data["height"] = height;
	}
	// if(badPeriod && badAngle) return;
	// winston.info(storeDate);
	// winston.info(dataPacketModel);

	var conditions	= { timestamp : storeDate };
	var update		= { $set : data};
	var options		= { multi : false };

	dataPacketModel.update(conditions, update, options, function(err, numberAffected){
		if(err) throw err;
		// winston.info("Update of " + numberAffected + " Documents Successful!")
	});

	// Extend object
	var averageData = {
		'timestamp': storeDate
	};
	for (var attrname in data) { averageData[attrname] = data[attrname]; }
	// Note: because averager does not wait on these fields to save to the database, the average values will be OFFSET!
	averager.input(averageData);
}
