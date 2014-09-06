// Implements then data logger component of the clock metrology project.
// Created by D & D Kirkby, Dec 2013

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

// Tracks the last seen data packet sequence number to enable sequencing errors to be detected.
var lastDataSequenceNumber = 0;

// Track the time of the initial PPS and then the subsequent packets
var lastTime;

// Maximum packet size : change this when you want to modify the number of samples
var MAX_PACKET_SIZE = 2094;

// Keep track of the dates we are waiting on a fit to process
// FIFO : unshift on, then pop off
var datesBeingProcessed = [];

// Log to file
var winston = new (winston_module.Logger)({
	transports: [
		new (winston_module.transports.Console)({ level: 'warn' }),
		new (winston_module.transports.File)({ filename: 'ticktock.log', level: 'verbose' })
	]
});

// Parses command-line arguments.
var noSerial = false;
var noDatabase = false;
var debug = false;
var debugLevel2 = false;
var runningData = false;
var pythonFlags = ["--load-template", "template2048.dat"];
var service = false;
process.argv.forEach(function(val,index,array) {
	if(val == '--no-serial') noSerial = true;
	else if(val == '--no-database') noDatabase = true;
	else if(val == '--debug') {
		winston.transports.console.level = 'debug';
		winston.transports.file.level = 'debug';
		debug = true;
	}	else if(val == '--running-data') runningData = true;
	else if(val == '--service') service = true;
	else if(val == '--physical')  pythonFlags = ["--physical"];
});

// Assumption: this command is being called with cwd /path/to/Gears/node

// Start process with data pipes
var fit = spawn("../fit/fit.py", pythonFlags, { cwd : "../fit", stdio : 'pipe'});
// Send all output to node stdout (readable.pipe(writable))
// fit.stdout.pipe(process.stdout);

// Make sure to kill the fit process when node is about to exit
process.on('SIGINT', function(){
	gracefulExit();
});

function gracefulExit()
{
	winston.info("Stopping Python");
	fit.kill();
	winston.info("Stopping Logger " + __filename);
	process.exit();
}

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
					baudrate: 57600,
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
				receive(data,assembler,averagerCollection,config.db.bootPacketModel,config.db.dataPacketModel,
					config.db.gpsStatusModel,config.db.averageDataModel);
			});

			// Handles incoming data packets from pipe to fit.py
			fit.stdout.on('data', function(data){
				storeRefinedPeriodAndAngle(data, config.db.dataPacketModel, averagerCollection);
			});
		}
	}
);

// Receives a new chunk of binary data from the serial port and returns the
// updated value of remaining that should be used for the next call.
function receive(data,assembler,averager,bootPacketModel,dataPacketModel,gpsStatusModel,averageDataModel) {
	assembler.ingest(data,function(ptype,buf) {
		var saveMe = true;
		var p = null;
		if(ptype === 0x00) {
			winston.verbose("Got Boot Packet!");
			// Prepares boot packet for storing to the database.
			// GPS Time
			var utcOffset			= buf.readFloatBE(56);
			var weekNumber			= buf.readUInt16BE(60);
			var timeOfWeek			= buf.readFloatBE(62);

			winston.debug("Week Number: " + weekNumber);
			winston.debug("Time of Week: " + timeOfWeek);
			winston.debug("UTC Offset: " + utcOffset);

			var GPS_EPOCH_IN_MS = 315964800000;			// January 6, 1980 UTC
			var MS_PER_WEEK = 7*24*60*60*1000;
			var timestamp = new Date(GPS_EPOCH_IN_MS + weekNumber*MS_PER_WEEK + timeOfWeek*1000);	// GPS time!!! 16 leap seconds ahead of UTC
			winston.debug("Date: " + timestamp);

			lastTime = new Date(GPS_EPOCH_IN_MS + weekNumber*MS_PER_WEEK + Math.floor(timeOfWeek)*1000);	// Truncate decimal to trim miliseconds
			winston.debug("PPS Time: " + lastTime);

			var d = new Date();
			winston.debug("Current UTC date: " + d);

			// NB: the boot packet layout is hardcoded here!
			hash = '';
			for(var offset = 11; offset < 31; offset++) hash += sprintf("%02x",buf.readUInt8(offset));
			p = new bootPacketModel({
				'timestamp': timestamp,
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
				'initialUTCOffset': utcOffset
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
			var rawPhase = buf.readUInt16LE(32);
			// winston.info(rawPhase);
			var initialReadOffset = 46;
			var initialReadOffsetWithPhase = initialReadOffset+(rawPhase);

			if(initialReadOffsetWithPhase >= MAX_PACKET_SIZE || initialReadOffsetWithPhase < 0){
				winston.warn("PROBLEMS!! Phase is " + initialReadOffsetWithPhase);
				// Probably shouldn't send next packet to fit.py
				return;
			}

			// Calculates the time since the last reading assuming 10MHz clock with prescaler set to 128.
			var samplesSince = buf.readUInt16LE(16);
			// NB: ADC Frequency hardcoded here
			var timeSince = samplesSince*64*13/10000;	// in ms

			// Create date object
			var date = new Date(lastTime.getTime() + timeSince);
			winston.debug("Date: " + date);
			lastTime = date;

			// Store last buffer entry
			var lastReading = buf.readUInt8(initialReadOffsetWithPhase);
			// Value to add to each reading to expand from 8 bit to 10 bit values
			var addToRawReading = QUADRANT * 3;

			// Assumption: The first data point is in the upper quadrant (between 75% and 100% light level)
			// Reconstruct first part of raw IR waveform from circular buffer
			for(var readOffsetA = initialReadOffsetWithPhase; readOffsetA < MAX_PACKET_SIZE; readOffsetA++) {
				raw[rawFill] = buf.readUInt8(readOffsetA);

				// This point minus last point (large positive means this goes down a quadrant, large negative means this goes up a quadrant
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

				// This point minus last point (large positive means this goes down a quadrant, large negative means this goes up a quadrant
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
			var humidity			= (buf.readUInt16LE(28)/65536.0 - 0.1515)/0.00636;
			var crudePeriod			= buf.readUInt16LE(16);
			var sequenceNumber		= buf.readInt32LE(0);
			var boardTemperature	= buf.readInt32LE(18)/160.0;
			var pressure			= buf.readInt32LE(22);
			var irLevel				= buf.readUInt16LE(30)/65536.0*5.0;				// convert IR level to volts
			// GPS status
			var gpsStatusValues = {
				recieverMode		: buf.readUInt8(34),
				discipliningMode	: buf.readUInt8(35),
				criticalAlarms		: buf.readUInt16BE(36),
				minorAlarms			: buf.readUInt16BE(38),
				gpsDecodingStatus	: buf.readUInt8(40),
				discipliningActivity: buf.readUInt8(41),
				clockOffset			: buf.readFloatBE(42)
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

			winston.debug("Clock Offset: " + gpsStatusValues.clockOffset + " ppb");

			// Calculates the thermistor resistance in ohms assuming 100uA current source.
			var rtherm = buf.readUInt16LE(26)/65536.0*5.0/100e-6;
			// Calculates the corresponding temperature in degC using a Steinhart-Hart model.
			var logr = Math.log(rtherm);
			var ttherm = 1.0/(0.000878844 + 0.000231913*logr + 7.70349e-8*logr*logr*logr) - 273.15;
			// Prepares data packet for storing to the database.
			// NB: the data packet layout is hardcoded here!
			p = new dataPacketModel({
				'timestamp': date,
				'crudePeriod': crudePeriod,
				'refinedPeriod': null,
				'angle': null,
				'sequenceNumber': sequenceNumber,
				'boardTemperature': boardTemperature,
				'pressure': pressure,
				'blockTemperature': ttherm,
				'humidity': humidity,
				'irLevel': irLevel,
				'raw': raw
			});

			averager.input({
				'timestamp': date,
				'crudePeriod': crudePeriod,
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
			}
			lastDataSequenceNumber = p.sequenceNumber;
			// Never save the first packet since there is usually a startup glitch
			// where the serial port sees a second boot packet arriving during the first
			// data packet that still needs to be debugged...
			// EDIT 8/22/14: I think this issue has been resolved?
			if(lastDataSequenceNumber == 1) {
				// winston.debug("Time: " + timeSince);
			} else{
				saveMe = true;
				// Write to first entry in file
				if(runningData) fs.appendFileSync('runningData.dat', samplesSince + '\n');
				// Push most recent date to the top of the FIFO stack
				datesBeingProcessed.unshift(date);
				// Write crude period to pipe
				fit.stdin.write(samplesSince + '\n');

				// Iterate through samples writing them to the fit pipe
				for(var i = 0; i < 2048; i++){
					fit.stdin.write(raw[i] + '\n');
					if(runningData) fs.appendFileSync('runningData.dat', raw[i] + '\n');
				}
			}
		}
		else {
			winston.warn('Got unexpected packet type',ptype);
			return;
		}
		if(saveMe) {
			p.save(function(err,p) {
				if(err) winston.warn('Error saving data packet',p);
			});
		}
		else {
			winston.warn('Packet not saved to db.');
		}
	});
}

// Write refined period and swing arc angle to database
// Format : period angle
function storeRefinedPeriodAndAngle(periodAndAngle, dataPacketModel, averager) {
	// Pop least recent date off FIFO stack
	var storeDate = datesBeingProcessed.pop();
	winston.info("Length :" + datesBeingProcessed.length, datesBeingProcessed);

	var period = Number(periodAndAngle.toString().split(" ")[0].toString());
	var angle = Number(periodAndAngle.toString()
		.split(" ")[1].toString());
	if(period <= 0 || isNaN(period)){
		winston.warn("Bad Period : " + period);
		return;
	} else if(angle <= 0 || isNaN(angle)){
		winston.warn("Bad Angle : " + angle);
		return;
	}
	winston.verbose(period);
	// winston.info(storeDate);
	// winston.info(dataPacketModel);

	var conditions	= { timestamp : storeDate };
	var update		= { $set : { refinedPeriod : period , angle : angle}};
	var options		= { multi : false };

	dataPacketModel.update(conditions, update, options, function(err, numberAffected){
		if(err) throw err;
		// winston.info("Update of " + numberAffected + " Documents Successful!")
	});

	// Note: because averager does not wait on these fields to save to the database, the average values will be OFFSET!
	averager.input({
		'timestamp': storeDate,
		'refinedPeriod': period,
		'angle': angle,
	});
}
