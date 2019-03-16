# led_push_button


MQTT button, supporting:  
- OTA
- Click patterns:
  * Click --> On
  * Dubble Click --> Media On
  * Hold --> Off
  * Long hold --> Ignore press
- A nice led that acts as a status indicator:
  * Off --> Not connected
  * Dim lid --> Off
  * Breath --> On
  * Bright on --> Pressed
  * Blinking --> send oof dignal when released
- Lisning to media_topic and power_toppic 
- Sending tele message every 5 minutes:
   
Known issue with:  
- Using serial debuger over USB (Sends lkast will over OTA) and WiFi (Asks non-excisting password)