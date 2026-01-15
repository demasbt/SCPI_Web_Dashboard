
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
// Init web socket when the page loads
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getReadings(){
    websocket.send("getReadings");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

// When websocket is established, call the getReadings() function
function onOpen(event) {
    console.log('Connection opened');
    getReadings();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}



Highcharts.setOptions({
    plotOptions: {
    scatter: {
      marker: {
        radius: 2.5,
        symbol: 'circle',
        states: {
          hover: {
            enabled: true,
            lineColor: 'rgb(100,100,100)'
                    }
                  }
                },
        states: {
          hover: {
            marker: {
              enabled: false
                      }
                  }
                },
        jitter: {
            x: 0.005
                }
            }
    }
});

// Create the Magnitude chart
var chart1 = new Highcharts.Chart({
  chart:{ 
    renderTo : 'chart-magnitude',
    type: 'scatter',
    zooming: {
        type: 'xy' }},
  title: { text: 'MAGNITUDE' },
  exporting: { enabled : true },
  xAxis: { 
    type: 'logarithmic',
    title: { text: 'Frequency' }
  },
  yAxis: {
    title: { text: 'Magnitude [dB]' }
  },
  credits: { enabled: false },
  tooltip: {
            pointFormat: 'Freq: {point.x} Hz <br/> Gain: {point.y}'
    },
  series: [{
    name: 'CH1',
    id: 'ch1',
    marker: {
        symbol: 'circle'
    }
},
{
    name: 'CH2',
    id: 'ch2',
    marker: {
        symbol: 'circle'
    }
}]
});

//Create the Phase chart
var chart2 = new Highcharts.Chart({
  chart:{ 
    renderTo : 'chart-phase',
    type: 'scatter',
    zooming: {
        type: 'xy' }},
  title: { text: 'PHASE' },
  exporting: { enabled : true },
  dataGrouping:{
    enabled: false
  },
  xAxis: { 
    type: 'logarithmic',
    title: { text: 'Frequency' }
  },
  yAxis: {
    title: { text: 'Phase [&deg]' },
    max: 90,
    min: -90,
    tickInterval: 45,
    minorTickInterval: 15
  },
  credits: { enabled: false },
  tooltip: {
            pointFormat: 'Freq: {point.x} Hz <br/> Phase: {point.y} &deg'
    },
  series: [{
    name: 'Phase',
    id: 'phase',
    marker: {
        symbol: 'circle'
    }
}]
});

// Function that receives the message from the ESP32 with the readings
function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);
    plotBode(myObj);

    for (var i = 0; i < keys.length; i++){
        var key = keys[i];
        document.getElementById(key).innerHTML = myObj[key];
    }
}

// Create calculation to plot Bode diagram
function plotBode(jsonValue) {
    var keys = Object.keys(jsonValue);
    const key0 = keys[0];
    const key1 = keys[1];
    const key2 = keys[2];
    const key3 = keys[3];
    var y1 = Number(jsonValue[key0]);
    var y2 = Number(jsonValue[key1]);
    var x = Number(jsonValue[key2]);
    var y3 = Number(jsonValue[key3]);
    if (x === 0 || y1 === 0 || y2 === 0 || y3 === 0) {
        return;
    }
    var db1 = 20 * Math.log10(y1/y1);
    var db2 = 20 * Math.log10(y2/y1);

    chart1.series[0].addPoint([x, db1], true, false, true);
    chart1.series[1].addPoint([x, db2], true, false, true);
    chart2.series[0].addPoint([x, y3], true, false, true);
}