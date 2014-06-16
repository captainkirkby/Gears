// Implements the combined data logger and web server component of the clock metrology project.
// Created by D & D Kirkby, Dec 2013

var fs = require('fs');
var util = require('util');
var async = require('async');
var express = require('express');
var serial = require('serialport');
var mongoose = require('mongoose');

var packet = require('./packet');

var sprintf = require('sprintf').sprintf;
var spawn = require('child_process').spawn;


// Tracks the last seen data packet sequence number to enable sequencing errors to be detected.
var lastDataSequenceNumber = 0;

// Maximum packet size : change this when you want to modify the number of samples
var MAX_PACKET_SIZE = 2082;

// Parses command-line arguments.
var noSerial = false;
var noDatabase = false;
var debug = false;
process.argv.forEach(function(val,index,array) {
	if(val == '--no-serial') noSerial = true;
	else if(val == '--no-database') noDatabase = true;
	else if(val == '--debug') debug = true;
});

// Start process with data pipes
var fit = spawn('../fit/fit.py', [], { stdout : ['pipe', 'pipe', 'pipe']});
// Send all output to node stdout (readable.pipe(writable))
// fit.stdout.pipe(process.stdout);


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
						if(firstFtdiPort == undefined) {
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
					// Flushes any pending data in the buffer.
					port.flush();
					// Forwards the open serial port.
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
		console.log('Connecting to the database...');
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
				sequenceNumber: Number,
				temperature: Number,
				pressure: Number,
				thermistor: Number,
				humidity: Number,
				irLevel: Number,
				raw: Array
			});
			var dataPacketModel = mongoose.model('dataPacketModel',dataPacketSchema);
			// Propagates our database connection and db models to data logger.
			callback(null,{
				'connection':db,
				'bootPacketModel':bootPacketModel,
				'dataPacketModel':dataPacketModel
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
			console.log('starting data logger with',config);
			// Initializes our binary packet assembler to initially only accept a boot packet.
			// NB: the maximum and boot packet sizes are hardcoded here!
			var assembler = new packet.Assembler(0xFE,3,MAX_PACKET_SIZE,{0x00:32},0);
			// Handles incoming chunks of binary data from the device.
			config.port.on('data',function(data) {
				receive(data,assembler,config.db.bootPacketModel,config.db.dataPacketModel);
			});
		}
		// Defines our webapp routes.
		var app = express();
		// Serves static files from our public subdirectory.
		app.use('/', express.static(__dirname + '/public'));
		// Serves a dynamically generated information page.
		app.get('/about', function(req, res) { return about(req,res,config); });
		if(config.db) {
			// Serves boot packet info.
			app.get('/boot', function(req,res) { return boot(req,res,config.db.bootPacketModel); });
			// Serves data dynamically via AJAX.
			app.get('/fetch', function(req,res) { return fetch(req,res,config.db.dataPacketModel); });
		}
		// Starts our webapp.
		console.log('starting web server on port 3000');
		app.listen(3000);
	}
);

// Receives a new chunk of binary data from the serial port and returns the
// updated value of remaining that should be used for the next call.
function receive(data,assembler,bootPacketModel,dataPacketModel) {
	assembler.ingest(data,function(ptype,buf) {
		var saveMe = true;
		if(ptype == 0x00) {
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

			// Calculates the time since the last reading assuming 10MHz clock with prescaler set to 128.
			var samplesSince = buf.readUInt16LE(16);
			var timeSince = samplesSince*128*13/10000000;

			// Write to first entry in file
			// fs.appendFileSync('runningData.dat', samplesSince + '\n');

			// Write crude period to pipe
			fit.stdin.write(samplesSince + '\n');		// Newline too?

			// Gets the raw data from the packet.raw field
			var raw = [];
			var rawFill = 0;
			var rawPhase = buf.readUInt16LE(32);
			// console.log(rawPhase);
			var initialReadOffset = 34;
			var initialReadOffsetWithPhase = initialReadOffset+(rawPhase*2);		// *2 beacuse raw phase is in 16 bit word offset

			for(var readOffsetA = initialReadOffsetWithPhase; readOffsetA < MAX_PACKET_SIZE; readOffsetA=readOffsetA+2) {
				raw[rawFill] = buf.readUInt16LE(readOffsetA);
				// Write data to pipe
				fit.stdin.write(raw[rawFill] + '\n');
				//fs.appendFileSync('runningData.dat', (raw[rawFill]).toString() + '\n');
				rawFill = rawFill + 1;
			}

			for(var readOffsetB = initialReadOffset; readOffsetB < initialReadOffsetWithPhase; readOffsetB=readOffsetB+2) {
				raw[rawFill] = buf.readUInt16LE(readOffsetB);
				// Write data to pipe
				fit.stdin.write(raw[rawFill] + '\n');
				// fs.appendFileSync('runningData.dat', (raw[rawFill]).toString() + '\n');
				rawFill = rawFill + 1;
			}

			// Synchronously insert refined period into database
			while(!fit.stdout.readable);
			var refinedPeriodBuffer = fit.stdout.read();
			var refinedPeriod = 0;
			if(refinedPeriodBuffer){
				refinedPeriod = refinedPeriodBuffer.readDoubleLE(0);
			}
			console.log(refinedPeriod);

			// Asynchronously insert refined period into database (need handle on dataPacketModel)
			// fit.stdout.on('readable', function(){
			// 	//Read
			// });

			// Calculates the thermistor resistance in ohms assuming 100uA current source.
			var rtherm = buf.readUInt16LE(26)/65536.0*5.0/100e-6;
			// Calculates the corresponding temperature in degC using a Steinhart-Hart model.
			var logr = Math.log(rtherm);
			var ttherm = 1.0/(0.000878844 + 0.000231913*logr + 7.70349e-8*logr*logr*logr) - 273.15;
			// Prepares data packet for storing to the database.
			// NB: the data packet layout is hardcoded here!
			p = new dataPacketModel({
				'timestamp': new Date(),
				'crudePeriod': buf.readUInt16LE(16),
				'refinedPeriod': refinedPeriod,
				'sequenceNumber': buf.readInt32LE(0),
				'temperature': buf.readInt32LE(18)/160.0,
				'pressure': buf.readInt32LE(22),
				'thermistor': ttherm,
				// use nominal 1st order fit from sensor datasheet to calculate RH in %
				'humidity': (buf.readUInt16LE(28)/65536.0 - 0.1515)/0.00636,
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
			if(lastDataSequenceNumber == 1) saveMe = false;
		}
		else {
			console.log('Got unexpected packet type',ptype);
			return;
		}
		if(saveMe) {
			//console.log(p);
			p.save(function(err,p) {
				if(err) console.log('Error saving data packet',p);
			});
		}
		else {
			console.log('Packet not saved to db.');
		}
	});
}

// Responds to a request for our about page.
function about(req,res,config) {
	res.send(
		'Server started ' + config.startupTime + '<br/>' +
		'Server command line: ' + process.argv.join(' ') + '<br/>' +
		(config.port === null ? 'No serial connection' : (
			'Connected to tty ' + config.port.path)) + '<br/>' +
		(config.db === null ? 'No database connection' : (
			'Connected to db "' + config.db.connection.name + '" at ' +
			config.db.connection.host + ':' + config.db.connection.port)) + '<br/>'
	);
}

// Responds to a request to view boot info.
function boot(req,res,bootPacketModel) {
	bootPacketModel.find()
		.limit(10).sort([['timestamp', -1]])
		.exec(function(err,results) { res.send(results); });
}

// Responds to a request to fetch data.
function fetch(req,res,dataPacketModel) {
	// Gets the date range to fetch.
	var from = ('from' in req.query) ? req.query.from : '-120';
	var to = ('to' in req.query) ? req.query.to : 'now';
	// Converts end date into a javascript Date object.
	to = new Date(Date.parse(to));
	if(to == 'Invalid Date') to = new Date();
	// Converts begin date into a javascript Date object.
	var relativeSeconds = parseInt(from);
	if(relativeSeconds < 0) {
		// Interprets from as number of seconds to fetch before end date.
		from = new Date(to.getTime() + 1000*relativeSeconds);
	}
	else {
		// Tries to interpret from as a date string.
		from = new Date(Date.parse(from));
		if(from == 'Invalid Date') {
			// Defaults to fetching 120 seconds.
			from = new Date(to.getTime() - 120000);
		}
	}
	if(debug) console.log('query',from,to);
	dataPacketModel.find()
		.where('timestamp').gt(from).lte(to)
		.limit(1000).sort([['timestamp', -1]])
		.select(('series' in req.query) ? 'timestamp ' + req.query.series.join(" ") : '')
		.exec(function(err,results) { res.send(results); });
}
