#include <EEPROM.h>            // Library for reading and writing to EEPROM
#include <ESP8266WiFi.h>       // Library for WiFi functionality on ESP8266
#include <ESP8266WebServer.h>  // Library for creating a web server on ESP8266
#include <DHT.h>               // Library for DHT sensor
#include <PubSubClient.h>      // Library for MQTT functionality
#include <ArduinoJson.h>       // Library for JSON handling

#define DHTPIN D4                 // Define the pin where the DHT sensor is connected
#define DHTTYPE DHT11             // Define the type of DHT sensor
DHT dht(DHTPIN, DHTTYPE);         // Initialize the DHT sensor

ESP8266WebServer server(80);      // Create a web server object on port 80
String ssid, password, deviceID;  // Variables to store WiFi credentials and device ID
String content;                   // Variable to store HTML content
float temperature, humidity;      // Variables to store temperature and humidity readings

const int relayPin = 5;                     // GPIO5 (D1) for relay control
bool relayState = LOW;                      // Initial relay state
bool manualRelayControl = false;            // Flag to indicate manual relay control
bool isAPMode = false;                      // Flag to indicate if the ESP is in AP mode
const char* mqtt_server = "152.42.250.12";  // MQTT server IP address
unsigned long lastMillis = 0;              // Timer for periodic tasks
const long interval = 30000;               // 30 seconds interval for sensor readings and updates
bool reset = false;                        // Flag for resetting the time to update configuration

WiFiClient espClient;
PubSubClient client(espClient);   // MQTT client object

float tempThreshold = 35.0;                   // Default temperature threshold
float humThreshold = 90.0;                    // Default humidity threshold
float co2Threshold = 500.0;                   // Default CO2 threshold
const int mq135Pin = A0;                      // Analog pin for MQ-135 sensor

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  EEPROM.begin(512);     // Initialize EEPROM with 512 bytes of space
  delay(100);            

  pinMode(relayPin, OUTPUT);           // Set relay pin as an output
  digitalWrite(relayPin, relayState);  // Set initial relay state

  readData();   // Read stored data from EEPROM
  dht.begin();  // Initialize the DHT sensor

  if (!testWiFi()) {  // Test WiFi connection
    // Setup Access Point mode if WiFi connection fails
    const char* ssidap = "NodeMCU-AP";
    const char* passap = "";
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssidap, passap);
    Serial.print("AP mode: http://");
    Serial.println(WiFi.softAPIP());
    isAPMode = true;
    createWebServer();
    server.begin();
  }

  client.setServer(mqtt_server, 1883);  // Set MQTT server
  client.setCallback(callback);         // Set callback function for MQTT
}

void loop() {
  server.handleClient();  // Handle incoming web server client requests
  if (!client.connected()) {
    reconnect();  // Connect to the MQTT server and check if disconnect
  }
  client.loop();  // Get message from MQTT with the subscribe topic

  if (relayState) {
    digitalWrite(relayPin, HIGH);  // Keep relay on if relayState is true
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastMillis >= interval || reset) { //Send data every 30seconds or there are update to eeprom
    reset = false;
    lastMillis = currentMillis;

    if (!isAPMode) {  // Only read and print sensor data if not in AP mode
      int sensorValue = analogRead(mq135Pin);  // Read analog value from MQ-135 sensor
      float voltage = sensorValue * (5.0 / 1023.0);  // Convert analog value to voltage
      float co2Concentration = voltage * 200;  // Convert voltage to CO2 concentration
      humidity = dht.readHumidity();  // Read humidity from DHT sensor
      temperature = dht.readTemperature();  // Read temperature from DHT sensor
      
      Serial.print("CO2 Concentration: ");
      Serial.print(co2Concentration);
      Serial.println(" ppm");
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");
      Serial.print("Temperature: ");
      Serial.print(temperature);      
      Serial.println(" °C");

      if (!manualRelayControl) {  // Check if relay control is not manual controlled
        if (temperature > tempThreshold || humidity > humThreshold || co2Concentration > co2Threshold) {
          digitalWrite(relayPin, HIGH);  // Turn on the relay if conditions are met
          relayState = true;             // Update relay state to true
        } else {
          digitalWrite(relayPin, LOW);  // Turn off the relay if conditions are not met
          relayState = false;           // Update relay state to false
        }
      }

      if (WiFi.status() == WL_CONNECTED) {
        //Store the message sending to the MQTT in Json format
        StaticJsonDocument<256> doc;
        doc["temp"] = temperature;
        doc["humi"] = humidity;
        doc["concentration"] = co2Concentration;
        doc["ssid"] = ssid;
        doc["password"] = password;
        doc["deviceID"] = deviceID;
        doc["relayState"] = relayState;
        doc["humiThreshold"] = humThreshold;
        doc["tempThreshold"] = tempThreshold;
        doc["co2Threshold"] = co2Threshold;
        String a;
        serializeJson(doc, a);
        Serial.println("Data sent to MQTT server successfully");
        client.publish("289669_store", a.c_str());
      } else {
        Serial.println("WiFi disconnected");
      }
      checkWiFiConnection();  // Check WiFi connection status and handle reconnection
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  //This function receive message from MQTT when there are subscribed topic
  Serial.println("Received MQTT message");
  if (String(topic) == "289669_update") {
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    //Get only needed data
    if (doc.containsKey("tempThreshold")) {
      tempThreshold = doc["tempThreshold"].as<float>();
    }
    if (doc.containsKey("humiThreshold")) {
      humThreshold = doc["humiThreshold"].as<float>();
    }
    if (doc.containsKey("co2Threshold")) {
      co2Threshold = doc["co2Threshold"].as<float>();
    }
    if (doc.containsKey("relayState")) {
      relayState = doc["relayState"].as<bool>();
      digitalWrite(relayPin, relayState ? HIGH : LOW);  // Update relay pin state
    }
    if (doc.containsKey("manualRelayControl")) {
      manualRelayControl = doc["manualRelayControl"].as<bool>();
    }

    writeData(ssid, password, deviceID, relayState, tempThreshold, humThreshold, co2Threshold);

    Serial.println("Updated configuration from MQTT:");
    Serial.print("Temp Threshold: ");
    Serial.println(tempThreshold);
    Serial.print("Humidity Threshold: ");
    Serial.println(humThreshold);
    Serial.print("CO2 Threshold: ");
    Serial.println(co2Threshold);
    Serial.print("Relay State: ");
    Serial.println(relayState ? "ON" : "OFF");
    Serial.print("Manual Relay Control: ");
    Serial.println(manualRelayControl);
    reset = true;
  }
}

bool reconnect() {
  while (!client.connected() && !isAPMode) {
    //Only connect to MQTT server when there are wifi and MQTT server not connected
    Serial.print("Attempting MQTT connection...");
    if (client.connect("123")) {
      Serial.println("connected");
      if(client.subscribe("289669_update")){
        Serial.println("Subscribed to topic successfully.");
      }
      return true;
    } else {
      //Reconnect every 3seconds if failed to connect to MQTT server
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");
      delay(3000);
      return false;
    }
  }
  return false;
}

void createWebServer() {
  server.on("/", []() {
    String content = R"=====(
      <!DOCTYPE HTML>
      <html>
      <head>
        <title>Access Point Mode</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center; }
          h1 { font-size: 2.5rem; color: #333; }
          h2 { font-size: 2.0rem; color: #555; }
          p { font-size: 1.5rem; color: #666; }
          label { font-size: 1.2rem; color: #444; }
          input[type='text'], input[type='password'] { font-size: 1.2rem; padding: 10px; margin: 10px 0; width: 80%; }
          input[type='submit'] { font-size: 1.5rem; padding: 10px 20px; }
        </style>
      </head>
      <body>
        <h1>ESP8266 Access Point</h1>
        <h2>Configuration Portal</h2>
        <p>Enter your WiFi credentials and device ID:</p>
        <form action="/store" method="POST">
          <label for="ssid">SSID:</label><br>
          <input type="text" id="ssid" name="ssid"><br>
          <label for="password">Password:</label><br>
          <input type="password" id="password" name="password"><br>
          <label for="deviceID">Device ID:</label><br>
          <input type="text" id="deviceID" name="deviceID"><br><br>
          <input type="submit" value="Submit">
        </form>
      </body>
      </html>
      )=====";
    server.send(200, "text/html", content);  // Send HTML content to the client
  });

  server.on("/store", []() {
    String content = R"=====(
      <!DOCTYPE HTML>
      <html>
        <body>
          <h1>New Internet configuration updated. ESP module will restart soon.</h1>
        </body>
      </html>
      )=====";
    String ssid = server.arg("ssid");  // Get SSID from the form
    String password = server.arg("password");  // Get password from the form
    String deviceID = server.arg("deviceID");  // Get device ID from the form

    writeData(ssid, password, deviceID, relayState, tempThreshold, humThreshold, co2Threshold);  // Write data to EEPROM
    server.send(200, "text/html", content);  // Send confirmation message to the client
    delay(1000);  // Short delay before restarting
    ESP.restart();  // Restart the ESP module
  });
}

void readData() {
  ssid = readStringFromEEPROM(0);  // Read SSID from EEPROM starting at address 0
  password = readStringFromEEPROM(64);  // Read password from EEPROM starting at address 64
  deviceID = readStringFromEEPROM(128);  // Read device ID from EEPROM starting at address 128
  relayState = EEPROM.read(192);  // Read relay state from EEPROM at address 192
  EEPROM.get(193, tempThreshold);  // Read temperature threshold from EEPROM starting at address 193
  EEPROM.get(197, humThreshold);  // Read humidity threshold from EEPROM starting at address 197
  EEPROM.get(201, co2Threshold);  // Read CO2 threshold from EEPROM starting at address 201
  Serial.println("Data read from EEPROM:");
  Serial.println("SSID: " + ssid);
  Serial.println("Password: " + password);
  Serial.println("Device ID: " + deviceID);
  Serial.println("Relay State: " + String(relayState));
  Serial.println("Temp Threshold: " + String(tempThreshold));
  Serial.println("Humidity Threshold: " + String(humThreshold));
  Serial.println("CO2 Threshold: " + String(co2Threshold));
}

void writeData(String ssid, String password, String deviceID, bool relayState, float tempThreshold, float humThreshold, float co2Threshold) {
  writeStringToEEPROM(0, ssid);  // Write SSID to EEPROM starting at address 0
  writeStringToEEPROM(64, password);  // Write password to EEPROM starting at address 64
  writeStringToEEPROM(128, deviceID);  // Write device ID to EEPROM starting at address 128
  EEPROM.write(192, relayState);  // Write relay state to EEPROM at address 192
  EEPROM.put(193, tempThreshold);  // Write temperature threshold to EEPROM starting at address 193
  EEPROM.put(197, humThreshold);  // Write humidity threshold to EEPROM starting at address 197
  EEPROM.put(201, co2Threshold);  // Write CO2 threshold to EEPROM starting at address 201
  EEPROM.commit();  // Commit changes to EEPROM
  Serial.println("Data written to EEPROM:");
  Serial.println("SSID: " + ssid);
  Serial.println("Password: " + password);
  Serial.println("Device ID: " + deviceID);
  Serial.println("Relay State: " + String(relayState));
  Serial.println("Temp Threshold: " + String(tempThreshold));
  Serial.println("Humidity Threshold: " + String(humThreshold));
  Serial.println("CO2 Threshold: " + String(co2Threshold));
}

String readStringFromEEPROM(int startAddress) {
  char data[64];  // Buffer to store the read string
  for (int i = 0; i < sizeof(data); i++) {
    data[i] = EEPROM.read(startAddress + i);  // Read each character from EEPROM
  }
  return String(data);  // Convert the character array to a String
}

void writeStringToEEPROM(int startAddress, String value) {
  char data[64];  // Buffer to store the string to be written
  value.toCharArray(data, sizeof(data));  // Convert the String to a character array
  for (int i = 0; i < sizeof(data); i++) {
    EEPROM.write(startAddress + i, data[i]);  // Write each character to EEPROM
  }
}

bool testWiFi() {
  int c = 0;
  Serial.println("Waiting for WiFi to connect");
  WiFi.mode(WIFI_STA);  // Set WiFi mode to station
  WiFi.begin(ssid.c_str(), password.c_str());  // Start WiFi connection
  while (c < 20) {  // Try to connect for 20 attempts
    if (WiFi.status() == WL_CONNECTED) {  // Check if connected
      Serial.println("Connected to WiFi");
      Serial.print("Local IP: ");
      Serial.println(WiFi.localIP());  // Print local IP address
      return true;  // Return true if connected
    }
    delay(500);  // Delay for 500 milliseconds
    Serial.print(".");
    c++;
  }
  Serial.println("Failed to connect to WiFi");
  return false;  // Return false if not connected
}

void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {  // Check if WiFi is disconnected
    Serial.println("Reconnecting to WiFi...");
    WiFi.begin(ssid.c_str(), password.c_str());  // Attempt to reconnect
    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < 20) {  // Try to reconnect for 20 attempts
      delay(500);  // Delay for 500 milliseconds
      Serial.print(".");
      retryCount++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Reconnected to WiFi");
    } else {
      Serial.println("Failed to reconnect to WiFi");
    }
  }
}