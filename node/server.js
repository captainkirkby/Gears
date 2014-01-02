// Implements the combined data logger and web server component of the clock metrology project.
// Created by D & D Kirkby, Dec 2013

var util = require('util');
var async = require('async');
var express = require('express');
var serial = require('serialport');
var mongoose = require('mongoose');

var packet = require('./packet');

sprintf = require('sprintf').sprintf;

// Parses command-line arguments.
var noSerial = false;
var noDatabase = false;
process.argv.forEach(function(val,index,array) {
	if(val == '--no-serial') noSerial = true;
	else if(val == '--no-database') noDatabase = true;
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
						ttyCallback(port.manufacturer == 'FTDI');
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
					baudrate: 9600, //115200,
					buffersize: 255,
					parser: serial.parsers.raw
				});
				port.on('open',function(err) {
					if(err) return portCallback(err);
					console.log('Port open');
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
		mongoose.connect('mongodb://localhost:27017/ticktock');
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
				commitTimestamp: Date,
				commitHash: String,
				commitStatus: Number
			});
			var bootPacketModel = mongoose.model('bootPacketModel',bootPacketSchema);
			// Defines the schema and model for our serial data packets
			var dataPacketSchema = mongoose.Schema({
				timestamp: { type: Date, index: true },
				temperature: Number,
				pressure: Number
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
			var assembler = new packet.Assembler(0xFE,3,{0x00:31});
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
	console.log('remaining',assembler.remaining,'received',data);
	assembler.ingest(data,function(ptype,buf) {
		console.log('assembled type',ptype,buf);
		if(ptype == 0x00) {
			// Prepares boot packet for storing to the database.
			// NB: packet layout is hardcoded here!
			hash = '';
			for(var offset = 6; offset < 26; offset++) hash += sprintf("%02x",buf.readUInt8(offset));
			p = new bootPacketModel({
				'timestamp': new Date(),
				'serialNumber': sprintf("%08x",buf.readUInt32LE(0)),
				'bmpSensorOk': buf.readUInt8(4),
				'gpsSerialOk': buf.readUInt8(5),
				'commitTimestamp': new Date(buf.readUInt32LE(6)*1000),
				'commitHash': hash,
				'commitStatus': buf.readUInt8(30)
			});
			// After seeing a boot packet, we accept data packets.
			assembler.addPacketType(0x01,32);
		}
		else if(ptype == 0x01) {
			// Prepares data packet for storing to the database.
			// NB: packet layout is hardcoded here!
			p = new dataPacketModel({
				'timestamp': new Date(),
				'temperature': buf.readInt32LE(12)/160.0,
				'pressure': buf.readInt32LE(16)
			});
		}
		else {
			console.log('Got unexpected packet type',ptype);
			return;
		}
		console.log(p);
		p.save(function(err,p) {
			if(err) console.log('Error saving data packet',p);
		});
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
	console.log('query',from,to);
	dataPacketModel.find()
		.where('timestamp').gt(from).lte(to)
		.limit(1000).sort([['timestamp', -1]])
		.select('timestamp temperature pressure')
		.exec(function(err,results) { res.send(results); });
}
