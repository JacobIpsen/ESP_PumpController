Project description
I needed an way to easy monitor and control my foundation drain pump well.
The spec where, monitor and log water level.
But got expanded to control the pump aswell.

Inside the well sits a 2 float senstor array. 
Sensor 1 sits below where the punp will normaly start,
and is used to determin how quickly the water rises.
Sensor 2 sits at the desired max water level, is used for triggering the pump to start.

When the pump starts the water will start to sink, sensor 2 low wil not trigger pump cut off.
when sensor 1 goes low, this will trigger an Pump stop delay(defined by time, not an delay)
after the desired stop delay the pump relay goes low, and the pump stops.

The delay ensures that the water sinks all the way down.

All sensor actions are logged, including start and stop og the pump.

Sensores are pulled low so the sensor only have ground and sensor feedback only return connsction to ground.

To monitor the log and the waterlevel i added a small webserver. 
Showing the sentors current state and the log.
The log is just an array of strings, the date and time are retrived from NTP server. 
I added a button to give an easy way to trigger the pump, in case somthing goes wrong with the sensor.


Pins used (D1 mini pinout) 
Pin 20 D1 (GPIO5)  Sensor 1
Pin 19 D2 (GPIO5)  Sensor 2
Pin 5  D5 (GPIO14) Relay
Pin 6  D6 (GPIO12) LED

Parts list 
Fotek SSR-25DA SSR-25 DA Solid State Relay 25A, input 4-32VDC, output load 25A at 24-380VAC
https://www.amazon.com/Fotek-SSR-25DA-SSR-25-4-32VDC-24-380VAC/dp/B01NGTE8M8

USB Powersupply

WeMos D1 mini V2.2.0 WIFI Internet Development Board Baseret ESP8266 4MB FLASH ESP-12S Chip
https://www.banggood.com/da/3Pcs-WeMos-D1-mini-V2_2_0-WIFI-Internet-Development-Board-Based-ESP8266-4MB-FLASH-ESP-12S-Chip-p-1150190.html?gmcCountry=DK&currency=DKK&createTmp=1&utm_source=googleshopping&utm_medium=cpc_union&utm_content=xibei&utm_campaign=xibei-ssc-dk-da-all-0302&ad_id=337427030565&gclid=EAIaIQobChMI1NCBguja6AIVQeJ3Ch1R-gtjEAQYAyABEgJdWfD_BwE&cur_warehouse=CN

200mm Liquid Float Switch Water Level Sensor Stainless Steel Double Ball
https://www.amazon.in/Liquid-Switch-Sensor-Stainless-Double/dp/B07412DSJZ



