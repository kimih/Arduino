/*
  ESP8266
  Climate controller for L72PC
  1. zabbix
  2. DS18B20
  3. PWM carbon filer fan
  4. Door sensor
  5. RPM carbon filetr
  6. Relay
*/


//RPM
volatile int rpmcarbon;
void ICACHE_RAM_ATTR rpmcarb() {
rpmcarbon++;
}


//Zabbix
#include <ESP8266ZabbixSender.h>


//Dallas
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 4 //Pin 4 for DS18B20
#define TEMPERATURE_PRECISION 12
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int numberOfDevices;
DeviceAddress tempDeviceAddress;


//PIN's
int pwm = 16; //PWM (D0 on board)
int pinDoorSensor = 5; //Door sensor (D1 on board)
int pinRPMCarbon = 13; //RPM carbon sensor (D7 on board)
int pinRelay = 12; //Relay (D6 on board)
int relayState = 0; //State relay temp
volatile int newrpmcarbon; //Count RPM carbon

unsigned long lastmillis=0;


//Zabbix
ESP8266ZabbixSender zSender;
#define SERVERADDR XX, XX, XX, XX // Zabbix server Address
#define ZABBIXPORT 10051      // Zabbix erver Port
#define ZABBIXAGHOST "l72pc"  // Zabbix item's host name


/* WiFi settings */
const char* ssid = "***";
const char* password = "***";
boolean checkConnection();
IPAddress ip(192,168,0,XX);  //статический IP
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
IPAddress primaryDNS(192, 168, 0, 1);   //optional
IPAddress secondaryDNS(8, 8, 8, 8); //optional


void setup() {
  Serial.begin(9600);
  
  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();
    
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  Serial.println();
  Serial.print("Connecting to "); // "Подключаемся к "
  Serial.println(ssid);

  WiFi.config(ip, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(ssid, password);

  zSender.Init(IPAddress(SERVERADDR), ZABBIXPORT, ZABBIXAGHOST); // Init zabbix server information
  
  Serial.println("");
  Serial.println("WiFi connected"); // "Подключение к WiFi выполнено"
  Serial.println(WiFi.localIP());
  pinMode(pwm, OUTPUT); //PWM carbon filter

  //RPM
  //attachInterrupt(digitalPinToInterrupt(pinRPMCarbon), rpmcarb, CHANGE);
  pinMode(pinRPMCarbon,INPUT);
  
  //Relay
  pinMode(pinRelay, OUTPUT);
}


void loop() {

  //DS18B20
  sensors.requestTemperatures();
  float tempinside;
  sensors.getAddress(tempDeviceAddress, 0);
  sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
  tempinside = sensors.getTempC(tempDeviceAddress);
  float tempoutside;
  sensors.getAddress(tempDeviceAddress, 1);
  sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
  tempoutside = sensors.getTempC(tempDeviceAddress);
  float tempproff;
  sensors.getAddress(tempDeviceAddress, 2);
  sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
  tempproff = sensors.getTempC(tempDeviceAddress);


  //PWN
  int minSpeedCarb=70; //60 не не стартует
  int pwmspeed = map(tempproff, 24, 37, minSpeedCarb, 255);
  if (pwmspeed<minSpeedCarb) {pwmspeed=minSpeedCarb;}
  if (pwmspeed>255) {pwmspeed=255;}
  analogWrite(pwm, pwmspeed);


  //Door sensor
  int doorsensor = digitalRead(pinDoorSensor);


  //RPM carbon fan
  rpmcarbon=0;
  attachInterrupt(digitalPinToInterrupt(pinRPMCarbon), rpmcarb, CHANGE);
  delay(1000);
  detachInterrupt(pinRPMCarbon);
  newrpmcarbon=rpmcarbon*60/4;


  //Relay
  digitalWrite(pinRelay,HIGH);
  delay(1000);
  digitalWrite(pinRelay,0);
  relayState++;
  

  Serial.print("Temp Inside: ");
  Serial.print(tempinside);
  Serial.print("°C; ");
  
  Serial.print("Temp Outside: ");
  Serial.print(tempoutside);
  Serial.print("°C; ");

  Serial.print("Temp proff: ");
  Serial.print(tempproff);
  Serial.print("°C; ");
  
  Serial.print("PWM Fan: ");
  Serial.print(pwmspeed);
  Serial.print("; ");

  Serial.print("Door sensor: ");
  Serial.print(doorsensor);
  Serial.print("; ");

  Serial.print("RPM Carbon: ");
  Serial.print(newrpmcarbon);
  Serial.print("; ");

  Serial.print("Relay: ");
  Serial.print(relayState);
  Serial.print("; ");

  Serial.println();
  

  checkConnection();
  zSender.ClearItem();
  
   zSender.AddItem("tempinside", (float)tempinside);
   zSender.AddItem("tempoutside", (float)tempoutside);
   zSender.AddItem("tempproff", (float)tempproff);
   zSender.AddItem("pwmspeed", (int)pwmspeed);
   if (zSender.Send() == EXIT_SUCCESS) {      // Send zabbix items
       Serial.println("ZABBIX SEND: OK");
     } else {
      Serial.println("ZABBIX SEND: NG");
     }
     zSender.ClearItem();
   zSender.AddItem("doorsensor", (int)doorsensor);
   zSender.AddItem("rpmcarbon", (int)newrpmcarbon);
   zSender.AddItem("relaystate", (int)relayState);
   if (zSender.Send() == EXIT_SUCCESS) {      // Send zabbix items
       Serial.println("ZABBIX SEND: OK");
     } else {
      Serial.println("ZABBIX SEND: NG");
     }
     zSender.ClearItem();
   
   Serial.println();
  
  delay(20000);
}

boolean checkConnection() {
  int count = 0;
  Serial.print("Waiting for Wi-Fi connection");
  while (count < 3) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("Connected!");
      return (true);
    }
    delay(500);
    Serial.print(".");
    count++;
  }
  Serial.println("Timed out.");
  return false;
}
