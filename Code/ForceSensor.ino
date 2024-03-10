#include "HX711.h"
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "PeelingDetection.h"

// OLED display width and height
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Pin definitions
#define BUTTON_PIN 9
#define LED_PIN 8
#define LOADCELL_DOUT_PIN 14
#define LOADCELL_SCK_PIN 13

// LED configuration
#define LED_COUNT 24
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// OLED display initialization
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Load cell configuration
HX711 scale;

// Calibration factor (should be adjusted based on your scale)
long calibration_factor = -1050;

// Graph settings
float graphValues[SCREEN_WIDTH] = {0};
int graphIndex = 0;

// Force thresholds
float maxPositiveForce = 30.0;
float maxNegativeForce = -30.0;

// Flashing control
unsigned long flashStartTime = 0;
int flashCount = 0;
bool flashing = false;

// Color settings
const uint32_t colorStartPositive = strip.Color(0, 20, 20);
const uint32_t colorEndPositive = strip.Color(0, 20, 0);
const uint32_t colorStartNegative = strip.Color(0, 0, 20);
const uint32_t colorEndNegative = strip.Color(20, 0, 0);

void handleButton() {
  static unsigned long pressTime = 0;
  static bool isPressing = false;
  static bool isLongPress = false;

  bool currentButtonState = digitalRead(BUTTON_PIN); // Read the button (active high with pull-down)

  if(currentButtonState && !isPressing) { // Button is pressed
    isPressing = true;
    pressTime = millis();
    isLongPress = false;
  } else if(!currentButtonState && isPressing) { // Button released
    if(isLongPress) {
      performCalibration();
    } else {
      resetGraphAndZeroWeight();
    }
    isPressing = false;
  } else if(isPressing && (millis() - pressTime > 2000)) { // Check for long press (2000 ms)
    isLongPress = true;
    // Long press logic here, or set a flag to indicate a long press occurred
  }
}

void resetGraphAndZeroWeight() {
  // Reset the graph to all zeroes
  for(int i = 0; i < SCREEN_WIDTH; i++) {
    graphValues[i] = 0;
  }
  graphIndex = 0;

  // Zero out the scale
  scale.tare();

  // Update the display immediately
  displayGraph(0);
}

void performCalibration() {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Empty scale");
    display.println("Press button");
    display.display();

    // Wait for button press to confirm scale is empty
    while(digitalRead(BUTTON_PIN) != HIGH);
    delay(500); // Debounce delay

    scale.tare(); // Zero the scale
    long offset = scale.get_offset();

    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Place 1000g");
    display.println("Press button");
    display.display();

    // Wait for button press to confirm weight is placed
    while(digitalRead(BUTTON_PIN) != HIGH);
    delay(500); // Debounce delay

    // Assuming you use a 1000g weight for calibration
    scale.calibrate_scale(1000.0, 5); // Calibrate with the known weight
    float calibrationFactor = scale.get_scale();

    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Calibration");
    display.println("Complete!");
    display.display();
    delay(2000); // Show completion message
}

void updateLEDs(float weight) {
  int ledToLight;
  uint32_t color;

  // Turn off all LEDs initially
  for(int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }

  // Always set the first LED to white
  strip.setPixelColor(0, strip.Color(50, 50, 50));

  if (weight >= 0) {
    // Positive weight, light LEDs clockwise
    ledToLight = map(weight, 0, maxPositiveForce, 0, LED_COUNT);
    color = blend(colorStartPositive, colorEndPositive, weight, maxPositiveForce);
    // Start from the second LED as the first is always white
    for (int i = 1; i < ledToLight; i++) {
      strip.setPixelColor(i, color);
    }
  } else {
    // Negative weight, light LEDs counterclockwise
    ledToLight = map(abs(weight), 0, abs(maxNegativeForce), 0, LED_COUNT);
    color = blend(colorStartNegative, colorEndNegative, abs(weight), abs(maxNegativeForce));
    // Ensure the first LED remains white even when counting down
    for (int i = LED_COUNT - 1; i >= LED_COUNT - ledToLight; i--) {
      if(i != 0) { // Skip the first LED
        strip.setPixelColor(i, color);
      }
    }
  }

  // Update the LED ring with new settings
  strip.show(); 
}

uint32_t blend(uint32_t colorStart, uint32_t colorEnd, float currentForce, float maxForce) {
  // Calculate the ratio of the current force to the max force
  float ratio = currentForce / maxForce;

  // Ensure the ratio is between 0 and 1
  ratio = max(0, min(ratio, 1));

  // Calculate the color components
  int red = ((uint8_t)(colorEnd >> 16) * ratio) + ((uint8_t)(colorStart >> 16) * (1 - ratio));
  int green = ((uint8_t)(colorEnd >> 8) * ratio) + ((uint8_t)(colorStart >> 8) * (1 - ratio));
  int blue = ((uint8_t)colorEnd * ratio) + ((uint8_t)colorStart * (1 - ratio));

  // Return the combined color
  return strip.Color(red, green, blue);
}

void flashPeelDetected() {
    if (!flashing) { // Start flashing
        flashing = true;
        flashCount = 0;
        flashStartTime = millis();
        strip.setPixelColor(0, strip.Color(0, 255, 0)); // Turn first LED green
        strip.show();
    }
}

void handleFlashing() {
    if (flashing) {
        unsigned long currentTime = millis();
        if (currentTime - flashStartTime > 50) { // Change state every 50 ms
            flashStartTime = currentTime; // Reset the timer
            if (flashCount % 2 == 0) { // Toggle LED
                strip.setPixelColor(0, strip.Color(0, 0, 0)); // Turn off LED
            } else {
                strip.setPixelColor(0, strip.Color(0, 255, 0)); // Turn LED green
            }
            strip.show();
            flashCount++;
            if (flashCount >= 3) { // After flashing 3 times (6 state changes)
                flashing = false; // Stop flashing
                strip.setPixelColor(0, strip.Color(0, 0, 0)); // Ensure LED is off
                strip.show();
            }
        }
    }
}

void flashCrashDetected() {
    for(int i = 0; i < 5; i++) { // Flash 5 times
        for(int j = 0; j < LED_COUNT; j++) {
            strip.setPixelColor(j, strip.Color(255, 0, 0)); // Set all LEDs to red
        }
        strip.show();
        delay(50); // Wait for half a second
        for(int j = 0; j < LED_COUNT; j++) {
            strip.setPixelColor(j, strip.Color(0, 0, 0)); // Turn off all LEDs
        }
        strip.show();
        delay(50); // Wait for half a second before next flash
    }
}



void displayGraph(float weight) {
  // Clear the display
  display.clearDisplay();
  
    // Assuming maxPositiveForce and maxNegativeForce represent the dynamic range
    float graphUpperBound = maxPositiveForce; // dynamic upper bound of the graph
    float graphLowerBound = -graphUpperBound; // dynamic lower bound, making sure zero is centered

    for (int i = 0; i < SCREEN_WIDTH - 1; i++) {
        int x1 = i;
        int y1 = map(graphValues[i], graphLowerBound, graphUpperBound, SCREEN_HEIGHT, 0);
        int x2 = i + 1;
        int y2 = map(graphValues[x2], graphLowerBound, graphUpperBound, SCREEN_HEIGHT, 0);
        display.drawLine(x1, y1, x2, y2, WHITE);
    }

  // Display current weight in the top left corner of the OLED
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Set cursor to top left corner
  display.print(weight, 2);
  display.print("g");

  // Display maxPositiveForce in the top right corner of the OLED
  String maxForceStr = String(maxPositiveForce, 2) + "g";
  int maxForceWidth = (maxForceStr.length() + 1) * 6; // Approx width of text
  display.setCursor(SCREEN_WIDTH - maxForceWidth, 0); // Set cursor to top right corner
  display.print(maxForceStr);

  // Update the display with everything we've drawn
  display.display();
}

void setup() {
  pinMode(BUTTON_PIN, INPUT);
  Serial.begin(9600);
  
  // Initialize scale
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();
  
  // Initialize LED strip
  strip.begin();
  strip.show();
  
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  
  setupPeelingDetection(); // Setup peeling detection if necessary
}

void loop() {

  handleButton(); // Call function to handle button logic

  handleFlashing(); // Call this every loop iteration

  // Read weight and update LEDs
  float weight = scale.get_units(5);
  updateLEDs(weight);  // Update the LED ring based on weight
  detectPeel(weight); // Use the function to detect peeling
  
  // Add the new weight value to the graph and update the OLED display
  graphValues[graphIndex] = weight;
  graphIndex = (graphIndex + 1) % SCREEN_WIDTH;
  displayGraph(weight);

     if (peelDetectedFlag) {
        flashPeelDetected();  // Call the flash function
        peelDetectedFlag = false;  // Reset the flag
    }

  // Print weight to the serial monitor
  Serial.print("Weight: ");
  Serial.print(weight, 2);
  Serial.println(" g");

  delay(50); // Graph update rate
}
