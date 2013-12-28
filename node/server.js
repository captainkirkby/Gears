// Implements the combined data logger and web server component of the clock metrology project.
// Created by D & D Kirkby, Dec 2013

var util = require('util');
var async = require('async');
var express = require('express');
var serial = require("serialport");

var app = express();

async.parallel({
	// Opens a serial port connection to the TickTock device.
	dev: function(callback) {
		async.waterfall([
			// Finds the tty device name of the serial port to open.
			function(devCallback) {
				console.log('Looking for the tty device...');
				serial.list(function(err,ports) {
					// Looks for the first port with 'FTDI' as the manufacturer
					async.detectSeries(ports,function(port,ttyCallback) {
						console.log('scanning port',port);
						ttyCallback(port.manufacturer == 'FTDI');
					},
					// Forwards the corresponding tty device name.
					function(firstFtdiPort) { devCallback(null,firstFtdiPort.comName) }
					);
				});
			},
			// Opens the serial port.
			function(portName,devCallback) {
				console.log('Opening device %s...',portName);
				devCallback(null,portName,'port');
			}],
			// Propagates our device info to data logger.
			function(err,portName,port) {
				if(!err) console.log('serial port is ready');
				callback(err,{'tty':portName,'port':port});
			}
		);
	},
	// Connects to the database where packets from TickTock are logged.
	db: function(callback) {
		console.log('Connecting to the database...');
		callback(null,'db');
	}},
	function(err,config) {
		if(err) throw err;
		// Logs TickTock packets from the serial port into the database.
		console.log('starting data logger with',config);
		// Defines our webapp routes.
		app.get('/config.txt', function(req, res) {
			res.send(util.format('tty is %s',config.dev.tty));
		});
		// Starts our webapp
		console.log('starting web server on port 3000');
		app.listen(3000);
	}
);
