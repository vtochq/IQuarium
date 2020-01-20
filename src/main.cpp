// IQuarium is a smart aquarium controller
// (c) Alexander Kolesnikov, 2020
//

#include <ESP8266WiFi.h>  //For ESP8266
#include <PubSubClient.h> //For MQTT
#include <ESP8266mDNS.h>  //For OTA
#include <WiFiUdp.h>      //For OTA
#include <ArduinoOTA.h>   //For OTA

#include "looper.h"

#include <NeoPixelBus.h>


looper myScheduler; //create a new istance of the class looper

//WIFI configuration
#define wifi_ssid "VHOME"
#define wifi_password "Whatthefucktoallaga"

//MQTT configuration
#define mqtt_server "192.168.11.4"
#define mqtt_user "test"
#define mqtt_password "Test123"
String hostname="iquarium-01";   //This text is concatenated with ChipId to get unique client_id
//MQTT Topic configuration
#define position_topic "/position"
#define temperature_topic "/temperature"


const uint16_t PixelCount = 30; // make sure to set this to the number of pixels in your strip
const uint8_t PixelPin = 3;  // make sure to set this to the correct pin, ignored for Esp8266

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

RgbColor desired[PixelCount];

RgbColor black(0,0,0);



class Anima {
  public:
    float progress = 0;
    float prog() {
      if (progress < 1)
        progress +=0.005;
      return progress;
    }
};


Anima progress;


//MQTT client
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

//Necesary to make Arduino Software autodetect OTA device
WiFiServer TelnetServer(8266);

void setup_wifi() {
  delay(10);
  Serial.print("Connecting to ");
  Serial.print(wifi_ssid);
  WiFi.hostname(hostname);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("OK");
  Serial.print("   IP address: ");
  Serial.println(WiFi.localIP());
}


void mqtt_reconnect() {
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {    
    if (mqtt_client.connect(hostname.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}


void iqmove(byte pos) {
  for(int i = 0; i < PixelCount; i++) {
    if (i < pos || i > pos+4)
      desired[i] = RgbColor(50,40,40);
    else
      desired[i] = RgbColor(110,90,0);
  }
  progress.progress = 0;
}


// MQTT sub callback
void callback(char* topic, unsigned char* payload, unsigned int length) {
  payload[length] = '\0';
  byte payl=atoi((char *)payload);
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(payl);
  Serial.println();
  iqmove(payl);
}


void task1()
{
    RgbColor white(100,100,100);
    RgbColor black(0,0,0);
    //  strip.ClearTo(whiteLeft, 0, 47);
    if (strip.GetPixelColor(5) != white) {
        strip.SetPixelColor(5, white);
    }
    else {
        strip.SetPixelColor(5, black);
    }
    
    strip.Show();
}



void blend()
{
  float p = progress.prog();
  for(int i = 0; i < PixelCount; i++) {
    strip.SetPixelColor(i, RgbColor().LinearBlend(strip.GetPixelColor(i), desired[i], p));
  }
  strip.Show();
}

void setup()
{
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

  Serial.println("Configuring MQTT server...");
  //hostname=hostname+ESP.getChipId();
  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setCallback(callback);


  Serial.printf("   Server IP: %s\r\n",mqtt_server);  
  Serial.printf("   Username:  %s\r\n",mqtt_user);
  Serial.println("   Cliend Id: "+hostname);  
  Serial.println("   MQTT configured!");

  Serial.println("Setup completed! Running app...");


  strip.Begin();
  RgbColor black(0,0,0);
  strip.ClearTo(black);
  strip.Show();

  //myScheduler.addJob(task1, 1000);
  myScheduler.addJob(blend, 40);

  for(int i = 0; i < PixelCount; i++) {
    if (i < 5 || i > 9)
      desired[i] = RgbColor(50,40,40);
    else
      desired[i] = RgbColor(110,90,0);
  }
  // desired[0] = RgbColor(50,0,0);
  // desired[1] = RgbColor(0,50,0);
  // desired[2] = RgbColor(0,0,50);
  

}


void loop()
{

    ArduinoOTA.handle();

    if (!mqtt_client.connected()) {
        mqtt_reconnect();
        mqtt_client.subscribe((hostname+position_topic).c_str());
    }
    mqtt_client.loop();
    
    //myScheduler.myDelay(1000);
    myScheduler.scheduler();

}