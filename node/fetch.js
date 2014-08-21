// Implements the Database Fetch component of the clock metrology project
// Created by D & D Kirkby, Dec 2013

var mongoose = require('mongoose');

// Tracks the date of the first boot packet
var latestDate = null;

// Parses command-line arguments.
var noSerial = false;
var noDatabase = false;
var debug = false;
var pythonFlags = ["--load-template", "template2048.dat"];
process.argv.forEach(function(val,index,array) {
	if(val == '--no-serial') noSerial = true;
	else if(val == '--no-database') noDatabase = true;
	else if(val == '--debug') debug = true;
	else if(val == '--physical')  pythonFlags = ["--physical"];
});

process.on('exit', function(){
	gracefulExit();
});
function gracefulExit()
{
	console.log("Stopping Fetch");
}

if(debug) console.log("Worker Starting!");
console.log(__filename + ' connecting to the database...');
mongoose.connect('mongodb://localhost:27017/ticktockDemoTest3');
var db = mongoose.connection;
db.on('error', console.error.bind(console, 'db connection error:'));
db.once('open', function() {
	if(debug) console.log('db connection established.');
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

	// Send ready message
	process.send({"ready" : true});

	process.on('message', function(message) {
		if(debug) console.log("Starting Fetch");
		fetch(message.query, dataPacketModel, bootPacketModel);
	});

});

// Maximum number of results to return from a query (don't exceed number of pixels on graph!)
var MAX_QUERY_RESULTS = 1000;

// Default time to fetch is 2 minutes (120 seconds)
var DEFAULT_FETCH = 120;

// Responds to a request to fetch data.
function fetch(query, dataPacketModel, bootPacketModel) {
	// Gets the date range to fetch.
	var from = ('from' in query) ? query.from : '-120';
	var to = ('to' in query) ? query.to : 'now';
	var mostRecent = false;

	// Tries to interpret to as a date string
	to = new Date(Date.parse(to));
	if(to == 'Invalid Date') to = new Date();

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
	if(debug) console.log('query', query);

	// Only fetch most recent (raw)
	if(mostRecent){
		dataPacketModel.find().limit(1).sort([['timestamp', -1]]).exec(function(err,results) {
			// Send message to parent
			process.send({
				"done"		: true,
				"results"	: results
			});
		});
	// Fetch many (not raw)
	} else {
		// TODO: expand fetch to natural borders
		var visibleSets = getVisibleSets(query);
		dataPacketModel.find()
			.where('timestamp').gt(from).lte(to)
			.limit(MAX_QUERY_RESULTS*1000).sort([['timestamp', 1]])
			.select(('series' in query) ? 'timestamp ' + visibleSets.join(" ") : '')
			.exec(function(err,results) {
				if(err) throw err;
	
				var binSize = getBins(to-from);		//in ms
				var numBins = (to-from)/binSize;
	
				if(binSize && numBins && binSize>0 && numBins>0){
	
					var newResults = [];
					var count = 0;
		
					for (var i = 0; i < numBins; i++) {
						var upperLimit = new Date(Date.parse(from) + binSize*(i+1));
						newResults[i] = {'timestamp' : upperLimit};
						var average = {};
						var averageCount = {};
	
						// Prepopulate with fields
						visibleSets.forEach(function (dataSetToPreFill, index, arr){
							average[dataSetToPreFill] = 0;
							averageCount[dataSetToPreFill] = 0;
						});
	
						var averageData = function (dataSetToAverage, index, arr){
							if(results[count][dataSetToAverage] !== null && results[count][dataSetToAverage] > 0){
								average[dataSetToAverage] += results[count][dataSetToAverage];
								averageCount[dataSetToAverage]++;
							}
						};
	
						// Average boxes out
						while(count < results.length && results[count].timestamp < upperLimit){
							visibleSets.forEach(averageData);
							count++;
						}
	
						// Don't divide by zero!  (if its zero, we want null so flot doesnt autoscale with the zero)
						visibleSets.forEach(function (dataSetToFill, index, arr){
							if(averageCount[dataSetToFill] === 0) newResults[i][dataSetToFill] = null;
							else newResults[i][dataSetToFill] = (average[dataSetToFill]/averageCount[dataSetToFill]);
						});
					}
	
					if(debug) console.log(newResults[0]);
					results = newResults;
				}
				// Send message to parent
				process.send({
					"done"		: true,
					"results"	: results
				});
			});
	}
}

function getVisibleSets(query) {
	var visibleSets = [];
	for(var k in query.series) {
		if(query.series[k].visible == 'true'){
			if(k != 'raw') visibleSets.push(k);
		}
	}
	return visibleSets;
}

// Find the smallest standard bin size that, when we break up the time period we're given, will result in under 1000 bins
// Inputs delta time in miliseconds, returns standard bin size in miliseconds
function getBins(dt){
	// console.log(dt);
	if(dt/1000.0 <= MAX_QUERY_RESULTS) return null;
	var standardBinSizes = [1,5,10,15,30,60,300,600,1800,3600,9000]; // in seconds
	var smallestBinSize = standardBinSizes[standardBinSizes.length-1];

	for(var i = 0; i < standardBinSizes.length; i++){
		var numBins = (dt/1000)/standardBinSizes[i];		// dt ms -> s
		// console.log("Trying bin size " + standardBinSizes[i] + ".  Results in this many bins: " + numBins);
		if(numBins <= MAX_QUERY_RESULTS && standardBinSizes[i] < smallestBinSize){
			// get largest bin size
			smallestBinSize = standardBinSizes[i];
		}
	}

	return smallestBinSize*1000;
}
