
function send() {
	var accelVal; 
	var ctx = document.getElementById("myChart").getContext("2d");
	var myNewChart = new Chart(ctx).PolarArea(data);

	new Chart(ctx).PolarArea(data, options);

}