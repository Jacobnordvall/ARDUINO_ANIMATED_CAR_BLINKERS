# ARDUINO_ANIMATED_CAR_LIGHTING

Diy modern animated car lighting system using adressable ledstrips (I used ws2812b strips). <br>
<br>
-
<br>


**IMPORTANT:**
- Cars use 12v!
- Micro controllers are usually 3v3 or 5v. 
- Use resistors accordingly unless you want magic smoke!
<br>
<br>

**GOOD TO KNOW:**
- This uses one output data pin for the blinkers. <br>
 Short input = cycle 3 times. Long input = cycle until released. 

- If the strips act weird or wont work with long wire runs,
<br>then you need a "Logic Level Converter" mosts strips expect a 5v signal. 
 
- Made in vscode using platformio.
<br>
<br>

**FEATURES:**
-  INDICATORS:
<br>Fills the led strip from the inside out progressively and then fades out, and repeats. just like on modern cars.

-  FRONT DRL:
<br>Static white. has optional bootup animation.

- BRAKES + DRL:
<br>Runs in DRL mode until low brake input is detected, Then it fills the strip at max brightness from the middle out to the sides simultaneously (and holds it). IF high brake is detected while low brake is also, then it will flash. has optional bootup animation.
<br>
<br>

**HOW TO USE INDICATORS:**

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
<br>
<br>

**TODO LIST:**
- 
- ~~Startup animation when it gets powered on.~~  DONE
- ~~Option for Front DRLs.~~  DONE
- Option for flashing indicators once on unlock and only run bootup on final ignition. (for diy vehicles)
- Reverse light.
- Fix variable naming (got a bit confusing when i decided to add more than just indicators)
- Cleaning up the code when im satisfied with the features...