#include <Wire.h>                     // Include the Wire library for I2C communication
#include <Adafruit_GFX.h>             // Include Adafruit_GFX library for graphics
#include <Adafruit_SSD1306.h>         // Include Adafruit_SSD1306 library for OLED display
#include <dht.h>                      // Include dht library for DHT sensor
#include <ESP8266WiFi.h>              // Include ESP8266WiFi library for ESP8266 Wi-Fi functionality
#include <WiFiClient.h>               // Include WiFiClient library for creating Wi-Fi clients
#include <ESP8266WebServer.h>         // Include ESP8266WebServer library for handling web server functionality

#define SCREEN_WIDTH 128             // Define OLED display width in pixels
#define SCREEN_HEIGHT 64             // Define OLED display height in pixels

#define D4 2                         // Define the pin connected to DHT11 sensor
dht DHT;                             // Create a DHT object

// Initialize SSD1306 display with specified dimensions and I2C address
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire, -1);

// Define the SSID and password for connecting to the Internet
const char* ssid = "ASG1";
const char* password = "23456789";

// Create an ESP8266WebServer object on port 80
ESP8266WebServer server(80);

String displayString;

// Handle root request
void handleRoot() {
  float humi  = DHT.humidity;        // Read humidity from DHT sensor
  float temp = DHT.temperature;      // Read temperature from DHT sensor

  // Check if any reads failed
  if (isnan(humi) || isnan(temp)) { // If reading of humidity or temperature is null
    server.send(500, "text/plain", "Failed to read from DHT sensor!"); // Server will send different response instead of the normal displaying the results
    return;
  }

  // Construct the HTML response to display temperature and humidity
  String response = "<!DOCTYPE html><html><head><title>SKIH3113-ASG1 HUMIDIFIER MOBILE TRACKING SYSTEM</title>";
  response += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  response += "<link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\" integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">";
  response += "<style>";
  response += "html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center; }";
  response += "h2 { font-size: 3.0rem; }";
  response += "p { font-size: 3.0rem; }";
  response += ".units { font-size: 1.2rem; }";
  response += ".dht-labels { font-size: 1.5rem; vertical-align: middle; padding-bottom: 15px; }";
  response += "</style></head><body>";
  response += "<h2>ESP8266 DHT Server</h2>";
  response += "<p><i class=\"fas fa-thermometer-half\" style=\"color:#059e8a;\"></i> <span class=\"dht-labels\">Temperature</span> <span id=\"temperature\">" + String(temp, 1) + "</span><sup class=\"units\">&deg;C</sup></p>";
  response += "<p><i class=\"fas fa-tint\" style=\"color:#00add6;\"></i> <span class=\"dht-labels\">Humidity</span> <span id=\"humidity\">" + String(humi, 1) + "</span><sup class=\"units\">%</sup></p>";
  response += "<script>setInterval(function() { fetch('/values').then(response => response.json()).then(data => { document.getElementById('temperature').innerText = data.temp; document.getElementById('humidity').innerText = data.humi; }); }, 1000);</script>";
  response += "</body></html>";

  server.send(200, "text/html", response);  // Send the HTML response
}

// Handle values request
void handleValues() {
  float humi  = DHT.humidity;              // Read humidity from DHT sensor
  float temp = DHT.temperature;            // Read temperature from DHT sensor

  // Construct JSON response with temperature and humidity values
  String json = "{\"temp\": " + String(temp, 1) + ", \"humi\": " + String(humi, 1) + "}";
  server.send(200, "application/json", json); // Send JSON response
}

void setup() {
  Serial.begin(115200);                    // Initialize serial communication
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Initialize OLED display
  display.display();                        // Turn on the OLED display
  delay(100);                               // Delay for stability
  display.clearDisplay();                   // Clear the OLED display buffer
  display.display();                        // Display the cleared buffer on the OLED display
  display.setTextSize(1.75);                // Set text size on OLED display
  display.setTextColor(WHITE);              // Set text color on OLED display to white

  WiFi.begin(ssid, password);               // Connect to Internet network declared previously

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);                             // Delay for 500 milliseconds
    Serial.print(".");                      // Print a dot to indicate the waiting process
  }
  Serial.print("\nIP address: ");           // Print message indicating IP address
  Serial.println(WiFi.localIP());           // Print the local IP address obtained

  // Define handlers for HTTP requests
  server.on("/", handleRoot);               // Assign handleRoot() function to handle requests to root URL
  server.on("/values", HTTP_GET, handleValues); // Assign handleValues() function to handle requests to "/values" URL with HTTP GET method and update the lastest data detected by the sensor
  server.begin();                           // Start the HTTP server
  Serial.println("Http server started");    // Print message indicating server started
}

void loop() {
  server.handleClient();                   // Handle incoming web server requests

  int chk = DHT.read11(D4);                // Read data from DHT11 sensor
  float humi  = DHT.humidity;              // Read humidity value from DHT sensor
  float temp = DHT.temperature;            // Read temperature value from DHT sensor

  // Check if any reads failed
  if (isnan(humi) || isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");  // Print error message if reading from DHT sensor fails
    return;  // Exit loop if reading fails
  }

  // Print temperature and humidity to serial monitor
  Serial.print("Temperature: "); // Print the text
  Serial.print(String(temp,1));  // Print temperature value with one decimal place
  Serial.print("°C \nHumidity: "); // Print the text
  Serial.print(String(humi,1));  // Print humidity value with one decimal place
  Serial.println("%");

  display.setCursor(0,0); // Set cursor to the top left corner of the OLED display
  display.clearDisplay(); // Clear the OLED content

  // Display and format temperature on OLED display
  display.setTextSize(1); // Set the displayed text size to be 1
  display.setCursor(0,0); // Set the cursor position to the top-left corner of the OLED display
  display.print("Temperature: "); // Print the text
  display.setTextSize(2); // Set the displayed text size to be 2
  display.setCursor(0,10); // Set the cursor position for the temperature value
  display.print(String(temp, 1));  // Print the temperature value with one decimal place
  display.print(" "); // Print a space
  display.setTextSize(1); // Set the displayed text size to be 1
  display.cp437(true); // Enable CP437 character set for extended character support
  display.write(167);  // Print the degree symbol (°)
  display.setTextSize(2); // Set the displayed text size to be 2
  display.print("C");  // Print the text
  
  // Display and format humidity on OLED display
  display.setTextSize(1); // Set the displayed text size to be 1
  display.setCursor(0, 35); // Set the cursor position for the humidity label
  display.print("Humidity: "); // Print the text
  display.setTextSize(2); // Set the displayed text size to be 2
  display.setCursor(0, 45); // Set the cursor position for the humidity value
  display.print(String(humi, 1));  // Print the humidity value with one decimal place
  display.print(" %");  // Print the text

  display.display();  // Display the updated OLED display
}

