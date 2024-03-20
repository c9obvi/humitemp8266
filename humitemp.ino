#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h> // Include the BearSSL library for secure connections


#define DHTPIN 4          // GPIO pin where the DHT11 is connected
#define DHTTYPE DHT11     // DHT 11
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "***8";        // Your network SSID (name)
const char* password = "*****";    // Your network password

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
bool humidityBelow30Notified = false;
bool humidityAbove75Notified = false;

bool tempAbove70FNotified = false;
bool tempAbove75FNotified = false;
bool tempAbove80FNotified = false;
bool tempAbove85FNotified = false;
bool tempAbove90FNotified = false;


bool tempBelow70FNotified = false;
bool tempBelow65FNotified = false;
bool tempBelow55FNotified = false;
bool tempBelow45FNotified = false;

void sendNotification(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure); // Use BearSSL for secure connection
    client->setInsecure(); // Bypass SSL certificate verification

    HTTPClient http;
    http.begin(*client, "https://ntfy.sh/*****"); // Use your Ntfy URL
    
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
  sendNotification("humitemp is online!🚀"); // Attempt to notify.

// Additionally, send a notification with the IP address
  String ipNotification = "Device IP: " + WiFi.localIP().toString();
  sendNotification(ipNotification);

// After sending IP address notification
float initialHumidity = dht.readHumidity();
float initialTemperatureC = dht.readTemperature(); // Read temperature in Celsius
float initialTemperatureF = initialTemperatureC * 9.0 / 5.0 + 32.0; // Convert to Fahrenheit

// Prepare the message with initial readings in Fahrenheit
String initialReadings = "Initial readings - Temperature: " + String(initialTemperatureF, 2) + " °F🌡️, Humidity: " + String(initialHumidity, 2) + "%💧";
sendNotification(initialReadings);


server.on("/", HTTP_GET, []() {
  String html = 
  "<!DOCTYPE html>"
  "<html>"
  "<head>"
  "<title>ESP8266 DHT11 Server</title>"
  "<meta charset='UTF-8'>" // Add this line
  "<meta name='viewport' content='width=device-width, initial-scale=1.0'>" // Make it responsive
  "<style>"
  "body { display: flex; justify-content: center; align-items: center; flex-direction: column; height: 100vh; margin: 0; }"
  ".container { text-align: center; max-width: 600px; width: 100%; padding: 20px; }"
  "</style>"
  "<meta http-equiv='refresh' content='10'>" // This line causes the page to refresh every 10 seconds
  "</head>"
  "<body>"
  "<div class='container'>"
  "<h1>Temperature and Humidity &#x1F3E0;</h1>";

  // Attempt to read from sensor
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // Read temperature as Celsius
  float f = (t * 9.0 / 5.0) + 32.0; // Convert to Fahrenheit

  // Check for NaN values from the sensor and display the data
  if (isnan(h) || isnan(t)) {
    html += "<p>Failed to read from DHT sensor!</p>";
  } else {
    html += "<p>Temperature: " + String(f) + "°F &#x1F321;</p>"
            "<p>Humidity: " + String(h) + " %&#x1F4A7;</p>";
  }

  html += 
    "</body>"
    "</html>";

  server.send(200, "text/html", html);
});

  server.on("/data", HTTP_GET, handleData);

  server.begin();
}

void checkTemperatureAndNotify(float temperatureF) {
    // Check for temperature above thresholds
    if (temperatureF > 90.0) {
        if (!tempAbove90FNotified) {
            sendNotification("⚠️🔥 WARNING ⚠️ temperature is above 90 °F🌡️🔥- Current Temp: " + String(temperatureF, 2) + " °F🌡️");
            tempAbove90FNotified = true;
        }
        tempAbove85FNotified = tempAbove80FNotified = tempAbove75FNotified = tempAbove70FNotified = false;
    } else if (temperatureF > 85.0) {
        if (!tempAbove85FNotified) {
            sendNotification("⚠️ Caution, temperature is above 85 °F🌡️- Current Temp: " + String(temperatureF, 2) + " °F🌡️");
            tempAbove85FNotified = true;
        }
        tempAbove90FNotified = tempAbove80FNotified = tempAbove75FNotified = tempAbove70FNotified = false;
    } else if (temperatureF > 80.0) {
        if (!tempAbove80FNotified) {
            sendNotification("Temperature is above 80 °F🌡️- Current Temp: " + String(temperatureF, 2) + " °F🌡️");
            tempAbove80FNotified = true;
        }
        tempAbove90FNotified = tempAbove85FNotified = tempAbove75FNotified = tempAbove70FNotified = false;
    } else if (temperatureF > 75.0) {
        if (!tempAbove75FNotified) {
            sendNotification("Temperature is above 75 °F🌡️- Current Temp: " + String(temperatureF, 2) + " °F🌡️");
            tempAbove75FNotified = true;
        }
        tempAbove90FNotified = tempAbove85FNotified = tempAbove80FNotified = tempAbove70FNotified = false;
    } else if (temperatureF > 70.0) {
        if (!tempAbove70FNotified) {
            sendNotification("Temperature is above 70 °F🌡️- Current Temp: " + String(temperatureF, 2) + " °F🌡️");
            tempAbove70FNotified = true;
        }
        tempAbove90FNotified = tempAbove85FNotified = tempAbove80FNotified = tempAbove75FNotified = false;
    }

    // Reset notifications when temperature falls below thresholds
    if (temperatureF <= 70.0 && (tempAbove70FNotified || tempAbove75FNotified || tempAbove80FNotified || tempAbove85FNotified || tempAbove90FNotified)) {
        sendNotification("Temperature is now below 70 °F🌡️- Current Temp: " + String(temperatureF, 2) + " °F🌡️");
        tempAbove70FNotified = tempAbove75FNotified = tempAbove80FNotified = tempAbove85FNotified = tempAbove90FNotified = false;
    }
     // Temperature falls below thresholds
    if (temperatureF <= 45.0 && !tempBelow45FNotified) {
        sendNotification("🥶 WARNING, temperature is below 45 °F❄️🌡️- Current Temp: " + String(temperatureF, 2) + " °F🌡️");
        tempBelow45FNotified = true;
        // Reset only when going below this to avoid multiple lower temp notifications
        tempBelow55FNotified = tempBelow65FNotified = tempBelow70FNotified = false;
    } else if (temperatureF <= 55.0 && !tempBelow55FNotified) {
        sendNotification("🥶 temperature is below 55 °F🌡️- Current Temp: " + String(temperatureF, 2) + " °F🌡️");
        tempBelow55FNotified = true;
        tempBelow45FNotified = tempBelow65FNotified = tempBelow70FNotified = false;
    } else if (temperatureF <= 65.0 && !tempBelow65FNotified) {
        sendNotification("Temperature is below 65 °F🌡️- Current Temp: " + String(temperatureF, 2) + " °F🌡️");
        tempBelow65FNotified = true;
        tempBelow45FNotified = tempBelow55FNotified = tempBelow70FNotified = false;
    } else if (temperatureF <= 70.0 && !tempBelow70FNotified) {
        sendNotification("Temperature is now below 70 °F again🌡️- Current Temp: " + String(temperatureF, 2) + " °F🌡️");
        tempBelow70FNotified = true;
        tempBelow45FNotified = tempBelow55FNotified = tempBelow65FNotified = false;
    }

    // Reset "above" notifications when temperature goes sufficiently below thresholds
    if (temperatureF < 90.0) tempAbove90FNotified = false;
    if (temperatureF < 85.0) tempAbove85FNotified = false;
    if (temperatureF < 80.0) tempAbove80FNotified = false;
    if (temperatureF < 75.0) tempAbove75FNotified = false;
    if (temperatureF < 70.0) tempAbove70FNotified = false;

    // Reset "below" notifications when temperature rises above the lower thresholds to allow re-notification
    if (temperatureF > 45.0) tempBelow45FNotified = false;
    if (temperatureF > 55.0) tempBelow55FNotified = false;
    if (temperatureF > 65.0) tempBelow65FNotified = false;
    if (temperatureF > 70.0) tempBelow70FNotified = false;
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

 // Humidity checks
  if (!isnan(humidity)) {
    if (humidity < 30.0 && !humidityBelow30Notified) {
      sendNotification("⚡️Humidity is below 30%: " + String(humidity));
      humidityBelow30Notified = true;
    } else if (humidity >= 30.0 && humidityBelow30Notified) {
      humidityBelow30Notified = false;
    }

    if (humidity > 75.0 && !humidityAbove75Notified) {
      sendNotification("💧Humidity is above 75%: " + String(humidity));
      humidityAbove75Notified = true;
    } else if (humidity <= 75.0 && humidityAbove75Notified) {
      humidityAbove75Notified = false;
    }
  }
  // Temperature checks
  // Inside your loop function
if (!isnan(temperatureF)) {
    checkTemperatureAndNotify(temperatureF);
}
}

