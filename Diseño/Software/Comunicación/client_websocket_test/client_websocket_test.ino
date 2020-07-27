#include <ArduinoWebsockets.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>   // Include the WebServer library
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define NETWORK "Arnet"
#define PASSWORD "canelajazmin"

using namespace websockets;

const char *ssid = "Arnet";  // Network
const char *password = "canelajazmin"; // Network's password

const int led = 2;

WiFiClient client;
WebsocketsClient ws_client;

ESP8266WiFiMulti wifiMulti;

void handleRoot();              // function prototypes for HTTP handlers
void handleNotFound();

void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Connnection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        // Serial.println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
    }
}

void onMessageCallback(WebsocketsMessage msg){
  // Serial.println("Got message: " + msg.data());
  Serial.print(msg.data());
}

void setup(void){
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  Serial.println("Connecting to ");
  Serial.println(ssid);

  //connect to your local wi-fi network
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  ws_client.onEvent(onEventsCallback);
  ws_client.onMessage(onMessageCallback);
  // ws_client.connect("http://ec2-18-220-192-186.us-east-2.compute.amazonaws.com:8000/controller/ws/device/0");
  ws_client.connect("http://181.171.18.228:8000/ws/controller/device/0");
}

void loop(void){
  //ws_client.send("Holaaaaaaaaaa");
  ws_client.poll();
  if(Serial.available())
  {
    //Serial.print(Serial.readString());
    ws_client.send(Serial.readString());
  }
  
  delay(5000);
}
