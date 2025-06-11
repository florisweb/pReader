#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "connectionManager.h"
#include "config.h"


WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

bool authenticated = false;
bool connectedToWiFi = false;
String jsonString;


unsigned long lastHeartBeat = 0;
void (*onMessagePointer)(DynamicJsonDocument message);
void (*onWiFiConnStateChangePointer)(bool _conned);
void(* rebootESP) (void) = 0; // create a standard reset function


typedef void (*RespondCallbackPointer)(DynamicJsonDocument);
int requestIdList[5];
RespondCallbackPointer requestCallbackList[5];


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
      Serial.printf("[ConnectionManager] Disconnected!\n");
      authenticated = false;
      break;
    case WStype_CONNECTED:
      Serial.printf("[ConnectionManager] Connected to url: %s\n", payload);
      authenticated = false;
      lastHeartBeat = millis();

      // Authentication request
      webSocket.sendTXT("{\"id\":\"" + DeviceId + "\", \"key\": \"" + DeviceKey + "\", \"requestId\": \"" + String(random(0, 10000)) + "\"}");
      break;
    case WStype_TEXT:
      Serial.printf("[ConnectionManager] Get text: %s\n", payload);
      if (authenticated)
      {
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        String type = doc["type"];
        bool isResponse = doc["isResponse"];
        int requestId = doc["requestId"];

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

        if (isResponse)
        {
          for (int i = 0; i < sizeof(requestIdList) / sizeof(int); i++)
          {
            if (requestIdList[i] != requestId) continue;
            requestCallbackList[i](doc);
            requestIdList[i] = 0; // Reset slot
            return;
          }
          Serial.println("[ConnectionManager] Error: response received but no request was send (or at least not found)");
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
          Serial.println("[ConnectionManager] Successfully authenticated.");
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





void connectionManager::setup(void _onMessage(DynamicJsonDocument message), void _onWiFiConnStateChange(bool conned)) {
  pinMode(LED_BUILTIN, OUTPUT);
  onMessagePointer = _onMessage;
  onWiFiConnStateChangePointer = _onWiFiConnStateChange;

  Serial.begin(115200);
  Serial.setDebugOutput(true);

  WiFi.mode(WIFI_STA);
  size_t SSIDCount = sizeof(HTTP_SSIDs) / sizeof(HTTP_SSIDs[0]);
  for (int i = 0; i < SSIDCount; i++) WiFiMulti.addAP(HTTP_SSIDs[i], HTTPpasswords[i]);
 
  attemptToConnect();
  setServerLocation(serverIP, serverPort);
}

void connectionManager::scanSSIDs() {
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  }  else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
    }
  }
}

void connectionManager::attemptToConnect() {
  Serial.print("[ConnectionManager] Started connecting to WiFi ");
  int tries = 0;
  if (connectedToWiFi) onWiFiConnStateChangePointer(false);
  connectedToWiFi = false;

  scanSSIDs();
  while (WiFiMulti.run() != WL_CONNECTED)
  {
    Serial.print(".");
    tries++;
    delay(100);
    if (tries < connAttemptsPerTry) continue;
    Serial.println("-> Failed to connect to WiFi!");
    return;
  }

  connectedToWiFi = true;
  onWiFiConnStateChangePointer(true);
  Serial.println("-> Connected!");
  connectToWebSocket();
}

void connectionManager::setServerLocation(String _ip, int _port) {
  serverIP = _ip;
  serverPort = _port;
  connectToWebSocket();
}

void connectionManager::connectToWebSocket() {
  Serial.print("Connecting to websocket. WiFi connected?: ");
  Serial.println(connectedToWiFi);

  if (!connectedToWiFi) return;
  if (webSocket.isConnected()) {
    Serial.print("Changed serverLoc, was already connected, disconnecting...");
    webSocket.disconnect();
  }

  Serial.print("Connecting to WS server at ");
  Serial.print(serverIP);
  Serial.print(":");
  Serial.println(serverPort);


  webSocket.begin(serverIP, serverPort, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); // try every 5000 again if connection has failed
}


void connectionManager::send(String _string) {
  webSocket.sendTXT(_string);
}

void connectionManager::sendRequest(String _type, String _data, void _onRespond(DynamicJsonDocument message)) {
  int requestId = random(0, 1000000);
  String dataString = "{\"type\": \"";
  dataString.concat(_type);
  dataString.concat("\", \"data\":");
  dataString.concat(_data);
  dataString.concat(", \"requestId\":");
  dataString.concat(requestId);
  dataString.concat("}");

  int curIndex = -1;
  for (int i = 0; i < sizeof(requestIdList) / sizeof(int); i++)
  {
    if (requestIdList[i] != 0) continue;
    curIndex = i;
    break;
  }
  if (curIndex == -1)
  {
    Serial.println("[ConnectionManager] Error: cannot send request, no empty request slots.");
    return;
  }

  Serial.print("Found index:");
  Serial.println(curIndex);

  requestIdList[curIndex] = requestId;
  requestCallbackList[curIndex] = _onRespond;
  Serial.print("Send request: ");
  Serial.println(dataString);
  webSocket.sendTXT(dataString);
}




bool connectionManager::isConnected() {
  return connectedToWiFi;
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
    Serial.println("[ConnectionManager] Disconnected due to 2 missing heartbeats");
    webSocket.disconnect();
  }

  if (deltaHeartbeat > deviceRestartFrequency)
  {
    Serial.println("[ConnectionManager] Restarting device due to lack of connection.");
    rebootESP();
  }

  if (!connectedToWiFi) attemptToConnect();
  webSocket.loop();
}
