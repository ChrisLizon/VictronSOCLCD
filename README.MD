![LCD mounted in Project box](images/projectbox.jpg?raw=true "LCD mounted in project box")

Background
----------

This was my experiment to interface with my CerboGX over MQTT. The original intent was to use the MicroController to control my dump load (water heater) when the sun was up and the batteries were full. Eventually I determined it was better to do this using Node Red on the Cerbo and use the built-in relays. Since my Cerbo GX is not connected to a GXTouch50 I strapped a 20x4 LCD display to the ESP so I could see SOC and other vital stats without having to pull out my phone. 

Building this for yourself
--------------------------
This project was built with PlatformIO and Ardunio around an [WEMOS S2 Mini](https://www.wemos.cc/en/latest/s2/s2_mini.html) Esp32-S2 based microcontroller. If you attempt to compile for another board, such as ESP8266 or ESP32 you may need to modify platformio.ini.

Getting this running on your own will require a little bit of knowledge of Arduino, some concepts of MQTT, and a splash of Python. 

Firstly, you must be able to determine which of Victrons 128 mqtt servers your GX device is connected to. There is some helpful code listed on the Victron dbus-MQTT project page https://github.com/victronenergy/dbus-mqtt#determining-the-broker-url-for-a-given-installation


Once you are able to determine which server you need to connect to  I would recommend you download an app such as [MQTT Explorer](https://mqtt-explorer.com/) to log in to the Victron MQTT server and grab the identifiers for your Multiplus and your shunt. (Hint: You may need to download the [venus-ca.crt](https://github.com/victronenergy/dbus-mqtt/blob/master/venus-ca.crt) from Victron's dbus-MQTT project and add it to the MQTT Explorer settings under Advanced > Certificates > Server Certificate.) The multiplus will be a numerical identifier under the vebus tree. The shunt will be a numerical identifier under the battery tree.  

To build, please copy src/credentials.h.example to src/credentials.h and fill it in with the values for your system.

The script currently goes out to the internet in order to connect to the Victron MQTT servers, even if you are on the same LAN. I will eventually try to add discovery to the code so that it will locate a GX device over the LAN for increased stability and decreased WAN bandwidth usage. 
