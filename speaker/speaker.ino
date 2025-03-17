#include "Arduino.h"
#include "BluetoothA2DPSink.h"
#include "SD.h"
#include "Audio.h"
#include "SPI.h"

// Setup I2S audio output
Audio audio;

// Bluetooth A2DP sink setup (optional if needed for Bluetooth audio)
BluetoothA2DPSink a2dp_sink;

void setup() {
    Serial.begin(115200);

    // Initialize SD card
    if (!SD.begin()) {
        Serial.println("SD Card initialization failed!");
        return;
    }

    // Setup I2S pins for audio output
    audio.setPinout(26, 25, 22); // BCLK, LRC, DOUT
    audio.setVolume(10); // Volume level (0-21)

    // Optional: Start Bluetooth A2DP if needed
    a2dp_sink.set_device_name("ESP32_Speaker");
    a2dp_sink.start("ESP32_Speaker");

    Serial.println("System Ready! Waiting for RFID...");
}

void loop() {
    // Check if RFID is detected (replace with our RFID condition)
    if (/* RFID detected */) {
        Serial.println("RFID detected! Playing message...");
        audio.connecttoFS(SD, "/message.wav"); // Play pre-recorded message
        delay(5000); // Wait for audio to finish before detecting next RFID tap
    }
}
