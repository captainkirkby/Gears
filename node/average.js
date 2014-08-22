// Averages different data fields over time

exports.Averager = Averager;

// Creates and averager object and initializes it with a period to average over
function Averager(seconds){
	this.data = {};
	this.date = null;
	this.lastDate = null;
	this.count = 0;
	this.seconds = seconds;
}

// Inputs a JSON structure of values to average
Averager.prototype.input = function(obj,saveCallback) {
	this.count++;
	for(var key in obj){
		if(obj[key] instanceof Date){
			// Set new date each time
			this.lastDate = this.date;
			this.date = obj[key];
			// console.log("New Date: " + this.date);
		} else if(!this.data[key]) {
			// Create new entry in data
			this.data[key] = obj[key];
			// console.log("New Entry: " + typeof obj[key]);
		} else {
			// Add to existing entry in data
			this.data[key] += obj[key];
			// console.log("Add to Existing Entry: " + obj[key]);
		}
	}
	if(topOfClock(this.date, this.lastDate, this.seconds)){
		var dataToSave = {};
		for(var field in this.data){
			// Average each data member
			dataToSave[field] = this.data[field]/this.count;
		}
		// Append date to data packet
		dataToSave["timestamp"] = this.date;
		dataToSave["averagingPeriod"] = this.seconds;
		console.log(dataToSave);
		// Save to Database
		saveCallback(dataToSave);
	}
};

// LastDate guaranteed to be before date
function topOfClock(date,lastDate,seconds){
	if(!date || !lastDate) return false;
	var sec = parseFloat(date.getTime()/1000).toFixed(0)%seconds;
	return (sec >= 0 && sec < parseFloat(lastDate.getTime()/1000).toFixed(0)%seconds);
}
