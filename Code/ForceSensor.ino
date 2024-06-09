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

// HX711 scale initialization
HX711 scale;

/////////////////////////////////////////////////////////
///////////////// USER DEFINED VALUES ///////////////////
////////////////////////////////////////////////////////

// Calibration factors for both channels
float calibrationFactorA = 500; // Adjust this to your specific calibration factor
float calibrationFactorB = 500; // Adjust this to your specific calibration factor

// Force thresholds
float maxPositiveForce = 300.0;
float maxNegativeForce = -300.0;

// Color settings
const uint32_t colorStartPositive = strip.Color(0, 20, 20);
const uint32_t colorEndPositive = strip.Color(0, 20, 0);
const uint32_t colorStartNegative = strip.Color(0, 0, 20);
const uint32_t colorEndNegative = strip.Color(20, 0, 0);

// Global variables to store offsets
long offsetA = 0;
long offsetB = 0;

// User-defined parameters
const int sampleRate = 60; // Samples per second
const int displayRefreshRate = 4; // Display refresh rate in Hz (also used for median smoothing)
const int numLoadCells = 1; // Number of load cells (1 or 2)
const int resolution = 20; // Scale resolution in grams
const int smoothingEnabled = 0; // Toggle for smoothing function (1 = on, 0 = off)

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

// Graph settings
float graphValues[SCREEN_WIDTH] = {0};
int graphIndex = 0;

// Flashing control
unsigned long flashStartTime = 0;
int flashCount = 0;
bool flashing = false;

// Variables for median smoothing
const int sampleSize = sampleRate / displayRefreshRate;
float samplesA[sampleSize];
float samplesB[sampleSize];
int sampleIndex = 0;

// Function declarations
void displayGraph(float weight);
float median(float* arr, int size);

void setup() {
  pinMode(BUTTON_PIN, INPUT);
  Serial.begin(9600);

  // Initialize scale
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  // Automatically tare the load cells at startup
  Serial.println("Taring the load cells...");
  scale.set_gain(128); // Select channel A
  scale.tare();
  offsetA = scale.get_offset();
  Serial.print("Offset A: ");
  Serial.println(offsetA);

  if (numLoadCells == 2) {
    scale.set_gain(32); // Select channel B
    scale.tare();
    offsetB = scale.get_offset();
    Serial.print("Offset B: ");
    Serial.println(offsetB);
  }

  // Initialize LED strip
  strip.begin();
  strip.show();

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();

  setupPeelingDetection(); // Setup peeling detection if necessary
}

float median(float* arr, int size) {
  float sorted[size];
  memcpy(sorted, arr, sizeof(float) * size);
  for (int i = 0; i < size - 1; i++) {
    for (int j = 0; j < size - i - 1; j++) {
      if (sorted[j] > sorted[j + 1]) {
        float temp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = temp;
      }
    }
  }
  if (size % 2 == 0) {
    return (sorted[size / 2 - 1] + sorted[size / 2]) / 2.0;
  } else {
    return sorted[size / 2];
  }
}

void loop() {
  handleButton(); // Call function to handle button logic
  handleFlashing(); // Call this every loop iteration

  // Read weight from channel A and channel B without delays
  float weightA = 0;
  float weightB = 0;

  if (numLoadCells >= 1) {
    scale.set_gain(128); // Select channel A
    scale.set_scale(calibrationFactorA); // Apply calibration factor for channel A
    scale.set_offset(offsetA); // Apply offset for channel A
    weightA = scale.get_units(1); // Read weight from channel A
    if (smoothingEnabled) {
      samplesA[sampleIndex] = weightA;
    }
  }

  if (numLoadCells == 2) {
    scale.set_gain(32);  // Select channel B
    scale.set_scale(calibrationFactorB); // Apply calibration factor for channel B
    scale.set_offset(offsetB); // Apply offset for channel B
    weightB = scale.get_units(1); // Read weight from channel B
    if (smoothingEnabled) {
      samplesB[sampleIndex] = weightB;
    }
  }

  if (smoothingEnabled) {
    sampleIndex = (sampleIndex + 1) % sampleSize;

    if (sampleIndex == 0) {
      // Calculate the median of the samples
      float medianA = numLoadCells >= 1 ? median(samplesA, sampleSize) : 0;
      float medianB = numLoadCells == 2 ? median(samplesB, sampleSize) : 0;

      // Calculate the average weight
      float weight = (medianA + medianB) / numLoadCells;

      // Apply resolution
      weight = round(weight / resolution) * resolution;

      // Update LEDs, detect peeling, and update the graph with the average weight
      updateLEDs(weight);
      detectPeel(weight);

      // Update the graph with the new weight value
      displayGraph(weight);

      if (peelDetectedFlag) {
        flashPeelDetected();
        peelDetectedFlag = false;
      }

      // Print average weight to the serial monitor for debugging
      Serial.print("Weight A: ");
      Serial.print(weightA, 0); // Print without decimals
      if (numLoadCells == 2) {
        Serial.print(" g, Weight B: ");
        Serial.print(weightB, 0); // Print without decimals
      }
      Serial.print(" g, Average Weight: ");
      Serial.print(weight, 0); // Print without decimals
      Serial.println(" g");
    }
  } else {
    // Calculate the average weight without smoothing
    float weight = (weightA + weightB) / numLoadCells;

    // Apply resolution
    weight = round(weight / resolution) * resolution;

    // Update LEDs, detect peeling, and update the graph with the average weight
    updateLEDs(weight);
    detectPeel(weight);

    // Update the graph with the new weight value
    displayGraph(weight);

    if (peelDetectedFlag) {
      flashPeelDetected();
      peelDetectedFlag = false;
    }

    // Print average weight to the serial monitor for debugging
    Serial.print("Weight A: ");
    Serial.print(weightA, 0); // Print without decimals
    if (numLoadCells == 2) {
      Serial.print(" g, Weight B: ");
      Serial.print(weightB, 0); // Print without decimals
    }
    Serial.print(" g, Average Weight: ");
    Serial.print(weight, 0); // Print without decimals
    Serial.println(" g");
  }

  delay(1000 / sampleRate); // Sample rate control
}

// Function to handle button presses
void handleButton() {
  static unsigned long pressTime = 0;
  static bool isPressing = false;
  static bool isLongPress = false;

  bool currentButtonState = digitalRead(BUTTON_PIN); // Read the button (active high with pull-down)

  if (currentButtonState && !isPressing) {
    isPressing = true;
    pressTime = millis();
    isLongPress = false;
  } else if (!currentButtonState && isPressing) {
    if (isLongPress) {
      performCalibration();
    } else {
      resetGraphAndZeroWeight();
    }
    isPressing = false;
  } else if (isPressing && (millis() - pressTime > 2000)) {
    isLongPress = true;
  }
}

// Function to reset the graph and tare the scale
void resetGraphAndZeroWeight() {
  for (int i = 0; i < SCREEN_WIDTH; i++) {
    graphValues[i] = 0;
  }
  graphIndex = 0;

  // Zero both scales
  scale.set_gain(128); // Select channel A
  scale.tare(); // Zero channel A
  offsetA = scale.get_offset();

  if (numLoadCells == 2) {
    scale.set_gain(32); // Select channel B
    scale.tare(); // Zero channel B
    offsetB = scale.get_offset();
  }

  displayGraph(0);
}

// Function to update the LEDs based on the weight
void updateLEDs(float weight) {
  int ledToLight;
  uint32_t color;

  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }

  strip.setPixelColor(0, strip.Color(50, 50, 50));

  if (weight >= 0) {
    ledToLight = map(weight, 0, maxPositiveForce, 0, LED_COUNT);
    color = blend(colorStartPositive, colorEndPositive, weight, maxPositiveForce);
    for (int i = 1; i < ledToLight; i++) {
      strip.setPixelColor(i, color);
    }
  } else {
    ledToLight = map(abs(weight), 0, abs(maxNegativeForce), 0, LED_COUNT);
    color = blend(colorStartNegative, colorEndNegative, abs(weight), abs(maxNegativeForce));
    for (int i = LED_COUNT - ledToLight; i < LED_COUNT; i++) {
      if (i != 0) {
        strip.setPixelColor(i, color);
      }
    }
  }

  strip.show();
}

// Function to blend colors for the LEDs
uint32_t blend(uint32_t colorStart, uint32_t colorEnd, float currentForce, float maxForce) {
  float ratio = currentForce / maxForce;
  ratio = max(0.0f, min(ratio, 1.0f));

  int red = ((uint8_t)(colorEnd >> 16) * ratio) + ((uint8_t)(colorStart >> 16) * (1 - ratio));
  int green = ((uint8_t)(colorEnd >> 8) * ratio) + ((uint8_t)(colorStart >> 8) * (1 - ratio));
  int blue = ((uint8_t)colorEnd * ratio) + ((uint8_t)colorStart * (1 - ratio));

  return strip.Color(red, green, blue);
}

// Function to flash LEDs when peel is detected
void flashPeelDetected() {
  if (!flashing) {
    flashing = true;
    flashCount = 0;
    flashStartTime = millis();
    strip.setPixelColor(0, strip.Color(0, 255, 0));
    strip.show();
  }
}

// Function to handle flashing LEDs
void handleFlashing() {
  if (flashing) {
    unsigned long currentTime = millis();
    if (currentTime - flashStartTime > 50) {
      flashStartTime = currentTime;
      if (flashCount % 2 == 0) {
        strip.setPixelColor(0, strip.Color(0, 0, 0));
      } else {
        strip.setPixelColor(0, strip.Color(0, 255, 0));
      }
      strip.show();
      flashCount++;
      if (flashCount >= 3) {
        flashing = false;
        strip.setPixelColor(0, strip.Color(0, 0, 0));
        strip.show();
      }
    }
  }
}

// Function to display the graph on the OLED
void displayGraph(float weight) {
    // Shift all graph values to the left
    for (int i = 0; i < SCREEN_WIDTH - 1; i++) {
        graphValues[i] = graphValues[i + 1];
    }

    // Add the new weight value to the end of the array
    graphValues[SCREEN_WIDTH - 1] = weight;

    // Clear the display
    display.clearDisplay();

    // Determine the dynamic range of the graph
    float maxGraphValue = maxPositiveForce;
    float minGraphValue = maxNegativeForce;
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        if (graphValues[i] > maxGraphValue) {
            maxGraphValue = graphValues[i];
        }
        if (graphValues[i] < minGraphValue) {
            minGraphValue = graphValues[i];
        }
    }

    // Ensure the graph accommodates both positive and negative peaks
    float graphUpperBound = max(maxGraphValue, -minGraphValue);
    float graphLowerBound = -graphUpperBound;

    // Draw the graph
    for (int i = 0; i < SCREEN_WIDTH - 1; i++) {
        int x1 = i;
        int y1 = map(graphValues[i], graphLowerBound, graphUpperBound, SCREEN_HEIGHT, 0);
        int x2 = i + 1;
        int y2 = map(graphValues[i + 1], graphLowerBound, graphUpperBound, SCREEN_HEIGHT, 0);
        display.drawLine(x1, y1, x2, y2, WHITE);
    }

    // Display the current weight in the top left corner of the OLED
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print(weight, 0);
    display.print("g");

    // Display maxPositiveForce in the top right corner of the OLED
    String maxForceStr = String(maxPositiveForce, 0) + "g";
    int maxForceWidth = (maxForceStr.length() + 1) * 6; // Approx width of text
    display.setCursor(SCREEN_WIDTH - maxForceWidth, 0); // Set cursor to top right corner
    display.print(maxForceStr);

    // Update the display with everything we've drawn
    display.display();
}

// Function to perform calibration for both channels with LED effects
void performCalibration() {
    display.clearDisplay();
    display.setTextSize(1); // Set larger font size for calibration text
    display.setCursor(0, 10); // Adjust cursor to center the text
    display.println("EMPTY");
    display.setCursor(0, 30); // Adjust for next line
    display.println("SCALE");
    display.setCursor(0, 50); // Adjust for next line
    display.println("PRESS SCREEN");
    display.display();

    // Start rainbow effect
    while (digitalRead(BUTTON_PIN) != HIGH) {
      rainbowEffect();
      delay(20); // Adjust this delay for rainbow speed
    }
    delay(500); // Debounce delay

    // Wait for 3 seconds to allow scale to stabilize
    startCountdownEffect();
    delay(3000);

    // Zero both scales
    Serial.println("Zeroing the scales...");
    scale.set_gain(128); // Select channel A
    scale.tare(); // Zero channel A
    offsetA = scale.get_offset();
    Serial.print("Offset A after tare: ");
    Serial.println(offsetA);

    if (numLoadCells == 2) {
      scale.set_gain(32); // Select channel B
      scale.tare(); // Zero channel B
      offsetB = scale.get_offset();
      Serial.print("Offset B after tare: ");
      Serial.println(offsetB);
    }

    display.clearDisplay();
    display.setCursor(0, 10); // Adjust cursor to center the text
    display.println("PLACE");
    display.setCursor(0, 30); // Adjust for next line
    display.println("1000g");
    display.setCursor(0, 50); // Adjust for next line
    display.println("PRESS SCREEN");
    display.display();

    // Start rainbow effect
    while (digitalRead(BUTTON_PIN) != HIGH) {
      rainbowEffect();
      delay(20); // Adjust this delay for rainbow speed
    }
    delay(500); // Debounce delay

    // Wait for 3 seconds to allow scale to stabilize
    startCountdownEffect();
    delay(3000);

    // Read raw values before calibration for both channels
    scale.set_gain(128); // Select channel A
    float readingA = scale.get_units(10);
    Serial.print("Reading A before calibration: ");
    Serial.println(readingA);

    if (numLoadCells == 2) {
      scale.set_gain(32); // Select channel B
      float readingB = scale.get_units(10);
      Serial.print("Reading B before calibration: ");
      Serial.println(readingB);
    }

    // Assuming you use a 1000g weight for calibration
    scale.set_gain(128); // Select channel A
    scale.calibrate_scale(1000.0, 10); // Calibrate with the known weight for channel A
    calibrationFactorA = scale.get_scale();
    Serial.print("Calibration factor A: ");
    Serial.println(calibrationFactorA);

    if (numLoadCells == 2) {
      scale.set_gain(32); // Select channel B
      scale.calibrate_scale(1000.0, 10); // Calibrate with the known weight for channel B
      calibrationFactorB = scale.get_scale();
      Serial.print("Calibration factor B: ");
      Serial.println(calibrationFactorB);
    }

    display.clearDisplay();
    display.setCursor(0, 20); // Adjust cursor to center the text
    display.println("CALIBRATION");
    display.setCursor(0, 40); // Adjust for next line
    display.println("COMPLETE!");

    // Display the calibration factors
    display.setCursor(0, 50);
    display.print("A: ");
    display.print(calibrationFactorA);
    if (numLoadCells == 2) {
      display.print(" B: ");
      display.print(calibrationFactorB);
    }
    display.display();
    
    delay(2000); // Show completion message

    // Verify Calibration
    Serial.println("Verifying Calibration...");

    // Check channel A
    scale.set_gain(128); // Select channel A
    scale.set_scale(calibrationFactorA); // Apply calibration factor for channel A
    scale.set_offset(offsetA); // Apply offset for channel A
    float weightA = scale.get_units(10);
    Serial.print("Weight A after calibration: ");
    Serial.println(weightA);

    if (numLoadCells == 2) {
      // Check channel B
      scale.set_gain(32); // Select channel B
      scale.set_scale(calibrationFactorB); // Apply calibration factor for channel B
      scale.set_offset(offsetB); // Apply offset for channel B
      float weightB = scale.get_units(10);
      Serial.print("Weight B after calibration: ");
      Serial.println(weightB);
    }
}

// Function to display a rainbow effect on the LED ring
void rainbowEffect() {
  static uint16_t j = 0;
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, Wheel((i + j) & 255));
  }
  strip.show();
  j++;
}

// Function to generate rainbow colors across 0-255 positions
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  uint8_t brightnessFactor = 40; // Adjust this value (0-255) to set brightness; 128 is 50% brightness
  if (WheelPos < 85) {
    return strip.Color((255 - WheelPos * 3) * brightnessFactor / 255, 0, (WheelPos * 3) * brightnessFactor / 255);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, (WheelPos * 3) * brightnessFactor / 255, (255 - WheelPos * 3) * brightnessFactor / 255);
  }
  WheelPos -= 170;
  return strip.Color((WheelPos * 3) * brightnessFactor / 255, (255 - WheelPos * 3) * brightnessFactor / 255, 0);
}

// Function to start the countdown effect
void startCountdownEffect() {
  // Clear the LEDs first
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
  delay(500); // Short delay to ensure LEDs are cleared

  // Fill LEDs sequentially
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(60, 0, 0));
    strip.show();
    delay(125); // 3 seconds / 24 LEDs = 125ms per LED
  }
}
