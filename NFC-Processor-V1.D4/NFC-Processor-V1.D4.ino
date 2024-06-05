#include <ArduinoMqttClient.h>
#include <WiFiS3.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h> // Hardware-specific library

// Pin definitions (modify these as per your connection)
#define TFT_CS     10
#define TFT_RST    6
#define TFT_DC     9

// Create the display object
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// WiFi info
char ssid[] = "RCCF";    // your network SSID (name)
char pass[] = "auvsiauvsi";    // your network password (use for WPA, or use as key for WEP)

// MQTT info
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "192.168.200.200"; // Node-Red host machine IP
int        port     = 1883;
const char topic[]  = "NFC/Scanner";

const long interval = 1000;
unsigned long previousMillis = 0;

#include <SoftwareSerial.h>
SoftwareSerial mySerial(2, 3); // RX, TX (use the same different pins as the Nano)

String receivedPayload = "";
String displayBuffer[15]; // Buffer to store up to 15 lines of text
int currentLine = 0;

unsigned long lastDataTime = 0; // Timestamp of the last received data
unsigned long screenSaverTimeout = 60000; // 1 minute timeout for the screen saver
bool screenSaverActive = false;
bool labOpen = false; // Default lab status

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // Initialize software serial communication with the Nano
  mySerial.begin(9600);

  // Initialize the display
  tft.begin();
  tft.setRotation(1); // Adjust rotation if necessary
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(2);

  // Display initializing message
  tft.setCursor(0, tft.height() / 2 - 8);
  tft.println("Initializing...");
  delay(1000);

  // Attempt to connect to WiFi network:
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  // Display text
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, tft.height() / 2 - 8);
  tft.println("Connecting to: ");
  tft.print(ssid);
  delay(2000);

  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    tft.print(".");
    delay(5000);
  }

  Serial.println("You're connected to the network");
  Serial.println();
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, tft.height() / 2 - 8);
  tft.print("Connected to the network!");
  delay(500);
  
  // Attempt to connect to MQTT broker:
  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, tft.height() / 2 - 8);
  tft.println("Attempting to connect to");
  tft.println("the MQTT broker:");
  tft.print(broker);
  delay(1000);

  while (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code: ");
    Serial.println(mqttClient.connectError());

    // If failed to connect to MQTT broker
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, tft.height() / 2 - 8);
    tft.println("MQTT connection failed!");
    tft.print("Error Code: ");
    tft.println(mqttClient.connectError());
    tft.print("Retrying...");
    delay(500);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, tft.height() / 2 - 8);
  tft.println("Connected to Broker!");
  delay(500);

  Serial.println("Waiting for data...");
  lastDataTime = millis(); // Initialize the last data timestamp
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, tft.height() / 2 - 8);
  tft.println("Scanner Ready!");

  // Send the initial lab status
  updateLabStatus();
}

void loop() {
  // call poll() regularly to allow the library to send MQTT keep alives which
  // avoids being disconnected by the broker
  mqttClient.poll();

  // Check for incoming data from SoftwareSerial
  while (mySerial.available()) {
    char receivedChar = (char)mySerial.read();
    receivedPayload += receivedChar;

    // Check for newline character indicating the end of the message
    if (receivedChar == '\n') {
      Serial.print("Received payload: ");
      Serial.println(receivedPayload);

      // Send received payload to MQTT broker
      Serial.print("Sending message to topic: ");
      Serial.println(topic);

      mqttClient.beginMessage(topic);
      mqttClient.print(receivedPayload);
      mqttClient.endMessage();

      Serial.println("Message sent!");

      // Update lab status
      updateLabStatusWithPayload(receivedPayload);

      // Display the received payload on the screen
      // addToDisplayBuffer(receivedPayload);
      updateDisplay();

      receivedPayload = "";  // Clear the string for the next message
      lastDataTime = millis(); // Update the last data timestamp
      screenSaverActive = false; // Deactivate the screen saver
    }
  }

  // Check if the screen saver should be activated
  if (millis() - lastDataTime >= screenSaverTimeout && !screenSaverActive) {
    activateScreenSaver();
  }

  // If screen saver is active, update the animation
  if (screenSaverActive) {
    updateScreenSaver();
  }
}

void addToDisplayBuffer(String message) {
  // Add the message to the display buffer and handle scrolling
  if (currentLine < 15) {
    displayBuffer[currentLine] = message;
    currentLine++;
  } else {
    // Shift all lines up
    for (int i = 1; i < 15; i++) {
      displayBuffer[i - 1] = displayBuffer[i];
    }
    displayBuffer[14] = message;
  }
}

void updateDisplay() {
  // Clear the screen
  tft.fillScreen(ILI9341_BLACK);

  // Display each line of the buffer
  for (int i = 0; i < currentLine; i++) {
    tft.setCursor(0, i * 16);
    tft.print(displayBuffer[i]);
  }

  // Display the lab status at the bottom of the screen
  tft.setCursor(0, tft.height() - 16);
  if (labOpen) {
    tft.print("Lab Status: Open");
  } else {
    tft.print("Lab Status: Closed");
  }
}

void activateScreenSaver() {
  screenSaverActive = true;
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, tft.height() / 2 - 8);
  tft.println("Screen Saver Active");
}

void updateScreenSaver() {
  static unsigned long lastMoveTime = 0;
  const long moveInterval = 1000; // Move text every second
  if (millis() - lastMoveTime >= moveInterval) {
    lastMoveTime = millis();
    static int x = 0;
    static int y = 0;
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(x, y);
    tft.println("*");
    x = (x + 10) % tft.width();
    y = (y + 10) % tft.height();
  }
}

void updateLabStatusWithPayload(String payload) {
  // Determine if the lab status should be toggled based on the payload
  if (labOpen) {
    // If lab is currently open, close it
    payload += " closed the lab";
    labOpen = false;
  } else {
    // If lab is currently closed, open it
    payload += " opened the lab";
    labOpen = true;
  }

  // Send the updated status to the MQTT broker
  mqttClient.beginMessage(topic);
  mqttClient.print(payload);
  mqttClient.endMessage();

  // Add the status update to the display buffer
  addToDisplayBuffer(payload);
  updateDisplay();
}

void updateLabStatus() {
  // Send the initial lab status to the MQTT broker
  String statusMessage = "Lab is currently closed";
  mqttClient.beginMessage(topic);
  mqttClient.print(statusMessage);
  mqttClient.endMessage();

  // Add the status message to the display buffer
  // addToDisplayBuffer(statusMessage);
  updateDisplay();
}