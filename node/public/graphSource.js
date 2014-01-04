$(function() {

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
		
		//alert(dataURL);
		var NUM_TEMPERATURE_SAMPLES = 11;		// o o X o o
		var NUM_PRESSURE_SAMPLES = 55;

		//displaySet("name")

		function onDataRecieved(data){
			var dataSet = [];
			// var temperatureSet = [];
			// var pressureSet = [];

			// var smoothTemperatureSet = [];
			// var smoothPressureSet = [];

			// // Number of samples to average with (must be an odd number)
			// var NUM_TEMPERATURE_SAMPLES = 11;		// o o X o o
			// var NUM_PRESSURE_SAMPLES = 55;

			var count = 0;

			displaySet("temperature", smoothing, NUM_TEMPERATURE_SAMPLES);
			displaySet("pressure", smoothing, NUM_PRESSURE_SAMPLES);
			displaySet("irLevel", smoothing, NUM_PRESSURE_SAMPLES);
			displaySet("thermistor", smoothing, NUM_PRESSURE_SAMPLES);
			displaySet("humidity", smoothing, NUM_PRESSURE_SAMPLES);


			function displaySet(name, dataSmoothing, smoothingAmount){
				var set = [];
				var smoothSet = [];
	
				// Iterate through retrieved data
				for(var index=0;index<data.length;index++){
					//Get timestamp as a date object
					var date = new Date(Date.parse(data[index].timestamp));
	
					set.push([date, data[index][name]]);
					smoothSet.push([date, smoothPoints(name, index, smoothingAmount)]);
				}

				dataSet.push({ data: dataSmoothing ? smoothSet : set , label: generateLabel(name), yaxis: (count + 1), color : count });

				if(smoothing){
					dataSet.push({ data: set, lines : { show : false}, points : {show : true, radius : 1}, yaxis: (count + 1), color : count });
				}

				count++;
			}

			function generateLabel(name){
				var lookUpTable = { "temperature"	: "Temperature (°C)",
									"pressure"		: "Pressure (Pa)",
									"irLevel"		: "IR Level (V)",
									"thermistor"	: "Thermistor (°C)",
									"humidity"		: "Humidity (%)" };
				
				var result = lookUpTable[name];
				if(!result) result = "UNKNOWN";
				return result;
			}

			function smoothPoints(field, index, smoothingRadius){
				//console.log(index);
				//console.log(smoothingRadius);
				if(smoothingRadius < 1){
					//console.log("Smallest radius reached");
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

			// // Iterate through retrieved data
			// for(var index=0;index<data.length;index++){
			// 	//Get timestamp as a date object
			// 	var date = new Date(Date.parse(data[index].timestamp));

			// 	temperatureSet.push([date, data[index].temperature]);
			// 	pressureSet.push([date, data[index].pressure]);

			// 	smoothTemperatureSet.push([date, smoothPoints("temperature", index, NUM_TEMPERATURE_SAMPLES)]);
			// 	smoothPressureSet.push([date, smoothPoints("pressure", index, NUM_PRESSURE_SAMPLES)]);
			// }

			// dataSet.push({ data: smoothing ? smoothTemperatureSet : temperatureSet , label: "Temperature (°C)", color : 0 },
			// 	{ data: smoothing ? smoothPressureSet : pressureSet, label: "Pressure (Pa)", yaxis: 2, color : 1 });

			// if(smoothing){
			// 	dataSet.push({ data: temperatureSet, lines : { show : false}, points : {show : true, radius : 1}, color : 0},
			// 	{ data: pressureSet, yaxis: 2, lines : { show : false}, points : {show : true, radius : 1}, color : 1  });
			// }
			
			$.plot("#placeholder", dataSet, {
				xaxes : [{ mode: "time", timezone: "browser" }],		//must include jquery.flot.time.min.js for this!
				yaxes : [{}, { position: "right" }],
				legend: {show : true, position : "nw" }
			});

		}

		$.ajax({
			url:dataURL,
			type:"GET",
			dataType:"json",
			success:onDataRecieved
		});
	}

	var TIMEOUT_VALUE = 333;
	var realTimeUpdates = false;

	var smoothing = false;


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
			if(!isNaN(parseInt(lengthValue))){
				length = Math.abs(parseInt(lengthValue)) * -1;		//ensure a negative number
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
