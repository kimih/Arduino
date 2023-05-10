/*
  Climate controller for L72PC
  Based on ESP8266
  
  l72pc-ZDPD.ino
  
  1. Z - zabbix;
  2. D - Dallas; Sensors DS18B20, pin 4 (D2 on board)
  3. P - PWM carbon filer fan, pin 16 (D0 on board)
  4. D - Door sensor, pin 5 (D1 on board)
*/
//Zabbix
#include <ESP8266ZabbixSender.h>

//Dallas
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 4 //Pin 4 for DS18B20 (D2 on board)
#define TEMPERATURE_PRECISION 12

int pwm = 16; //Pin 16 For PWM (D0 on board)
int pinDoorSensor = 5; //Pin 5 For Door sensor (D1 on board)

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int numberOfDevices;
DeviceAddress tempDeviceAddress;


//Zabbix
ESP8266ZabbixSender zSender;

/* WiFi settings */
const char* ssid = "***";
const char* password = "***";

/* Zabbix server setting */
#define SERVERADDR **, **, **, ** // Zabbix server Address
#define ZABBIXPORT 10051      // Zabbix erver Port
#define ZABBIXAGHOST "l72pc"  // Zabbix item's host name

boolean checkConnection();

IPAddress ip(192,168,0,**);  //статический IP
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
}

void loop() {
  
  //Temperature
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
  int pwmspeed = map(tempproff, 25, 35, 0, 255);
  if (pwmspeed<60) {pwmspeed=60;}
  if (pwmspeed>255) {pwmspeed=255;}
  analogWrite(pwm, pwmspeed);

  //Door sensor
  int doorsensor = digitalRead(pinDoorSensor);

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

  Serial.println();
   

  //Zabbix sender
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
  if (zSender.Send() == EXIT_SUCCESS) {      // Send zabbix items
    Serial.println("ZABBIX SEND: OK");
  } else {
    Serial.println("ZABBIX SEND: NG");
  }
  zSender.ClearItem();
  
  

  Serial.println();
  delay(5000);
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
