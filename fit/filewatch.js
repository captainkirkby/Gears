var fs = require('fs');
var spawn = require('child_process').spawn;


var fit = spawn("./say.py", [], { stdio : 'pipe'});
fit.stdout.pipe(process.stdout);

// HUGE debounce time
var debounceTime = 2000;
var s = 0;
fs.watch("template.dat",{ persistent: true}, function(event,filename){
	if(s === 0){
		s = 1;
		setTimeout(function(){
			console.log("File " + filename + " Changed by event " + event);
			fit.kill();
			fit = spawn("./say.py", [], { stdio : 'pipe'});
			fit.stdout.pipe(process.stdout);
			s = 0;
		}, debounceTime);
	}
});
