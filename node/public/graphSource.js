$(function() {

	function displayRecentData(){
		var dataURL = "recent";

		function onDataRecieved(data){
			var temperatureSet = [];
			var pressureSet = [];

			// Number of samples to average with
			var NUM_TEMPERATURE_SAMPLES = 5;		// o o X o o
			var NUM_PRESSURE_SAMPLES = 5;

			// Iterate through retrieved data
			for(var index=0;index<data.length;index++){
				//Get timestamp as a date object
				var date = new Date(Date.parse(data[index].timestamp));

				var smoothing = true;

				var temperature = NaN;
				var pressure = NaN;

				if(smoothing){

					var temperatureSum = 0;
					var pressureSum = 0;

					var temperatureAverageSpace = Math.floor(NUM_TEMPERATURE_SAMPLES/2);
					var pressureAverageSpace = Math.floor(NUM_PRESSURE_SAMPLES/2);

					// Average smooth temperature and pressure
					for(var i=(-1*temperatureAverageSpace);i<temperatureAverageSpace;i++){
						var t_index = i+index;
						if(t_index<0 || t_index>=data.length){
							temperature = data[index].temperature;
						} else {
							temperatureSum += data[t_index].temperature;
						}
					}
					for(var j=(-1*pressureAverageSpace);j<pressureAverageSpace;j++){
						var p_index = j+index;
						if(p_index<0 || p_index>=data.length){
							pressure = data[index].pressure;
						} else {
							pressureSum += data[p_index].pressure;
						}
					}

					// If average was taken, use it.
					if(isNaN(temperature)){
						temperature = temperatureSum/NUM_TEMPERATURE_SAMPLES;
					}

					if(isNaN(pressure)){
						pressure = pressureSum/NUM_PRESSURE_SAMPLES;
					}
				} else {
					temperature = data[index].temperature;
					pressure = data[index].pressure;
				}

				temperatureSet.push([date, temperature]);
				pressureSet.push([date, pressure]);
			}

			// $.each(data, function(key, value){
			//	var d = new Date(Date.parse(value.timestamp));

			//	temperatureSet.push([d, value.temperature]);
			//	pressureSet.push([d, value.pressure]);
			// });
			
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

	$("#footer").prepend("Flot " + $.plot.version + " &ndash; ");

	displayRecentData();
});

function zeroArray(myArray){
	for(var i=0;i<myArray.length;i++){
		myArray[i] = 0;
	}
}