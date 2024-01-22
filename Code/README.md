This code is only for exemple and reseach purposes. It contains very simple routine to scale the graph and a simple peel detection algorithm.

How it works:

To calibrate the load cell the user needs to long press the screen then a calibration procedure will be shown. A short press of the screen zeroed out the current value.
After few cicles the microcontroller average the peaks to set the max value of the scale ( the number will be set at the top rigt corner ). The value at the top left is the real time value detected.
Everytime the scale is updated the ''0'' value is zeroed out so the scale doesnt drift after few layers of printing. It means that it will not care about the weight of the parts on the build plate.
The LED ring light represente the forces in real time. The 0 position represente the ''normal'' state and the end of the ring light (100%) represente the max value of the graph.

The peeling algorithm

For now the few tests we did looks promessing even with this super simple algorithm. A threshold value is set just above the 0 line. When a peak is detected and the value return close to 0 the peel detection is done.
*Sometimes with larger cross section the peeling is done in stages and its probable that you have multiple peak. The treshold value is there so you are not detecting a wrong peel.
When the sensor will be linked with Odyssey, we will be abble to send commands both ways. It means that we will have something like ''wait before peel'' so the peeling will be in sink with the printer states.
