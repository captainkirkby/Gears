// Implements the combined data logger and web server component of the clock metrology project.
// Created by D & D Kirkby, Dec 2013

var util = require('util');
var async = require('async');
var express = require('express');
var serial = require('serialport');
var mongoose = require('mongoose');

// Assembles fixed-size packets in the specified buffer using the data provided.
// Call with remaining = 0 the first time, then update remaining with the return value.
// Calls the specified handler with each completed buffer. Packets are assumed to
// start with 4 consecutive bytes of 0xFE, which will be included in the assembled
// packet sent to the handler. Automatically aligns to packet boundaries when called
// with remaining = 0 or when an assembled packet has an unexpected header.
function assemblePacket(data,buffer,remaining,handler) {
	var nextAvail = 0;
	while(nextAvail < data.length) {
		if(remaining <= 0) {
			// Look for a header byte.
			if(data[nextAvail] == 0xFE) {
				buffer[-remaining] = 0xFE;
				remaining -= 1;
				if(remaining == -4) {
					// We have found a complete packet header, so start reading its payload.
					remaining = buffer.length - 4;
				}
			}
			else {
				// Forget any previously seen header bytes.
				remaining = 0;
			}
			nextAvail++;
		}
		else {
			var toCopy = Math.min(remaining,data.length-nextAvail);
			data.copy(buffer,buffer.length-remaining,nextAvail,nextAvail+toCopy);
			nextAvail += toCopy;
			remaining -= toCopy;
			if(remaining == 0) {
				if(buffer.readUInt32LE(0) == 0xFEFEFEFE) {
					handler(buffer);
					remaining = buffer.length;
				}
				else {
					console.log("ignoring packet with bad header",buffer);
					// Go back to header scanning.
					remaining = 0;
				}
			}
		}
	}
	return remaining;
}

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
					parser: serial.parsers.raw
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
		var PacketModel = config.db.model;
		var buffer = new Buffer(12);
		var remaining = 0;
		config.port.on('data',function(data) {
			console.log('received',data);
			remaining = assemblePacket(data,buffer,remaining,function(buf) {
				console.log('assembled',buf);
			});
			/*
			async.map(packet.split(' '),
			// Converts one string token into a floating point value.
			function(token,handler) {
				// Result will be NaN for an invalid token.
				handler(null,parseInt(token));
			},
			// Processes the converted values from one packet.
			function(err,values) {
				if(err || values.length != 2) {
					console.log('Ignoring badly formatted packet "%s"',packet);
				}
				else {
					timestamp = new Date();
					temperature = values[0]/160.0;
					pressure = values[1];
					// %d handles both integer and float values (there is no %f)
					console.log('timestamp = %s, temperature = %d, pressure = %d',
						timestamp,temperature,pressure);
					var p = new PacketModel({
						'timestamp': timestamp,
						'temperature': temperature,
						'pressure': pressure
					});
					p.save(function(err,p) {
						if(err) console.log('Error writing packet',p);
					});
				}
			});
			*/
		});
		// Defines our webapp routes.
		var app = express();

		app.use('/', express.static(__dirname + '/public'));

		app.get('/about', function(req, res) {
			res.send(util.format('tty path is %s and db is %s at %s:%d',
				config.port.path,config.db.connection.name,config.db.connection.host,
				config.db.connection.port));
		});
		app.get('/recent', function(req,res) {
			PacketModel.find().limit(120).sort([['timestamp', -1]]).select('timestamp temperature pressure').exec(function(err,results) {
				res.send(results);
			});
		});
		// Starts our webapp.
		console.log('starting web server on port 3000');
		app.listen(3000);
	}
);
