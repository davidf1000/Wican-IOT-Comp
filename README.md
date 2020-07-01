# Wican-IOT-Comp
A Hazard Monitoring System for Manufacture Industry Using IoT-Based Approach and MQTT Protocol.
Main purpose of this product is to track and monitor variables that is directly related to worker's health in factory environment.
Created for Wican International Instrumentation and Control Competition 2019 in Bandung .
## Spec
Using 4 Main Sensor :
- Sound 
- Gas Leak 
- Humidity and Temperature
- Dust particle <br>
Data is read by NodeMCU ESP32 Board , filtered using Averaging measurement and Recursive Exponential Filter
Data is then sent using MQTT Protocol using Adafruit.io as broker , and then displayed in adafruit's dashboard 
Use Ticker to precisely read all data in a periodic time and EEPROM to backup filter's constant in case of sudden power spike in ESP32 board to avoid data loss.
