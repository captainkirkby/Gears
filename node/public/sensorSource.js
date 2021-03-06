$(function() {

	// Reverse Order
	var seriesToPlot = ["boardTemperature", "pressure", "crudePeriod", "blockTemperature", "humidity", "refinedPeriod", "angle", "height"];

	// Drawing Constants
	var NORMAL = 2;
	var BOLD = 3;

	// Recent Fetch Cache
	var cache = {};
	var MAX_CACHE_LENGTH = 500;

	// Tick significant figures
	var TICK_SIG_FIGS = 6;

	// Whether or not to show the dots on a smoothed curve
	var showDots = true;

	// Plot Settings
	var dataToPlot = {
		"boardTemperature" : { "visible" : false, "width" : NORMAL, "color" : 0, "autoID" : "#boardTempEnableAuto",
			"lowID" : "#lowBoardTemp", "highID" : "#highBoardTemp"},
		"pressure" : { "visible" : false, "width" : NORMAL, "color" : 1, "autoID" : "#pressureEnableAuto",
			"lowID" : "#lowPressure", "highID" : "#highPressure"},
		"crudePeriod" : { "visible" : false, "width" : NORMAL, "color" : 2, "autoID" : "#coarsePeriodEnableAuto",
			"lowID" : "#lowCoarsePeriod", "highID" : "#highCoarsePeriod"},
		"blockTemperature" : { "visible" : true, "width" : NORMAL, "color" : 3, "autoID" : "#blockTempEnableAuto",
			"lowID" : "#lowBlockTemp", "highID" : "#highBlockTemp"},
		"humidity" : { "visible" : false, "width" : NORMAL, "color" : 4, "autoID" : "#humidityEnableAuto",
			"lowID" : "#lowHumidity", "highID" : "#highHumidity"},
		"refinedPeriod" : { "visible" : true, "width" : NORMAL, "color" : 6, "autoID" : "#calculatedPeriodEnableAuto",
			"lowID" : "#lowCalculatedPeriod", "highID" : "#highCalculatedPeriod"},
		"angle" : { "visible" : false, "width" : NORMAL, "color" : 7, "autoID" : "#angleEnableAuto",
			"lowID" : "#lowAngle", "highID" : "#highAngle"},
		"height" : { "visible" : false, "width" : NORMAL, "color" : 18, "autoID" : "#heightEnableAuto",
			"lowID" : "#lowHeight", "highID" : "#highHeight"}
	};

	var defaults = null;

	function defaultsSetup(firstTime) {
		var defaultsStr = localStorage.getItem("defaults");
	
		// If no defaults exist, write some
		if (defaultsStr == null) {
			defaults = {
				"angle" : { "Low" : 5,"High" : 25,"Auto" : false},
				"blockTemperature" : { "Low" : 15,"High" : 30,"Auto" : false},
				"boardTemperature" : { "Low" : 15,"High" : 30,"Auto" : false},
				"height" : { "Low" : 200,"High" : 700,"Auto" : false},
				"humidity" : { "Low" : 20,"High" : 50,"Auto" : false},
				"refinedPeriod" : { "Low" : -20,"High" : 20,"Auto" : false},
				"crudePeriod" : { "Low" : 11000,"High" : 13000,"Auto" : false},
				"pressure" : { "Low" : 100000,"High" : 110000,"Auto" : false}
			};
	
			localStorage.setItem("defaults", JSON.stringify(defaults));
		}
		// Otherwise, use the existing defaults
		else {
			defaults = JSON.parse(defaultsStr);
		}
	
		// Default Setup
		$.each(dataToPlot, function(series, options) {
			var highID = dataToPlot[series].highID;
			var lowID = dataToPlot[series].lowID;
			var autoID = dataToPlot[series].autoID;
	
			var defaultHigh = defaults[series].High.toString();
			var defaultLow = defaults[series].Low.toString();
			var defaultAuto = defaults[series].Auto;
	
			$(highID).val(defaultHigh);
			$(lowID).val(defaultLow);
			$(autoID).prop("checked", defaultAuto);
	
			if (firstTime) {
				$(highID).change(function() {
					var high = $(highID).val();
					defaults[series].High = high;
					localStorage.setItem("defaults", JSON.stringify(defaults));
				});
				$(lowID).change(function() {
					var low = $(lowID).val();
					defaults[series].Low = low;
					localStorage.setItem("defaults", JSON.stringify(defaults));
		
				});
				$(autoID).change(function() {
					var auto = $(autoID).prop("checked");
					defaults[series].Auto = auto;
					localStorage.setItem("defaults", JSON.stringify(defaults));
				});
			}

		});
	
		$("#length").prop('disabled', true);
		$("#length").val(120);
	}

	function saveCurrentValues() {
		if (defaults !== null) {
			$.each(dataToPlot, function(series, options) {
				var highID = dataToPlot[series].highID;
				var lowID = dataToPlot[series].lowID;
				var autoID = dataToPlot[series].autoID;
	
				var high = $(highID).val();
				var low = $(lowID).val();
				var auto = $(autoID).prop("checked");
	
				defaults[series].High = high;
				defaults[series].Low = low;
				defaults[series].Auto = auto;

				localStorage.setItem("defaults", JSON.stringify(defaults));
			});
		}

	}

	defaultsSetup(true);

	// Keep doing the default population + reading!

	// Store last request parameters
	var lastFrom;
	var lastTo;

	// Smoothing factors
	var NUM_BOARD_TEMPERATURE_SAMPLES = 30;		// o o X o o
	var NUM_PRESSURE_SAMPLES = 30;
	var NUM_IRLEVEL_SAMPLES = 30;
	var NUM_BLOCK_TEMPERATURE_SAMPLES = 30;
	var NUM_HUMIDITY_SAMPLES = 30;
	var NUM_CRUDE_PERIOD_SAMPLES = 0;
	var NUM_REFINED_PERIOD_SAMPLES = 30;
	var NUM_ANGLE_SAMPLES = 30;
	var NUM_HEIGHT_SAMPLES = 30;

	// Real time parameters
	var TIMEOUT_VALUE = 333;
	var realTimeUpdates = false;

	// Smoothing or not
	var smoothing = false;

	function smoothingForSet(set){
		var lookUpTable = {
			"boardTemperature" : NUM_BOARD_TEMPERATURE_SAMPLES,
			"pressure" : NUM_PRESSURE_SAMPLES,
			"crudePeriod" : NUM_CRUDE_PERIOD_SAMPLES,
			"blockTemperature" : NUM_BLOCK_TEMPERATURE_SAMPLES,
			"humidity" : NUM_HUMIDITY_SAMPLES,
			"refinedPeriod" : NUM_REFINED_PERIOD_SAMPLES,
			"angle" : NUM_ANGLE_SAMPLES,
			"height" : NUM_HEIGHT_SAMPLES
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
		var lookUpTable = { "boardTemperature"	: "Board Temperature",
							"pressure"			: "Pressure",
							"crudePeriod"		: "Coarse Period",
							"blockTemperature"	: "Block Temperature",
							"humidity"			: "Humidity",
							"refinedPeriod"		: "Calculated Period",
							"angle"				: "Calculated Angle",
							"height"			: "Calculated Height"};
		
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

	function cacheDataPoint(date, dataPoint){
		// Insert data point in beginning of cache, pop from end if we exceed the cache limit
		if(cache.unshift({key : date, value : dataPoint}) > MAX_CACHE_LENGTH){
			cache.pop();
		}
	}

	function hasCachedPointForDate(date){
		return date in cache;
	}

	// Save data in browser so if we are doing same request we don't have to re-fetch from server (caching does this maybe?)
	function displayData(from, to){
		// Figure out if we already have the data we need in our cache and make the modified request

		if(from === undefined){
			from = "";			// Use server default
		}

		if(to === undefined){
			to = "now";
		}

		var dataURL = "fetch?" + $.param({
			"from" : from,
			"to" : to,
			"series" : dataToPlot
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

			// // Cache data point if we had to retrieve it
			// $.each(data, function(index, set){
			// 	cacheDataPoint(data[index].timestamp, data[index]);
			// });


			function displaySet(name, dataSmoothing, smoothingAmount, visible, width){

				var set = [];
				var smoothSet = [];
				var smoothingRadius = (smoothingAmount - 1)/2;
	
				// Iterate through retrieved data
				for(var index=0;index<data.length;index++){
					// Get timestamp as a date object
					var date = new Date(Date.parse(data[index].timestamp));

					// If we're looking at the refined period, subract 2, divide by 2, and multiply by a million
					if(name == "refinedPeriod"){
						if(data[index][name] > 0){
							set.push([date, (data[index][name]-2)*500000]);
							if (index > smoothingRadius && index < data.length - smoothingRadius) {
								smoothSet.push([date, (smoothPoints(name, index, smoothingAmount)-2)*500000]);
							}
						}
					} else {
						set.push([date, data[index][name]]);
						if (index > smoothingRadius && index < data.length - smoothingRadius) {
							smoothSet.push([date, smoothPoints(name, index, smoothingAmount)]);
						}
						
					}
				}

				dataSet.push({	data: dataSmoothing ? smoothSet : set,
								lines : { lineWidth : width, show : visible },
								label: generateLabel(name),
								yaxis: (count + 1),
								color : dataToPlot[name].color});

				if(smoothing && showDots){
					dataSet.push({	data: set,
									lines : { show : false},
									points : { show : visible, radius : POINT_SIZE},
									yaxis: (count + 1),
									color : dataToPlot[name].color });
				}

				//left or right and visible or invisible
				var orientation = axesCount%2 ? "right" : "left";



				// For each name, if quantity has auto checked, set min/max to null
				// if quantity has auto unchecked, set min/max to parsed input values
				auto = $(dataToPlot[name].autoID).is(':checked');
				min = parseFloat($(dataToPlot[name].lowID).val(), 10);
				max = parseFloat($(dataToPlot[name].highID).val(), 10);

				// Sensible default values if input is confusing
				if (isNaN(min)) {
					min = dataToPlot[name].defaultLow;
				}
				if (isNaN(max)) {
					max = dataToPlot[name].defaultHigh;
				}
				if (min > max) {
					min = dataToPlot[name].defaultLow;
					max = dataToPlot[name].defaultHigh;
				}


				YAxesSet.push({	position : orientation,
								show : visible,
								min : auto ? null : min,
								max : auto ? null : max,
								tickFormatter : function(val, axis){ return val.toPrecision(TICK_SIG_FIGS); },
								font : { color : width == BOLD ? "black" : "lightgrey", weight : width == BOLD ? "bold" : "normal"}});

				count++;
				if(visible) axesCount++;

				saveCurrentValues()
			}

			function smoothPoints(field, index, smoothingWidth){
				if(smoothingWidth < 1){
					return data[index][field];
				}

				if(smoothingWidth%2 === 0){
					smoothingWidth += 1;
				}

				var averageSpace = Math.floor(smoothingWidth/2);
				var sum = 0;
				
				for(var i=(-1*averageSpace);i<=averageSpace;i++){
					var t_index = i+index;
					if(t_index<0 || t_index>=data.length){
						//console.log("Decreasing radius");
						return smoothPoints(field, index, smoothingWidth-2);
					} else {
						sum += data[t_index][field];
					}
				}

				return sum/smoothingWidth;
			}

			somePlot = $.plot("#placeholder", dataSet, {
				series : { shadowSize : 0},
				xaxes : [{ mode: "time", timezone: "browser", twelveHourClock: true }],		// *
				yaxes : YAxesSet,
				legend: { show : true, position : "nw", sorted : "ascending"}
			});
			// *must include jquery.flot.time.min.js for this!

			// Spinner is handled by switch in real time mode
			if(!realTimeUpdates) stopSpinner();

		}

		// Spinner is handled by switch in real time mode
		if(!realTimeUpdates) startSpinner();

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
		$("#length").prop('disabled', enabled);
	}

	function startSpinner(){
		$(".loading").show();
		setManualFetchEnabled(false);
	}

	function stopSpinner(){
		$(".loading").hide();
		setManualFetchEnabled(true);
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

	$("#fetch").click(function(){
		var from = new Date($("#from").val());
		var to = new Date($("#to").val());

		var fromArg;
		var toArg;

		// Check for valid from
		if($("#from").val().toUpperCase() === 'START'){
			fromArg = $("#from").val();
		} else if(!isNaN(from)){
			fromArg = from.toISOString();
		}

		// Check for valid to
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
			// Start Spinner
			startSpinner();
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
			// Stop Spinner
			stopSpinner();
			$(this).val("Start Real Time Updates");
			// enable manual fetch
			setManualFetchEnabled(true);
		}

		continuousUpdate(length);
	});

	$("#toggleSmoothing").change(function(){
		if($(this).is(":checked")) {
			smoothing = true;
		} else {
			smoothing = false;
		}
	});

	$("#showDots").change(function(){
		if($(this).is(":checked")) {
			showDots = true;
		} else {
			showDots = false;
		}
	});

	$("#restoreDefaults").click(function() {
		localStorage.removeItem("defaults");
		defaultsSetup(false);
	});

	function pad(number) {
		if (number < 10) {
			return '0' + number;
		}
		return number;
	}

	function noZero(number) {
		return ((number === 0) ? 12 : number);
	}

	Date.parseDate = function( input, format ){ return Date.parse(input); };
	Date.prototype.dateFormat = function( format ){
		// Return 12 hour time
		if(format == 'H:i') return noZero(this.getHours()%12) + ":" + pad(this.getMinutes()) + " " + (this.getHours() < 12 ? "AM" : "PM");
		else return this.toISOString();
	};

	defaultOptions = {
		timepickerScrollbar: false,
		scrollMonth : false
	};
	$('#from').datetimepicker(defaultOptions);
	$('#to').datetimepicker(defaultOptions);
	displayData();
	setManualFetchEnabled(true);
});
