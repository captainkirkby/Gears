$(function() {

	// Reverse Order
	var seriesToPlot = ["temperature", "pressure", "irLevel", "thermistor", "humidity"];

	// Drawing Constants
	var NORMAL = 2;
	var BOLD = 3;

	// Recent Fetch Stack
	var stack = [];
	var maxStackHeight = 1000;

	// Plot Settings
	var dataToPlot = {
		"temperature" : { "visible" : true, "width" : NORMAL, "color" : 0},
		"pressure" : { "visible" : true, "width" : NORMAL, "color" : 1},
		"irLevel" : { "visible" : true, "width" : NORMAL, "color" : 2},
		"thermistor" : { "visible" : true, "width" : NORMAL, "color" : 3},
		"humidity" : { "visible" : true, "width" : NORMAL, "color" : 4}
	};

	// Store last request parameters
	var lastFrom;
	var lastTo;

	// Smoothing factors
	var NUM_TEMPERATURE_SAMPLES = 11;		// o o X o o
	var NUM_PRESSURE_SAMPLES = 55;
	var NUM_IRLEVEL_SAMPLES = 55;
	var NUM_THERMISTOR_SAMPLES = 55;
	var NUM_HUMIDITY_SAMPLES = 55;

	// Real time parameters
	var TIMEOUT_VALUE = 333;
	var realTimeUpdates = false;

	// Smoothing or not
	var smoothing = false;

	function smoothingForSet(set){
		var lookUpTable = {
			"temperature" : NUM_TEMPERATURE_SAMPLES,
			"pressure" : NUM_PRESSURE_SAMPLES,
			"irLevel" : NUM_IRLEVEL_SAMPLES,
			"thermistor" : NUM_THERMISTOR_SAMPLES,
			"humidity" : NUM_HUMIDITY_SAMPLES
		};
		var result = lookUpTable[set];
		if(!result) result = 0;
		return result;
	}

	function togglePlot(seriesID){
		dataToPlot[seriesID].visible = !dataToPlot[seriesID].visible;
		displayData(lastFrom, lastTo);
	}

	//Draw on top of others
	function boldPlot(seriesID, bold){
		// Reset boldness of all series
		$.each(dataToPlot, function(series, options) {
			dataToPlot[series].width = NORMAL;
		});
		// Bold the series we're interested in
		dataToPlot[seriesID].width = bold ? BOLD : NORMAL;
		// Change order of data to plot
		seriesToPlot.push(seriesToPlot.splice(seriesToPlot.indexOf(seriesID),1)[0]);
		displayData(lastFrom, lastTo);
	}

	function generateLabel(name){
		var lookUpTable = { "temperature"	: "Temperature (°C)",
							"pressure"		: "Pressure (Pa)",
							"irLevel"		: "IR Level (V)",
							"thermistor"	: "Thermistor (°C)",
							"humidity"		: "Humidity (%)" };
		
		var result = lookUpTable[name];
		if(!result) {
			$.each(lookUpTable, function(key, value){
				if(value == name){
					result = key;
				} else if(key == name){
					result = value;
				}
			});
		}
		return result;
	}

	// Save data in browser so if we are doing same request we don't have to re-fetch from server (caching does this maybe?)
	function displayData(from, to){
		if(from === undefined){
			from = "DEFAULT";			// Use server default
		}

		if(to === undefined){
			to = "now";
		}

		var dataURL = "fetch?" + $.param({
			"from" : from,
			"to" : to
		});

		lastFrom = from;
		lastTo = to;

		var POINT_SIZE = 0.5;

		function onDataRecieved(data){
			var dataSet = [];
			var YAxesSet = [];

			var count = 0;
			var axesCount = 0;

			$.each(seriesToPlot, function(index, set){
				displaySet(set, smoothing, smoothingForSet(set), dataToPlot[set].visible, dataToPlot[set].width);
			});


			function displaySet(name, dataSmoothing, smoothingAmount, visible, width){
				var set = [];
				var smoothSet = [];
	
				// Iterate through retrieved data
				for(var index=0;index<data.length;index++){
					//Get timestamp as a date object
					var date = new Date(Date.parse(data[index].timestamp));
	
					set.push([date, data[index][name]]);
					smoothSet.push([date, smoothPoints(name, index, smoothingAmount)]);
				}

				dataSet.push({	data: dataSmoothing ? smoothSet : set,
								lines : { lineWidth : width, show : visible },
								label: generateLabel(name),
								yaxis: (count + 1),
								color : dataToPlot[name].color});

				if(smoothing){
					dataSet.push({	data: set,
									lines : { show : false},
									points : { show : visible, radius : POINT_SIZE},
									yaxis: (count + 1),
									color : dataToPlot[name].color });
				}

				//left or right and visible or invisible
				var orientation = axesCount%2 ? "right" : "left";
				YAxesSet.push({	position : orientation,
								show : visible,
								font : { color : width == BOLD ? "black" : "lightgrey", weight : width == BOLD ? "bold" : "normal"}});

				count++;
				if(visible) axesCount++;
			}

			function smoothPoints(field, index, smoothingRadius){
				if(smoothingRadius < 1){
					return data[index][field];
				}

				if(smoothingRadius%2 === 0){
					smoothingRadius += 1;
				}

				var averageSpace = Math.floor(smoothingRadius/2);
				var sum = 0;
				
				for(var i=(-1*averageSpace);i<=averageSpace;i++){
					var t_index = i+index;
					if(t_index<0 || t_index>=data.length){
						//console.log("Decreasing radius");
						return smoothPoints(field, index, smoothingRadius-2);
					} else {
						sum += data[t_index][field];
					}
				}

				return sum/smoothingRadius;
			}

			somePlot = $.plot("#placeholder", dataSet, {
				series : { shadowSize : 0},
				xaxes : [{ mode: "time", timezone: "browser" }],		//must include jquery.flot.time.min.js for this!
				yaxes : YAxesSet,
				legend: { show : true, position : "nw", sorted : "ascending"}
			});

		}

		$.ajax({
			url:dataURL,
			type:"GET",
			dataType:"json",
			success:onDataRecieved
		});
	}

	function continuousUpdate(length){
		if(realTimeUpdates){
			displayData(length);
			setTimeout(continuousUpdate, TIMEOUT_VALUE, length);
		}
	}

	function setManualFetchEnabled(enabled){
		$("#fetch").prop('disabled', !enabled);
		$("#from").prop('disabled', !enabled);
		$("#to").prop('disabled', !enabled);
		$("#length").prop('disabled', !enabled);
	}

	$("#placeholder").on("click", ".legendColorBox",function(){
		$(this).css('opacity', '0.50');
		var clickedField = generateLabel($(this).next().text());
		togglePlot(clickedField);
	});

	$("#placeholder").on("mouseenter", ".legendColorBox", function(){
		//alert("Mouse Entered!");
		var clickedField = generateLabel($(this).next().text());
		boldPlot(clickedField, true);
	});

	$("#placeholder").on("mouseleave", ".legendColorBox", function(){
		//alert("Mouse Entered!");
		var clickedField = generateLabel($(this).next().text());
		boldPlot(clickedField, false);
	});

	$("#fetch").click(function(){
		var from = new Date($("#from").val());
		var to = new Date($("#to").val());

		var fromArg;
		var toArg;

		// Check for valid dates
		if(!isNaN(from)){
			fromArg = from.toISOString();
		}
		if(!isNaN(to)){
			toArg = to.toISOString();
		}

		displayData(fromArg,toArg);
	});

	$("#continuousMode").click(function(){
		var length;
		if(!realTimeUpdates){
			// Real Time On
			realTimeUpdates = true;
			$(this).val("Stop Real Time Updates");
			// Get value of length field
			var lengthValue = $("#length").val();
			if(!isNaN(parseInt(lengthValue,10))){
				length = Math.abs(parseInt(lengthValue,10)) * -1;		//ensure a negative number
			}
			// disable manual fetch
			setManualFetchEnabled(false);
		} else {
			// Real Time Off
			realTimeUpdates = false;
			$(this).val("Start Real Time Updates");
			// enable manual fetch
			setManualFetchEnabled(true);
		}

		continuousUpdate(length);
	});

	$("#toggleSmoothing").click(function(){
		if(smoothing){
			$(this).val("Use Smoothing");
			smoothing = false;
		} else {
			$(this).val("Don't Use Smoothing");
			smoothing = true;
		}
	});

	displayData();
	setManualFetchEnabled(true);
});
