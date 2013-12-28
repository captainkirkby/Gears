// Implements the combined data logger and web server component of the clock metrology project.
// Created by D & D Kirkby, Dec 2013

async = require('async');

async.parallel({
	// Opens a serial port connection to the TickTock module.
	port: function(callback) {
		async.waterfall([
			// Finds the tty device name of the serial port to open.
			function(portCallback) {
				console.log('Looking for the tty device...')
				portCallback(null,'portName');
			},
			// Opens the serial port.
			function(portName,portCallback) {
				console.log('Opening device %s...',portName);
				portCallback(null,'port');
			}],
			// Propagates our open port to the data logger.
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
	// Logs TickTock packets from the serial port into the database.
	function(err,config) {
		if(err) throw err;
		console.log('starting data logger with',config);
	}
);
