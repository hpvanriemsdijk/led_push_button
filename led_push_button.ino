/*
 * MQTT button, supporting:  
 *  OTA
 *  Click, Dubble Click and Hold
 *  Long hold -- Ignore press
 *
 * A nice led that acts as a status indicator:
 *  Off --> Not connected
 *  Dim lid --> Off
 *  Breath --> On
 *  Bright on --> Pressed
 *  Blinking --> In press hold zone.
 *  
 * Lisning to led_topic to set led state to ON or OFF
 * sending last button action as tele message
 *    
 * Known issue with:  
 *  - Using serial debuger over USB (Sends lkast will over OTA) and WiFi (Asks non-excisting password)
 *  
 */
#include <ESP8266WiFi.h>  // For ESP8266
#include <PubSubClient.h> // For MQTT
#include <ESP8266mDNS.h>  // For OTA
#include <WiFiUdp.h>      // For OTA
#include <ArduinoOTA.h>   // For OTA
#include <JC_Button.h>    // https://github.com/JChristensen/JC_Button
#include <NTPClient.h>    // Get time for tele message

//Pin assesment
const int BUTTON_PIN =D3;
const int BUTTON_LED_PIN = D5;

//States
String lastCommand = "null";
int holdState = 0;
String led_state = "OFF";

/* 
 *  WiFi
 *  credentials.h has to be in your libary folder. its content is:
 *  #define mySSID "yourSSID"
 *  #define myMqttServer "your MQTT server IP"
 *  #define myMqttUID "Your MQTT user name "
 *  #define myMqttPass "your MQTT Password"
 *  #define myMqttTopic "Base topic"
 *  If you do not want this file, hard-code your credentials in this sketch
 */
#include "credentials.h";
const char* ssid = mySSID;
const char* password = myPASSWORD;
WiFiClient espClient;

//MQTT
PubSubClient client(espClient);
const char* mqtt_server = myMqttServer;
const char* mqtt_uid = myMqttUID;
const char* mqtt_pass = myMqttPass;
const String mqtt_base_topic = myMqttTopic;
const String stat_prefix = "stat/";
const String cmnd_prefix = "cmnd/";
const String tele_prefix = "tele/";
const String led_topic = "/LED"; 
const String button_topic = "/BTN"; 
const String available_topic = "/LWT";
String clientId = "Push-button-";

//Button
Button button(BUTTON_PIN);
unsigned long dubbleClickSpeed = 500;
unsigned long lastPushed = 0;
unsigned long timeToLastClick = 0;
const int hold_short = 2000;
const int hold_long = 5000;
boolean buttonReleased = false;

//led
int ledBlinkState = 0;

//OTA
WiFiServer TelnetServer(8266);

//time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String formattedDate;

void setup_wifi() {
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  ArduinoOTA.setHostname("OTA Push-button 1");

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String stringPayload;

  //Echo received msg
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    stringPayload += (char)payload[i];
  }
  Serial.println();

  //Handle state topic
  if (strcmp(topic, (cmnd_prefix + mqtt_base_topic + led_topic).c_str() )==0) {
    if( stringPayload == "ON" ){
      led_state = "ON";
      Serial.println("ON");
    }else if( stringPayload == "OFF" ){
      led_state = "OFF";
      Serial.println("OFF");
    }

    client.publish((stat_prefix+mqtt_base_topic+led_topic).c_str(), (led_state).c_str(), false);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) { 
    Serial.print("Attempting MQTT connection...");
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_uid, mqtt_pass, (tele_prefix+mqtt_base_topic+available_topic).c_str(), 0, true, "Offline")) {
      Serial.println("Connected with id: " + clientId );
      client.subscribe((cmnd_prefix + mqtt_base_topic + led_topic).c_str());
      Serial.println("subscribed to " + cmnd_prefix + mqtt_base_topic + led_topic);
      
      client.publish((tele_prefix+mqtt_base_topic+available_topic).c_str(), "Online", true);
      client.publish((stat_prefix+mqtt_base_topic+led_topic).c_str(), (led_state).c_str(), false);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/*
 * Button press actions: 
 * click: On
 * dubble click: Media on
 * hold: turn off
 * long hold: cancel action
*/
void button_action() { 
  //Read button state
  button.read(); 
  
  if(button.wasReleased()){
    buttonReleased = true;
    lastPushed = millis();
  }
  
  timeToLastClick = millis() - lastPushed;

  //Set button hold state
  if ( button.pressedFor(hold_long) ){
    holdState = 3;
  }else if( button.pressedFor(hold_short) ){
    holdState = 2;
  }else if( button.isPressed() && holdState == 0 && timeToLastClick <= dubbleClickSpeed ){
    holdState = 1;
  }

  //Set release action
  if( timeToLastClick > dubbleClickSpeed && buttonReleased ){
      if( holdState == 0 ){
        //On
        Serial.print("Click: ON, ");
        lastCommand = "SHORTCLICK";    
      }else if ( holdState == 1 ){
        //Media on
        Serial.print("Dubble click: Media ON, ");
        lastCommand = "DUBBLECLICK";
      }else if ( holdState == 2 ){
        //Off 
        Serial.print("Hold: Off, ");
        lastCommand = "HOLD";
      }else if ( holdState == 3 ){
        //Cancel click
        Serial.print("Hold long: cancel, "); 
      }else{
        lastCommand = "NOTHING";
      }

      if(lastCommand != "NOTHING" ){
         client.publish((tele_prefix+mqtt_base_topic+button_topic).c_str(), (lastCommand).c_str(), false);
      }      
      
      holdState = 0;
      buttonReleased = false;
  }    
}

void set_led(){
  if(button.isPressed()){
    if( holdState == 3 ){
      //Cancel action, led off
      analogWrite(BUTTON_LED_PIN, 0);
    }else if( holdState == 2 ){
      //State to off, blink
      blink(100, 255);
    }else{
      //Short & medium press
      analogWrite(BUTTON_LED_PIN, 255); //bright on 
    }
  }else{
    if( led_state == "ON"){
      breath(); //breath
    }else{
      analogWrite(BUTTON_LED_PIN, 20); //dim on
    }  
  }
}

void blink(long interval, int brightness){
  static unsigned long previousblinktime = 0;
  unsigned long currentblinktime = millis();
  if (currentblinktime - previousblinktime > interval){
    previousblinktime = currentblinktime;
    if( ledBlinkState == 0){
      ledBlinkState = brightness;
      analogWrite(BUTTON_LED_PIN, ledBlinkState);
    }else{
      ledBlinkState = 0;
      analogWrite(BUTTON_LED_PIN, ledBlinkState);
    }
  }
}

void breath(){
  static unsigned long previousFadeTime = 0;
  static unsigned long startRestTime = 0;
  static int stage = 1;
  static int brightness = 0;
  unsigned long currentFadeTime = millis();
  unsigned long restTime = 5000;
  int fadeDelay = 30;
  int maxBrightness = 150;

  if(currentFadeTime - previousFadeTime > fadeDelay){
    previousFadeTime = currentFadeTime;
    if(stage == 1){
      //Rise
      if(brightness <= maxBrightness ){
        brightness++;
      }else{
        stage = 2;
      }
    } else if( stage == 2 ){
      //Fall
      if(brightness >  0 ){
        brightness--;
      }else{
        startRestTime = currentFadeTime;
        stage = 3;
      }
    } else if ( stage == 3){
      //rest
      if(currentFadeTime - startRestTime > restTime){
        stage = 1;
      }
    }
    analogWrite(BUTTON_LED_PIN, brightness);
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT); 
  pinMode(BUTTON_LED_PIN, OUTPUT);
  analogWrite(BUTTON_LED_PIN, 0);
  
  Serial.begin(115200);
  Serial.println("\r\nBooting...");
  
  setup_wifi();

  Serial.print("Configuring OTA device...");
  TelnetServer.begin();   //Necesary to make Arduino Software autodetect OTA device  
  ArduinoOTA.onStart([]() {Serial.println("OTA starting...");});
  ArduinoOTA.onEnd([]() {Serial.println("OTA update finished!");Serial.println("Rebooting...");});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {Serial.printf("OTA in progress: %u%%\r\n", (progress / (total / 100)));});  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OK");
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("");
  
  timeClient.begin();  
 
  button.begin();
}

void loop() {
  //OTA
  ArduinoOTA.handle();

  //MQTT
  if (!client.connected()) {
    analogWrite(BUTTON_LED_PIN, 0);
    reconnect();
  }

  client.loop();
  
  //Button & led actions  
  button_action();
  set_led();    
}
