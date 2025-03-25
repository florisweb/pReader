#ifndef connectionManager_h
#define connectionManager_h
#include <WString.h>
#include <ArduinoJson.h>

class connectionManager
{    
  public:
    constexpr static float version = 1.4;
    
    void setup(const char* _ssid, const char* _password, const String _deviceId, const String _deviceKey, void _onMessage(DynamicJsonDocument message));
    void loop();
    void defineEventDocs(String JSONString);
    void defineAccessPointDocs(String JSONString);
    void send(String _string);
    bool isConnected();
    bool isAuthenticated();
  private:
    String serverIP = "192.168.0.158";
    int serverPort = 8081;
    int heartbeatFrequency = 10000; // ms
    int deviceRestartFrequency = 15 * 60 * 1000; // ms
};

#endif
