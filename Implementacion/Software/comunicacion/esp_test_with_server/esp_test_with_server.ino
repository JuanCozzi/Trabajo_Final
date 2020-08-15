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

#define PAGE "\
  <form action=\"submit\" method=\"POST\">\n\
  "NETWORK":<br>\n\
  <input type=\"text\" name=\""NETWORK"\" value=\"\">\n\
  <br><br>\n\
  "PASSWORD":<br>\n\
  <input type=\"text\" name=\""PASSWORD"\" value=\"\">\n\
  <br><br>\n\
  <input type=\"submit\" value=\"Send\">\n\
  </form>"

const char *ssid = "ESP8266 Access Point"; // The name of the Wi-Fi network that will be created
const char *password = "thereisnospoon";   // The password required to connect to it, leave blank for an open network

const int led = 2;

WiFiClient client;
WebsocketsClient ws_client;

ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80

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

  pinMode(led, OUTPUT);

  WiFi.softAP(ssid, password);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started");

  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP());         // Send the IP address of the ESP8266 to the computer

  if (MDNS.begin("esp8266")) {              // Start the mDNS responder for esp8266.local
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  server.on("/", HTTP_GET, handleRoot);     // Call the 'handleRoot' function when a client requests URI "/"
  server.on("/submit", HTTP_POST, handleSubmit);
  server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");
  
  ws_client.onMessage(onMessageCallback);
}

void loop(void){
  server.handleClient();                    // Listen for HTTP requests from clients
  
  ws_client.poll();
  if(Serial.available())
  {
    //Serial.print(Serial.readString());
    ws_client.send(Serial.readString());
  }
  
  // delay(5000);
}

void handleRoot() {                         // When URI / is requested, send a web page with a button to toggle the LED
  server.send(200, "text/html", PAGE);
}

void handleSubmit() {                          // If a POST request is made to URI /submit
  String ssid;
  String password;
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == NETWORK) {
      Serial.println(NETWORK);
      ssid = server.arg(i);
      Serial.println(ssid); 
    }
    if (server.argName(i) == PASSWORD) {
      Serial.println(PASSWORD);
      password = server.arg(i);
      Serial.println(password); 
    }
  }
  

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

  
  ws_client.connect("http://181.171.18.228:8000/ws/controller/device/0");
  
  digitalWrite(led,!digitalRead(led));      // Change the state of the LED
  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
