#include <ArduinoWebsockets.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>   // Include the WebServer library
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <FS.h>   // Include the SPIFFS library
#include <EEPROM.h>



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
#define REQUEST_UNLINK   215 // SV to Device
#define RESPONSE_UNLINK  216 // Device to SV
#define UNLINK_CONFIRMED_BY_SERVER   217 // SV to Device



#define URI_ROOT "/"
#define URI_DEVICE_STATUS "/device/status"
#define URI_NETWORK_CONFIG "/network"
#define URI_USER_CONFIG "/user"

#define FORM_FIELD_NETWORK "network"
#define FORM_FIELD_PASSWORD "password"
#define FORM_FIELD_USER_NAME "user_name"
#define FORM_FIELD_EMAIL "email"
#define FORM_FIELD_DEVICE_NAME "device_name"
#define PAGE_ROOT "\
  <form action="URI_NETWORK_CONFIG">\
      <input type=\"submit\" value=\"Network\" />\
  </form>\
  <form action="URI_USER_CONFIG">\
      <input type=\"submit\" value=\"Account\" />\
  </form>"
#define PAGE_NETWORK_CONFIG "\
  <form action="URI_NETWORK_CONFIG" method=\"POST\">\n\
  Network:<br>\n\
  <input type=\"text\" name=\""FORM_FIELD_NETWORK"\" value=\"\">\n\
  <br><br>\n\
  Password:<br>\n\
  <input type=\"password\" name=\""FORM_FIELD_PASSWORD"\" value=\"\">\n\
  <br><br>\n\
  <input type=\"submit\" value=\"Send\">\n\
  </form>"
#define PAGE_USER_CONFIG "\
  <form action="URI_USER_CONFIG" method=\"POST\">\n\
  Username:<br>\n\
  <input type=\"text\" name=\""FORM_FIELD_USER_NAME"\" value=\"\">\n\
  <br><br>\n\
  Email:<br>\n\
  <input type=\"email\" name=\""FORM_FIELD_EMAIL"\" value=\"\">\n\
  <br><br>\n\
  Device name:<br>\n\
  <input type=\"text\" name=\""FORM_FIELD_DEVICE_NAME"\" value=\"\">\n\
  <br><br>\n\
  Password:<br>\n\
  <input type=\"password\" name=\""FORM_FIELD_PASSWORD"\" value=\"\">\n\
  <br><br>\n\
  <input type=\"submit\" value=\"Send\">\n\
  </form>"

#define USER_NETWORK_SSID_MAX_LEN 50
#define USER_NETWORK_PASSWORD_MAX_LEN 50
#define DEVICE_USER_MAX_LEN 50
#define EEPROM_NETWORK_DATA_PRESENT_START_POS 0
#define EEPROM_DEVICE_USER_DATA_PRESENT_START_POS 1
#define EEPROM_USER_SSID_START_POS EEPROM_DEVICE_USER_DATA_PRESENT_START_POS + 1
#define EEPROM_USER_NETWORK_PASSWORD_START_POS EEPROM_USER_SSID_START_POS + USER_NETWORK_SSID_MAX_LEN + 1
#define EEPROM_DEVICE_USER_START_POS EEPROM_USER_NETWORK_PASSWORD_START_POS + DEVICE_USER_MAX_LEN + 1

#define SERVER_URL "http://181.171.18.228:8000"

const char *ssid = "ESP8266 Access Point"; // The name of the Wi-Fi network that will be created
const char *network_password = "thereisnospoon";   // The password required to connect to it, leave blank for an open network

#define AUTHENTICATION_JSON_SIZE JSON_OBJECT_SIZE(2) + 60

using namespace websockets;

// const char *user_ssid = "Fibertel WiFi360 2.4GHz";  // Network
// const char *user_network_password = "0041697312"; // Network's password

const char *device_id = "pd3";
const char *device_password = "qwertyas";

const char *header_keys[] = {"Set-Cookie"};
const size_t header_keys_num = 1;

String session_id;
String csrftoken;

const int led = 2;

WiFiClient client;
WebsocketsClient ws_client;

ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80

ESP8266WiFiMulti wifiMulti;

bool authenticated = false;
bool cookie_added = false;
bool websocket_connected = false;

bool user_network_data = false;
bool device_associated = false;
String device_user;
// String user_ssid;
// String user_network_password;

void handle_root();              // function prototypes for HTTP handlers
void handle_device_status();
void handle_network_config_get();
void handle_network_config_post();
void handle_user_config_get();
void handle_user_config_post();
void handle_not_found();
bool get_cookie_value(const String &cookies, const String &id, String &value);
void onEventsCallback(WebsocketsEvent event, String data);
void onMessageCallback(WebsocketsMessage msg);
void server_start();
void wifi_connect(const char *ssid, const char *password);
void websocket_connect();
void authenticate();
void load_user_data(String &user_ssid, String &user_network_password);
void save_network_data(const String &user_ssid, const String &user_network_password);
void save_device_user();
void delete_device_user();


void setup(void) {
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  pinMode(led, OUTPUT);

  EEPROM.begin(512);

  String user_ssid, user_network_password;
  load_user_data(user_ssid, user_network_password);
  Serial.print("User network data");
  Serial.print(user_ssid);
  Serial.println(user_network_password);

  if (user_network_data)
    wifi_connect(user_ssid.c_str(), user_network_password.c_str());

  server_start();

  // configure websocket
  ws_client.onEvent(onEventsCallback);
  ws_client.onMessage(onMessageCallback);
  
}

void loop(void){
  server.handleClient();                    // Listen for HTTP requests from clients

  if (WiFi.isConnected()) {
    if (!authenticated) {
      Serial.println("Authenticating");
      authenticate();
    }
    if (device_associated) {
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
    } else {
      Serial.println("User not linked");
      delay(1000);
    }
  } else {
    delay(1000);
  }
  
  // delay(5000);
}

void load_user_data(String &user_ssid, String &user_network_password) {
  int pos = EEPROM_NETWORK_DATA_PRESENT_START_POS;
  user_network_data = EEPROM.read(pos++);
  Serial.print("user_network_data ");
  Serial.println(user_network_data);
  device_associated = EEPROM.read(pos++);
  Serial.print("device_associated ");
  Serial.println(device_associated);
  if (user_network_data) {
    user_ssid = "";
    pos = EEPROM_USER_SSID_START_POS;
    byte user_ssid_len = EEPROM.read(pos++);
    Serial.print("Largo ssid");
    Serial.println(user_ssid_len);
    for (byte i = 0; i < user_ssid_len; i++)
    {
      String c = String(char(EEPROM.read(pos++)));
      user_ssid += c;
      Serial.println(c);

    }
    user_network_password = "";
    pos = EEPROM_USER_NETWORK_PASSWORD_START_POS;
    byte user_network_password_len = EEPROM.read(pos++);
    Serial.print("Largo password");
    Serial.println(user_network_password_len);
    for (byte i = 0; i < user_network_password_len; i++)
    {
      String c = String(char(EEPROM.read(pos++)));
      user_network_password += c;
      Serial.println(c);

    }
  }

  // if (device_associated)
  // {
  //   device_user = "";
  //   pos = EEPROM_DEVICE_USER_START_POS;
  //   byte device_user_len = EEPROM.read(pos++);
  //   Serial.print("Largo device user");
  //   Serial.println(device_user_len);
  //   for (byte i = 0; i < device_user_len; i++)
  //   {
  //     String c = String(char(EEPROM.read(pos++)));
  //     device_user += c;
  //     Serial.println(c);

  //   }
  // }
}

void save_network_data(const String &user_ssid, const String &user_network_password) {
  int pos = EEPROM_NETWORK_DATA_PRESENT_START_POS;
  EEPROM.write(pos, true);
  pos = EEPROM_USER_SSID_START_POS;
  byte user_ssid_len = user_ssid.length();
  Serial.println("SSID");
  Serial.println(user_ssid_len);
  EEPROM.write(pos++, user_ssid_len);
  for (byte i = 0; i < user_ssid_len; i++)
  {
    Serial.println(user_ssid[i]);
    EEPROM.write(pos++, user_ssid[i]);

  }
  pos = EEPROM_USER_NETWORK_PASSWORD_START_POS;
  byte user_network_password_len = user_network_password.length();
  Serial.println("PASSWORD");
  Serial.println(user_network_password_len);
  EEPROM.write(pos++, user_network_password_len);
  for (byte i = 0; i < user_network_password_len; i++)
  {
    Serial.println(user_network_password[i]);
    EEPROM.write(pos++, user_network_password[i]);

  }

  EEPROM.commit();
}

void save_device_user() {
  int pos = EEPROM_DEVICE_USER_DATA_PRESENT_START_POS;
  EEPROM.write(pos, true);
  EEPROM.commit();

  device_associated = true;
}

void delete_device_user() {
  int pos = EEPROM_DEVICE_USER_DATA_PRESENT_START_POS;
  Serial.println("Desacociando dispositivo");
  Serial.println("Antes de borrar");
  Serial.println(char(EEPROM.read(pos)));
  Serial.println(char(EEPROM.read(pos+1)));
  EEPROM.write(pos, false);
  Serial.println("Antes de commitear");
  Serial.println(char(EEPROM.read(pos)));
  Serial.println(char(EEPROM.read(pos+1)));
  EEPROM.commit();
  Serial.println("Después de commitear");
  Serial.println(char(EEPROM.read(pos)));
  Serial.println(char(EEPROM.read(pos+1)));
  
  device_associated = false;
}

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
      doc["data"] = doc.createNestedObject();
      for (size_t i = 0; i < 10; i++)
      {
        doc["data"]["Output"] = i + 1;
        if (i > 2 && i < 5) {
          doc["data"]["HourOn"] = "10:30";
          doc["data"]["TimeOn"] = 20;
          doc["data"]["Status"] = true;
          doc["data"]["OperationTime"] = 100;
        } else {
          doc["data"]["HourOn"] = "xx:xx";
          doc["data"]["TimeOn"] = 0;
          doc["data"]["Status"] = false;
          doc["data"]["OperationTime"] = 0;
        }
        doc["data"]["TimeMax"] = 200;
        if (i < 2) {
          doc["data"]["Level"] = 1;
          doc["data"]["Force"] = 1;
          doc["data"]["Control"] = 2;
          doc["data"]["OperationTime"] = 10;
        } else {
          doc["data"]["Level"] = 2;
          doc["data"]["Force"] = 0;
          doc["data"]["Control"] = 1;
        }
        Serial.println(doc.as<String>());
        char response[measureJson(doc) + 1];
        serializeJson(doc, response, measureJson(doc) + 1);
        Serial.println("Response");
        Serial.println(response);
        ws_client.send(response);
      }
      response_needed = false;
      break;
    case RESPONSE_ALL_PARAMETER:
      break;
    case REQUEST_UNLINK:
      doc["msg"] = RESPONSE_UNLINK;
      response_needed = true;
      break;
    case RESPONSE_UNLINK:
    case UNLINK_CONFIRMED_BY_SERVER:
      delete_device_user();
      ws_client.close(CloseReason_None);
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

void server_start() {
  WiFi.softAP(ssid, network_password);             // Start the access point
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

  // server.on(URI_ROOT, HTTP_GET, handle_root);     // Call the 'handle_root' function when a client requests URI "/"
  server.on(URI_DEVICE_STATUS, HTTP_GET, handle_device_status);
  server.on(URI_NETWORK_CONFIG, HTTP_GET, handle_network_config_get);
  server.on(URI_USER_CONFIG, HTTP_GET, handle_user_config_get);
  server.on(URI_NETWORK_CONFIG, HTTP_POST, handle_network_config_post);
  server.on(URI_USER_CONFIG, HTTP_POST, handle_user_config_post);
  server.onNotFound(handle_not_found);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handle_not_found"

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");

  SPIFFS.begin();                           // Start the SPI Flash Files System
}

void wifi_connect(const char *ssid, const char *password) {
  Serial.println("Connecting to ");
  Serial.println(ssid);

  Serial.println("Current");
  Serial.println(WiFi.SSID());
  WiFi.disconnect();
  Serial.println("After disconnect");
  Serial.println(WiFi.SSID());
  //connect to your local wi-fi network
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("");
  // Serial.println("WiFi connected");
}

void websocket_connect() {
  if (!cookie_added)
  {
    ws_client.addHeader("Cookie", session_id);
    cookie_added = true;
  }
  websocket_connected = ws_client.connect(SERVER_URL "/ws/controller/device");
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
  http.begin(client, SERVER_URL "/accounts/login/"); //Specify request destination
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
  } else {
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
  }

  http.end();  //Close connection
}

void handle_root() {                         // When URI / is requested, send a web page with a button to toggle the LED
  server.send(200, "text/html", PAGE_ROOT);
}

void handle_device_status() {
  Serial.println("Device status");
  Serial.println(WiFi.SSID());

  DynamicJsonDocument doc(200);
  doc["connected"] = WiFi.isConnected();
  doc["ssid"] = WiFi.SSID();
  doc["device_associated"] = device_associated;
  // doc["device_user"] = device_user;
  Serial.println(doc["connected"].as<String>());
  Serial.println(doc["ssid"].as<String>());
  Serial.println(doc["device_associated"].as<String>());

  char response[measureJson(doc) + 1];
  serializeJson(doc, response, measureJson(doc) + 1);
  Serial.println("Response");
  Serial.println(response);

  // server.send(200, "application/json", response);
  server.send(200, "application/json", response);
}

void handle_network_config_get() {
  Serial.println("Network get");
  server.send(200, "text/html", PAGE_NETWORK_CONFIG);
}

void handle_network_config_post() {                          // If a POST request is made to URI /submit
  Serial.println("Body");
  Serial.println(server.arg("plain"));
  DynamicJsonDocument doc(800);
  deserializeJson(doc, server.arg("plain"));

  Serial.println(doc["ssid"].as<String>());
  Serial.println(doc["password"].as<String>());
  //connect to your local wi-fi network
  wifi_connect(doc["ssid"], doc["password"]);

  user_network_data = true;
  save_network_data(doc["ssid"], doc["password"]);
  
  digitalWrite(led,!digitalRead(led));      // Change the state of the LED
  // server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  // server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
  server.send(200, "application/json", "{}");
}

void handle_user_config_get() {
  Serial.println("User get");
  server.send(200, "text/html", PAGE_USER_CONFIG);
}

void handle_user_config_post() {
  Serial.println("User config");
  Serial.println(server.arg("plain"));

  HTTPClient http;    //Declare object of class HTTPClient
  
  //Post Data
  http.begin(client, SERVER_URL "/accounts/register/user/"); //Specify request destination
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Cookie", session_id);

  int http_code = http.POST(server.arg("plain"));   //Send the request
  // String payload = http.getString();    //Get the response payload

  Serial.println(http_code);   //Print HTTP return code
  Serial.println(http.getString());

  int status;
  String response;
  if (http_code > 0) {
    status = http_code;
    response = http.getString();
    save_device_user();
  } else {
    status = 400;
    response = "{\"error\":\"Something went wrong.\"}";
  }

  http.end();

  // String user_name;
  // String email;
  // String device_name;
  // String password;
  // for (uint8_t i = 0; i < server.args(); i++) {
  //   if (server.argName(i) == FORM_FIELD_USER_NAME) {
  //     Serial.println(FORM_FIELD_USER_NAME);
  //     user_name = server.arg(i);
  //     Serial.println(user_name);
  //   }
  //   if (server.argName(i) == FORM_FIELD_EMAIL) {
  //     Serial.println(FORM_FIELD_EMAIL);
  //     email = server.arg(i);
  //     Serial.println(email);
  //   }
  //   if (server.argName(i) == FORM_FIELD_DEVICE_NAME) {
  //     Serial.println(FORM_FIELD_DEVICE_NAME);
  //     device_name = server.arg(i);
  //     Serial.println(device_name);
  //   }
  //   if (server.argName(i) == FORM_FIELD_PASSWORD) {
  //     Serial.println(FORM_FIELD_PASSWORD);
  //     password = server.arg(i);
  //     Serial.println(password);
  //   }
  // }

  Serial.println("RESPONSE STATUS AND BODY");
  Serial.println(status);
  Serial.println(response);
  server.send(status, "application/json", response);
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}

void handle_not_found(){
  if (!handleFileRead(server.uri()))
    server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
