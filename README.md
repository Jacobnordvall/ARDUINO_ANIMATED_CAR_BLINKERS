# ARDUINO_ANIMATED_CAR_BLINKERS

Diy modern animated car indicators with a led strip (i used a ws2812b). <br>
<br>
Fills the led strip from the inside out progressively and then fades out, and repeats. just like on modern cars.
<br>
<br>

**IMPORTANT:**
- Cars use 12v!
- Micro controllers are usually 3v3 or 5v. 
- Use resistors accordingly unless you want magic smoke!
<br>
<br>

**GOOD TO KNOW:**
- This uses one output data pin for the animation. 
- Short input = cycle 3 times. Long input = cycle until released.
- Made in vscode using platformio.
<br>
<br>

**HOW TO USE:**

- Option 1: <br>
one microcontroller per light. <br>

- Option 2: Relays: <br>
Use the blinker power wire to trigger a relay that turns on that sides ledstrip and have that signal also go into the input pin so it starts animating. this way you turn on the ledstrip you need and tell the microcontroller to start animating. 
\
\
Both sides have their own relays to trigger that specifc indicator ledstrip. both sides go into the same microcontroller input pin, and both led strips use the same ledpin for data.
\
\
(This prevents ledstrip ghost draw. use solid state relays for lowest power consumption)

- Option 3 <br>
Modify the code :)