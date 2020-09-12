#include <ArduinoWebsockets.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>   // Include the WebServer library
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>



// Mensajes del protocolo MSG:
#define OK                      11  // Bidirectional
#define BAD_REQUEST             12  // Device to SV
#define BAD_PARAMETER           13  // Device to SV
#define SV_ERROR_REQUEST        14  // SV to Device
#define CONTROL_TEMP_ON         51  // Device to SV
#define CONTROL_TEMP_OFF        52  // Device to SV
#define MANUAL_OPERATION        101 // Device to SV
#define HOUR_OPERATION          102 // Device to SV
#define SHUNTDOWN_LEVEL         111 // Device to SV
#define SHUNTDOWN_TIME_ON       112 // Device to SV
#define SHUNTDOWN_TMAX          113 // Device to SV
#define SHUTDOWN_CURRENT_FAIL   114 // Device to SV
#define REQUEST_ADD_OUTPUT      201 // SV to Device
#define RESPONSE_ADD_OUTPUT     202 // Device to SV
#define REQUEST_DELETE_OUTPUT   203 // SV to Device
#define RESPONSE_DELETE_OUTPUT  204 // SV to Device
#define REQUEST_EDIT_PARAMETER  205 // SV to Device
#define RESPONSE_EDIT_PARAMETER 206 // Device to SV
#define REQUEST_EDIT_TEMP       207 // SV to Device
#define RESPONSE_EDIT_TEMP      208 // Device to SV
#define REQUEST_OPERATING_TIME  211 // SV to Device
#define RESPONSE_OPERATING_TIME 212 // Device to SV
#define REQUEST_ALL_PARAMETER   213 // SV to Device
#define RESPONSE_ALL_PARAMETER  214 // Device to SV



#define NETWORK "Network"
#define PASSWORD "Password"

#define AUTHENTICATION_JSON_SIZE JSON_OBJECT_SIZE(2) + 60

using namespace websockets;

const char *ssid = "";  // Network
const char *password = ""; // Network's password

const char *device_id = "tyWj5hd34k";
const char *device_password = "qwertyas";

const char *header_keys[] = {"Set-Cookie"};
const size_t header_keys_num = 1;

String session_id;
String csrftoken;

const int led = 2;

WiFiClient client;
WebsocketsClient ws_client;

ESP8266WiFiMulti wifiMulti;

bool authenticated = false;
bool websocket_connected = false;

bool get_cookie_value(const String &cookies, const String &id, String &value) {
  // const char *id_start = strstr(cookies.c_str(), id.c_str());
  // const char *value_start = id_start + id.length() + 1; // + 1 por el =
  // const char *value_end = strstr(id_start, ";");
  int id_start = cookies.indexOf(id);
  if (id_start == -1)
    return false;
  int value_end = cookies.indexOf(";", id_start) + 1;
  if (value_end == -1)
    return false;
  // Cookie with the id and the semicolon
  value = cookies.substring(id_start, value_end);
  // Serial.println("Cookie parsed");
  // Serial.println(value);
}

void handleRoot();              // function prototypes for HTTP handlers
void handleNotFound();

void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        websocket_connected = true;
        Serial.println("Connnection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        websocket_connected = false;
        Serial.println("Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
    }
}

void onMessageCallback(WebsocketsMessage msg){
  // Serial.println("Got message: " + msg.data());
  Serial.println("Received");
  Serial.println(msg.data());
  DynamicJsonDocument doc(800);
  deserializeJson(doc, msg.data());

  if (!doc.containsKey("msg"))
    return;

  bool response_needed = false;

  switch (doc["msg"].as<int>()) {
    case OK:
      break; // Go out whit any change
    case BAD_REQUEST:
      break;
    case BAD_PARAMETER:
      break;
    case SV_ERROR_REQUEST:
      break; 
    case CONTROL_TEMP_ON:
      break;
    case CONTROL_TEMP_OFF:
      break;
    case MANUAL_OPERATION:
      break; 
    case HOUR_OPERATION:
      break;
    case SHUNTDOWN_LEVEL:
      break;
    case SHUNTDOWN_TIME_ON:
      break;
    case SHUTDOWN_CURRENT_FAIL:
      break;
    case REQUEST_ADD_OUTPUT:
      doc["msg"] = RESPONSE_ADD_OUTPUT;
      response_needed = true;
      break; // keep going
    case RESPONSE_ADD_OUTPUT:
      break;
    case REQUEST_DELETE_OUTPUT:
      doc["msg"] = RESPONSE_DELETE_OUTPUT;
      response_needed = true;
      break;
    case RESPONSE_DELETE_OUTPUT:
      break;
    case REQUEST_EDIT_PARAMETER:
      doc["msg"] = RESPONSE_EDIT_PARAMETER;
      response_needed = true;
      break;
    case RESPONSE_EDIT_PARAMETER:
      break;
    case REQUEST_EDIT_TEMP:
      doc["msg"] = RESPONSE_EDIT_TEMP;
      response_needed = true;
      break;
    case RESPONSE_EDIT_TEMP:
      break;
    case REQUEST_OPERATING_TIME:
      doc["msg"] = RESPONSE_OPERATING_TIME;
      doc["data"] = doc.createNestedObject();
      doc["data"]["Temp"] = doc.createNestedObject();
      doc["data"]["Temp"].add(23.5);
      doc["data"]["Temp"].add(33.8);
      doc["data"]["OperationTime"] = doc.createNestedObject();
      for (size_t i = 0; i < 10; i++)
        doc["data"]["OperationTime"].add((i + 1) * 10);
      response_needed = true;
      break;
    case RESPONSE_OPERATING_TIME:
      break;
    case REQUEST_ALL_PARAMETER:
      doc["msg"] = RESPONSE_ALL_PARAMETER;
      response_needed = true;
      break;
    case RESPONSE_ALL_PARAMETER:
      break;
    default:
      break;
  }

  if (response_needed)
  {
    Serial.println(doc.as<String>());
    char response[measureJson(doc) + 1];
    serializeJson(doc, response, measureJson(doc) + 1);
    Serial.println("Response");
    Serial.println(response);
    ws_client.send(response);
  }
}

void wifi_connect() {
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
}

void websocket_connect() {
  ws_client.addHeader("Cookie", session_id);
  websocket_connected = ws_client.connect("http://181.171.18.228:8000/ws/controller/device");
  // Serial.println("Estoy conectado?");
  // Serial.println(websocket_connected);
}

void authenticate() {
  // create JSON body with credentials
  StaticJsonDocument<AUTHENTICATION_JSON_SIZE> doc;
  doc["username"] = device_id;
  doc["password"] = device_password;
  Serial.println("Memory usage");
  // Serial.println(doc.memoryUsage());
  Serial.println(measureJson(doc));
  // char authentication_body[AUTHENTICATION_JSON_SIZE];
  char authentication_body[measureJson(doc) + 1];
  serializeJson(doc, authentication_body, measureJson(doc) + 1);
  Serial.println(authentication_body);

  // make authentication request
  HTTPClient http;    //Declare object of class HTTPClient
  
  //Post Data
  http.begin(client, "http://181.171.18.228:8000/accounts/login/"); //Specify request destination
  http.collectHeaders(header_keys, header_keys_num);
  http.addHeader("Content-Type", "application/json");

  int http_code = http.POST(authentication_body);   //Send the request
  String payload = http.getString();    //Get the response payload

  Serial.println(http_code);   //Print HTTP return code
  Serial.println(payload);    //Print request response payload
  Serial.println(payload.length());

  if (http_code <= 0) {
    authenticated = false;
    Serial.print("Error authenticating ");
    Serial.println(http_code);
    return;
  }

  String cookies_header = http.header("Set-Cookie");
  Serial.println(cookies_header);

  if (get_cookie_value(cookies_header, String("sessionid"), session_id)) {
    Serial.println("Cookie parsed");
    Serial.println(session_id);
    authenticated = true;
  } else {
    authenticated = false;
    Serial.println("Error parsing sessionid cookie");
  }

  http.end();  //Close connection
}

void setup(void) {
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  wifi_connect();

  authenticate();

  // configure websocket
  ws_client.onEvent(onEventsCallback);
  ws_client.onMessage(onMessageCallback);
  
}

void loop(void){
  //ws_client.send("Holaaaaaaaaaa");
  if (!authenticated) {
    Serial.println("Authenticating");
    authenticate();
  }
  if (!websocket_connected && authenticated)
  {
    Serial.println("Trying to reconnect");
    websocket_connect();    
  }
  if (ws_client.available())
  {
    //Serial.println("Websocket available");
    ws_client.poll();
    // ws_client.send("{\"data\":{\"algo\":1},\"msg\":212}");
  }
  else
  {
    Serial.println("Websocket unavailable");
  }
  
  if (Serial.available())
  {
    //Serial.print(Serial.readString());
    ws_client.send(Serial.readString());
  }
  
  // delay(5000);
}
