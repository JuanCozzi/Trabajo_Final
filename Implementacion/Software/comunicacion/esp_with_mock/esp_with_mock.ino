#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>   // Include the WebServer library
#include <ESP8266HTTPClient.h>
#include <FS.h>   // Include the SPIFFS library
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>


/** Namespace. */
using namespace websockets;


/*******************************************************************************
 * Constants
*******************************************************************************/

/** Protocol messages. */

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


/** Webserver URIs. */

#define URI_DEVICE_STATUS "/device/status"
#define URI_NETWORKS_AVAILABLE "/network/available"
#define URI_NETWORK_CONFIG "/network"
#define URI_USER_CONFIG "/user"

/** Remote server URL */

#define REMOTE_SERVER_URL "http://181.171.18.228:8000"
#define REMOTE_SERVER_WEBSOCKET_CHANNEL REMOTE_SERVER_URL "/ws/controller/device"
#define REMOTE_SERVER_AUTHENTICATION_ENDPOINT REMOTE_SERVER_URL "/accounts/login/"
#define REMOTE_SERVER_USER_REGISTRATION_ENDPOINT REMOTE_SERVER_URL "/accounts/register/user/"


/** JSON documents sizes. */

#define NETWORK_CONFIG_JSON_SIZE JSON_OBJECT_SIZE(2) + 130
#define AUTHENTICATION_JSON_SIZE JSON_OBJECT_SIZE(2) + 60
#define SERVER_MSG_JSON_SIZE JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(11) + 130
#define DEVICE_STATUS_JSON_SIZE JSON_OBJECT_SIZE(3) + 110


/** Data length. */

#define USER_NETWORK_SSID_MAX_LEN 50
#define USER_NETWORK_PASSWORD_MAX_LEN 50
#define DEVICE_USER_MAX_LEN 50

/** EEPROM positions. */

#define EEPROM_NETWORK_DATA_PRESENT_START_POS 0
#define EEPROM_DEVICE_USER_DATA_PRESENT_START_POS 1
#define EEPROM_USER_SSID_START_POS EEPROM_DEVICE_USER_DATA_PRESENT_START_POS + 1
#define EEPROM_USER_NETWORK_PASSWORD_START_POS EEPROM_USER_SSID_START_POS + USER_NETWORK_SSID_MAX_LEN + 1
#define EEPROM_DEVICE_USER_START_POS EEPROM_USER_NETWORK_PASSWORD_START_POS + DEVICE_USER_MAX_LEN + 1


/*******************************************************************************
 * Function prototypes
*******************************************************************************/

/** Webserver. */

void server_start();
void handle_device_status();
void handle_get_networks_available();
void handle_network_config_post();
void handle_user_config_post();
void handle_not_found();


/** WiFi network. */

void wifi_connect(const char *ssid, const char *password);


/** Remote server. */

void onEventsCallback(WebsocketsEvent event, String data);
void onMessageCallback(WebsocketsMessage msg);
void websocket_connect();
void authenticate();


/** EEPROM. */

void load_user_data(String &user_ssid, String &user_network_password);
void save_network_data(const String &user_ssid, const String &user_network_password);
void save_device_user();
void delete_device_user();


/** Utils. */

bool get_cookie_value(const String &set_cookie_header, const String &id, String &value);
int get_rssi_as_quality(int rssi);


/*******************************************************************************
 * Globals
*******************************************************************************/

/** Webserver credentials. */

const char *ssid = "ESP8266 Access Point"; // The name of the Wi-Fi network that will be created
const char *network_password = "thereisnospoon";   // The password required to connect to it, leave blank for an open network


/** Device credentials. */

const char *device_id = "pd3";
const char *device_password = "qwertyas";


/** Device session ID obtained when authenticating. */
String session_id;

/** CSRF Token. */
String csrftoken;

/** LED. */
const int led = 2;

/** WiFi client. */
WiFiClient client;

/** WebSocket client. */
WebsocketsClient ws_client;

/** Webserver instance. */
ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80

/** Authentication flag. */
bool authenticated = false;

/** Cookie added to websocket client flag. */
bool cookie_added = false;

/** Websocket connected flag */
bool websocket_connected = false;

/** User networkd configured flag. */
bool user_network_data = false;

/** Device linked to user flag. */
bool device_linked = false;

/** Device user username. */
String device_user;


/*******************************************************************************
 * ESP8266 core functions
*******************************************************************************/

void setup(void) {
  // Start the Serial communication to send messages to the computer
  Serial.begin(115200);
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

  // Configure WebSocket
  ws_client.onEvent(onEventsCallback);
  ws_client.onMessage(onMessageCallback);
  
}

void loop(void){
  // Listen for HTTP requests from clients
  server.handleClient();

  // Check the WiFi is connected
  if (WiFi.isConnected()) {
    // Authenticate in case it's not already authenticated
    if (!authenticated) {
      Serial.println("Authenticating");
      authenticate();
    }

    // Only interact with the remote server WebSocket channel when the device is linked to a user
    if (device_linked) {
      // Connect to the WebSocket channel if needed
      if (!websocket_connected && authenticated) {
        Serial.println("Trying to reconnect");
        websocket_connect();    
      }

      // Poll the WebSocket client if it's available
      if (ws_client.available())
        ws_client.poll();
      else
        Serial.println("Websocket unavailable");
      
      // Send the message received in the serial port to the remote server WebSocket channel
      if (Serial.available()) {
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


/*******************************************************************************
 * Function definitions
*******************************************************************************/

/**
 * @brief Loads the user data from the EEPROM (i.e. whether there is a WiFi network configured or the device is
 * linked to a user, the WiFi network SSID and password.)
 * 
 * @param user_ssid WiFi network SSID (output).
 * @param user_network_password WiFi network password (output).
 */
void load_user_data(String &user_ssid, String &user_network_password) {
  // Load whether there is a WiFi network configured
  int pos = EEPROM_NETWORK_DATA_PRESENT_START_POS;
  user_network_data = EEPROM.read(pos++);
  Serial.print("user_network_data ");
  Serial.println(user_network_data);

  // Load whether the device is linked to a user
  device_linked = EEPROM.read(pos++);
  Serial.print("device_linked ");
  Serial.println(device_linked);

  // Load the WiFi network SSID and password if there was one configured
  if (user_network_data) {
    user_ssid = "";
    pos = EEPROM_USER_SSID_START_POS;
    byte user_ssid_len = EEPROM.read(pos++);
    Serial.print("Largo ssid");
    Serial.println(user_ssid_len);
    for (byte i = 0; i < user_ssid_len; i++)
      user_ssid += String(char(EEPROM.read(pos++)));

    user_network_password = "";
    pos = EEPROM_USER_NETWORK_PASSWORD_START_POS;
    byte user_network_password_len = EEPROM.read(pos++);
    Serial.print("Largo password");
    Serial.println(user_network_password_len);
    for (byte i = 0; i < user_network_password_len; i++)
      user_network_password += String(char(EEPROM.read(pos++)));
  }

  // if (device_linked)
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


/**
 * @brief Saves the user WiFi network data to the EEPROM.
 * 
 * @param user_ssid WiFi network SSID.
 * @param user_network_password WiFi network password.
 */
void save_network_data(const String &user_ssid, const String &user_network_password) {
  // Save the WiFi network configured flag
  int pos = EEPROM_NETWORK_DATA_PRESENT_START_POS;
  EEPROM.write(pos, true);

  // Save the SSID
  pos = EEPROM_USER_SSID_START_POS;
  byte user_ssid_len = user_ssid.length();
  Serial.println("SSID");
  Serial.println(user_ssid_len);
  EEPROM.write(pos++, user_ssid_len);
  for (byte i = 0; i < user_ssid_len; i++)
    EEPROM.write(pos++, user_ssid[i]);

  // Save the password
  pos = EEPROM_USER_NETWORK_PASSWORD_START_POS;
  byte user_network_password_len = user_network_password.length();
  Serial.println("PASSWORD");
  Serial.println(user_network_password_len);
  EEPROM.write(pos++, user_network_password_len);
  for (byte i = 0; i < user_network_password_len; i++)
    EEPROM.write(pos++, user_network_password[i]);

  // Commit the changes to EEPROM
  EEPROM.commit();
}


/**
 * @brief Sets the flag that indicates the device is linked to a user in the EEPROM.
 * 
 */
void save_device_user() {
  int pos = EEPROM_DEVICE_USER_DATA_PRESENT_START_POS;
  EEPROM.write(pos, true);
  EEPROM.commit();

  device_linked = true;
}


/**
 * @brief Unsets the flag that indicates the device is linked to a user in the EEPROM.
 * 
 */
void delete_device_user() {
  int pos = EEPROM_DEVICE_USER_DATA_PRESENT_START_POS;
  EEPROM.write(pos, false);
  EEPROM.commit();
  
  device_linked = false;
}


/**
 * @brief Gets the cookie value from the 'Set-Cookie' header.
 * 
 * @param set_cookie_header 'Set-Cookie' header.
 * @param id Cookie ID.
 * @param value Cookie value.
 * @return true on success, false otherwise.
 */
bool get_cookie_value(const String &set_cookie_header, const String &id, String &value) {
  int id_start = set_cookie_header.indexOf(id);
  if (id_start == -1)
    return false;
    
  int value_end = set_cookie_header.indexOf(";", id_start) + 1;
  if (value_end == -1)
    return false;
    
  value = set_cookie_header.substring(id_start, value_end);

  return true;
}


/**
 * @brief Callback for WebSocket client events.
 * 
 * @param event Event (ConnectionOpened, ConnectionClosed, GotPing, GotPong).
 * @param data Data associated to the event.
 */
void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        // Set the WebSocket connected flag to true.
        websocket_connected = true;
        Serial.println("Connnection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        // Set the WebSocket connected flag to false.
        websocket_connected = false;
        Serial.println("Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
    }
}


/**
 * @brief Callback for the messages received by the WebSocket client.
 * 
 * @param msg Message received.
 */
void onMessageCallback(WebsocketsMessage msg){
  Serial.println("Received");
  Serial.println(msg.data());
  StaticJsonDocument<SERVER_MSG_JSON_SIZE> doc;
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
        serializeJson(doc, response, sizeof(response));
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
      // Delete the user linking data and close the WebSocket connection channel
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
    serializeJson(doc, response, sizeof(response));
    Serial.println("Response");
    Serial.println(response);
    ws_client.send(response);
  }
}


/**
 * @brief Starts the webserver.
 * 
 */
void server_start() {
  // Start the access point
  WiFi.softAP(ssid, network_password);
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started");

  Serial.print("IP address:\t");
  // Send the IP address of the ESP8266 to the computer
  Serial.println(WiFi.softAPIP());

  // Start the mDNS responder for esp8266.local
  if (MDNS.begin("esp8266")) {
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  // Set the functions associated with the corresponding webserver endpoints and HTTP methods
  server.on(URI_DEVICE_STATUS, HTTP_GET, handle_device_status);
  server.on(URI_NETWORKS_AVAILABLE, HTTP_GET, handle_get_networks_available);
  server.on(URI_NETWORK_CONFIG, HTTP_POST, handle_network_config_post);
  server.on(URI_USER_CONFIG, HTTP_POST, handle_user_config_post);
  // When a client requests an unknown URI, call function "handle_not_found"
  server.onNotFound(handle_not_found);

  // Actually start the server
  server.begin();
  Serial.println("HTTP server started");

  // Start the SPI Flash Files System
  SPIFFS.begin();
}


/**
 * @brief Connect to the given WiFi network.
 * 
 * @param ssid WiFi network SSID.
 * @param password WiFi network password.
 */
void wifi_connect(const char *ssid, const char *password) {
  Serial.println("Connecting to ");
  Serial.println(ssid);

  Serial.println("Current");
  Serial.println(WiFi.SSID());
  WiFi.disconnect();
  Serial.println("After disconnect");
  Serial.println(WiFi.SSID());

  // Connect to the WiFi network
  WiFi.begin(ssid, password);
}


/**
 * @brief Connect to the remote server WebSocket channel.
 * 
 */
void websocket_connect() {
  // Add the cookie only if it wasn't already added.
  if (!cookie_added)
  {
    ws_client.addHeader("Cookie", session_id);
    cookie_added = true;
  }

  websocket_connected = ws_client.connect(REMOTE_SERVER_WEBSOCKET_CHANNEL);
}


/**
 * @brief Authenticate with the remote server providing the device credentials.
 * 
 */
void authenticate() {
  // Create JSON body with credentials
  StaticJsonDocument<AUTHENTICATION_JSON_SIZE> doc;

  doc["username"] = device_id;
  doc["password"] = device_password;

  Serial.println(measureJson(doc));

  char authentication_body[measureJson(doc) + 1];
  serializeJson(doc, authentication_body, sizeof(authentication_body));
  Serial.println(authentication_body);

  HTTPClient http;
  
  // Headers to collect
  const char *header_keys[] = {"Set-Cookie"};
  const size_t header_keys_num = 1;
  
  // Make authentication request
  http.begin(client, REMOTE_SERVER_AUTHENTICATION_ENDPOINT);
  http.collectHeaders(header_keys, header_keys_num);
  http.addHeader("Content-Type", "application/json");

  int http_code = http.POST(authentication_body);

  // Get the response payload
  String payload = http.getString();
  Serial.println(http_code);
  Serial.println(payload);
  Serial.println(payload.length());

  // Check the request was successful
  if (http_code == 200) {
    String set_cookie_header = http.header("Set-Cookie");
    Serial.println(set_cookie_header);

    if (get_cookie_value(set_cookie_header, String("sessionid"), session_id)) {
      Serial.println("Cookie parsed");
      Serial.println(session_id);
      authenticated = true;
    } else {
      authenticated = false;
      Serial.println("Error parsing sessionid cookie");
    }
  } else {
    authenticated = false;
    Serial.print("Error authenticating ");
    Serial.println(http_code);
  }

  // Close connection
  http.end();
}


/**
 * @brief Returns the device status, i.e. whether the device is connected to a WiFi network, the
 * WiFi network SSID and whether the device is linked to a user.
 * 
 */
void handle_device_status() {
  Serial.println("Device status");
  Serial.println(WiFi.SSID());

  StaticJsonDocument<DEVICE_STATUS_JSON_SIZE> doc;

  doc["wifi_connected"] = WiFi.isConnected();
  doc["ssid"] = WiFi.SSID();
  doc["device_linked"] = device_linked;
  // doc["device_user"] = device_user;

  Serial.println(doc["connected"].as<String>());
  Serial.println(doc["ssid"].as<String>());
  Serial.println(doc["device_linked"].as<String>());

  char response[measureJson(doc) + 1];
  serializeJson(doc, response, sizeof(response));

  Serial.println("Response");
  Serial.println(response);

  server.send(200, "application/json", response);
}


/**
 * @brief Transforms the RSSI into quality.
 * 
 * @param rssi WiFi network RSSI.
 * @return int Quality.
 */
int get_rssi_as_quality(int rssi) {
  int quality = 0;

  if (rssi <= -100) {
    quality = 0;
  } else if (rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * (rssi + 100);
  }

  return quality;
}


/**
 * @brief Searches for available networks.
 * 
 */
void handle_get_networks_available() {
  Serial.println("Networks available");

  DynamicJsonDocument doc(2048);

  JsonArray networks = doc.createNestedArray("networks");

  int8_t n = WiFi.scanNetworks();

  for (size_t i = 0; i < n; i++) {
    Serial.println(WiFi.SSID(i));
    JsonObject network = networks.createNestedObject();

    network["ssid"] = WiFi.SSID(i);
    network["quality"] = get_rssi_as_quality(WiFi.RSSI(i));
    network["encrypted"] = WiFi.encryptionType(i) != ENC_TYPE_NONE;
  }

  WiFi.scanDelete();

  char response[measureJson(doc) + 1];
  serializeJson(doc, response, sizeof(response));

  Serial.println(response);
  
  server.send(200, "application/json", response);
}


/**
 * @brief Connects to the given WiFi network and saves its credentials to the EEPROM.
 * 
 */
void handle_network_config_post() {
  Serial.println("Body");
  Serial.println(server.arg("plain"));

  StaticJsonDocument<NETWORK_CONFIG_JSON_SIZE> doc;

  deserializeJson(doc, server.arg("plain"));

  Serial.println(doc["ssid"].as<String>());
  Serial.println(doc["password"].as<String>());

  // Set the user network data flag
  user_network_data = true;

  // Save the WiFi network credentials
  save_network_data(doc["ssid"], doc["password"]);
  
  // Connect to the given WiFi network
  wifi_connect(doc["ssid"], doc["password"]);

  // Change the state of the LED
  digitalWrite(led, !digitalRead(led));

  server.send(200, "application/json", "{}");
}


/**
 * @brief Links the device with the given user.
 * 
 */
void handle_user_config_post() {
  Serial.println("User config");
  Serial.println(server.arg("plain"));

  HTTPClient http;
  
  // Make link request
  http.begin(client, REMOTE_SERVER_USER_REGISTRATION_ENDPOINT);
  http.addHeader("Content-Type", "application/json");
  // Provide the session ID
  http.addHeader("Cookie", session_id);

  int http_code = http.POST(server.arg("plain"));

  Serial.println(http_code);
  Serial.println(http.getString());

  int status;
  String response;

  // Check the request was correctly sent
  if (http_code > 0) {
    status = http_code;
    response = http.getString();
    // If the request was successful, set the device as linked with a user
    if (http_code == 200)
      save_device_user();
  } else {
    status = 400;
    response = "{\"error\":\"Something went wrong.\"}";
  }

  // Close connection
  http.end();

  Serial.println("RESPONSE STATUS AND BODY");
  Serial.println(status);
  Serial.println(response);
  
  server.send(status, "application/json", response);
}


/**
 * @brief Converts the file extension to the MIME type.
 * 
 * @param filename File name.
 * @return String Content type.
 */
String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}


/**
 * @brief Sends the right file to the client (if it exists).
 * 
 * @param path Requested path.
 * @return true on success, false otherwise.
 */
bool handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);
  
  // If a folder is requested, send the index file
  if (path.endsWith("/")) path += "index.html";

  // Get the MIME type
  String contentType = getContentType(path);

  // If the file exists
  if (SPIFFS.exists(path)) {
    // Open it
    File file = SPIFFS.open(path, "r");
    // And send it to the client
    size_t sent = server.streamFile(file, contentType);
    // Then close the file again
    file.close();
    return true;
  }

  Serial.println("\tFile Not Found");
  
  // If the file doesn't exist, return false
  return false;
}


/**
 * @brief Handles requests to unknown URIs.
 * 
 */
void handle_not_found(){
  if (!handleFileRead(server.uri()))
    server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
