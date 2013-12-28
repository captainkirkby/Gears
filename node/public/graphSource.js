$(function() {

	function displayRecentData(){
		var dataURL = "recent";

		function onDataRecieved(data){
			var temperatureSet = [];
			var pressureSet = [];

			$.each(data, function(key, value){
				temperatureSet.push([key, value.temperature]);
				pressureSet.push([key, value.pressure]);
			});
			
			$.plot("#placeholder", [
				{ data: temperatureSet, label: "Temperature (°C)" },
				{ data: pressureSet, label: "Pressure (Pa)", yaxis: 2 }
			], {
				//xaxes : [{ mode: "time"}],		//must include jquery.flot.time.min.js for this!
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

	function continuousUpdate(){
		displayRecentData();
		setTimeout(continuousUpdate, TIMEOUT_VALUE);
	}

	$("#fetch").click(displayRecentData);

	$("#continuousMode").click(continuousUpdate);

	$("#footer").prepend("Flot " + $.plot.version + " &ndash; ");

	displayRecentData();
});