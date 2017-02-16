// Implements the Web Server Component of the clock metrology project
// Created by D & D Kirkby, Dec 2013


var express = require('express');
var winston_module = require('winston');

var fork = require('child_process').fork;
var exec = require('child_process').exec;

var connectToDB = require('./dbConnection').connectToDB;

// Global status for fetchWorker
var fetchWorker = null;
var fetchWorkerReady = false;

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
var pythonFlags = ["--load-template", "template.dat"];
var service = false;
process.argv.forEach(function(val,index,array) {
	if(val == '--no-serial') noSerial = true;
	else if(val == '--no-database') noDatabase = true;
	else if(val == '--debug') {
		winston.transports.console.level = 'debug';
		winston.transports.file.level = 'debug';
		debug = true;
	}	else if(val == '--service') service = true;
	else if(val == '--physical')  pythonFlags = ["--physical"];
});

// Assumption: this command is being called with cwd /path/to/Gears/node
if(!noSerial && !service){
	// Start process with data pipes
	var logger = fork('./logger.js', process.argv.slice(2,process.argv.length), { stdio : 'inherit'});

	// Thank you to https://www.exratione.com/2013/05/die-child-process-die/ for the following snippets
	// Helper function added to the child process to manage shutdown.
	logger.onUnexpectedExit = function (code, signal) {
		winston.error("Child process terminated with code: " + code);
		process.exit(1);
	};
	logger.on("exit", logger.onUnexpectedExit);

	// A helper function to shut down the child.
	logger.shutdown = function () {
		winston.info("Stopping Data Logger");
		// Get rid of the exit listener since this is a planned exit.
		this.removeListener("exit", this.onUnexpectedExit);
		this.kill("SIGTERM");
	};
	// The exit event shuts down the child.
	process.once("exit", function () {
		logger.shutdown();
	});
	// This is a somewhat ugly approach, but it has the advantage of working
	// in conjunction with most of what third parties might choose to do with
	// uncaughtException listeners, while preserving whatever the exception is.
	process.once("uncaughtException", function (error) {
		// If this was the last of the listeners, then shut down the child and rethrow.
		// Our assumption here is that any other code listening for an uncaught
		// exception is going to do the sensible thing and call process.exit().
		if (process.listeners("uncaughtException").length === 0) {
			logger.shutdown();
			throw error;
		}
	});
}

// Handle Exit Signals
process.once('SIGTERM', function(){
	winston.info("Stopping Server " + __filename);
	process.exit(0);
});
process.once('SIGINT', function(){
	winston.info("Stopping Server " + __filename);
	process.exit(0);
});

winston.verbose(__filename + ' connecting to the database...');

var dbCallback = dbCallbackFunction;
connectToDB(dbCallback);

function dbCallbackFunction(err, config) {
	if(err) {
		throw err;
	}
	newConfig = {};
	newConfig.db = config;
	newConfig.startupTime = new Date();

	// Start the webapp once we have the serial port
	var portChild = exec("echo -n '/dev/' && dmesg | egrep 'FTDI.*tty' | rev | cut -d' ' -f1 | rev",
		function (error, stdout, stderr) {
			if (error) throw error;
			newConfig.portPath = stdout.trim();
			startWebApp(newConfig);
		}
	);
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
		// Serve requests to update template
		app.get('/template', function(req,res) { return updateTemplate(req,res,config.db.dataPacketModel); });
		// Serve requests to update template
		app.get('/setIRLength', function(req,res) { return setIRLength(req,res); });
		// Serves data dynamically via AJAX.
		// app.get('/fetch', function(req,res) { return fetch(req,res,config.db.dataPacketModel); });
		app.get('/fetch', function(req,res) {
			if(!fetchWorkerReady){
				// Create new Worker
				winston.verbose("Offloading to new Worker");
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
								winston.verbose("Done Fetching, send to page");
								res.send(message.results);
							}
						});
					}
				});

				fetchWorker.on('exit', function(code, signal){
					winston.verbose("Code : " + code);
					winston.verbose("Signal : " + signal);
				});
			} else {
				// Use existing workera
				winston.verbose("Offloading to existing worker");
				// Already ready, send the query
				fetchWorker.send({
					'query' : req.query,
				});
				fetchWorker.once('message', function(message){
					if(message.done){
						winston.verbose("Done Fetching, send to page");
						res.send(message.results);
					}
				});
			}
		});

	}
	// Starts our webapp.
	winston.info('starting web server on port 3000');
	app.listen(3000);
}

// Responds to a request for our about page.
function about(req,res,config) {
	res.send(
		'Server started ' + config.startupTime + '<br/>' +
		'Server command line: ' + process.argv.join(' ') + '<br/>' +
		(config.portPath === null ? 'No serial connection' : (
			'Connected to tty ' + config.portPath)) + '<br/>' +
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

// Updates the template file from the web server
function updateTemplate(req,res,dataPacketModel) {
	// console.log(dataPacketModel.collection.name);
	var update = exec("../fit/fit.py --from-db --save-template db", { cwd : "../fit" });
	res.send("");
}

// Run the SetIRLength.sh script
function setIRLength(req, res) {
	if (!isNaN(req.query.irlength)) {
		exec("echo " + req.query.irlength + " > IRLength.txt", {cwd : ".."},
			function (error, stdout, stderr) {
				if (error)
					throw error;
				exec("./SetIRLength.sh", {cwd : "../mcu"},
					function (error, stdout, stderr) {
						if (error)
							throw error;
						res.send("Successfully updated IR Length!");
					}
				);
			}
		);
	}
	else {
		res.send("Invalid Request");
	}
}

