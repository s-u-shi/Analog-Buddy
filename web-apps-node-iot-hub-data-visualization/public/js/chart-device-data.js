/* eslint-disable max-classes-per-file */
/* eslint-disable no-restricted-globals */
/* eslint-disable no-undef */

//Convert value to percentage
function toPercentage(chart, value, maxValue) {
  //Ensure no out-of-bounds
  const rounded = Math.round(Math.min(value, maxValue));
  chart.data.datasets[0].data = [rounded, maxValue - rounded];
  chart.options.title.text = rounded + chart.unit;
}

//Convert Date to formatted string
function formateDate(date) {
  const year = date.getFullYear();
  const month = String(date.getMonth() + 1).padStart(2, '0');
  const day = String(date.getDate()).padStart(2, '0');
  const hours = String(date.getHours()).padStart(2, '0');
  const minutes = String(date.getMinutes()).padStart(2, '0');
  return `${year}-${month}-${day} ${hours}:${minutes}`;
}

$(document).ready(() => {
  // if deployed to a site supporting SSL, use wss://
  const protocol = document.location.protocol.startsWith('https') ? 'wss://' : 'ws://';
  const webSocket = new WebSocket(protocol + location.host);

  // A class for holding the last N points of telemetry for a device
  class DeviceData {
    constructor(deviceId) {
      this.deviceId = deviceId;
      this.maxLen = 50;
      this.timeData = new Array(this.maxLen);
      this.temperatureData = new Array(this.maxLen);
      this.humidityData = new Array(this.maxLen);
      this.brightnessData = new Array(this.maxLen);
    }

    addData(time, temperature, humidity, brightness) {
      this.timeData.push(time);
      this.temperatureData.push(temperature);
      this.humidityData.push(humidity || null);
      this.brightnessData.push(brightness || null);

      if (this.timeData.length > this.maxLen) {
        this.timeData.shift();
        this.temperatureData.shift();
        this.humidityData.shift();
        this.brightnessData.shift();
      }
    }
  }

  // All the devices in the list (those that have been sending telemetry)
  class TrackedDevices {
    constructor() {
      this.devices = [];
    }

    // Find a device based on its Id
    findDevice(deviceId) {
      for (let i = 0; i < this.devices.length; ++i) {
        if (this.devices[i].deviceId === deviceId) {
          return this.devices[i];
        }
      }

      return undefined;
    }

    getDevicesCount() {
      return this.devices.length;
    }
  }

  const trackedDevices = new TrackedDevices();

  // Define the chart axes
  const chartData = {
    datasets: [
      {
        fill: false,
        label: 'Temperature',
        yAxisID: 'Temperature',
        borderColor: 'rgba(178, 34, 34, 1)',
        pointBorderColor: 'rgba(205, 92, 92, 1)',
        backgroundColor: 'rgba(178, 34, 34, 0.4)',
        pointHoverBackgroundColor: 'rgba(220, 20, 60, 1)',
        pointHoverBorderColor: 'rgba(205, 92, 92, 1)',
        spanGaps: true,
      },
      {
        fill: false,
        label: 'Humidity',
        yAxisID: 'Humidity',
        borderColor: 'rgba(0, 128, 128, 1)',
        pointBorderColor: 'rgba(72, 209, 204, 1)',
        backgroundColor: 'rgba(0, 128, 128, 0.4)',
        pointHoverBackgroundColor: 'rgba(0, 206, 209, 1)',
        pointHoverBorderColor: 'rgba(72, 209, 204, 1)',
        spanGaps: true,
      },
      {
        fill: false,
        label: 'Brightness',
        yAxisID: 'Brightness',
        borderColor: 'rgba(255, 165, 79, 1)',
        pointBorderColor: 'rgba(255, 200, 124, 1)',
        backgroundColor: 'rgba(255, 165, 79, 0.4)',
        pointHoverBackgroundColor: 'rgba(255, 140, 0, 1)',
        pointHoverBorderColor: 'rgba(255, 200, 124, 1)',
        spanGaps: true,
      }
    ]
  };

  const chartOptions = {
    scales: {
      yAxes: [{
        id: 'Temperature',
        type: 'linear',
        scaleLabel: {
          fontColor: 'rgba(43, 47, 40, 1)',
          labelString: 'Temperature (ºC)',
          display: true,
        },
        position: 'right',
        ticks: {
          fontColor: 'rgba(43, 47, 40, 1)',
          suggestedMin: 0,
          suggestedMax: 40,
          beginAtZero: true
        }
      },
      {
        id: 'Humidity',
        type: 'linear',
        scaleLabel: {
          fontColor: 'rgba(43, 47, 40, 1)',
          labelString: 'Humidity (%)',
          display: true,
        },
        position: 'right',
        ticks: {
          fontColor: 'rgba(43, 47, 40, 1)',
          suggestedMin: 0,
          suggestedMax: 100,
          beginAtZero: true
        }
      },
      {
        id: 'Brightness',
        type: 'linear',
        scaleLabel: {
          fontColor: 'rgba(43, 47, 40, 1)',
          labelString: 'Brightness',
          display: true,
        },
        position: 'left',
        ticks: {
          fontColor: 'rgba(43, 47, 40, 1)',
          suggestedMin: 0,
          suggestedMax: 4095,
          beginAtZero: true
        }
      }]
    },
    legend: {
      fontColor: 'rgba(43, 47, 40, 1)',
      position: 'bottom'
    }
  };

  // Get the context of the canvas element we want to select
  const ctx = document.getElementById('iotChart').getContext('2d');
  const myLineChart = new Chart(
    ctx,
    {
      type: 'line',
      data: chartData,
      options: chartOptions,
    });

  //Doughnut Chart for Temperature
  const tempDoughnut = document.getElementById('tempDoughnut').getContext('2d');
  const myTempDoughnutChart = new Chart(
    tempDoughnut,
    {
      type: 'doughnut',
      data: {
        labels: ['', ''],
        datasets: [{
            data: [0, 40],
            backgroundColor: [
                'rgba(178, 34, 34, 1)',
                'rgba(200, 200, 200, 0.3)'
            ],
            borderColor: [
                'rgba(205, 92, 92, 1)',
                'rgba(200, 200, 200, 1)'
            ],
            borderWidth: 1
        }]
    },
    options: {
        responsive: true,
        legend: {
            display: false
        },
        title: {
            display: true,
            position: 'bottom',
            text: '--',
        },
        animation: {
            animateScale: true,
            animateRotate: true
        },
        tooltips: {
          enabled: false
        }
    }
    }
  );
  myTempDoughnutChart.unit = "°C";

  //Doughnut Chart for Humidity
  const humDoughnut = document.getElementById('humDoughnut').getContext('2d');
  const myHumDoughnutChart = new Chart(
    humDoughnut,
    {
      type: 'doughnut',
      data: {
        labels: ['', ''],
        datasets: [{
            data: [0, 100],
            backgroundColor: [
                'rgba(0, 128, 128, 1)',
                'rgba(200, 200, 200, 0.3)'
            ],
            borderColor: [
                'rgba(72, 209, 204, 1)',
                'rgba(200, 200, 200, 1)'
            ],
            borderWidth: 1
        }]
    },
    options: {
        responsive: true,
        legend: {
            display: false
        },
        title: {
            display: true,
            position: 'bottom',
            text: '--',
        },
        animation: {
            animateScale: true,
            animateRotate: true
        },
        tooltips: {
          enabled: false
        }
    }
    }
  );
  myHumDoughnutChart.unit = "%";

  //Doughnut Chart for Brightness
  const brightDoughnut = document.getElementById('brightDoughnut').getContext('2d');
  const myBrightDoughnutChart = new Chart(
    brightDoughnut,
    {
      type: 'doughnut',
      data: {
        labels: ['', ''],
        datasets: [{
            data: [0, 4095],
            backgroundColor: [
                'rgba(255, 165, 79, 1)',
                'rgba(200, 200, 200, 0.3)'
            ],
            borderColor: [
                'rgba(255, 200, 124, 1)',
                'rgba(200, 200, 200, 1)'
            ],
            borderWidth: 1
        }]
    },
    options: {
        responsive: true,
        legend: {
            display: false
        },
        title: {
            display: true,
            position: 'bottom',
            text: '--',
        },
        animation: {
            animateScale: true,
            animateRotate: true
        },
        tooltips: {
          enabled: false
        }
    }
    }
  );
  myBrightDoughnutChart.unit = "";

  // Manage a list of devices in the UI, and update which device data the chart is showing
  // based on selection
  let needsAutoSelect = true;
  const deviceCount = document.getElementById('deviceCount');
  const listOfDevices = document.getElementById('listOfDevices');
  function OnSelectionChange() {
    const device = trackedDevices.findDevice(listOfDevices[listOfDevices.selectedIndex].text);
    chartData.labels = device.timeData;
    chartData.datasets[0].data = device.temperatureData;
    chartData.datasets[1].data = device.humidityData;
    chartData.datasets[2].data = device.brightnessData;

    //Temperature
    toPercentage(myTempDoughnutChart, device.temperatureData[device.temperatureData.length - 1], 40);
    myTempDoughnutChart.update();

    //Humidity
    toPercentage(myHumDoughnutChart, device.humidityData[device.humidityData.length - 1], 100);
    myHumDoughnutChart.update();

    //Brightness
    toPercentage(myBrightDoughnutChart, device.brightnessData[device.brightnessData.length - 1], 4095);
    myBrightDoughnutChart.update();
    
    myLineChart.update();
  }
  listOfDevices.addEventListener('change', OnSelectionChange, false);

  // When a web socket message arrives:
  // 1. Unpack it
  // 2. Validate it has date/time and temperature
  // 3. Find or create a cached device to hold the telemetry data
  // 4. Append the telemetry data
  // 5. Update the chart UI
  webSocket.onmessage = function onMessage(message) {
    try {
      const messageData = JSON.parse(message.data);
      console.log(messageData);

      // time and either temperature or humidity are required
      if (!messageData.MessageDate || (!messageData.IotData.temperature && !messageData.IotData.humidity && !messageData.IotData.brightness)) {
        return;
      }

      // find or add device to list of tracked devices
      const existingDeviceData = trackedDevices.findDevice(messageData.DeviceId);
      const formatted_date = formateDate(new Date(messageData.MessageDate));

      if (existingDeviceData) {
        //existingDeviceData.addData(messageData.MessageDate, messageData.IotData.temperature, messageData.IotData.humidity, messageData.IotData.brightness);
        existingDeviceData.addData(formatted_date, messageData.IotData.temperature, messageData.IotData.humidity, messageData.IotData.brightness);
      } else {
        const newDeviceData = new DeviceData(messageData.DeviceId);
        trackedDevices.devices.push(newDeviceData);
        const numDevices = trackedDevices.getDevicesCount();
        deviceCount.innerText = numDevices === 1 ? `${numDevices} device` : `${numDevices} devices`;
        //newDeviceData.addData(messageData.MessageDate, messageData.IotData.temperature, messageData.IotData.humidity, messageData.IotData.brightness);
        newDeviceData.addData(formatted_date, messageData.IotData.temperature, messageData.IotData.humidity, messageData.IotData.brightness);

        // add device to the UI list
        const node = document.createElement('option');
        const nodeText = document.createTextNode(messageData.DeviceId);
        node.appendChild(nodeText);
        listOfDevices.appendChild(node);

        // if this is the first device being discovered, auto-select it
        if (needsAutoSelect) {
          needsAutoSelect = false;
          listOfDevices.selectedIndex = 0;
          OnSelectionChange();
        }
      }
      
      //Temperature
      toPercentage(myTempDoughnutChart, messageData.IotData.temperature, 40);
      myTempDoughnutChart.update();

      //Humidity
      toPercentage(myHumDoughnutChart, messageData.IotData.humidity, 100);
      myHumDoughnutChart.update();

      //Brightness
      toPercentage(myBrightDoughnutChart, messageData.IotData.brightness, 4095);
      myBrightDoughnutChart.update();

      myLineChart.update();
    } catch (err) {
      console.error(err);
    }
  };
});