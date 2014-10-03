var connectToDB = require('./dbConnection').connectToDB;
var winston_module = require('winston');

// Log to file
var winston = new (winston_module.Logger)({
	transports: [
		new (winston_module.transports.Console)({ level: 'warn' }),
		new (winston_module.transports.File)({ filename: 'ticktock.log', level: 'verbose' })
	]
});

winston.verbose("Worker Starting!");
winston.verbose(__filename + ' connecting to the database...');

var dbCallback = dbCallbackFunction;
connectToDB(dbCallback);

function dbCallbackFunction(err, config) {
	if(err) throw err;
	// Do things
}
