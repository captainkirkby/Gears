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