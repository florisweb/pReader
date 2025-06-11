#ifndef connectionManager_h
#define connectionManager_h
#include <WString.h>
#include <ArduinoJson.h>



class connectionManager
{    
  public:
    constexpr static float version = 1.8;
    
    void setup(void _onMessage(DynamicJsonDocument message), void _onWiFiConnStateChange(bool conned));
    void loop();
    void setServerLocation(String _ip, int _port);
    void defineEventDocs(String JSONString);
    void defineAccessPointDocs(String JSONString);
    void send(String _string);
    void sendRequest(String _type, String _data, void _onRespond(DynamicJsonDocument message));
    bool isConnected();
    bool isAuthenticated();
    void scanSSIDs();
  private:
    void connectToWebSocket();
    String serverIP = "192.168.0.158";
    int serverPort = 8081;
    int heartbeatFrequency = 10000; // ms
    int deviceRestartFrequency = 15 * 60 * 1000; // ms
    int connAttemptsPerTry = 200;
    void attemptToConnect();
};

#endif
