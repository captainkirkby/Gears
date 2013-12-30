$(function() {

	function displayRecentData(){
		var dataURL = "fetch";
		
		function onDataRecieved(data){
			var temperatureSet = [];
			var pressureSet = [];

			// Number of samples to average with (must be an odd number)
			var NUM_TEMPERATURE_SAMPLES = 11;		// o o X o o
			var NUM_PRESSURE_SAMPLES = 55;

			function smoothPoints(field, index, smoothingRadius){
				console.log(index);
				console.log(smoothingRadius);
				if(smoothingRadius < 1){
					console.log("Smallest radius reached");
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
						console.log("Decreasing radius");
						return smoothPoints(field, index, smoothingRadius-2);
					} else {
						sum += data[t_index][field];
					}
				}

				return sum/smoothingRadius;
			}

			// Iterate through retrieved data
			for(var index=0;index<data.length;index++){
				//Get timestamp as a date object
				var date = new Date(Date.parse(data[index].timestamp));

				temperatureSet.push([date, smoothPoints("temperature", index, smoothing ? NUM_TEMPERATURE_SAMPLES : 0)]);
				pressureSet.push([date, smoothPoints("pressure", index, smoothing ? NUM_PRESSURE_SAMPLES : 0)]);
			}
			
			$.plot("#placeholder", [
				{ data: temperatureSet, label: "Temperature (°C)" },
				{ data: pressureSet, label: "Pressure (Pa)", yaxis: 2 }
			], {
				xaxes : [{ mode: "time", timezone: "browser"}],		//must include jquery.flot.time.min.js for this!
				yaxes : [{}, {position: "right"}]
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


	function continuousUpdate(){
		if(realTimeUpdates){
			displayRecentData();
			setTimeout(continuousUpdate, TIMEOUT_VALUE);
		}
	}

	$("#fetch").click(displayRecentData);

	$("#continuousMode").click(function(){
		if(!realTimeUpdates){
			realTimeUpdates = true;
			$(this).val("Stop Real Time Updates");
		} else {
			realTimeUpdates = false;
			$(this).val("Start Real Time Updates");
		}
		continuousUpdate();
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

	displayRecentData();
});

function zeroArray(myArray){
	for(var i=0;i<myArray.length;i++){
		myArray[i] = 0;
	}
}