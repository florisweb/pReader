#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "connectionManager.h"

WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

String deviceId;
String deviceKey;
bool authenticated = false;
String jsonString;


unsigned long lastHeartBeat = 0;
void (*onMessagePointer)(DynamicJsonDocument message);
void(* rebootESP) (void) = 0; // create a standard reset function


String eventDocs = "[]";
String accessPointDocs = "[]";
void sendDeviceInfo(String _requestId = "") {
  String dataString = "{\"type\": \"deviceInfo\",";
  if (_requestId != "")
  {
    dataString.concat("\"isResponse\": true, \"requestId\": \"");
    dataString.concat(_requestId);
    dataString.concat("\", ");
    dataString.concat("\"response\": {\"events\": ");
  } else {
    dataString.concat("\"data\": {\"events\": ");
  }

  dataString.concat(eventDocs);
  dataString.concat(", \"endPoints\": ");
  dataString.concat(accessPointDocs);
  dataString.concat(", \"connectionManagerVersion\":");
  dataString.concat(connectionManager::version);
  dataString.concat("}}");

  Serial.println(dataString);
  webSocket.sendTXT(dataString);
}



void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      authenticated = false;
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      authenticated = false;
      lastHeartBeat = millis();

      // Authentication request
      webSocket.sendTXT("{\"id\":\"" + deviceId + "\", \"key\": \"" + deviceKey + "\", \"requestId\": \"" + String(random(0, 10000)) + "\"}");
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);
      if (authenticated)
      {
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        String type = doc["type"];

        if (type == "identify")
        {
          digitalWrite(LED_BUILTIN, HIGH);
          delay(400);
          digitalWrite(LED_BUILTIN, LOW);
          return;
        } else if (type == "heartbeat")
        {
          lastHeartBeat = millis();
          return;
        } else if (type == "getDeviceInfo") {
          String requestId = doc["requestId"];
          sendDeviceInfo(requestId);
          return;
        }
        onMessagePointer(doc);
      } else {
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        String type = doc["type"];
        String responseContent = doc["response"];

        if (type == "curState")
        {
          onMessagePointer(doc);
        } else if (responseContent == "{\"type\":\"auth\",\"data\":true}");
        {
          Serial.println("Successfully authenticated.");
          authenticated = true;
          sendDeviceInfo();
        }
      }

      break;
    case WStype_BIN:
      //      hexdump(payload, length);
      // webSocket.sendBIN(payload, length);
      break;
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }

}






void connectionManager::setup(const char* _ssid, const char* _password, const String _deviceId, const String _deviceKey, void _onMessage(DynamicJsonDocument message)) {
  pinMode(LED_BUILTIN, OUTPUT);
  deviceId            = _deviceId;
  deviceKey           = _deviceKey;
  onMessagePointer    = _onMessage;

  Serial.begin(115200);
  Serial.setDebugOutput(true);

  WiFiMulti.addAP(_ssid, _password);
  Serial.print("Started connecting to network: ");
  Serial.println(_ssid);
  while (WiFiMulti.run() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }
  Serial.println("-> Connected!");

  webSocket.begin(serverIP, serverPort, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); // try every 5000 again if connection has failed
}
void connectionManager::send(String _string) {
  webSocket.sendTXT(_string);
}
bool connectionManager::isConnected() {
  return true;
}

bool connectionManager::isAuthenticated() {
  return authenticated;
}


void connectionManager::defineEventDocs(String JSONString) {
  eventDocs = JSONString;
}
void connectionManager::defineAccessPointDocs(String JSONString) {
  accessPointDocs = JSONString;
}




long deltaHeartbeat = 0;
void connectionManager::loop() {
  deltaHeartbeat = millis() - lastHeartBeat;
  if (deltaHeartbeat > heartbeatFrequency * 2 && webSocket.isConnected())
  {
    Serial.println("Disconnected due to 2 missing heartbeats");
    webSocket.disconnect();
  }

  if (deltaHeartbeat > deviceRestartFrequency)
  {
    Serial.println("Restarting device due to lack of connection.");
    rebootESP();
  }


  webSocket.loop();
}
