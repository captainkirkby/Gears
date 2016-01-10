// Implements the Database Fetch component of the clock metrology project
// Created by D & D Kirkby, Dec 2013

var winston_module = require('winston');

var connectToDB = require('./dbConnection').connectToDB;
var bins = require('./bins');

// Tracks the date of the first boot packet
var latestDate = null;

// Log to file
var winston = new (winston_module.Logger)({
	transports: [
		new (winston_module.transports.Console)({ level: 'warn' }),
		new (winston_module.transports.File)({ filename: 'ticktock.log', level: 'verbose' })
	]
});

// Parses command-line arguments.
var noSerial = false;
var noDatabase = false;
var debug = false;
var pythonFlags = ["--load-template", "template2048.dat"];
process.argv.forEach(function(val,index,array) {
	if(val == '--no-serial') noSerial = true;
	else if(val == '--no-database') noDatabase = true;
	else if(val == '--debug') {
		winston.transports.console.level = 'debug';
		winston.transports.file.level = 'debug';
		debug = true;
	}
	else if(val == '--physical')  pythonFlags = ["--physical"];
});

process.on('exit', function(){
	gracefulExit();
});

function gracefulExit()
{
	winston.info("Stopping Fetch");
}


winston.verbose("Worker Starting!");
winston.verbose(__filename + ' connecting to the database...');

var dbCallback = dbCallbackFunction;
connectToDB(dbCallback);

function dbCallbackFunction(err, config) {
	if(err) throw err;
	// Send ready message
	process.send({"ready" : true});

	process.on('message', function(message) {
		winston.verbose("Starting Fetch");
		fetch(message.query, config.dataPacketModel, config.bootPacketModel, config.averageDataModel, config.gpsStatusModel, config.rawDataModel);
	});
}

// Maximum number of results to return from a query (don't exceed number of pixels on graph!)
var MAX_QUERY_RESULTS = 120;

// Default time to fetch is 2 minutes (120 seconds)
var DEFAULT_FETCH = 120;

// Responds to a request to fetch data.
function fetch(query, dataPacketModel, bootPacketModel, averageDataModel, gpsStatusModel, rawDataModel) {
	// Gets the date range to fetch.
	var from = ('from' in query) ? query.from : '-120';
	var to = ('to' in query) ? query.to : 'now';
	var mostRecent = false;

	// Tries to interpret to as a date string
	to = new Date(Date.parse(to));
	// NB: Hardcoded 16 Second GPS-UTC Offset
	var utcNow = new Date();
	var GPS_UTC_OFFSET = 16;
	if(to == 'Invalid Date') to = new Date(utcNow.getTime()+GPS_UTC_OFFSET*1000);

	// Tries to interpret from as start keyword
	if(from == 'start' && latestDate !== null){
		from = latestDate;
	// Tries to interpret from as "the most recent sample in the database"
	} else if(from == '-1') {
		mostRecent = true;
	// Tries to interpret from as a date string
	} else {
		from = new Date(Date.parse(from));
		if(from == 'Invalid Date') {
			// Defaults to fetching DEFAULT_FETCH seconds.
			from = new Date(to.getTime() - DEFAULT_FETCH*1000);
		}
	}
	winston.verbose('query', query);
	var dbCollection = dataPacketModel;

	if(query.db == "gps"){
		// We need to fetch from gpsStatusModel
		dbCollection = gpsStatusModel;
	} else if(query.db == "raw"){
		// We need to fetch from gpsStatusModel
		dbCollection = rawDataModel;
	}

	if(mostRecent){
		// Only fetch most recent (raw)
		dbCollection.find().
			limit(1).sort([['timestamp', -1]])
			.exec(sendData);
	} else {
		// Fetch many (not raw)
		var visibleSets = getVisibleSets(query);
		var binSize = getBins(to-from);		//in sec

		console.log(('series' in query) ? 'timestamp ' + visibleSets.join(" ") : '');

		if(binSize && binSize>0){
			// We need averaging
			winston.verbose("Averaging bin size: " + binSize);
			averageDataModel.find()
				.where('timestamp').gt(from).lte(to)
				.where('averagingPeriod').equals(binSize)
				.limit(MAX_QUERY_RESULTS).sort([['timestamp', -1]])
				.select(('series' in query) ? 'timestamp ' + visibleSets.join(" ") : '')
				.exec(sendData);
		} else {
			// No averaging needed
			winston.debug("Direct Fetch");
			dbCollection.find()
				.where('timestamp').gt(from).lte(to)
				.limit(MAX_QUERY_RESULTS).sort([['timestamp', -1]])
				.select(('series' in query) ? 'timestamp ' + visibleSets.join(" ") : '')
				.exec(sendData);
		}
	}
}

var sendData = function(err,results) {
	if(err) throw err;
	// Send message to parent
	process.send({
		"done"		: true,
		"results"	: results
	});
};

function getVisibleSets(query) {
	var visibleSets = [];
	for(var k in query.series) {
		if(query.series[k].visible == 'true'){
			if(k != 'raw') visibleSets.push(k);
		}
	}
	return visibleSets;
}

// Find the smallest standard bin size that, when we break up the time period
// we're given, will result in under MAX_QUERY_RESULTS bins
// Inputs delta time in miliseconds, returns standard bin size in seconds
function getBins(dt){
	// winston.info(dt);
	if(dt/1000.0 <= MAX_QUERY_RESULTS) return null;
	var standardBinSizes = bins.stdBinSizes();			// in seconds
	var smallestBinSize = standardBinSizes[standardBinSizes.length-1];

	for(var i = 0; i < standardBinSizes.length; i++){
		var numBins = (dt/1000)/standardBinSizes[i];		// dt ms -> s
		// winston.info("Trying bin size " + standardBinSizes[i] + ".  Results in this many bins: " + numBins);
		if(numBins <= MAX_QUERY_RESULTS && standardBinSizes[i] < smallestBinSize){
			// get largest bin size
			smallestBinSize = standardBinSizes[i];
		}
	}

	return smallestBinSize;
}
