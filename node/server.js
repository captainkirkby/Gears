// Implements the combined data logger and web server component of the clock metrology project.
// Created by D & D Kirkby, Dec 2013

var util = require('util');
var async = require('async');
var express = require('express');
var serial = require('serialport');
var mongoose = require('mongoose');

// Assembles packets in the specified buffer using the data provided.
// Call with remaining = 0 the first time, then update remaining with the return value.
// Calls the specified handler with each completed buffer. Packets are assumed to
// start with 3 consecutive bytes of 0xFE followed by an unsigned byte that specifies
// the packet type. Payload sizes for different packet types are hardcoded.
// Automatically aligns to packet boundaries when called with remaining = 0 or when
// an assembled packet has an unexpected header.
function assemblePacket(data,buffer,remaining,handler) {
	var nextAvail = 0;
	while(nextAvail < data.length) {
		if(remaining == -3) {
			// The next byte is the packet type.
			type = data.readUInt8(nextAvail);
			if(type == 0x01) {
				// NB: hardcoded payload size for a DATA_PACKET
				remaining = 32;
			}
			else {
				console.log('Skipping unexpected packet type',type);
				return 0;
			}
			console.log('start packet',type,remaining);
			// Check that the buffer provided is big enough.
			if(remaining + 4 > buffer.length) {
				console.log('Skipping packet with payload overflow',remaining+4,buffer.length);
				return 0;
			}
			buffer[4] = type;
			nextAvail++;
		}
		else if(remaining <= 0) {
			// Look for a header byte.
			if(data[nextAvail] == 0xFE) {
				buffer[-remaining] = 0xFE;
				remaining -= 1;
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
			if(remaining === 0) {
				// Calls the specified handler with the assembled packet.
				handler(buffer);
			}
		}
	}
	return remaining;
}

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
					function(firstFtdiPort) { portCallback(null,firstFtdiPort.comName); }
					);
				});
			},
			// Opens the serial port.
			function(ttyName,portCallback) {
				console.log('Opening device %s...',ttyName);
				var port = new serial.SerialPort(ttyName, {
					baudrate: 115200,
					buffersize: 255,
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
		if(noDatabase) return callback(null,null);
		console.log('Connecting to the database...');
		mongoose.connect('mongodb://localhost:27017/ticktock');
		var db = mongoose.connection;
		db.on('error', console.error.bind(console, 'db connection error:'));
		db.once('open', function() {
			console.log('db connection established.');
			// Defines the data model for our serial packets
			var packetSchema = mongoose.Schema({
				timestamp: { type: Date, index: true },
				temperature: Number,
				pressure: Number
			});
			var dataModel = mongoose.model('dataModel',packetSchema);
			// Propagates our database connection and data model to data logger.
			callback(null,{'connection':db,'model':dataModel});
		});
	}},
	// Performs steps that require both an open serial port and database connection.
	// Note that either of config.port or config.db might be null if they are disabled
	// with command-line flags.
	function(err,config) {
		if(err) throw err;
		if(config.db && config.port) {
			// Logs TickTock packets from the serial port into the database.
			console.log('starting data logger with',config);
			var PacketModel = config.db.model;
			// NB: maximum possible packet size is hard coded here!
			var buffer = new Buffer(36);
			var remaining = 0;
			config.port.on('data',function(data) {
				console.log('received',data);
				remaining = assemblePacket(data,buffer,remaining,function(buf) {
					console.log('assembled',buf);
					// Prepares packet data for storing to the database.
					// NB: packet layout is hardcoded here!
					var p = new PacketModel({
						'timestamp': new Date(),
						'temperature': buf.readInt32LE(16)/160.0,
						'pressure': buf.readInt32LE(20)
					});
					console.log(p);
					p.save(function(err,p) {
						if(err) console.log('Error writing packet',p);
					});
				});
			});
		}
		// Defines our webapp routes.
		var app = express();
		// Serves static files from our public subdirectory.
		app.use('/', express.static(__dirname + '/public'));
		// Serves a dynamically generated information page.
		app.get('/about', function(req, res) {
			// TODO: flesh this out and improve handling of --no-serial or --no-database
			res.send(util.format('tty path is %s and db is %s at %s:%d',
				config.port.path,config.db.connection.name,config.db.connection.host,
				config.db.connection.port));
		});
		if(config.db) {
			// Serves data dynamically via AJAX.
			var PacketModel = config.db.model;
			app.get('/fetch', function(req,res) {
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
				PacketModel.find()
					.where('timestamp').gt(from).lte(to)
					.limit(1000).sort([['timestamp', -1]])
					.select('timestamp temperature pressure')
					.exec(function(err,results) { res.send(results); });
			});
		}
		// Starts our webapp.
		console.log('starting web server on port 3000');
		app.listen(3000);
	}
);
