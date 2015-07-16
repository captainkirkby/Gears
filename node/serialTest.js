// Test program to probe serial port

/***********************************************
	Requires
***********************************************/

var serial = require('serialport');

/***********************************************
	Tests
***********************************************/

serial.list(function(err,ports) {
	console.log(ports);
});
// Outputs comName: '/dev/ttyUSB0'

var port = new serial.SerialPort('/dev/ttyUSB0', {
	baudrate: 78125,
	buffersize: 255,
	parser: serial.parsers.raw
});

port.on('open',function(err) {
	console.log('Serial Port open event');
	if(err) throw err;
	port.flush();
});

port.on('data',function(data) {
	console.log(data);
	console.log(data.toString());
});

console.log('Done');
