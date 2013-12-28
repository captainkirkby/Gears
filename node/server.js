// Implements the combined data logger and web server component of the clock metrology project.
// Created by D & D Kirkby, Dec 2013

var util = require('util');
var async = require('async');
var express = require('express');
var serial = require('serialport');
var mongoose = require('mongoose');

async.parallel({
	// Opens a serial port connection to the TickTock device.
	port: function(callback) {
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
					function(firstFtdiPort) { portCallback(null,firstFtdiPort.comName); }
					);
				});
			},
			// Opens the serial port.
			function(ttyName,portCallback) {
				console.log('Opening device %s...',ttyName);
				var port = new serial.SerialPort(ttyName, {
					baudrate: 9600,
					parser: serial.parsers.readline("\r\n")
				});
				port.on('open',function(err) {
					if(err) return portCallback(err);
					console.log('Port open');
					// Waits one second, while the device resets.
					setTimeout(function() {
						// Flushes any data received but not yet read.
						port.flush();
						// Forwards the open serial port.
						portCallback(null,port);
					},2000);
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
		console.log('Connecting to the database...');
		mongoose.connect('mongodb://localhost:27017/ticktock');
		var db = mongoose.connection;
		db.on('error', console.error.bind(console, 'db connection error:'));
		db.once('open', function() {
			console.log('db connection established.');
			// Defines the data model for our serial packets
			var packetSchema = mongoose.Schema({
				timestamp: Date,
				temperature: Number,
				pressure: Number
			});
			var dataModel = mongoose.model('dataModel',packetSchema);
			// Propagates our database connection and data model to data logger.
			callback(null,{'connection':db,'model':dataModel});
		});
	}},
	// Performs steps that require both an open serial port and database connection.
	function(err,config) {
		if(err) throw err;
		// Logs TickTock packets from the serial port into the database.
		console.log('starting data logger with',config);
		var Packet = config.db.model;
		config.port.on('data',function(packet) {
			async.map(packet.split(' '),
			// Converts one string token into a floating point value.
			function(token,handler) {
				// Result will be NaN for an invalid token.
				handler(null,parseFloat(token));
			},
			// Processes the converted values from one packet.
			function(err,values) {
				if(err || values.length != 2) {
					console.log('Ignoring badly formatted packet "%s"',packet);
				}
				else {
					timestamp = new Date();
					temperature = values[0];
					pressure = values[1];
					// %d handles both integer and float values (there is no %f)
					console.log('timestamp = %s, temperature = %d, pressure = %d',
						timestamp,temperature,pressure);
					var p = new Packet({
						'timestamp': timestamp,
						'temperature': temperature,
						'pressure': pressure
					});
					p.save(function(err,p) {
						if(err) console.log('Error writing packet',p);
					});
				}
			});
		});
		// Defines our webapp routes.
		var app = express();
		app.get('/about', function(req, res) {
			res.send(util.format('tty path is %s and db is %s at %s:%d',
				config.port.path,config.db.connection.name,config.db.connection.host,
				config.db.connection.port));
		});
		app.get('/recent', function(req,res) {
			Packet.find().limit(10).select('timestamp temperature pressure').exec(function(err,results) {
				res.send(JSON.stringify(results));
			});
		});
		// Starts our webapp.
		console.log('starting web server on port 3000');
		app.listen(3000);
	}
);
