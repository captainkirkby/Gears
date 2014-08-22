// Implements the Web Server Component of the clock metrology project
// Created by D & D Kirkby, Dec 2013


var mongoose = require('mongoose');
var express = require('express');
var fork = require('child_process').fork;

var connectToDB = require('./dbConnection').connectToDB;

// Global status for fetchWorker
var fetchWorker = null;
var fetchWorkerReady = false;

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

if(!noSerial){
	// Start process with data pipes
	var logger = fork('logger.js', process.argv.slice(2,process.argv.length), { stdio : 'inherit'});
	
	// Make sure to kill the fit process when node is about to exit
	process.on('SIGINT', function(){
		gracefulExit();
	});
}

function gracefulExit()
{
	console.log("Stopping Data Logger");
	logger.kill('SIGINT');
	console.log("Stopping Server " + __filename);
	process.exit();
}

console.log(__filename + ' connecting to the database...');

var dbCallback = dbCallbackFunction;
connectToDB(dbCallback);

function dbCallbackFunction(err, config) {
	if(err) throw err;
	newConfig = {};
	newConfig.port = null;			// Fix Me... probably not true
	newConfig.db = config;
	newConfig.startupTime = new Date();
	startWebApp(newConfig);
}

function startWebApp(config)
{
	// Defines our webapp routes.
	var app = express();
	// Serves static files from our public subdirectory.
	app.use('/', express.static(__dirname + '/public'));
	// Serves a dynamically generated information page.
	app.get('/about', function(req, res) { return about(req,res,config); });
	if(config.db) {
		dataPacketModel = config.db.dataPacketModel;
		// Serves boot packet info.
		app.get('/boot', function(req,res) { return boot(req,res,config.db.bootPacketModel); });
		// Serves data dynamically via AJAX.
		// app.get('/fetch', function(req,res) { return fetch(req,res,config.db.dataPacketModel); });
		app.get('/fetch', function(req,res) {
			if(!fetchWorkerReady){
				// Create new Worker
				if(debug) console.log("Offloading to new Worker");
				fetchWorker = fork('fetch.js', process.argv.slice(2,process.argv.length), { stdio: 'inherit'});

				// Listen for ready signal and done response
				fetchWorker.once('message', function(message){
					if(message.ready){
						fetchWorkerReady = message.ready;
						// Send query when we're ready
						fetchWorker.send({
							'query' : req.query,
						});
						fetchWorker.once('message', function(message){
							if(message.done){
								if(debug) console.log("Done Fetching, send to page");
								res.send(message.results);
							}
						});
					}
				});

				fetchWorker.on('exit', function(code, signal){
					if(debug) console.log("Code : " + code);
					if(debug) console.log("Signal : " + signal);
				});
			} else {
				// Use existing workera
				if(debug) console.log("Offloading to existing worker");
				// Already ready, send the query
				fetchWorker.send({
					'query' : req.query,
				});
				fetchWorker.once('message', function(message){
					if(message.done){
						if(debug) console.log("Done Fetching, send to page");
						res.send(message.results);
					}
				});
			}
		});

	}
	// Starts our webapp.
	console.log('starting web server on port 3000');
	app.listen(3000);
}

// Responds to a request for our about page.
function about(req,res,config) {
	res.send(
		'Server started ' + config.startupTime + '<br/>' +
		'Server command line: ' + process.argv.join(' ') + '<br/>' +
		(config.port === null ? 'No serial connection' : (
			'Connected to tty ' + config.port.path)) + '<br/>' +
		(config.db === null ? 'No database connection' : (
			'Connected to db "' + config.db.connection.name + '" at ' +
			config.db.connection.host + ':' + config.db.connection.port)) + '<br/>'
	);
}

// Responds to a request to view boot info.
function boot(req,res,bootPacketModel) {
	bootPacketModel.find()
		.limit(10).sort([['timestamp', -1]])
		.exec(function(err,results) { res.send(results); });
}

