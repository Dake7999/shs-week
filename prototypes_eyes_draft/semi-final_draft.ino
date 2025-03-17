#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LED_PIN 2 // Replace with your LED pin
#define BUTTON_BACK 15 // Replace with your button pins
#define BUTTON_FORWARD 4
#define BUTTON_CONFIRM 16

unsigned long idleTime = 0;
bool idleMode = false;  // Keeps track of idling
bool inMenu = true;     // Keeps track of whether user is in menu or not

int menuIndex = 0; // Current menu index
String menuOptions[] = {"Comfort Room", "Library", "Canteen", "Clinic", 
                        "Faculty", "Cluster Head's Office", 
                        "Guidance Counselor", "JHS Faculty", 
                        "Principal's Office", "Assistant Principal's Office", 
                        "Registrar Office", "Finance Office"};
const int menuSize = sizeof(menuOptions) / sizeof(menuOptions[0]);

bool isLoggedIn = false; // Tracks login state

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_BACK, INPUT_PULLUP);
  pinMode(BUTTON_FORWARD, INPUT_PULLUP);
  pinMode(BUTTON_CONFIRM, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Update with your OLED's I2C address
    for (;;); // Loop forever if OLED init fails
  }

  Serial.begin(115200);
  display.clearDisplay();
  display.display();

  // Show initial menu
  showMenu();
}

void loop() {
  unsigned long currentMillis = millis();

  // Check if the system should idle
  if (inMenu && (currentMillis - idleTime > 5000)) { // 5 seconds of inactivity
    idleMode = true;
    inMenu = false;
    drawIdleEyes();  // Show idling animation
  }

  // Handle button inputs
  handleButtons();

  // If in idle mode, animate eyes
  if (idleMode) {
    animateIdleEyes();
  }
}

void handleButtons() {
  if (digitalRead(BUTTON_BACK) == LOW) {
    if (idleMode) {
      exitIdleMode();  // Exit idle and show menu
    }
    idleTime = millis();
    menuIndex = (menuIndex - 1 + menuSize) % menuSize; // Wrap-around navigation
    showMenu();
  }

  if (digitalRead(BUTTON_FORWARD) == LOW) {
    if (idleMode) {
      exitIdleMode();  // Exit idle and show menu
    }
    idleTime = millis();
    menuIndex = (menuIndex + 1) % menuSize; // Wrap-around navigation
    showMenu();
  }

  if (digitalRead(BUTTON_CONFIRM) == LOW) {
    if (idleMode) {
      exitIdleMode();  // Exit idle and show menu
    }
    idleTime = millis();
    handleConfirm();
  }
}

void drawIdleEyes() {
  display.clearDisplay();
  display.fillCircle(40, 32, 8, WHITE); // Left eye
  display.fillCircle(88, 32, 8, WHITE); // Right eye
  display.display();
  Serial.println("Idling: Eyes are displayed.");
}

void animateIdleEyes() {
  static unsigned long lastBlinkTime = 0;
  static bool blinkState = false;

  if (millis() - lastBlinkTime > 500) { // Blink every 500ms
    lastBlinkTime = millis();
    blinkState = !blinkState;

    if (blinkState) {
      drawIdleEyes(); // Eyes open
    } else {
      display.clearDisplay();
      display.display(); // Eyes closed
      Serial.println("Eyes closed (blink).");
    }
  }
}

void exitIdleMode() {
  idleMode = false;
  inMenu = true;
  Serial.println("Exiting idle mode. Returning to menu.");
  showMenu();
}

void showMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Select Option:");
  display.setCursor(0, 20);
  display.println(menuOptions[menuIndex]);
  display.display();
  Serial.println("Displayed menu option: " + menuOptions[menuIndex]);
}

void handleConfirm() {
  String action = menuOptions[menuIndex];

  if (!isLoggedIn) {
    logIn(action); // Perform login
  } else {
    logOut(action); // Perform logout
  }
}

void logIn(String location) {
  // Logic for successful log-in
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Log-In Successful");
  display.setCursor(0, 20);
  display.println("Location: " + location);
  display.display();
  Serial.println("Logged in at: " + location);

  digitalWrite(LED_PIN, HIGH);
  delay(5000); // LED on for 5 seconds
  digitalWrite(LED_PIN, LOW);

  isLoggedIn = true; // Update login state
  idleMode = true;   // Transition to idle after login
  inMenu = false;
  drawIdleEyes();    // Show idle animation
}

void logOut(String location) {
  // Logic for successful log-out
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Log-Out Successful");
  display.setCursor(0, 20);
  display.println("Location: " + location);
  display.display();
  Serial.println("Logged out from: " + location);

  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }

  isLoggedIn = false; // Update login state
  idleMode = true;    // Transition to idle after logout
  inMenu = false;
  drawIdleEyes();     // Show idle animation
}
