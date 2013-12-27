// Retrieve
var MongoClient = require('mongodb').MongoClient;

// Connect to the db
MongoClient.connect("mongodb://localhost:27017/BMPReadout", function(err, db) {
	if(err) throw err;
	console.log("We are connected");

	var collection = db.collection('data');

	collection.find().toArray(function(err, items) {
		console.log(items);
	});

	var doc2 = {'temperature':22.0,'pressure':101100};

	//collection.insert();

	collection.insert(doc2, {w:1}, function(err, result) {});
});
