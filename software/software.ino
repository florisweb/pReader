#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "connectionManager.h"


const char* ssid = "";
const char* password = "";
const String deviceId = "";
const String deviceKey = "";

connectionManager ConnectionManager;

const int displayResetPin = 16;
GxEPD2_BW < GxEPD2_1330_GDEM133T91, GxEPD2_1330_GDEM133T91::HEIGHT / 2 >
display(GxEPD2_1330_GDEM133T91(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ displayResetPin, /*BUSY=*/ 4)); // GDEM133T91 960x680, SSD1677, (FPC-7701 REV.B)

String HelloWorld = "pReader 3";
void onMessage(DynamicJsonDocument message) {
  String packetType = message["type"];
  String error = message["error"];

  Serial.print("[OnMessage] Error: ");
  Serial.println(error);
  Serial.print("[OnMessage] type: ");
  Serial.println(packetType);

  if (packetType == "setText")
  {
    String text = message["data"];
    HelloWorld = text;
    helloWorld();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(displayResetPin, OUTPUT);
  display.init(); // Initialize the display
  helloWorld();

  Serial.println("Setting up WiFi...");

  ConnectionManager.defineEventDocs("[]");
  ConnectionManager.defineAccessPointDocs("["
                                          "{"
                                          "\"type\": \"setText\","
                                          "\"description\": \"Sets text\""
                                          "}"
                                          "]");
  ConnectionManager.setup(ssid, password, deviceId, deviceKey, &onMessage);
}


void loop() {
  ConnectionManager.loop();
}



void helloWorld()
{
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.fillRect(x - 7, y - 5 - tbh/2, tbw + 14, tbh + 10, 0);
    display.fillRect(x - 5, y - 3 - tbh/2, tbw + 10, tbh + 6, 1);

    display.setCursor(x, y);
    display.print(HelloWorld);


  }
  while (display.nextPage());
}
