**This code is only for example and research purposes. It contains very simple routine to scale the graph and a simple peel detection algorithm.**

How it works:

To calibrate the load cell the user needs to long press the screen then a calibration procedure will appear. A short press of the screen will zero out (tare) the current value.
After a few cycles the microcontroller will average the peaks to set the max value of the scale ( the number will be set at the top right corner ). The value at the top left is the real time value detected.
Everytime the scale is updated the ''0'' value will zero out so the scale doesnt drift after a few layers of printing. It means that it will not take in account the weight of the parts on the build plate.
The LED ring light represents the forces in real time. The 0 position represents the ''normal'' state and the end of the ring light (+100%) represents the max value of the graph.

**The peeling algorithm**

For now the few tests we did look promising even with this super simple algorithm. A threshold value is set just above the 0 line. When a peak is detected and the value returns close to 0 the peel detection is done.
*Sometimes with larger cross sections the peeling is done in stages and its probable that you will have multiple peaks. The treshold value is there so you are not detecting a wrong peel.
When the sensor will be linked with Odyssey, we will be able to send commands both ways. It means that we will have something like ''wait for peel'' so the peeling will be in sink with the printer states.

User parameter Of the sensor:

-How many load cell ( if the sensor is used alone we suggest to use only one because processing all the numbers and calculations will slow down the microcontroller. Later all the calculation will be processed by Odyssey and the raspberry pi )

-Calibration factor ( Will be shown after the calibration procedure )

-Sample rate ( How many samples the microcontroller takes per second. Higher the number is the slower the microcontroller will be. )

-Display refresh rate ( How many time per second the screen will be updated. This number is also used for the simple filter to handle the noise.)
