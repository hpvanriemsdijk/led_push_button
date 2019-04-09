# led push button

Made for those push buttons with an led light ring or symbol. Button actions and led state can be set using MQTT

supporting:  
- MQTT
- OTA
- Click patterns:
  * Click --> On
  * Dubble Click --> Media On
  * Hold --> Off
  * Long hold --> Ignore press
- A nice led that acts as a status indicator:
  * Off --> Not connected to wifi / MQTT
  * Dim lid --> Off
  * Breath --> On
  * Bright on --> Pressed
  * Blinking --> send OFF command when released
- Lisning to media_topic and power_toppic 
- Sending tele message every 5 minutes:
   
Known issue:  
- Using serial debuger triggers MQTT last will message to be send.
