#include <Adafruit_SSD1306.h>
#include <splash.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPSRedirect.h>
#include <Wire.h>

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//---------------------------------------------------------------------------------------------------------
// Enter Google Script Deployment ID:
const char *GScriptId = "AKfycbz9TJ1djWLqvcvgpU6VkAajeRHwfu-PuMecpsJWUdK_L4jsELb8kbqWLG4oxNPBFb-9-w"; // Deployment ID for Google Apps Script

//---------------------------------------------------------------------------------------------------------
// Enter network credentials:
const char* ssid     = "ABCD1234";
const char* password = "fruclino";

//---------------------------------------------------------------------------------------------------------
// Enter command (insert_row or append_row) and your Google Sheets sheet name (default is Sheet1):
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = ""; // Will hold the final JSON payload

//---------------------------------------------------------------------------------------------------------
// Google Sheets Setup
const char* host        = "script.google.com";
const int   httpsPort   = 443;
String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect* client = nullptr;

//---------------------------------------------------------------------------------------------------------
// Declare variables that will be published to Google Sheets
String student_id = ""; // This will be extracted from the card data

//---------------------------------------------------------------------------------------------------------
int blocks[] = {4,5,6,8,9}; 
#define total_blocks  (sizeof(blocks) / sizeof(blocks[0]))

//---------------------------------------------------------------------------------------------------------
#define RST_PIN  0  // RFID reset pin (D3)
#define SS_PIN   2  // RFID slave select pin (D4)

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

byte bufferLen = 18; 
byte readBlockData[18];

//---------------------------------------------------------------------------------------------------------
// Idle animation for OLED display
void idleAnimation() {
  static int x = 0; // Position of the eyes
  static bool blink = false; // Control for blinking

  display.clearDisplay();

  int eyeSize = 12; // Eye size
  int eyeX1 = x;      // Right eye position
  int eyeX2 = x + 30; // Left eye position

  display.fillRect(eyeX1, 20, eyeSize, eyeSize, SSD1306_WHITE);
  display.fillRect(eyeX2, 20, eyeSize, eyeSize, SSD1306_WHITE);
  display.setCursor(0,50);
  display.print("Scan your Tag");
  display.display();
  
  // Blink effect
  if (blink) {
    display.fillRect(eyeX1, 20, eyeSize, eyeSize, SSD1306_BLACK);
    display.fillRect(eyeX2, 20, eyeSize, eyeSize, SSD1306_BLACK);
    display.setCursor(0,50);
    display.print("Scan your Tag");
    display.display();
  }
  
  blink = !blink;
  x += 3;
  if (x > 60) {
    x = 0;
  }
  
  delay(500); // Control the animation speed
}

//---------------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  delay(10);
  Serial.println();

  SPI.begin(); 
  mfrc522.PCD_Init();  // Initialize RFID reader once during setup

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Connect to WiFi
  WiFi.begin(ssid, password);           
  Serial.print("Connecting to ");
  Serial.print(ssid); 
  Serial.println(" ...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }
  Serial.println();
  Serial.println("WiFi Connected!");
  Serial.println(WiFi.localIP());
 
  // Set up HTTPSRedirect client
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");

  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Connecting to Google");
  display.display();
  delay(5000);

  Serial.print("Connecting to ");
  Serial.println(host);

  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = client->connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      String msg = "Connected. OK";
      Serial.println(msg);
      display.clearDisplay();                                
      display.setCursor(0,0);
      display.print(msg);
      display.display();
      delay(2000);
      break;
    } else {
      Serial.println("Connection failed. Retrying...");
    }
  }
  if (!flag) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Connection fail");
    display.display();
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    delay(5000);
    return;
  }
  delete client;
  client = nullptr;
}

void loop() {
  static bool flag = false;
  if (!flag) {
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }
  
  if (client != nullptr) {
    if (!client->connected()) {
      int retval = client->connect(host, httpsPort);
      if (retval != 1) {
        Serial.println("Disconnected. Retrying...");
        display.clearDisplay();
        display.setCursor(0,0);
        display.print("Disconnected.");
        display.setCursor(0,10);
        display.print("Retrying...");
        display.display();
        return;
      }
    }
  } else {
    Serial.println("Error creating client object!"); 
  }
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Scan your Tag");
  display.display();

  // Look for a new card
  if (!mfrc522.PICC_IsNewCardPresent()) {
    idleAnimation();
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println();
  Serial.println(F("Reading data from RFID..."));

  String values = "";
  String data; 
  for (byte i = 0; i < total_blocks; i++) {
    ReadDataFromBlock(blocks[i], readBlockData);
    data = String((char*)readBlockData);
    data.trim();
    values += data + ",";
  }
  
  // Remove the trailing comma and extract student_id (assumes first value is the student ID)
  if(values.length() > 0) {
    values = values.substring(0, values.length() - 1);
    int commaIndex = values.indexOf(",");
    if (commaIndex > 0) {
      student_id = values.substring(0, commaIndex);
    } else {
      student_id = values;
    }
  }
  
  // Create JSON payload to send to Google Sheets
  payload = payload_base + "\"" + values + "\"}";

  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Publishing Data");
  display.setCursor(0,10);
  display.print("Please Wait...");
  display.display();

  Serial.println("Publishing data...");
  Serial.println(payload);

  if (client->POST(url, host, payload)) {
    Serial.println("[OK] Data published.");
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Student ID: " + student_id);
    display.setCursor(0,10);
    display.print("Thanks");
    display.display();
  } else {
    Serial.println("Error while connecting");
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Failed.");
    display.setCursor(0,10);
    display.print("Try Again");
    display.display();
  }
  
  Serial.println("[TEST] delay(5000)");
  delay(5000);
}

//---------------------------------------------------------------------
// ReadDataFromBlock() function: Reads data from a specified block on the RFID card.
void ReadDataFromBlock(int blockNum, byte readBlockData[]) {
  // Prepare the key for authentication
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  
  // Authenticate the desired block using Key A
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed for Read: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    Serial.println("Authentication success");
  }
  
  // Read data from the block
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    // Null-terminate the data to avoid unwanted characters
    readBlockData[16] = ' ';
    readBlockData[17] = ' ';
    Serial.println("Block read successfully");
  }
}
