var mongoose = require('mongoose');
// var winston = require('winston');

// Connects to the database where packets from TickTock are logged.
// Intended for use by the parallel function of the async package
var connectToDB = function (callback) {
	//if(noDatabase) return callback(null,null);
	mongoose.connect('mongodb://localhost:27017/TickTock');
	var db = mongoose.connection;
	db.on('error', function(){callback(new Error("Could not connect to Database.  Make sure mongod is running."));});
	db.once('open', function() {
		// winston.info('db connection established.');		// Doesn't make it to the file
		// Defines the schema and model for our serial boot packets
		var bootPacketSchema = mongoose.Schema({
			timestamp: { type: Date, index: true },
			computerTimestamp: { type: Date, index: true },
			serialNumber: String,
			bmpSensorOk: Boolean,
			gpsSerialOk: Boolean,
			sensorBlockOK: Boolean,
			commitTimestamp: Date,
			commitHash: String,
			commitStatus: Number,
			latitude: Number,
			longitude: Number,
			altitude: Number,
			initialWeekNumber: Number,
			initialTimeOfWeek: Number,
			initialUTCOffset: Number
		});
		var bootPacketModel = mongoose.model('bootPacketModel',bootPacketSchema);
		// Defines the schema and model for our serial data packets
		var dataPacketSchema = mongoose.Schema({
			timestamp: { type: Date, index: true },
			computerTimestamp: { type: Date, index: true },
			crudePeriod: Number,
			initialCrudePeriod: Number,
			samplesSinceBoot: Number,
			refinedPeriod: Number,
			angle: Number,
			height: Number,
			sequenceNumber: Number,
			boardTemperature: Number,
			pressure: Number,
			blockTemperature: Number,
			humidity: Number,
			irLevel: Number,
			recieverMode: Number,
			discipliningMode: Number,
			criticalAlarms: Number,
			minorAlarms: Number,
			gpsDecodingStatus: Number,
			discipliningActivity: Number,
			clockOffset: Number
		});
		var dataPacketModel = mongoose.model('dataPacketModel',dataPacketSchema);
		// Defines the schema and model for our raw data
		var rawDataSchema = mongoose.Schema({
			expiryTimestamp	: { type: Date, expires: 3600 },
			timestamp		: { type: Date, index: true },
			sequenceNumber	: Number,
			raw				: Array
		});
		var rawDataModel = mongoose.model('rawDataModel',rawDataSchema);
		// Defines the schema and model for the running averages
		var averageDataSchema = mongoose.Schema({
			timestamp: { type: Date, index: true },
			crudePeriod: Number,
			refinedPeriod: Number,
			angle: Number,
			height: Number,
			boardTemperature: Number,
			pressure: Number,
			blockTemperature: Number,
			humidity: Number,
			averagingPeriod : Number,
			clockOffset: Number
		});
		var averageDataModel = mongoose.model('averageDataModel',averageDataSchema);
		// Define the schema and model for the gps status schema
		var gpsStatusSchema = mongoose.Schema({
			timestamp: { type: Date, index: true },
			recieverMode: Number,
			discipliningMode: Number,
			criticalAlarms: Number,
			minorAlarms: Number,
			gpsDecodingStatus: Number,
			discipliningActivity: Number,
			clockOffset: Number
		});
		var gpsStatusModel = mongoose.model('gpsStatusModel',gpsStatusSchema);
		// Propagates our database connection and db models to data logger.
		callback(null,{
			'connection':db,
			'bootPacketModel':bootPacketModel,
			'dataPacketModel':dataPacketModel,
			'gpsStatusModel':gpsStatusModel,
			'averageDataModel':averageDataModel,
			'rawDataModel':rawDataModel,
		});
	});
};

exports.connectToDB = connectToDB;
