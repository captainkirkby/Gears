var mongoose = require('mongoose');

// Connects to the database where packets from TickTock are logged.
// Intended for use by the parallel function of the async package
var connectToDB = function (callback) {
	//if(noDatabase) return callback(null,null);
	console.log(__filename + ' connecting to the database...');
	mongoose.connect('mongodb://localhost:27017/ticktockDemoTest3');
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
			sensorBlockOK: Boolean,
			commitTimestamp: Date,
			commitHash: String,
			commitStatus: Number
		});
		var bootPacketModel = mongoose.model('bootPacketModel',bootPacketSchema);
		// Defines the schema and model for our serial data packets
		var dataPacketSchema = mongoose.Schema({
			timestamp: { type: Date, index: true },
			crudePeriod: Number,
			refinedPeriod: Number,
			angle: Number,
			sequenceNumber: Number,
			temperature: Number,
			pressure: Number,
			thermistor: Number,
			humidity: Number,
			irLevel: Number,
			raw: Array
		});
		var dataPacketModel = mongoose.model('dataPacketModel',dataPacketSchema);
		// Propagates our database connection and db models to data logger.
		callback(null,{
			'connection':db,
			'bootPacketModel':bootPacketModel,
			'dataPacketModel':dataPacketModel
		});
	});
};

exports.connectToDB = connectToDB;