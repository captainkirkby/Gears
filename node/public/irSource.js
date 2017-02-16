$(function() {

	// Reverse Order
	var seriesToPlot = ["raw"];

	// Drawing Constants
	var NORMAL = 2;
	var BOLD = 3;

	// Tick significant figures
	var TICK_SIG_FIGS = 6;

	// Plot Settings
	var dataToPlot = {
		"raw" : { "visible" : true, "width" : NORMAL, "color" : 0}
	};

	// Store last request parameters
	var lastDate;

	// Real time parameters
	var TIMEOUT_VALUE = 333;
	var realTimeUpdates = false;

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
		var lookUpTable = { "raw" : "IR Level"};
		
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

	function displayData(date){
		var from;
		var to;

		if(date === undefined){
			from = "-1";			// Last data point
		}

		var dataURL = "fetch?" + $.param({
			"from" : from,
			"to" : to,
			"series" : dataToPlot,
			"db" : "raw"
		});

		lastFrom = from;
		lastTo = to;

		var POINT_SIZE = 0.5;

		function onDataRecieved(data){
			//console.log(data[0]);
			var dataSet = [];
			var YAxesSet = [];

			var count = 0;
			var axesCount = 0;

			$('#irLength').attr('placeholder', data[0]['raw'].length);

			$.each(seriesToPlot, function(index, set){
				displaySet(set, dataToPlot[set].visible, dataToPlot[set].width);
			});

			function displaySet(name, visible, width){
				var set = [];
	
				// Iterate through retrieved data
				for(var index=0;index<data[count][name].length;index++){
					// Get timestamp as a date object
					var date = new Date(Date.parse(data[count].timestamp));

					set.push([index, data[count][name][index]]);
				}
				//console.log(set);

				dataSet.push({	data: set,
								lines : { lineWidth : width, show : visible },
								label: generateLabel(name),
								yaxis: (count + 1),
								color : dataToPlot[name].color});

				count++;
			}

			somePlot = $.plot("#placeholder", dataSet, {
				series : { shadowSize : 0},
				xaxes : [{ timezone: "browser" }],
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

	function continuousUpdate(){
		if(realTimeUpdates){
			displayData();
			setTimeout(continuousUpdate, TIMEOUT_VALUE);
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

// Note: The following two bindings give a considerable performance hit, and in my case, the click no longer fired.
/*
	$("#placeholder").on("mouseenter", ".legendColorBox", function(){
		console.log("Mouse Entered!");
		var clickedField = generateLabel($(this).next().text());
		boldPlot(clickedField, true);
	});

	$("#placeholder").on("mouseleave", ".legendColorBox", function(){
		//alert("Mouse Entered!");
		var clickedField = generateLabel($(this).next().text());
		boldPlot(clickedField, false);
	});
*/

	$("#fetchLatest").click(function(){
		displayData();
	});

	$("#continuousMode").click(function(){
		var length;
		if(!realTimeUpdates){
			// Real Time On
			realTimeUpdates = true;
			$(this).val("Stop Real Time Updates");
			// disable manual fetch
			setManualFetchEnabled(false);
		} else {
			// Real Time Off
			realTimeUpdates = false;
			$(this).val("Start Real Time Updates");
			// enable manual fetch
			setManualFetchEnabled(true);
		}

		continuousUpdate();
	});

	$('#setIRLength').click(function(){
		var irLength = $('#irLength').val();
		if (!isNaN(parseInt(irLength, 10))) {
			$.confirm({
				title : 'Confirm Set IR Length to ' + irLength,
				content : 'This will trigger a microcontroller reflash and take ~20s.  Are you sure you want to proceed?',
				boxWidth: '30%',
				buttons : {
					confirm : function () {
						confirmFunction(parseInt(irLength, 10));
					},
					cancel : function() {
						cancelFunction();
					}
				}
			});
		}
	});

	function confirmFunction(length) {
		var setIRLengthURL = "setIRLength?" + $.param({
			"irlength" : length
		});

		// Disable Buttons
		$("#fetchLatest").prop('disabled', true);
		$("#continuousMode").prop('disabled', true);
		$("#setIRLength").prop('disabled', true);
		$("#irLength").prop('disabled', true);
		$(".loading").show();
		
		$.ajax({
			url:setIRLengthURL,
			type:"GET",
			dataType:"json",
			complete:setIRLengthComplete
		});
	}

	function setIRLengthComplete() {
		// Reenable buttons
		$("#fetchLatest").prop('disabled', false);
		$("#continuousMode").prop('disabled', false);
		$("#setIRLength").prop('disabled', false);
		$("#irLength").prop('disabled', false);
		$(".loading").hide();

		$.alert({
			title: 'Success!',
			content: 'Microcontroller Successfully Reprogrammed',
			autoClose: 'ok|2000',
			buttons: {
				ok: {
					text: 'OK'
				}
			}
		});
	}

	function cancelFunction() {
		return;
	}

	displayData();
	setManualFetchEnabled(true);
});
