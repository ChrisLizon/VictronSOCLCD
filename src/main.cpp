#include <Arduino.h>
#include <PubSubClient.h>
#include <string.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <credentials.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>




//Battery SOC
const char* SOC_topic = "%c/%s/battery/%s/Soc";

//Values coming from the Multiplus
const char* AC_state_topic = "%c/%s/vebus/%s/Ac/ActiveIn/Connected";
const char* AC_mode_topic = "%c/%s/vebus/%s/State";
const char* AC_out_topic = "%c/%s/vebus/%s/Ac/Out/L1/P";
const char* AC_in_topic = "%c/%s/vebus/%s/Ac/ActiveIn/L1/P";
const char* AC_in_request = "%c/%s/vebus/%s/Ac/ActiveIn/L1/P";

//PV Values, assumes one MPPT device
const char* PV_topic = "%c/%s/solarcharger/0/Yield/Power";

//system settings, 
const char* Relay2_topic = "%c/%s/system/0/Relay/1/State";

const char* keepalivetopic = "%c/%s/keepalive";
const char* keepalivepayload =  "[\"battery/+/Soc\", \"solarcharger/+/Yield/Power\", \"vebus/+/Ac/ActiveIn/Connected\", \"vebus/276/Ac/ActiveIn/L1/P\", \"vebus/+/Ac/Out/L1/P\", \"vebus/+/State\", \"system/0/Relay/1/State\"]" ;

//get the battery topic with subscribe (N) vs publish (R) path
// BATTERY_ID would be the device identifyer for your Shunt. 
char* getBatteryTopic(const char* topicstr, char mode){
  char* buffer = new char[35];
  snprintf(buffer, 35, topicstr, mode, INSTALLATION_ID, BATTERY_ID);
  return buffer;
}

//get the multiplus topics with subscribe (N) vs publish (R) path
// MULTIPLUS_ID would be the device identifyer for your multiplus. 
char* getMultiplusTopic(const char* topicstr, char mode){
  char* buffer = new char[50];
  snprintf(buffer, 50, topicstr, mode, INSTALLATION_ID, MULTIPLUS_ID);
  return buffer;
}

//get the multiplus topics with subscribe (N) vs publish (R) path
//these paths are relative to the GX device and don't have unique IDS included. 
char* getTopic(const char* topicstr, char mode){
  char* buffer = new char[50];
  snprintf(buffer, 50, topicstr, mode, INSTALLATION_ID);
  return buffer;
}

//drive an external relay.
int relay2Pin = 16;

WiFiClientSecure wifiClient;
PubSubClient mqttClient;

//state information
int lastSOC = -1;
int lastACState = -1;
int lastACIn = -1;
int lastSolarYield = -1;
int lastACOut = -1;
int lastInverterMode = 12;
int lastRelay2state = 0;

//inverter mode array. 
const char *inverterMode[]= { "Off         ",
                              "Low Power   ",
                              "Fault       ",
                              "Bulk        ",
                              "Absorption  ",
                              "Float       ",
                              "Storage     ", 
                              "Equalize    ",
                              "Passthru    ",
                              "Inverting   ",
                              "Assisting   ",
                              "Power Supply",
                              "Connecting.." }; 




LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4);


//upadte what's on the LCD
void refreshLCD(){
  lcd.setCursor(0,0);
  lcd.print("SOLAR MONITOR:");
  lcd.setCursor(0,1);
  lcd.print("PV IN  AC IN  AC OUT");
  lcd.setCursor(0,2);
  lcd.printf("%4dW", lastSolarYield);
  lcd.setCursor(7,2);
  lcd.printf("%4dW", lastACIn);
  lcd.setCursor(15,2);  
  lcd.printf("%4dW", lastACOut);
  lcd.setCursor(0,3);
  lcd.print(inverterMode[lastInverterMode]);
  lcd.setCursor(11,3);
  lcd.print("SOC: ");
  lcd.printf("%3d%%", lastSOC);
}
//print the current state information to the serial console
void printout(){
  
  Serial.print("SOC: ");
  Serial.print(lastSOC);
  Serial.println("%");
  Serial.print("AC Input Connected: ");
  Serial.println(lastACState == 0? "No" : "Yes");
  Serial.print("Inverter Mode: ");
  Serial.println(inverterMode[lastInverterMode]);
  Serial.print("AC Input: ");
  Serial.print(lastACIn);
  Serial.println("W");
  Serial.print("AC Output: ");
  Serial.print(lastACOut);
  Serial.println("W");
  Serial.print("Solar Yieild: ");
  Serial.print(lastSolarYield);
  Serial.println("W");
  Serial.print("Relay 2 On: ");
  Serial.println(lastRelay2state == 1 ? "Yes" : "No");
}

//this function fires when a MQTT message is received. 
void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("\nMessage arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if(length ==0){
    return;
  }
  payload[length] = '\0';  
    
  //convert to string, parse JSON
  String value = String((char*)payload);
  StaticJsonDocument<80> doc;

  DeserializationError error = deserializeJson(doc, value);
  
  //all of the Victron MQTT subjects have a JSON object. If we can't
  //deserialize it we will just escape the fucntion. This happens
  //when subscribed to a topic that isn't currently set to keepalive
  if (error)
  {
    Serial.print("parseObject(");
    Serial.print(value);
    Serial.println(") failed");
    return;
  }
  
  String t = String(topic);

  if(t.indexOf(String("Soc")) > 0){
    int z = 0;

    String s = doc["value"];
    z = s.toInt();

    lastSOC = z;

  }else if(t.indexOf(String("Connected")) > 0){
    //Serial.println("Processing AC Input.");
    int z = 0;

    String s = doc["value"];
    z = s.toInt();

    lastACState = z;
  }else if(t.indexOf(String("Yield/Power")) > 0){
    
    int z = 0;

    String s = doc["value"];
    z = s.toInt();
    
    lastSolarYield = z;
    
  }else if(t.indexOf(String("Out/L1/P")) > 0){
    
    int z = 0;

    String s = doc["value"];
    z = s.toInt();

    lastACOut = z;
    
    
  }else if(t.indexOf(String("Relay/1/State")) > 0){
    
    int z = 0;

    String s = doc["value"];
    z = s.toInt();

    lastRelay2state = z;
    digitalWrite(relay2Pin, z);
    
    
  }else if(t.indexOf(String("/vebus/")) > 0 && t.endsWith(String("/State"))){
    
    int z = 0;

    String s = doc["value"];
    z = s.toInt();

    lastInverterMode = z;
  }


  //refresh the serial console now that we've recieved a new value
  printout();
  //refresh the LCD display
  refreshLCD();
}


void setup() {

  Wire.begin(I2C_SDA, I2C_SCL);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  refreshLCD();

  //only here so I can switch to my serial console to debug in time
  delay(5000); 

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PW);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);  
  }
  Serial.println();
  Serial.println(WiFi.localIP());
  Serial.println();
  
  // this is probably bad, but we need to connect to SSL with weird certs.
  wifiClient.setInsecure();

  mqttClient.setClient(wifiClient);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(callback);
  

  //Setup GPIO for local relay/LED
  pinMode(relay2Pin, OUTPUT);
  digitalWrite(relay2Pin, LOW);
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");

    // Attempt to connect
    if (mqttClient.connect(CLIENT_ID, VRM_USERNAME, VRM_PASSWORD)) {
      Serial.print("Connected to ");
      Serial.println(MQTT_SERVER);
      //subscribe to our MQTT topics
      mqttClient.subscribe(getBatteryTopic(SOC_topic, 'N'));
      mqttClient.subscribe(getMultiplusTopic(AC_state_topic, 'N'));
      mqttClient.subscribe(getMultiplusTopic(AC_out_topic, 'N'));
      mqttClient.subscribe(getMultiplusTopic(AC_mode_topic, 'N'));
      mqttClient.subscribe(getMultiplusTopic(AC_in_topic, 'N'));
      mqttClient.subscribe(getTopic(PV_topic,'N'));
      mqttClient.subscribe(getTopic(Relay2_topic, 'N'));
      

    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

int i = 6000;

void loop() {
  // put your main code here, to run repeatedly:
  if (!mqttClient.connected()) {
    lcd.clear();
    lcd.print("Lost connection");
    lcd.setCursor(0,1);
    lcd.print("to MQTT...");
    reconnect();
  }

  //every so many loops we will request a new update from the MQTT brokers
  if (i>600){
    Serial.println("\nRequesting SOC refresh");
    mqttClient.publish(getBatteryTopic(SOC_topic, 'R'), "", true);
    mqttClient.publish(getTopic(keepalivetopic,'R'), keepalivepayload, true);
    i=0;
  }

  i++;
  
  mqttClient.loop();
  delay(100);
  yield();
  
}