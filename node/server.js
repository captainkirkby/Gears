// Implements the combined data logger and web server component of the clock metrology project.
// Created by D & D Kirkby, Dec 2013

var util = require('util');
var async = require('async');
var express = require('express');
var serial = require("serialport");

var app = express();

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
					function(firstFtdiPort) { portCallback(null,firstFtdiPort.comName) }
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
					// Flushes any data received but not yet read.
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
		console.log('Connecting to the database...');
		callback(null,'db');
	}},
	// Performs steps that require both an open serial port and database connection.
	function(err,config) {
		if(err) throw err;
		// Logs TickTock packets from the serial port into the database.
		console.log('starting data logger with',config);
		config.port.on('data',function(packet) {
			console.log('got packet',packet);
		});
		// Defines our webapp routes.
		app.get('/config.txt', function(req, res) {
			res.send(util.format('tty path is %s',config.port.path));
		});
		// Starts our webapp
		console.log('starting web server on port 3000');
		app.listen(3000);
	}
);
