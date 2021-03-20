char ressource_html[] = R"=====(
<!DOCTYPE html>
<html>
	<head>
		<meta charset="utf-8">
		<script src = "script.js"></script>
	</head>
	<body onload="init()">
		<canvas id=demo width = 128 height=64 style="image-rendering: crisp-edges; image-rendering: pixelated; height: 256px; width: 512px; display: block; background-color:gray">
			Canvas not working!
		</canvas>
		<div style="width: 512px; height: 128px; display: block; background-color:gray">
			<div onclick="btnLeft()" style="display:inline-block"><svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 24 24"><path d="M16 24l4-4 -8-8 8-8 -4-4 -11 12z"/></svg></div><!--
		 --><div onclick="btnClick()" style="display:inline-block; margin: 0 64px"><svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 24 24"><path d="M12 24l12-12 -12-12 -12 12z"/></svg></div><!--
		 --><div onclick="btnRight()" style="display:inline-block"><svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 24 24"><path d="M8 24l-4-4 8-8-8-8 4-4 11 12z"/></svg></div>
		</div>
	</body>
</html>
)=====";

char ressource_js[] = R"=====(
var dbuffer = [];
var color_on = [255,255,255,255] // RGBA of active Color
var color_off = [0,0,0,255] // RGBA of inactive Color

function init() {
	//window.addEventListener('resize', canvasResize, false);
    canvasInit();
    websocketSetup();
}

function canvasInit() {
	var c = document.getElementById("demo");
	var ctx = c.getContext("2d");
	ctx.clearRect(0, 0, c.width, c.height);
	var imageData = ctx.getImageData(0, 0, c.width, c.height);
	var data = imageData.data;
	for (var i = 0; i < data.length; i += 4) {
		data[i]     = 100; // red
		data[i + 1] = 100; // green
		data[i + 2] = 100; // blue
		data[i + 3] = 255; // alpha
	}
	ctx.putImageData(imageData, 0, 0);
}
function canvasUpdate(bitdata) {
	var c = document.getElementById("demo");
	var ctx = c.getContext("2d");
	var imageData = ctx.getImageData(0, 0, c.width, c.height);
	var data = imageData.data;
	var i = 0;
	for (var byte = 0; byte < 1024; byte++){
		for (var mask = 0x80; mask > 0; mask >>= 1){
			var col = ((bitdata[byte] & mask) > 0) ? color_on : color_off;
				data[i++] = col[0]; // red
				data[i++] = col[1]; // green
				data[i++] = col[2]; // blue
				data[i++] = col[3]; // alpha
		}
	}
	ctx.putImageData(imageData, 0, 0);
}

function btnLeft() {
	websock.send("action left");
}
function btnRight() {
	websock.send("action right");
}
function btnClick() {
	websock.send("action click");
}

function websocketSetup() {
	websock = new WebSocket("ws://"+window.location.hostname+":80/ws");
	//websock = new WebSocket("ws://127.0.0.1:8765/");
	websock.binaryType = 'arraybuffer';
	websock.onmessage=websocketOnMessage;
}
function websocketOnMessage(evt) {
	//console.log(evt);
	if (evt.data instanceof ArrayBuffer) {
		var buffer = new Uint8Array(evt.data);
		canvasUpdate(buffer);
		//console.log(buffer);
	} else {
		console.log(evt.data);
	}
}
)=====";
