# SonicVision

Creation of a prototype of glasses for visually impaired individuals, designed to aid mobility. Equipped with an ultrasonic sensor, 
the glasses help avoid collisions with objects or walls. Additionally, the prototype integrates a geolocation system that utilizes 
the "Geolocation API" along with the "Directions API" to determine the user's location through network triangulation. This information
is then sent to the integrated Raspberry Pi Zero 2W, which receives the longitude and latitude data. Using the second API, the system
processes the request and provides step-by-step navigation or a route from the user's current location to a specified destination.

### Requirements

* Raspberry Pi Zero 2w
* Esp32 (Generic)
* Ultrasonic Sensor
* Passive Buzzer
* Module Lm2596
* 18650 Battery Holder Case Box
* Batterys 18650*2

## Connections Schema

![Model 0.1](https://github.com/Trex-Codes/SonicVision/blob/master/Images/schema.png?raw=true)


## Result of the proyect

![Model 0.2](https://github.com/Trex-Codes/SonicVision/blob/master/Images/20241120_131713.jpg?raw=true)
