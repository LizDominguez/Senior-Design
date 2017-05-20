# Senior-Design
An IoT project including an RFID reader that updates a website listing adoptable pets. 

MYshelter is an animal shelter management system that keeps track of the pets that are surrendered and adopted. 
Each animal is given an RF tag with a unique ID. 
The ID is scanned once when the animal is surrendered to the shelter, and a second time once the animal is adopted. 
This information will be sent to a database that keeps track of the animals and other relevant information.

<img src="https://github.com/LizDominguez/Senior-Design/blob/master/about-us-preview.png" height="220px"> <img src="https://github.com/LizDominguez/Senior-Design/blob/master/adopt-preview.png" height="220px">
The MyShelter Website

Project Features:

This design requires RFID tags, a receiver circuit, and a Wi-Fi module. 
The receiver will use a 125kHz carrier signal to power the RFID tag via an antenna. 
An AM signal demodulator will retrieve the stored information on the RFID tag in digital form. 
Amplifiers will be used in the receiver circuit to amplify the analog signals for better data interpretation. 
The microprocessor will then retrieve the signal information and decode it. The microprocessors uses UARTs, LCDs, XBEEs, the PWM, timers,
interrupts, ISP, a wifi module, and other features.

<div align="center">

[![Watch the video](http://img.youtube.com/vi/Qfhe00SMSjw/0.jpg)](https://youtu.be/Qfhe00SMSjw)

Click to watch the project in action
</div>


For more information, view the <a href="https://github.com/LizDominguez/Senior-Design/blob/master/Final%20Report.pdf">design report</a>. 
