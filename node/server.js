// Implements the combined data logger and web server component of the clock metrology project.
// Created by D & D Kirkby, Dec 2013

async = require('async');

async.parallel({
	port: function(callback) {
		setTimeout(function() {
			callback(null,'port');
		},200);
	},
	db: function(callback) {
		setTimeout(function() {
			callback(null,'db');
		},400);
	}},
	function(err,config) {
		console.log(config);
	}
);
