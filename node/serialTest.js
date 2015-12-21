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

var port = new serial.SerialPort('/dev/tty.usbserial-AL00XH9G', {
	baudrate: 125000,
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
	try
	{
		console.log((data.readInt16BE(0)/500.0 + 24.0) + "°C");
		console.log(data.readUInt32BE(2) + " Pa");
		console.log((data.readUInt32BE(6)/1024.0) + " %rH");
	}
	catch(err)
	{
		
	}
});

console.log('Done');
