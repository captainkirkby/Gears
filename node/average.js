// Averages different data fields over time

// exports.Averager = Averager;
exports.AveragerCollection = AveragerCollection;

// Creates an AveragerCollection object which is a collection of averagers
// Inputs an array of integers
function AveragerCollection(binSizes){
	this.collection = [];
	for(var i=0;i<binSizes.length;i++){
		this.collection[i] = new Averager(binSizes[i]);
	}
}

AveragerCollection.prototype.input = function(obj,saveCallback) {
	for(var i=0;i<this.collection.length;i++){
		if(this.collection[i]) {
			// If it exists, call the input function
			this.collection[i].input(obj,saveCallback);
		}
	}
};

// Creates and averager object and initializes it with a period to average over
function Averager(seconds){
	this.clear();
	this.date = null;
	this.seconds = seconds;
}

// Inputs a JSON structure of values to average
Averager.prototype.input = function(obj,saveCallback) {
	for(var key in obj){
		if(obj[key] instanceof Date){
			// Set new date each time
			this.lastDate = this.date;
			this.date = obj[key];
			// console.log("New Date: " + this.date);
		} else if(!this.data[key]) {
			// Create new entry in data
			this.data[key] = obj[key];
			this.counts[key] = 1;
			// console.log("New Entry: " + typeof obj[key]);
		} else if(key == 'minorAlarms') {
			// If there is an existing minor alarm in this bin
			// do a 'bitwise average' (set new bits preserve old ones)
			// e.g. first entry: 1000010010 second entry: 1000001101
			// Result is 1000011111
			this.data[key] = this.data[key] | obj[key];
			// Leave counts at 1 so averaging has no effect.
		} else {
			// Add to existing entry in data
			this.data[key] += obj[key];
			this.counts[key] += 1;
			// console.log("Add to Existing Entry: " + obj[key]);
		}
	}
	if(saveCallback && topOfClock(this.date, this.lastDate, this.seconds)){
		var dataToSave = {};
		for(var field in this.data){
			// Average each data member
			dataToSave[field] = this.data[field]/this.counts[field];
		}
		// Append date to data packet
		dataToSave["timestamp"] = this.date;
		dataToSave["averagingPeriod"] = this.seconds;
		//console.log("Averaging With Period " + this.seconds);
		// Clear data structure while saving the date and seconds
		this.clear();
		// Save to Database
		saveCallback(dataToSave);
	}
};

Averager.prototype.clear = function(seconds) {
	this.data = {};
	this.counts = {};
	this.lastDate = null;
};

// LastDate guaranteed to be before date
function topOfClock(date,lastDate,seconds){
	if(!date || !lastDate) return false;
	var sec = parseFloat(date.getTime()/1000).toFixed(0)%seconds;
	return (date.getTime() > lastDate.getTime()+1000*seconds || sec < parseFloat(lastDate.getTime()/1000).toFixed(0)%seconds);
}
