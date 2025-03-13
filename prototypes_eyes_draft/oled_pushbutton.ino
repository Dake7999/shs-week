#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ezButton.h>

#define SCREEN_WIDTH 128 // OLED display width,  in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // create SSD1306 display object connected to I2C
ezButton button(27);  // create ezButton object that attaches to the ESP32 pin GPIO27
unsigned long lastCount = 0;

void setup() {
  Serial.begin(9600);

  // initialize OLED display with address 0x3C for 128x64
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  delay(2000);         // wait for initializing
  oled.clearDisplay(); // clear display

  oled.setTextSize(2);          // text size
  oled.setTextColor(WHITE);     // text color
  oled.setCursor(0, 10);        // position to display

  button.setDebounceTime(150); // set debounce time to 50 milliseconds
  button.setCountMode(COUNT_FALLING);
}

void loop() {
  button.loop(); // MUST call the loop() function first

  unsigned long count = button.getCount();
  if (lastCount != count) {
    Serial.println(count); // print count to Serial Monitor
    oled.clearDisplay();   // clear display
    oled.setCursor(0, 10); // reset cursor position
    oled.println(count);   // display count
    oled.display();        // show on OLED
    lastCount = count;     // update lastCount
  }
}

