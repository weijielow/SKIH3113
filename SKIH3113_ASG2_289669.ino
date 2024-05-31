#include <EEPROM.h>            // Library for reading and writing to EEPROM
#include <ESP8266WiFi.h>       // Library for WiFi functionality on ESP8266
#include <ESP8266WebServer.h>  // Library for creating a web server on ESP8266
#include <DHT.h>               // Library for DHT sensor

#define DHTPIN D4          // Define the pin where the DHT sensor is connected
#define DHTTYPE DHT11      // Define the type of DHT sensor
DHT dht(DHTPIN, DHTTYPE);  // Initialize the DHT sensor

ESP8266WebServer server(80);      // Create a web server object on port 80
String ssid, password, deviceID;  // Variables to store WiFi credentials and device ID
String content;                   // Variable to store HTML content
float temperature, humidity;      // Variables to store temperature and humidity readings
const int relayPin = 5;           // GPIO5 (D1) for relay control
bool relayState = LOW;            // Initial relay state
bool manualRelayControl = false;  // Flag to indicate manual relay control

void readData();                                                                                // Function to read data from EEPROM
boolean testWiFi();                                                                             // Function to test WiFi connection
void handleData();                                                                              // Function to handle data requests
void handleToggleRelay();                                                                       // Function to handle relay toggle requests
void writeData(String a, String b, String c, bool d, float tempThreshold, float humThreshold);  // Function to write data to EEPROM

float tempThreshold = 35.0;  // Default temperature threshold
float humThreshold = 90.0;   // Default humidity threshold

void setup() {
  Serial.begin(115200);  // Initialize serial communication at 115200 baud rate
  EEPROM.begin(512);     // Initialize EEPROM with 512 bytes of space
  delay(100);            // Delay for 100 milliseconds

  pinMode(relayPin, OUTPUT);           // Set relay pin as an output
  digitalWrite(relayPin, relayState);  // Set initial relay state

  readData();   // Read stored data from EEPROM
  dht.begin();  // Initialize the DHT sensor

  if (testWiFi()) {  // Test WiFi connection
    launchWeb(0);    // Launch web server in WiFi mode
  } else {
    const char* ssidap = "NodeMCU-AP";  // SSID for Access Point mode
    const char* passap = "";            // Password for Access Point mode
    WiFi.mode(WIFI_AP);                 // Set WiFi mode to Access Point
    WiFi.softAP(ssidap, passap);        // Start Access Point with specified SSID and password
    Serial.print("AP mode-http://");    // Print AP mode message to serial
    Serial.println(WiFi.softAPIP());    // Print Access Point IP address
    launchWeb(1);                       // Launch web server in Access Point mode
  }
}

void launchWeb(int webtype) {
  createWebServer(webtype);  // Create web server based on mode
  server.begin();            // Start the web server
}

void loop() {
  server.handleClient();                // Handle incoming client requests
  humidity = dht.readHumidity();        // Read humidity from DHT sensor
  temperature = dht.readTemperature();  // Read temperature from DHT sensor
  Serial.print("humidity: ");           // Print humidity to serial
  Serial.println(humidity);             // Print humidity value to serial
  Serial.print("temperature: ");        // Print temperature to serial
  Serial.println(temperature);          // Print temperature value to serial

  if (!manualRelayControl) {  // Check if relay control is not manual
    if (temperature > tempThreshold || humidity > humThreshold) {
      digitalWrite(relayPin, HIGH);  // Turn on the relay if conditions are met
      relayState = true;             // Set relay state to true
    } else {
      digitalWrite(relayPin, LOW);  // Turn off the relay if conditions are not met
      relayState = false;           // Set relay state to false
    }
  }

  delay(1000);  // Delay for 1 second
}

void createWebServer(int webtype) {
  if (webtype == 0) {
    server.on("/", []() {
      String content = R"=====(
<!DOCTYPE HTML>
<html>
<head>
  <title>Humidifier Tracking Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center; }
    h1 { font-size: 2.5rem; color: #ffffff; background-color: #00a6b6; padding: 20px; }
    h2 { font-size: 2.0rem; color: #333; }
    h3 { font-size: 1.5rem; color: #ffffff; background-color: #00a6b6; padding: 10px; margin: 0; }
    p { font-size: 1.5rem; margin: 10px 0; }
    .container { width: 80%; margin: 20px auto; border: 1px solid #ddd; border-radius: 10px; overflow: hidden; }
    .header { background-color: #00a6b6; color: #ffffff; padding: 10px 0; }
    .content { padding: 20px; background-color: #f9f9f9; }
    .row { display: flex; justify-content: space-between; margin: 10px 0; }
    .row p { margin: 0; }
    button { font-size: 1.2rem; padding: 10px 20px; margin-top: 20px; background-color: #00a6b6; color: #ffffff; border: none; border-radius: 5px; cursor: pointer; }
    button:hover { background-color: #008a99; }
    input[type='text'] { font-size: 1.2rem; padding: 10px; margin: 10px 0; width: 80%; }
  </style>
</head>
<body>
  <h1>Humidifier Tracking Web Server</h1>
  <h2>WiFi Mode</h2>
  <p>Welcome back, )====="+ String(deviceID) + R"=====()</p>
  <p>SSID: )=====" + String(ssid)+ R"=====()</p>
  <div class="container">
    <div class="header">Current Value</div>
    <div class="content">
      <div class="row"><p>Temperature:</p><p><span id='temperature'>)====="+ String(temperature, 2) + R"=====(&deg;C</span></p></div>
      <div class="row"><p>Humidity:</p><p><span id='humidity'>)====="+ String(humidity, 2) + R"=====(%</span></p></div>
    </div>
  </div>
  <div class="container">
    <div class="header">Current Threshold</div>
    <div class="content">
      <div class="row"><p>Temperature:</p><p><span id='tempThreshold'>)====="+ String(tempThreshold, 2) + R"=====(&deg;C</span></p></div>
      <div class="row"><p>Humidity:</p><p><span id='humThreshold'>)====="+ String(humThreshold, 2) + R"=====(%</span></p></div>
      <form method='get' action='threshold'>
        <label for='tempThreshold'>Temperature Threshold: </label><br><input type='text' id='tempThreshold' name='tempThreshold' value=')=====" + String(tempThreshold) + R"=====(' maxlength='5'><br>
        <label for='humThreshold'>Humidity Threshold: </label><br><input type='text' id='humThreshold' name='humThreshold' value=')====="+ String(humThreshold) + R"=====(' maxlength='5'><br>
        <input type='submit' value='Update Thresholds'>
      </form>
    </div>
  </div>
  <div class="container">
    <div class="header">Relay Status</div>
    <div class="content">
      <div class="row"><p>Status:</p><p><span id='relayState'>)====="+ String(relayState ? "ON" : "OFF") + R"=====()</span></p></div>
      <button id='toggleButton' onclick="toggleRelay()">)====="+ String(relayState ? "Turn Off" : "Turn On") + R"=====() Relay</button>
    </div>
  </div>
  <script>
    function fetchData() {
      fetch('/data').then(response => response.json()).then(data => {
        document.getElementById('temperature').innerHTML = data.temperature.toFixed(2) + '&deg;C';
        document.getElementById('humidity').innerText = data.humidity.toFixed(2) + '%';
        document.getElementById('relayState').innerText = data.relayState ?'ON' : 'OFF';
        document.getElementById('toggleButton').innerText = data.relayState ? 'Turn Off Relay' : 'Turn On Relay';
        document.getElementById('tempThreshold').innerHTML = data.tempThreshold.toFixed(2) + '&deg;C';
        document.getElementById('humThreshold').innerText = data.humThreshold.toFixed(2) + '%';
      });
    }
    function toggleRelay() {
      fetch('/toggleRelay').then(response => response.json()).then(data => {
        document.getElementById('relayState').innerText = data.relayState ? 'ON' : 'OFF';
        document.getElementById('toggleButton').innerText = data.relayState ? 'Turn Off Relay' : 'Turn On Relay';
      });
    }
    setInterval(fetchData, 1000);  // Fetch data every second
  </script>
</body>
</html>
)=====";
      server.send(200, "text/html", content);  // Send the HTML content to the client
    });

    server.on("/data", handleData);                // Handle data requests
    server.on("/toggleRelay", handleToggleRelay);  // Handle relay toggle requests
    server.on("/threshold", []() {
      tempThreshold = server.arg("tempThreshold").toFloat();                         // Update temperature threshold
      humThreshold = server.arg("humThreshold").toFloat();                           // Update humidity threshold
      writeData(ssid, password, deviceID, relayState, tempThreshold, humThreshold);  // Write updated data to EEPROM
      content = "Thresholds updated successfully. <a href='/'>Go back</a>";          // Confirmation message
      server.send(200, "text/html", content);                                        // Send confirmation message to the client
    });
  }

  if (webtype == 1) {  // Access Point mode
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
    input[type='submit'] { font-size: 1.5rem; padding: 10px 20px; margin-top: 20px; }
    form { margin: 20px 0; }
  </style>
</head>
<body>
  <h1>Access Point Mode</h1>
  <h2>Welcome to NodeMCU WiFi Configuration</h2>
  <p><b>Your current configuration</b></p>
  <p>SSID: )=====" + ssid + R"=====(<br>
  Password: )=====" + password+ R"=====(
  </p>
  <p><b>New configuration</b></p>
  <form method='get' action='setting'>
    <label for='SSID'>SSID: </label><br><input type='text' id='SSID' name='SSID' maxlength='32'><br>
    <label for='password'>Password: </label><br><input type='password' id='password' name='password' maxlength='32'><br>
    <label for='deviceID'>Device ID: </label><br><input type='text' id='deviceID' name='deviceID' maxlength='32'><br>
    <label for='initialStatus'>Initial Device Status: </label><br>
    <input type='radio' id='statusOn' name='initialStatus' value='on'><label for='statusOn'>On</label><br>
    <input type='radio' id='statusOff' name='initialStatus' value='off' checked><label for='statusOff'>Off</label><br>
    <br><input type='submit' value='Save Configuration'>
  </form>
</body>
</html>
)=====";
      server.send(200, "text/html", content);  // Send HTML content to the client
    });

    server.on("/setting", []() {
      ssid = server.arg("SSID");                           // Get SSID from the form
      password = server.arg("password");                   // Get password from the form
      deviceID = server.arg("deviceID");                   // Get device ID from the form
      String initialStatus = server.arg("initialStatus");  // Get initial status from the form
      relayState = (initialStatus == "on");                // Set relay state based on initial status

      writeData(ssid, password, deviceID, relayState, tempThreshold, humThreshold);  // Write data to EEPROM
      content = "Success. Please reboot to take effect";                             // Confirmation message
      server.send(200, "text/html", content);                                        // Send confirmation message to the client
    });
  }
}

void handleData() {
  // Create JSON response with current data
  String json = "{\"temperature\":" + String(temperature) + ", \"humidity\":" + String(humidity) + ", \"relayState\":" + String(relayState ? "true" : "false") + ", \"tempThreshold\":" + String(tempThreshold) + ", \"humThreshold\":" + String(humThreshold) + "}";
  server.send(200, "application/json", json);  // Send JSON response to the client
}

void handleToggleRelay() {
  relayState = !relayState;                         // Toggle the relay state
  digitalWrite(relayPin, relayState ? HIGH : LOW);  // Update relay pin state
  manualRelayControl = true;                        // Indicate manual relay control

  if (!relayState) {
    manualRelayControl = false;  // Reset manual control flag if relay is off
  }

  // Create JSON response with new relay state
  String json = "{\"relayState\":" + String(relayState ? "true" : "false") + "}";
  server.send(200, "application/json", json);  // Send JSON response to the client

  // Print to Serial for debugging
  Serial.println("Relay toggled via web interface");
  Serial.print("New relay state: ");
  Serial.println(relayState ? "ON" : "OFF");
}

boolean testWiFi() {
  WiFi.begin(ssid.c_str(), password.c_str());  // Start WiFi connection
  int c = 0;                                   // Counter for connection attempts
  while (c < 15) {                             // Try to connect for 15 attempts
    if (WiFi.status() == WL_CONNECTED) {       // Check if connected
      Serial.println();                        // New line for readability
      Serial.print("WiFi Status: ");           // Print WiFi status
      Serial.println(WiFi.status());           // Print WiFi status value
      Serial.print("WiFi local IP: ");         // Print local IP
      Serial.println(WiFi.localIP());          // Print local IP address
      return true;                             // Return true if connected
    }
    Serial.print(".");  // Print dot for each attempt
    delay(500);         // Delay for 500 milliseconds
    c++;                // Increment counter
  }
  Serial.println("Connection time out");  // Print timeout message
  return false;                           // Return false if not connected
}

void writeData(String a, String b, String c, bool d, float tempThresh, float humThresh) {
  Serial.println("Writing to EEPROM");                               // Print message to serial
  Serial.println("SSID to write: " + a);                             // Print SSID
  Serial.println("Password to write: " + b);                         // Print password
  Serial.println("Device ID to write: " + c);                        // Print device ID
  Serial.println("Relay state to write: " + String(d));              // Print relay state
  Serial.println("Temp threshold to write: " + String(tempThresh));  // Print temp threshold
  Serial.println("Hum threshold to write: " + String(humThresh));    // Print hum threshold

  // Write SSID to EEPROM
  for (int i = 0; i < 20; i++) {
    if (i < a.length()) {
      EEPROM.write(i, a[i]);  // Write each character of SSID
    } else {
      EEPROM.write(i, 0);  // Write null character if SSID is shorter
    }
  }

  // Write Password to EEPROM
  for (int i = 20; i < 40; i++) {
    if (i - 20 < b.length()) {
      EEPROM.write(i, b[i - 20]);  // Write each character of password
    } else {
      EEPROM.write(i, 0);  // Write null character if password is shorter
    }
  }

  // Write Device ID to EEPROM
  for (int i = 40; i < 60; i++) {
    if (i - 40 < c.length()) {
      EEPROM.write(i, c[i - 40]);  // Write each character of device ID
    } else {
      EEPROM.write(i, 0);  // Write null character if device ID is shorter
    }
  }

  // Write Relay State to EEPROM
  EEPROM.write(60, d ? 1 : 0);  // Write 1 if relay state is true, otherwise 0

  // Write Temperature Threshold to EEPROM
  EEPROM.put(61, tempThresh);  // Write temperature threshold starting at address 61

  // Write Humidity Threshold to EEPROM
  EEPROM.put(65, humThresh);  // Write humidity threshold starting at address 65

  EEPROM.commit();                     // Commit changes to EEPROM
  Serial.println("Write successful");  // Print success message
}

void readData() {
  Serial.println("Reading from EEPROM....");  // Print message to serial
  char ssidArr[21];                           // Array to store SSID (20 characters + null terminator)
  char passwordArr[21];                       // Array to store password (20 characters + null terminator)
  char deviceIDArr[21];                       // Array to store device ID (20 characters + null terminator)

  // Read SSID from EEPROM
  for (int i = 0; i < 20; i++) {
    ssidArr[i] = char(EEPROM.read(i));  // Read each character of SSID
  }
  ssidArr[20] = '\0';  // Null terminate the SSID string

  // Read Password from EEPROM
  for (int i = 20; i < 40; i++) {
    passwordArr[i - 20] = char(EEPROM.read(i));  // Read each character of password
  }
  passwordArr[20] = '\0';  // Null terminate the password string

  // Read Device ID from EEPROM
  for (int i = 40; i < 60; i++) {
    deviceIDArr[i - 40] = char(EEPROM.read(i));  // Read each character of device ID
  }
  deviceIDArr[20] = '\0';  // Null terminate the device ID string

  ssid = String(ssidArr);          // Convert SSID array to String
  password = String(passwordArr);  // Convert password array to String
  deviceID = String(deviceIDArr);  // Convert device ID array to String

  relayState = EEPROM.read(60) == 1;  // Read relay state from EEPROM

  // Read Temperature Threshold from EEPROM
  EEPROM.get(61, tempThreshold);  // Get temperature threshold starting at address 61

  // Read Humidity Threshold from EEPROM
  EEPROM.get(65, humThreshold);  // Get humidity threshold starting at address 65

  // Print read values to serial
  Serial.println("WiFi SSID from EEPROM: " + ssid);
  Serial.println("WiFi password from EEPROM: " + password);
  Serial.println("Device ID from EEPROM: " + deviceID);
  Serial.println("Relay state from EEPROM: " + String(relayState));
  Serial.println("Temp threshold from EEPROM: " + String(tempThreshold));
  Serial.println("Hum threshold from EEPROM: " + String(humThreshold));
  Serial.println("Reading successful");  // Print success message
}
