var TZOffset = 0;

Pebble.addEventListener("ready", function(e) {
	TZOffset = (new Date()).getTimezoneOffset() * 60;
	sendConfig();
});

function sendConfig() {
	console.log("TZOffset = " + TZOffset);
	Pebble.sendAppMessage(JSON.parse('{"TZOffset":'+TZOffset+'}'));
}