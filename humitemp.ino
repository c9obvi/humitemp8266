#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h> // Include the BearSSL library for secure connections


#define DHTPIN 4          // GPIO pin where the DHT11 is connected
#define DHTTYPE DHT11     // DHT 11
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "wifi_name";        // Your network SSID (name)
const char* password = "PW";    // Your network password

ESP8266WebServer server(80);

void handleData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // Celsius
  // Check for NaN values from the sensor
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    server.send(500, "text/plain", "Failed to read from DHT sensor!");
    return;
  }
  
  float f = (t * 9.0 / 5.0) + 32.0; // Fahrenheit
  String jsonData = "{\"temperature\": " + String(t, 2) + ", \"humidity\": " + String(h, 2) + ", \"fahrenheit\": " + String(f, 2) + "}";
  server.send(200, "application/json", jsonData);
}

// Notification flags
bool tempAbove25CNotified = false;
bool humidityBelow30Notified = false;
bool humidityAbove75Notified = false;
bool tempAbove75FNotified = false;
bool tempBelow45FNotified = false;

void sendNotification(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure); // Use BearSSL for secure connection
    client->setInsecure(); // Bypass SSL certificate verification

    HTTPClient http;
    http.begin(*client, "https://ntfy.sh/humitemp"); // Use your Ntfy URL
    
    Serial.println("Sending notification: " + message);
    
    int httpResponseCode = http.POST(message);
    
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    http.end();
    
    if (httpResponseCode == 200) {
      Serial.println("Notification sent successfully.");
    } else {
      Serial.println("Failed to send notification. Response code: " + String(httpResponseCode));
    }
  } else {
    Serial.println("WiFi not connected. Cannot send notification.");
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());

  delay(1000); // Give a moment for the connection to stabilize.
  sendNotification("humitemp is online!"); // Attempt to notify.

server.on("/", HTTP_GET, []() {
  String html = 
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<title>ESP8266 DHT11 Server</title>"
    "<meta http-equiv='refresh' content='10'>" // This line causes the page to refresh every 10 seconds
    "</head>"
    "<body>"
    "<h1>Temperature and Humidity</h1>";

  // Attempt to read from sensor
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // Read temperature as Celsius
  float f = (t * 9.0 / 5.0) + 32.0; // Convert to Fahrenheit

  // Check for NaN values from the sensor and display the data
  if (isnan(h) || isnan(t)) {
    html += "<p>Failed to read from DHT sensor!</p>";
  } else {
    html += "<p>Temperature: " + String(t) + " °C (" + String(f) + " °F)</p>"
            "<p>Humidity: " + String(h) + " %</p>";
  }

  html += 
    "</body>"
    "</html>";

  server.send(200, "text/html", html);
});

  server.on("/data", HTTP_GET, handleData);

  server.begin();
}

void loop() {

  // static bool firstLoop = true;
  // if (firstLoop) {
  //   sendNotification("Testing notification from loop");
  //   firstLoop = false;
  // }

  server.handleClient();

  float temperatureC = dht.readTemperature();
  float temperatureF = dht.readTemperature(true);
  float humidity = dht.readHumidity();

  // Temperature above 25°C
  if (!isnan(temperatureC) && temperatureC > 25.0 && !tempAbove25CNotified) {
    sendNotification("Temperature is above 25 °C: " + String(temperatureC));
    tempAbove25CNotified = true;
  } else if (!isnan(temperatureC) && temperatureC <= 25.0 && tempAbove25CNotified) {
    tempAbove25CNotified = false;
  }

  // Humidity below 30%
  if (!isnan(humidity) && humidity < 30.0 && !humidityBelow30Notified) {
    sendNotification("Humidity is below 30%: " + String(humidity));
    humidityBelow30Notified = true;
  } else if (!isnan(humidity) && humidity >= 30.0 && humidityBelow30Notified) {
    humidityBelow30Notified = false;
  }

  // Humidity above 75%
  if (!isnan(humidity) && humidity > 75.0 && !humidityAbove75Notified) {
    sendNotification("Humidity is above 75%: " + String(humidity));
    humidityAbove75Notified = true;
  } else if (!isnan(humidity) && humidity <= 75.0 && humidityAbove75Notified) {
    humidityAbove75Notified = false;
  }

  // Temperature above 75°F
  if (!isnan(temperatureF) && temperatureF > 75.0 && !tempAbove75FNotified) {
    sendNotification("Temperature is above 75 °F: " + String(temperatureF));
    tempAbove75FNotified = true;
  } else if (!isnan(temperatureF) && temperatureF <= 75.0 && tempAbove75FNotified) {
    tempAbove75FNotified = false;
  }

  // Temperature below 45°F
  if (!isnan(temperatureF) && temperatureF < 45.0 && !tempBelow45FNotified) {
    sendNotification("Temperature is below 45 °F: " + String(temperatureF));
    tempBelow45FNotified = true;
  } else if (!isnan(temperatureF) && temperatureF >= 45.0 && tempBelow45FNotified) {
    tempBelow45FNotified = false;
  }
}
