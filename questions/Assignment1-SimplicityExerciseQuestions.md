Please include your answers to the questions below with your submission, entering into the space below each question
See [Mastering Markdown](https://guides.github.com/features/mastering-markdown/) for github markdown formatting if desired.

**1. How much current does the system draw (instantaneous measurement) when a single LED is on with the GPIO pin set to StrongAlternateStrong?**
   Answer: The system draw 5.34mA of current when a single LED is ON with drive strength set to "StrongALternateStrong". 


**2. How much current does the system draw (instantaneous measurement) when a single LED is on with the GPIO pin set to WeakAlternateWeak?**
   Answer: The system draw 5.31mA of current when a single LED is ON with drive strength set to "WeakALternateWeak". 


**3. Is there a meaningful difference in current between the answers for question 1 and 2? Please explain your answer, 
referencing the [Mainboard Schematic](https://www.silabs.com/documents/public/schematic-files/WSTK-Main-BRD4001A-A01-schematic.pdf) and [AEM Accuracy](https://www.silabs.com/documents/login/user-guides/ug279-brd4104a-user-guide.pdf) section of the user's guide where appropriate. Extra credit is avilable for this question and depends on your answer.**
   Answer: There is no meaningful difference observed in the instantaneous current when changing the drive strength between strong and weak. However, when I checked the chip datasheet, setting the drive strength do limit the maximum current that can flow through the GPIO. 
   Strong Drive Strength limits the current sourced through the GPIO to 10mA while the Weak Drive strength limits the current to 1mA. However as the LED is drawing current less than 1 mA, it doesn't matter if the drive strength is strong or weak. However, we can't run a load that needs more than 1mA of current for its operation if the drive strength is set to Weak. 
   Another thing I observed in the code was as LED0 and LED1 is connected to the same Port, if we configure the drive strength for LED0 as weak and then LED1 as strong it would overwrite the drive strength for LED0 as strong.  


**4. With the WeakAlternateWeak drive strength setting, what is the average current for 1 complete on-off cycle for 1 LED with an on-off duty cycle of 50% (approximately 1 sec on, 1 sec off)?**
   Answer: With LED 1 ON with WeakAlternateWeak Setting, the average current I got is 5.02mA.


**5. With the WeakAlternateWeak drive strength setting, what is the average current for 1 complete on-off cycle for 2 LEDs (both on at the time same and both off at the same time) with an on-off duty cycle of 50% (approximately 1 sec on, 1 sec off)?**
   Answer:With both LED 1 and LED0 ON/OFF, the average current I got is 5.19mA. 


