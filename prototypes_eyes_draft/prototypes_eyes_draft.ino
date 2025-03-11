#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // Typical I2C address for most OLED displays
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Eye parameters
const int eyeSize = 20;       // Size of the eye
const int pupilSize = 8;      // Size of the pupil
const int leftEyeX = 40;      // X position of left eye
const int rightEyeX = 88;     // X position of right eye
const int eyeY = 32;          // Y position of both eyes
int pupilLeftX, pupilLeftY;   // Current position of left pupil
int pupilRightX, pupilRightY; // Current position of right pupil
int movementTimer = 0;        // Timer for movement
int blinkTimer = 0;           // Timer for blinking
bool isBlinking = false;      // Flag to track blinking state

void setup() {
  Serial.begin(9600);

  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Set initial pupil positions
  pupilLeftX = leftEyeX;
  pupilLeftY = eyeY;
  pupilRightX = rightEyeX;
  pupilRightY = eyeY;
  
  // Seed the random number generator
  randomSeed(analogRead(0));
}

void loop() {
  // Increment timers
  movementTimer++;
  blinkTimer++;
  
  // Check if it's time for a new eye movement
  if (movementTimer > 50) {
    // Random eye movement
    int maxEyeMove = eyeSize/3 - pupilSize/3;
    pupilLeftX = leftEyeX + random(-maxEyeMove, maxEyeMove);
    pupilLeftY = eyeY + random(-maxEyeMove, maxEyeMove);
    pupilRightX = rightEyeX + random(-maxEyeMove, maxEyeMove);
    pupilRightY = eyeY + random(-maxEyeMove, maxEyeMove);
    
    movementTimer = 0;
  }
  
  // Check if it's time to blink
  if (blinkTimer > 100 && !isBlinking) {
    isBlinking = true;
    blinkTimer = 0;
  }
  
  // Reset blink after a short period
  if (isBlinking && blinkTimer > 5) {
    isBlinking = false;
    blinkTimer = 0;
  }
  
  // Clear the buffer
  display.clearDisplay();
  
  // Draw the eyes
  drawEyes();
  
  // Display the buffer
  display.display();
  
  // Small delay for animation
  delay(20);
}

void drawEyes() {
  // Draw eye whites (circles)
  display.drawCircle(leftEyeX, eyeY, eyeSize, SSD1306_WHITE);
  display.drawCircle(rightEyeX, eyeY, eyeSize, SSD1306_WHITE);
  
  // Draw eyelids if blinking
  if (isBlinking) {
    // Draw horizontal lines to simulate closed eyes
    for (int i = -eyeSize; i <= eyeSize; i++) {
      display.drawPixel(leftEyeX + i, eyeY, SSD1306_WHITE);
      display.drawPixel(rightEyeX + i, eyeY, SSD1306_WHITE);
    }
  } else {
    // Draw pupils (filled circles)
    display.fillCircle(pupilLeftX, pupilLeftY, pupilSize, SSD1306_WHITE);
    display.fillCircle(pupilRightX, pupilRightY, pupilSize, SSD1306_WHITE);
  }
}