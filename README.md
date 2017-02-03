# AltzLight
Flashing a string of LEDs at 40Hz to combat Alzheimer's 

Runs on Adafruit Feather using Arduino 1.8.1 compiler
Add all sound files in the "cuts" directory to the root of a micro SD card.  Do not change the file names.
Schematic Word document shows how to interconnect the two boards.
The .mp3 files are processed by the Adafruit "Music Maker" shield (P1788).  
The SD card is placed in the Music Maker shield, and NOT in the Feather (if it even has an SD card slot).
I used 30V/60A TO-220 N-Chaneel power MOSFETs (like Adafruit P355), with one FET per color of the LED strip.  
So, a total of 4 MOSFETs (R, G, B and White).
Power is applied to the strips (12VDC) on teh black wire, then feeds through the LEDs and into the FETs (low-side switching).
The gates are driven by the Feather outputs 10, 11, 12 & 13.  These are setup for 40Hz/40% duty cycle.
The sources are all tied together and directly to ground.
Low impedance paths should be used for the 12v stuff, so thicker wires and short runs.
I ran three 6' strings in parallel, so my total 12V current could hit 28A!  That is divided into four FETs, so each FET doesn't see
more than 7A, so they run very cool.
I added a PowerTail II Switch to kill the 12V, but this is not totally needed.  
If I forgot to unplug it and the FETs turned on a little, they could get very hot and cause a fire, so I kill the 12V at the end of the code, using the PowerTail and GPIO 18 (marked as "A4" on the Feather)
The light starts up on initial boot and says Hi.  It then waits for the user to press a button.
The button is connected to GPIO 19 (A5).
Each press of the button adds 15 minutes of run time, and is debounced.
The unit limits total run time to three hours.
Each press will speak the time remaining.
Every five minutes, the time remaining will be announced.
At the end, the last minute will be counted down in 10 second intervals.
When the last minute is over, a goodbye message is played and then the lights are turned off.
The lights turn off by taking the lights from 100% duty cycle to 0% duty cycle over about 40 seconds.
Before this, a warning that they are turning off and a request to turn on the room lights is played.
Once the duty cycle hits zero, the outputs are turned off and the PowerTail is switched off, then an endless loop is entered.

The sound files in "Cuts" are all .mp3 files that are called by filename in the program.  Numbers are called by converting the current number needed (such as "35" in 35 minutes) to a string, appending ".mp3" to it, and running it through the call to the Music Maker play file routine.  Note:  The Music Maker rountine only uses 8.3 naming convention, and I don't check for that, so if you call a file with a longer filename than 8 characters, it won't play and won't throw an error.
