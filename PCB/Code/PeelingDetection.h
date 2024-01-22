#ifndef PeelingDetection_h
#define PeelingDetection_h

#include "Arduino.h"

// Define any global variables or constants here
extern float peelThresholdPercentage; // Percentage threshold for peel detection
extern float someStartPeelThreshold;
extern bool peelDetectedFlag;
extern bool crashDetectedFlag; // Defined in your .ino file

void setupPeelingDetection();
void detectPeel(float currentWeight);

#endif