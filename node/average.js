// Averages different data fields over time

exports.Averager = Averager;

function Averager(){
	this.data = {};
	this.date = null;
	this.lastDate = null;
	this.count = 0;
}

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
	if(topOfTheMinute(this.date, this.lastDate)){
		for(var field in this.data){
			// Average each data member
			this.data[field] = this.data[field]/this.count;
		}
		console.log(this.data);
		// Save to Database!
		saveCallback(this.data);
	}
};

// LastDate guaranteed to be before date
function topOfTheMinute(date,lastDate){
	if(!date || !lastDate) return false;
	return (date.getSeconds() > 0 && date.getSeconds() < lastDate.getSeconds());
}
