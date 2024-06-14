#include <ESP8266WiFi.h>  // Include the ESP8266 WiFi library
#include <DHT.h>          // Include the DHT sensor library

#define DHTPIN D4      // Define the pin where the DHT11 is connected
#define DHTTYPE DHT11  // Define the type of DHT sensor

// Initialize the DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// WiFi credentials
const char* ssid = "midterm";       // Your WiFi SSID
const char* password = "12345678";  // Your WiFi Password
const char* privateServer = "192.168.119.154";  // Server IP address
const int mq135Pin = A0;  // Define the analog pin where the MQ-135 sensor is connected

void setup() {
  Serial.begin(115200);  // Initialize serial communication at 115200 baud rate
  WiFi.begin(ssid, password);  // Connect to WiFi
  dht.begin();  // Initialize the DHT sensor
  Serial.print("\nConnecting to WiFi...");
  // Wait until the WiFi is connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  // Print the IP address once connected
  Serial.println("\nConnected to WiFi");
}

void loop() {
  // Read sensors
  int sensorValue = analogRead(mq135Pin);        // Read the analog value from the MQ-135 sensor
  float voltage = sensorValue * (5.0 / 1023.0);  // Convert the analog value to voltage

  // Convert voltage to CO2 concentration
  float co2Concentration = voltage * 200;  // Assuming a simple linear relationship between voltage and CO2 concentration

  // Read temperature and humidity from DHT11
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Check if any reads failed and exit early (to try again).
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;  // Exit the loop if DHT sensor reading failed
  } else if (isnan(sensorValue)) {
    Serial.println("Failed to read from Gas sensor!");
    return;  // Exit the loop if Gas sensor reading failed
  }

  // Print the sensor readings to the Serial Monitor
  Serial.print("CO2 Concentration: ");
  Serial.print(co2Concentration);
  Serial.println(" ppm");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  // Send data to the server
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;  // Create a WiFi client
    String url = "/input/input.php?co2=" + String(co2Concentration) + "&temp=" + String(temperature) + "&hum=" + String(humidity);

    // Check if WiFi is still connected
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;  // Create a WiFi client
      String url = "/input/input.php?co2=" + String(co2Concentration) + "&temp=" + String(temperature) + "&hum=" + String(humidity);

      // Connect to the server
      if (client.connect(privateServer, 80)) {
        // Send the HTTP GET request
        client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + privateServer + "\r\n" + "Connection: close\r\n\r\n");
        delay(500);

        // Read the response from the server
        while (client.available()) {
          String line = client.readStringUntil('\r');
          // Check if the line contains the desired message
          if (line.indexOf("New record created successfully") != -1) {
            Serial.println(line);  // Print the desired message
          }
        }
      }
      client.stop();  // Close the connection
    } else {
      Serial.println("WiFi not connected");  // Print if WiFi is not connected
    }
    delay(600000);  // Delay for 10 minutes before the next loop iteration
  }
}