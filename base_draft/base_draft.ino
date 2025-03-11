/**
 * ESP32-CAM Based Attendance System
 * Features:
 * - RFID Authentication using MFRC522
 * - Facial Recognition verification
 * - Google Sheets integration for attendance tracking
 * - Voice command interface using I2S Microphone
 * - OLED Display for user interface
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include "esp_camera.h"
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/i2s.h>
#include <HTTPClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// WiFi Credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Google Script ID for Google Sheets integration
const char* googleScriptId = "YOUR_GOOGLE_SCRIPT_ID";

// Pin Definitions
#define I2S_WS_PIN 15  // I2S word select pin
#define I2S_SD_PIN 13  // I2S serial data pin
#define I2S_SCK_PIN 2  // I2S serial clock pin
#define SDA_PIN 14     // I2C data pin for OLED
#define SCL_PIN 12     // I2C clock pin for OLED
#define RFID_SS_PIN 33 // SPI Slave Select for RFID
#define RFID_RST_PIN 32 // Reset pin for RFID
#define LED_SUCCESS 4   // Success indicator LED
#define LED_ERROR 0     // Error indicator LED

// OLED Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// RFID
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

// I2S Configuration
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE 16000  // 16kHz sample rate
#define I2S_BUFFER_SIZE 512    // Buffer size

// Camera Configuration
camera_config_t camera_config;

// Function Declarations
void setupCamera();
void setupRFID();
void setupOLED();
void setupI2S();
void setupWiFi();
bool authenticateRFID(String &userId);
bool verifyFace(String userId);
void recordAttendance(String userId);
void processVoiceCommand();
void displayMessage(String message, bool clearDisplay = true);
void captureImage();
void sendToGoogleSheets(String userId, bool verified);

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout detector
  
  Serial.begin(115200);
  Serial.println("ESP32-CAM Attendance System");
  
  // Initialize LED indicators
  pinMode(LED_SUCCESS, OUTPUT);
  pinMode(LED_ERROR, OUTPUT);
  digitalWrite(LED_SUCCESS, LOW);
  digitalWrite(LED_ERROR, LOW);
  
  // Initialize all components
  setupWiFi();
  setupCamera();
  setupRFID();
  setupOLED();
  setupI2S();
  
  displayMessage("System Ready");
  Serial.println("System Ready");
}

void loop() {
  // Check if an RFID card is present
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String userId = "";
    
    // Convert RFID UID to string
    for (byte i = 0; i < rfid.uid.size; i++) {
      userId += (rfid.uid.uidByte[i] < 0x10 ? "0" : "");
      userId += String(rfid.uid.uidByte[i], HEX);
    }
    userId.toUpperCase();
    
    Serial.println("RFID Detected: " + userId);
    displayMessage("Card detected\nVerifying...");
    
    // Authenticate user with RFID
    if (authenticateRFID(userId)) {
      displayMessage("RFID Valid\nChecking face...");
      
      // Capture image and verify face
      if (verifyFace(userId)) {
        digitalWrite(LED_SUCCESS, HIGH);
        displayMessage("Access Granted\nWelcome, User");
        recordAttendance(userId);
        delay(2000);
        digitalWrite(LED_SUCCESS, LOW);
      } else {
        digitalWrite(LED_ERROR, HIGH);
        displayMessage("Face Verification\nFailed!");
        delay(2000);
        digitalWrite(LED_ERROR, LOW);
      }
    } else {
      digitalWrite(LED_ERROR, HIGH);
      displayMessage("Invalid Card!");
      delay(2000);
      digitalWrite(LED_ERROR, LOW);
    }
    
    // Halt PICC and stop encryption
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
  
  // Process voice commands if available
  processVoiceCommand();
  
  delay(100); // Small delay to prevent CPU hogging
}

void setupWiFi() {
  Serial.println("Connecting to WiFi...");
  displayMessage("Connecting to\nWiFi...");
  
  WiFi.begin(ssid, password);
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.println("IP address: " + WiFi.localIP().toString());
    displayMessage("WiFi Connected\nIP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connection failed");
    displayMessage("WiFi Failed!\nOperating offline");
  }
}

void setupCamera() {
  Serial.println("Initializing Camera...");
  displayMessage("Initializing\nCamera...");
  
  // Configure camera pins for ESP32-CAM
  camera_config.ledc_channel = LEDC_CHANNEL_0;
  camera_config.ledc_timer = LEDC_TIMER_0;
  camera_config.pin_d0 = 5;
  camera_config.pin_d1 = 18;
  camera_config.pin_d2 = 19;
  camera_config.pin_d3 = 21;
  camera_config.pin_d4 = 36;
  camera_config.pin_d5 = 39;
  camera_config.pin_d6 = 34;
  camera_config.pin_d7 = 35;
  camera_config.pin_xclk = 0;
  camera_config.pin_pclk = 22;
  camera_config.pin_vsync = 25;
  camera_config.pin_href = 23;
  camera_config.pin_sscb_sda = 26;
  camera_config.pin_sscb_scl = 27;
  camera_config.pin_pwdn = 32;
  camera_config.pin_reset = -1;
  camera_config.xclk_freq_hz = 20000000;
  camera_config.pixel_format = PIXFORMAT_JPEG;
  
  // Configure camera quality
  camera_config.frame_size = FRAMESIZE_VGA; // 640x480 resolution
  camera_config.jpeg_quality = 10;          // Lower number means higher quality
  camera_config.fb_count = 2;
  
  // Initialize camera
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    displayMessage("Camera Init\nFailed!");
  } else {
    Serial.println("Camera initialized successfully");
  }
}

void setupRFID() {
  Serial.println("Initializing RFID Reader...");
  displayMessage("Initializing\nRFID Reader...");
  
  SPI.begin();      // Initialize SPI bus
  rfid.PCD_Init();  // Initialize MFRC522
  
  Serial.print("RFID Reader firmware version: ");
  Serial.println(rfid.PCD_GetVersion(), HEX);
}

void setupOLED() {
  Serial.println("Initializing OLED Display...");
  
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Starting...");
    display.display();
  }
}

void setupI2S() {
  Serial.println("Initializing I2S Microphone...");
  displayMessage("Initializing\nMicrophone...");
  
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = I2S_BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  
  // I2S pin configuration
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD_PIN
  };
  
  // Initialize I2S driver
  esp_err_t result = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (result != ESP_OK) {
    Serial.printf("Error initializing I2S driver: %d\n", result);
    displayMessage("Mic Init Failed");
  } else {
    // Set I2S pins
    result = i2s_set_pin(I2S_PORT, &pin_config);
    if (result != ESP_OK) {
      Serial.printf("Error setting I2S pins: %d\n", result);
    }
  }
}

bool authenticateRFID(String &userId) {
  // In a real application, you would check the RFID against a database
  // For this demo, we'll accept certain cards (you would replace this with your logic)
  
  Serial.println("Authenticating RFID: " + userId);
  
  // Example of checking against hardcoded authorized IDs
  // Replace with your own authentication logic
  if (userId == "ABCD1234" || userId == "EFGH5678") {
    Serial.println("RFID authenticated successfully");
    return true;
  }
  
  Serial.println("RFID authentication failed");
  return false;
}

bool verifyFace(String userId) {
  // 1. Capture an image
  Serial.println("Capturing image for facial verification...");
  displayMessage("Look at camera");
  
  // Allow time for person to position in front of camera
  delay(1500);
  
  // Get camera frame buffer
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }
  
  // In a real application, you would:
  // 1. Process the image for facial recognition
  // 2. Compare against stored templates for the userId
  // 3. Return true if face matches, false otherwise
  
  // For this demo, we'll just simulate facial recognition with a delay
  Serial.println("Processing facial recognition...");
  delay(2000);  // Simulate processing time
  
  // Release frame buffer
  esp_camera_fb_return(fb);
  
  // For demo purposes, we'll randomly succeed/fail
  // Replace with actual facial recognition logic
  bool recognized = (random(10) > 3);  // 70% success rate for demo
  
  Serial.println(recognized ? "Face verified successfully" : "Face verification failed");
  return recognized;
}

void recordAttendance(String userId) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Can't record attendance - WiFi not connected");
    displayMessage("Offline mode\nData not sent");
    return;
  }
  
  Serial.println("Recording attendance for user: " + userId);
  displayMessage("Recording\nAttendance...");
  
  // Send data to Google Sheets
  sendToGoogleSheets(userId, true);
}

void processVoiceCommand() {
  // This is a simplified placeholder. In a real implementation, you would:
  // 1. Read audio data from I2S microphone
  // 2. Process it for wake word detection
  // 3. If wake word detected, capture voice command
  // 4. Process command with speech-to-text
  // 5. Execute relevant action
  
  // Example reading from I2S microphone (basic demonstration)
  int32_t samples[I2S_BUFFER_SIZE];
  size_t bytes_read;
  
  esp_err_t result = i2s_read(I2S_PORT, samples, sizeof(samples), &bytes_read, 0);
  
  if (result == ESP_OK && bytes_read > 0) {
    // Compute simple audio energy for demo
    int32_t sum = 0;
    for (int i = 0; i < bytes_read / sizeof(int32_t); i++) {
      sum += abs(samples[i]);
    }
    
    int32_t average = sum / (bytes_read / sizeof(int32_t));
    
    // Very basic threshold detection for demo
    // In a real app, you would implement proper wake word detection and speech processing
    if (average > 50000) {  // Arbitrary threshold, would need tuning
      Serial.println("Audio threshold exceeded, potential voice command");
      // Here you would process the audio command
      // For demo, we'll just show a message
      displayMessage("Voice detected\nProcessing...");
      delay(1000);
      displayMessage("Command not\nunderstood", true);
      delay(1000);
    }
  }
}

void displayMessage(String message, bool clearDisplay) {
  if (clearDisplay) {
    display.clearDisplay();
  }
  display.setCursor(0, 0);
  display.println(message);
  display.display();
}

void sendToGoogleSheets(String userId, bool verified) {
  HTTPClient http;
  String url = "https://script.google.com/macros/s/" + String(googleScriptId) + "/exec";
  
  // Add parameters
  url += "?userId=" + userId;
  url += "&verified=" + String(verified ? "true" : "false");
  url += "&timestamp=" + String(millis());  // In real app, use proper timestamp
  
  Serial.println("Sending data to Google Sheets: " + url);
  
  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  
  // Send HTTP GET request
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("HTTP response: " + String(httpCode));
    Serial.println("Response: " + payload);
    
    if (httpCode == HTTP_CODE_OK) {
      displayMessage("Attendance\nRecorded!");
    } else {
      displayMessage("Recording Error\nHTTP: " + String(httpCode));
    }
  } else {
    Serial.println("HTTP request failed: " + http.errorToString(httpCode));
    displayMessage("Server Error\nCheck connection");
  }
  
  http.end();
}

// Utility function to capture and save an image (not used in main flow but useful for debugging)
void captureImage() {
  camera_fb_t * fb = esp_camera_fb_get();
  
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  
  Serial.println("Camera capture success:");
  Serial.println("Size: " + String(fb->len) + " bytes");
  Serial.println("Width: " + String(fb->width) + ", Height: " + String(fb->height));
  
  // Here you would save or process the image
  // For example, send it over WiFi or save to SD card
  
  // Release frame buffer
  esp_camera_fb_return(fb);
}