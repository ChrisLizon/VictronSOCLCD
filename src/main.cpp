#include <Arduino.h>
#include <PubSubClient.h>
#include <string.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <credentials.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>





const char* SOC_topic = "N/d41243cd374d/battery/279/Soc";
const char* SOC_request = "R/d41243cd374d/battery/279/Soc";


const char* AC_state_topic = "N/d41243cd374d/vebus/276/Ac/ActiveIn/Connected";
const char* AC_mode_topic = "N/d41243cd374d/vebus/276/State";
const char* AC_out_topic = "N/d41243cd374d/vebus/276/Ac/Out/L1/P";
const char* AC_in_topic = "N/d41243cd374d/vebus/276/Ac/ActiveIn/L1/P";
const char* AC_in_request = "R/d41243cd374d/vebus/276/Ac/ActiveIn/L1/P";

const char* Relay2_topic = "N/d41243cd374d/system/0/Relay/1/State";

const char* PV_topic = "N/d41243cd374d/solarcharger/0/Yield/Power";
const char* PV_request = "R/d41243cd374d/solarcharger/0/Yield/Power";

const char* keepalivetopic = "R/d41243cd374d/keepalive";
const char* keepalivepayload =  "[\"battery/+/Soc\", \"solarcharger/+/Yield/Power\", \"vebus/+/Ac/ActiveIn/Connected\", \"vebus/276/Ac/ActiveIn/L1/P\", \"vebus/+/Ac/Out/L1/P\", \"vebus/+/State\", \"system/0/Relay/1/State\"]" ;



int relayPin = 16;

WiFiClientSecure wifiClient;
PubSubClient mqttClient;

int lastSOC = 51;
int lastACState = 0;
int lastACIn = 0;
int lastSolarYield = 0;
int lastACOut = 0;
int lastInverterMode = 0;
int lastRelay2state = 0;

const char *inverterMode[]= { "Off", "Low Power", "Fault", "Bulk", "Absorption","Float","Storage", "Equalize","Passthru","Inverting","Assisting","Power Supply" }; 




LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4);

void printout(){
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("SOLAR MONITOR:");
  lcd.setCursor(0,1);
  lcd.print("PV IN  AC IN  AC OUT");
  lcd.setCursor(0,2);
  lcd.print(lastSolarYield);
  lcd.print("W");
  lcd.setCursor(8,2);
  lcd.print(lastACIn);
  lcd.print("W");
  lcd.setCursor(14,2);  
  lcd.print(lastACOut);
  lcd.print("W"); 
  lcd.setCursor(0,3);
  lcd.print(lastInverterMode);
  lcd.setCursor(11,3);
  lcd.print("SOC: ");
  lcd.print(lastSOC);
  lcd.print("%");

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
  
  if (error)
  {
    Serial.print("parseObject(");
    Serial.print(value);
    Serial.println(") failed");
    return;
  }
  
  String t = String(topic);

  if(t.indexOf(String("Soc")) > 0){
    Serial.println("Processing SOC measurement.");
    int z = 0;

    String s = doc["value"];
    z = s.toInt();

    if( z != 0 && lastSOC > 50){
      lastSOC = z;
    }

  }else if(t.indexOf(String("Connected")) > 0){
    Serial.println("Processing AC Input.");
    int z = 0;

    String s = doc["value"];
    z = s.toInt();

    lastACState = z;
  }else if(t.indexOf(String("Yield/Power")) > 0){
    
    int z = 0;

    String s = doc["value"];
    z = s.toInt();

    lastSolarYield = z;
    
    
  }  else if(t.indexOf(String("Out/L1/P")) > 0){
    
    int z = 0;

    String s = doc["value"];
    z = s.toInt();

    lastACOut = z;
    
    
  }
  else if(t.indexOf(String("276/State")) > 0){
    
    int z = 0;

    String s = doc["value"];
    z = s.toInt();

    lastInverterMode = z;
    
    
  }else if(t.indexOf(String("1Relay/1/State")) > 0){
    
    int z = 0;

    String s = doc["value"];
    z = s.toInt();

    lastRelay2state = z;
    
    
  }

  
  if(lastACState == 1){
    //Serial.println("Generator Online, Water heater ON.");
    digitalWrite(relayPin, HIGH);
  } else if (lastSOC >= 95){
    //Serial.print("Battery SOC = ");
    //Serial.print(lastSOC);
    //Serial.println(". Water heater ON.");
    
    digitalWrite(relayPin, HIGH);
  }
  else{
    //Serial.print("No AC and battery SOC = ");
    //Serial.print(lastSOC);
    //Serial.println(". Water heater OFF.");
    digitalWrite(relayPin, LOW);
  }

  printout();
  
  
}


void setup() {

  Wire.begin(I2C_SDA, I2C_SCL);

  lcd.init();
  lcd.backlight();

  delay(3000);
  // put your setup code here, to run once:
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

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");

    // Attempt to connect
    if (mqttClient.connect(CLIENT_ID, VRM_USERNAME, VRM_PASSWORD)) {
      Serial.print("Connected to ");
      Serial.println(MQTT_SERVER);
      //subscribe to battery SOC
      mqttClient.subscribe(SOC_topic);
      mqttClient.subscribe(AC_state_topic);
      mqttClient.subscribe(AC_out_topic);
      mqttClient.subscribe(AC_mode_topic);
      mqttClient.subscribe(AC_in_topic);
      mqttClient.subscribe(PV_topic);
      
      //publish request for S
      mqttClient.publish(SOC_request, "", true);
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

int i = 600;

void loop() {
  // put your main code here, to run repeatedly:
  if (!mqttClient.connected()) {
     reconnect();
  }

  if (i>60){
    Serial.println("\nRequesting SOC refresh");
    mqttClient.publish(SOC_request, "", true);

    mqttClient.publish(keepalivetopic, keepalivepayload, true);
    i=0;
  }

  i++;
  //Serial.print(".");
  mqttClient.loop();
  delay(1000);
  yield();
  
}