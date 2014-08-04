// Implements then data logger component of the clock metrology project.
// Created by D & D Kirkby, Dec 2013

var fs = require('fs');
var async = require('async');
var serial = require('serialport');
var mongoose = require('mongoose');
var sprintf = require('sprintf').sprintf;
var spawn = require('child_process').spawn;

var packet = require('./packet');

// Tracks the last seen data packet sequence number to enable sequencing errors to be detected.
var lastDataSequenceNumber = 0;

// Maximum packet size : change this when you want to modify the number of samples
var MAX_PACKET_SIZE = 2082;

// Keep track of the dates we are waiting on a fit to process
// FIFO : unshift on, then pop off
var datesBeingProcessed = [];

// Parses command-line arguments.
var noSerial = false;
var noDatabase = false;
var debug = false;
var pythonFlags = ["--load-template", "template2048.dat"];
process.argv.forEach(function(val,index,array) {
	if(val == '--no-serial') noSerial = true;
	else if(val == '--no-database') noDatabase = true;
	else if(val == '--debug') debug = true;
	else if(val == '--physical')  pythonFlags = ["--physical"];
});

// Log to file
var access = fs.createWriteStream('node.access.log', { flags: 'a' });
var error = fs.createWriteStream('node.error.log', { flags: 'a' });

// redirect stdout and stderr
process.stdout.pipe(access);
process.stderr.pipe(error);

// Start process with data pipes
var fit = spawn('../fit/fit.py', pythonFlags, { cwd : "../fit", stdio : 'pipe'});
// Send all output to node stdout (readable.pipe(writable))
// fit.stdout.pipe(process.stdout);

// Make sure to kill the fit process when node is about to exit
process.on('exit', function(){
	console.log("Stopping Python");
	fit.kill();
});

async.parallel({
	// Opens a serial port connection to the TickTock device.
	port: function(callback) {
		if(noSerial) return callback(null,null);
		async.waterfall([
			// Finds the tty device name of the serial port to open.
			function(portCallback) {
				console.log('Looking for the tty device...');
				serial.list(function(err,ports) {
					// Looks for the first port with 'FTDI' as the manufacturer
					async.detectSeries(ports,function(port,ttyCallback) {
						console.log('scanning port',port);
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
				console.log('Opening device %s...',ttyName);
				var port = new serial.SerialPort(ttyName, {
					baudrate: 57600,
					buffersize: 255,
					parser: serial.parsers.raw
				});
				port.on('open',function(err) {
					if(err) return portCallback(err);
					console.log('Port open');
					portCallback(null,port);
				});
			}],
			// Propagates our device info to data logger.
			function(err,port) {
				if(!err) console.log('serial port is ready');
				callback(err,port);
			}
		);
	},
	// Connects to the database where packets from TickTock are logged.
	db: function(callback) {
		if(noDatabase) return callback(null,null);
		console.log(__filename + ' connecting to the database...2');
		mongoose.connect('mongodb://localhost:27017/ticktockDemoTest3');
		var db = mongoose.connection;
		db.on('error', console.error.bind(console, 'db connection error:'));
		db.once('open', function() {
			console.log('db connection established.');
			// Defines the schema and model for our serial boot packets
			var bootPacketSchema = mongoose.Schema({
				timestamp: { type: Date, index: true },
				serialNumber: String,
				bmpSensorOk: Boolean,
				gpsSerialOk: Boolean,
				sensorBlockOK: Boolean,
				commitTimestamp: Date,
				commitHash: String,
				commitStatus: Number
			});
			var bootPacketModel = mongoose.model('bootPacketModel',bootPacketSchema);
			// Defines the schema and model for our serial data packets
			var dataPacketSchema = mongoose.Schema({
				timestamp: { type: Date, index: true },
				crudePeriod: Number,
				refinedPeriod: Number,
				angle: Number,
				sequenceNumber: Number,
				temperature: Number,
				pressure: Number,
				thermistor: Number,
				humidity: Number,
				irLevel: Number,
				raw: Array
			});
			var dataPacketModel = mongoose.model('dataPacketModel',dataPacketSchema);
			// Defines the schema and model for Minute averages
			var minuteAverageSchema = mongoose.Schema({
				timestamp: { type: Date, index: true },
				average: Number
			});
			var minuteAverageModel = mongoose.model('minuteAverageModel',minuteAverageSchema);
			// Defines the schema and model for Hour averages
			var hourAverageSchema = mongoose.Schema({
				timestamp: { type: Date, index: true },
				average: Number
			});
			var hourAverageModel = mongoose.model('hourAverageModel',hourAverageSchema);

			var runningTotals = {};
			runningTotals.lastMinute = -1;
			runningTotals.minutesRunningTotal = 0;
			runningTotals.samplesPerMinute = 0;
			runningTotals.lastHour = -1;
			runningTotals.hoursRunningTotal = 0;
			runningTotals.samplesPerHour = 0;

			// Propagates our database connection and db models to data logger.
			callback(null,{
				'bootPacketModel':bootPacketModel,
				'dataPacketModel':dataPacketModel,
				'minuteAverageModel':minuteAverageModel,
				'hourAverageModel':hourAverageModel,
				'runningTotals':runningTotals
			});
		});
	}},
	// Performs steps that require both an open serial port and database connection.
	// Note that either of config.port or config.db might be null if they are disabled
	// with command-line flags.
	function(err,config) {
		if(err) throw err;
		// Record our startup time.
		config.startupTime = new Date();
		if(config.db && config.port) {
			// Logs TickTock packets from the serial port into the database.
			if(debug) console.log('starting data logger with',config);
			// Initializes our binary packet assembler to initially only accept a boot packet.
			// NB: the maximum and boot packet sizes are hardcoded here!
			var assembler = new packet.Assembler(0xFE,3,MAX_PACKET_SIZE,{0x00:32},0);
			// Handles incoming chunks of binary data from the device.
			config.port.on('data',function(data) {
				receive(data,assembler,config.db.bootPacketModel,config.db.dataPacketModel,
					config.db.minuteAverageModel,config.db.hourAverageModel,config.db.runningTotals);
			});

			fit.stdout.on('data', function(data){
				storeRefinedPeriodAndAngle(data, config.db.dataPacketModel);
			});
		}
	}
);

// Receives a new chunk of binary data from the serial port and returns the
// updated value of remaining that should be used for the next call.
function receive(data,assembler,bootPacketModel,dataPacketModel,minuteAverageModel,hourAverageModel,runningTotals) {
	assembler.ingest(data,function(ptype,buf) {
		var saveMe = true;
		if(ptype === 0x00) {
			if(debug) console.log("Got Boot Packet!");
			// Prepares boot packet for storing to the database.
			// NB: the boot packet layout is hardcoded here!
			hash = '';
			for(var offset = 11; offset < 31; offset++) hash += sprintf("%02x",buf.readUInt8(offset));
			p = new bootPacketModel({
				'timestamp': new Date(),
				'serialNumber': sprintf("%08x",buf.readUInt32LE(0)),
				'bmpSensorOk': buf.readUInt8(4),
				'gpsSerialOk': buf.readUInt8(5),
				'sensorBlockOK': buf.readUInt8(6),
				'commitTimestamp': new Date(buf.readUInt32LE(7)*1000),
				'commitHash': hash,
				'commitStatus': buf.readUInt8(31)
			});
			// After seeing a boot packet, we accept data packets.
			// NB: the data packet size is hardcoded here!
			assembler.addPacketType(0x01,MAX_PACKET_SIZE);
			// Resets the last seen sequence number.
			lastDataSequenceNumber = 0;
		}
		else if(ptype == 0x01) {
			if(debug) console.log("Got Data!");

			// Prepare to recieve data
			var date = new Date();

			// New minute, new average
			if(date.getMinutes() != runningTotals.lastMinute){
				// Store with date object on the most recent side of the window
				var b = runningTotals.minutesRunningTotal/runningTotals.samplesPerMinute;
				console.log(b);
				// store(runningTotals.minutesRunningTotal/runningTotals.samplesPerMinute, date);
				runningTotals.minutesRunningTotal = 0;
				runningTotals.samplesPerMinute = 0;
			}

			runningTotals.lastMinute = date.getMinutes();


			// New hour, new average
			if(date.getHours() != runningTotals.lastHour){
				// Store with date object on the most recent side of the window
				var c = runningTotals.hoursRunningTotal/runningTotals.samplesPerHour;
				console.log(c);
				// store(runningTotals.hoursRunningTotal/runningTotals.samplesPerHour, date);
				runningTotals.hoursRunningTotal = 0;
				runningTotals.samplesPerHour = 0;
			}
	
			runningTotals.lastHour = date.getHours();


			// 2^8 when we're running the ADC in 8 bit mode
			var QUADRANT = 0xFF;

			// Gets the raw data from the packet.raw field
			var raw = [];
			var rawFill = 0;
			var rawPhase = buf.readUInt16LE(32);
			// console.log(rawPhase);
			var initialReadOffset = 34;
			var initialReadOffsetWithPhase = initialReadOffset+(rawPhase);

			if(initialReadOffsetWithPhase >= MAX_PACKET_SIZE || initialReadOffsetWithPhase < 0){
				console.log("PROBLEMS!! Phase is " + initialReadOffsetWithPhase);
				// Probably shouldn't send next packet to fit.py
				return;
			}

			// Calculates the time since the last reading assuming 10MHz clock with prescaler set to 128.
			var samplesSince = buf.readUInt16LE(16);
			var timeSince = samplesSince*128*13/10000000;

			// Store last buffer entry
			var lastReading = buf.readUInt8(initialReadOffsetWithPhase);
			// Value to add to each reading to expand from 8 bit to 10 bit values
			var addToRawReading = QUADRANT * 3;

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

			// Calculates the thermistor resistance in ohms assuming 100uA current source.
			var rtherm = buf.readUInt16LE(26)/65536.0*5.0/100e-6;
			// Calculates the corresponding temperature in degC using a Steinhart-Hart model.
			var logr = Math.log(rtherm);
			var ttherm = 1.0/(0.000878844 + 0.000231913*logr + 7.70349e-8*logr*logr*logr) - 273.15;
			var temperature = buf.readInt32LE(18)/160.0;
			var pressure = buf.readInt32LE(22);
			var humidity = (buf.readUInt16LE(28)/65536.0 - 0.1515)/0.00636;
			// Prepares data packet for storing to the database.
			// NB: the data packet layout is hardcoded here!
			p = new dataPacketModel({
				'timestamp': date,
				'crudePeriod': buf.readUInt16LE(16),
				'refinedPeriod': null,
				'angle': null,
				'sequenceNumber': buf.readInt32LE(0),
				'temperature': temperature,
				'pressure': pressure,
				'thermistor': ttherm,
				// use nominal 1st order fit from sensor datasheet to calculate RH in %
				'humidity': humidity,
				// convert IR level to volts
				'irLevel': buf.readUInt16LE(30)/65536.0*5.0,
				'raw': raw
			});
			//console.log(raw);
			// Checks for a packet sequence error.
			if(p.sequenceNumber != lastDataSequenceNumber+1) {
				console.log('Got packet #%d when expecting packet #%d',
					p.sequenceNumber,lastDataSequenceNumber+1);
			}
			lastDataSequenceNumber = p.sequenceNumber;
			// Never save the first packet since there is usually a startup glitch
			// where the serial port sees a second boot packet arriving during the first
			// data packet that still needs to be debugged...
			if(lastDataSequenceNumber == 1) {
				saveMe = false;
			} else{
				saveMe = true;
				// Write to first entry in file
				fs.appendFileSync('runningData.dat', samplesSince + '\n');
				// Push most recent date to the top of the FIFO stack
				datesBeingProcessed.unshift(date);
				// Write crude period to pipe
				fit.stdin.write(samplesSince + '\n');

				// Iterate through samples writing them to the fit pipe
				for(var i = 0; i < 2048; i++){
					fit.stdin.write(raw[i] + '\n');
					fs.appendFileSync('runningData.dat', raw[i] + '\n');
				}
			}
		}
		else {
			console.log('Got unexpected packet type',ptype);
			return;
		}
		if(saveMe) {
			p.save(function(err,p) {
				if(err) console.log('Error saving data packet',p);
			});
		}
		else {
			console.log('Packet not saved to db.');
		}
	});
}

// Write refined period and swing arc angle to database
// Format : period angle
function storeRefinedPeriodAndAngle(periodAndAngle, dataPacketModel) {
	// Pop least recent date off FIFO stack
	var storeDate = datesBeingProcessed.pop();

	var period = Number(periodAndAngle.toString().split(" ")[0].toString());
	var angle = Number(periodAndAngle.toString()
		.split(" ")[1].toString());
	if(period <= 0 || isNaN(period)){
		console.log("Bad Period : " + period);
		return;
	} else if(angle <= 0 || isNaN(angle)){
		console.log("Bad Angle : " + angle);
		return;
	}
	if(debug) console.log(period);
	// console.log(storeDate);
	// console.log(dataPacketModel);

	var conditions	= { timestamp : storeDate };
	var update		= { $set : { refinedPeriod : period , angle : angle}};
	var options		= { multi : false };

	dataPacketModel.update(conditions, update, options, function(err, numberAffected){
		if(err) throw err;
		// console.log("Update of " + numberAffected + " Documents Successful!")
	});
}
