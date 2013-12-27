////////////////////////////////////////////////////////
// Use the cool library                               //
// git://github.com/voodootikigod/node-serialport.git //
// to read the serial port where arduino is sitting.  //
////////////////////////////////////////////////////////
var serialPort = require("serialport");

serialPort.list(function(err, ports){
	var usbSerialPort = "";
	if(err) throw err;

	// Synchronous blocking call
	ports.forEach(function(port){
		console.log(port.comName);
		console.log(port.pnpId);
		console.log(port.manufacturer);

		// Find the first serial port that looks like a usb serial port
		if(port.comName.indexOf("usb") > -1){
			usbSerialPort = port.comName;
			console.log("Found port to use: " + usbSerialPort);
		}
	});

	console.log("At end of for each.  Value of usbSerialPort is " + usbSerialPort + ".  Creating instance of SerialPort with it.");

	var usbSerial = new serialPort.SerialPort(usbSerialPort, {
		baudrate: 9600,
		parser: serialPort.parsers.readline('\r\n')
	});

	usbSerial.on('open',function(err) {
		if(err) throw err;
		console.log('Port open');
	});

	var HEADER = "Celsius";
	var foundHeader = false;

	usbSerial.on('data', function(data){
		if(data.indexOf(HEADER) > -1){
			foundHeader = true;
			console.log("Header");
		} else if(!foundHeader){
			console.log("Junk");
		} else {
			console.log(data);
		}
	});

});
