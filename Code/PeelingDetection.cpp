#include "PeelingDetection.h"

float peelThresholdPercentage = 10.0; 
float someStartPeelThreshold = 1.0;
extern float maxPositiveForce;
float maxPeelForce = 0;
bool potentialPeel = false;
bool peelDetectedFlag = false;
float peakValues[10] = {0}; // Array to store the last 10 peaks
int numPeaks = 0; // Number of peaks recorded

void setupPeelingDetection() {
  // Initialization related to peeling detection, if any
}

void detectPeel(float currentWeight) {
    if (!potentialPeel && currentWeight > someStartPeelThreshold) {
        potentialPeel = true;
        maxPeelForce = currentWeight; // Initialize the max force with the current weight
    } else if (potentialPeel) {
        // Update the maximum force seen during this potential peel
        if (currentWeight > maxPeelForce) {
            maxPeelForce = currentWeight;
        }
        
        // Define the threshold for considering the end of a peel as 10% of maxPositiveForce
        float peelEndThreshold = maxPositiveForce * (peelThresholdPercentage / 100.0);
        if (currentWeight <= peelEndThreshold) {
            // Detected the end of a peel event
            peelDetectedFlag = true;  // Set the flag
            Serial.println("PEEL DETECTED");
            // Record the peak if it's part of the last 10 peaks
            if(numPeaks < 10) {
                peakValues[numPeaks++] = maxPeelForce;
            } else {
                for(int i = 1; i < 10; i++) {
                    peakValues[i - 1] = peakValues[i];
                }
                peakValues[9] = maxPeelForce;
            }

            // Update maxPositiveForce with the average of peaks
            float sum = 0;
            for(int i = 0; i < numPeaks; i++) {
                sum += peakValues[i];
            }
            maxPositiveForce = sum / numPeaks; // Calculate new average

            potentialPeel = false; // Reset for the next potential peel
            maxPeelForce = 0; // Reset max force for the next potential peel
        }
    }
    // Additional logic for resetting or handling other cases...
}