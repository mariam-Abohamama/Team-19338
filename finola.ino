#include "DHT.h"
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

// WiFi Credentials
const char* ssid = "HUAWEI_B320_9BF6";     // Change it with your WiFi network
const char* password = "dMG3GBy7476";  

// Google Apps Script URL
const char* googleScriptURL = "https://script.google.com/macros/s/AKfycbzd88LMKQ7shJuEmd5S2xQRHNZJxandX-7ehpuo8jsC3HzyAWfPlQ3gCGglb3WM6QffTA/exec"; // Replace with your actual Apps Script URL

// Create a WebServer object on port 80
WebServer server(80);

// Pin Definitions
#define soundSensorPin 35  // Sound sensor connected to analog pin 35
#define BUZZER_PIN 18
#define DHTPIN0 33
#define LED_PIN 23          // Control IR LED pin for dust sensor
#define AN_PIN 34           // Analog input pin for dust sensor output

DHT dht0(DHTPIN0, DHT11);       // Define DHT11

// Variables for storing sensor data
float humidity0;
float temperature0;
float dustDensity = 0.0;
int soundLevel;

// Timer variables
unsigned long lastGoogleUpdate = 0;
unsigned long googleUpdateInterval = 60000; // 60 seconds

void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
      <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>19338</title>
        <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
        <style>
          body { 
            font-family: Arial, sans-serif; 
            text-align: center; 
            max-width: 800px; 
            margin: 0 auto;
            background-color: #1a1a1a; /* Background color */
            color: #ffffff; /* Font color for the entire page text */
          }
          .chart-container { 
            display: flex; 
            justify-content: space-around;
            flex-wrap: wrap;
          }
          canvas { 
            width: 100%; 
            max-width: 300px;
            height: 200px; 
          }
        </style>
      </head>
      <body>
        <h1>Group 19338</h1>
        <div>
          <p><b>Sound Level:</b> <span id="soundValue">0</span></p>
          <p><b>Dust Density:</b> <span id="dustValue">0</span> mg/m³</p>
          <p><b>Temperature:</b> <span id="tempValue">0</span>°C</p>
          <p><b>Humidity:</b> <span id="humidityValue">0</span>%</p>
        </div>

        <!-- Chart Containers -->
        <div class="chart-container">
          <canvas id="tempChart"></canvas>
          <canvas id="dustChart"></canvas>
          <canvas id="soundChart"></canvas>
          <!-- New Humidity Line Chart -->
          <canvas id="humidityLineChart"></canvas>
          <canvas id="humidityChart"></canvas>

        </div>

        <script>
          // Setup chart contexts
          let tempChartCtx = document.getElementById('tempChart').getContext('2d');
          let dustChartCtx = document.getElementById('dustChart').getContext('2d');
          let soundChartCtx = document.getElementById('soundChart').getContext('2d');
          let humidityLineChartCtx = document.getElementById('humidityLineChart').getContext('2d');
          let humidityChartCtx = document.getElementById('humidityChart').getContext('2d');


          

          // Temperature line chart
          let tempChart = new Chart(tempChartCtx, {
            type: 'line',
            data: {
              labels: [],
              datasets: [{
                label: 'Temperature (°C)',
                data: [],
                borderColor: '#FF5733',
                fill: false
              }]
            },
            options: { scales: { y: { beginAtZero: false }}}
          });

          // Dust density line chart
          let dustChart = new Chart(dustChartCtx, {
            type: 'line',
            data: {
              labels: [],
              datasets: [{
                label: 'Dust Density (mg/m³)',
                data: [],
                borderColor: '#4CAF50',
                fill: false
              }]
            },
            options: { scales: { y: { beginAtZero: false }}}
          });

          // Sound sensor line chart
          let soundChart = new Chart(soundChartCtx, {
            type: 'line',
            data: {
              labels: [],
              datasets: [{
                label: 'Sound Level',
                data: [],
                borderColor: '#800080',
                fill: false
              }]
            },
            options: { scales: { y: { beginAtZero: false }}}
          });

          // New Humidity line chart
          let humidityLineChart = new Chart(humidityLineChartCtx, {
            type: 'line',
            data: {
              labels: [],
              datasets: [{
                label: 'Humidity (%)',
                data: [],
                borderColor: '#36A2EB',
                fill: false
              }]
            },
            options: { scales: { y: { beginAtZero: false }}}
          });

          // Humidity doughnut chart
          let humidityChart = new Chart(humidityChartCtx, {
            type: 'doughnut',
            data: {
              labels: ['Humidity', 'Remaining'],
              datasets: [{
                data: [0, 100],
                backgroundColor: ['#36A2EB', '#ffffff'],
              }]
            },
            options: { cutoutPercentage: 70 }
          });

          // Function to update values
          function updateValues() {
            fetch("/data")
              .then(response => response.json())
              .then(data => {
                document.getElementById("soundValue").textContent = data.soundLevel;
                document.getElementById("dustValue").textContent = data.dustDensity;
                document.getElementById("tempValue").textContent = data.temp;
                document.getElementById("humidityValue").textContent = data.humidity;

                
                // Update temperature chart
                if (tempChart.data.labels.length > 10) {
                  tempChart.data.labels.shift();
                  tempChart.data.datasets[0].data.shift();
                }
                let now = new Date();
                tempChart.data.labels.push(now.getHours() + ':' + now.getMinutes() + ':' + now.getSeconds());
                tempChart.data.datasets[0].data.push(data.temp);
                tempChart.update();

                // Update dust chart
                if (dustChart.data.labels.length > 10) {
                  dustChart.data.labels.shift();
                  dustChart.data.datasets[0].data.shift();
                }
                dustChart.data.labels.push(now.getHours() + ':' + now.getMinutes() + ':' + now.getSeconds());
                dustChart.data.datasets[0].data.push(data.dustDensity);
                dustChart.update();

                // Update sound chart
                if (soundChart.data.labels.length > 10) {
                  soundChart.data.labels.shift();
                  soundChart.data.datasets[0].data.shift();
                }
                soundChart.data.labels.push(now.getHours() + ':' + now.getMinutes() + ':' + now.getSeconds());
                soundChart.data.datasets[0].data.push(data.soundLevel);
                soundChart.update();

                // Update new humidity line chart
                if (humidityLineChart.data.labels.length > 10) {
                  humidityLineChart.data.labels.shift();
                  humidityLineChart.data.datasets[0].data.shift();
                }
                humidityLineChart.data.labels.push(now.getHours() + ':' + now.getMinutes() + ':' + now.getSeconds());
                humidityLineChart.data.datasets[0].data.push(data.humidity);
                humidityLineChart.update();

                // Update humidity chart
                humidityChart.data.datasets[0].data = [data.humidity, 100 - data.humidity];
                humidityChart.update();

              });
          }

          setInterval(updateValues, 1000); // Update every second
        </script>
      </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}


void handleData() {
  String json = "{";
  json += "\"soundLevel\":" + String(soundLevel) + ",";
  json += "\"dustDensity\":" + String(dustDensity) + ",";
  json += "\"temp\":" + String(temperature0) + ",";
  json += "\"humidity\":" + String(humidity0);
  json += "}";
  server.send(200, "application/json", json);
}

void sendToGoogleSheet() { 
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String postData = "{\"soundLevel\":" + String(soundLevel) +
                      ",\"dustDensity\":" + String(dustDensity) +
                      ",\"temp\":" + String(temperature0) +
                      ",\"humidity\":" + String(humidity0) + "}";

    http.begin(googleScriptURL);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      Serial.println("Data sent successfully: " + String(httpResponseCode));
    } else {
      Serial.println("Error sending data: " + http.errorToString(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(soundSensorPin, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  dht0.begin();
  pinMode(LED_PIN, OUTPUT);
  pinMode(AN_PIN, INPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi. IP address: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/data", handleData);

  server.begin();
}

void loop() {
  // Read raw sound sensor value and map it to 0-100
  int rawSoundLevel = analogRead(soundSensorPin);
  soundLevel = map(rawSoundLevel, 0, 4095, 0, 160);

  humidity0 = dht0.readHumidity();
  temperature0 = dht0.readTemperature();

  // Read dust sensor data
  digitalWrite(LED_PIN, LOW);
  delayMicroseconds(280);
  int sensorValue = analogRead(AN_PIN);
  digitalWrite(LED_PIN, HIGH);

  float voltage = sensorValue * (3.3 / 4095.0);
  dustDensity = voltage;

  Serial.println("Temperature: " + String(temperature0) + "°C");
  Serial.println("Humidity: " + String(humidity0) + "%");
  Serial.println("Sound Level: " + String(soundLevel));
  Serial.println("Dust Density: " + String(dustDensity) + " mg/m³");

  // Buzzer alerts based on conditions
  if (dustDensity > 1.5) {
    Serial.println("Dust density exceeds limit! Activating buzzer for 2 seconds.");
    digitalWrite(BUZZER_PIN, HIGH);
    delay(2000);
    digitalWrite(BUZZER_PIN, LOW);
  } else if (soundLevel > 85) {
    Serial.println("Sound level exceeds limit! Activating buzzer for 1 second.");
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
  }
  unsigned long currentMillis = millis();

  if (currentMillis - lastGoogleUpdate >= googleUpdateInterval) {
    sendToGoogleSheet();
    lastGoogleUpdate = currentMillis;
  }

  server.handleClient();
  delay(800);
}
