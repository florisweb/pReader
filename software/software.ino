#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "connectionManager.h"
extern "C" {
#include "crypto/base64.h"
}
#include <Arduino.h>


const char* ssid = "";
const char* password = "";
const String deviceId = "";
const String deviceKey = "";

connectionManager ConnectionManager;

const int displayResetPin = 16;
GxEPD2_BW < GxEPD2_1330_GDEM133T91, GxEPD2_1330_GDEM133T91::HEIGHT / 2 > // /2 makes it two pages
display(GxEPD2_1330_GDEM133T91(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ displayResetPin, /*BUSY=*/ 4)); // GDEM133T91 960x680, SSD1677, (FPC-7701 REV.B)




enum UIPage {
  HOME,
  MUSIC
};
int musicPage_curMusicIndex = 0;
int musicPage_curPageIndex = 0;
UIPage curPage = HOME;

int availableMusicCount = 0;
String availableMusic_names[10];
int availableMusic_pageCount[10];




int musicImageRequestId = 0;
int remoteImageLength = 0;
int imageWidth = 0;
int curImageBufferIndex = 0;
const int imageSectionLength = 15000;
uint8_t imageOverlapBuffer[imageSectionLength]; // Max buffer length is screen width
int overlapBufferLength = 0;
int summedYPos = 0;
const int displayWidth = 680;


void onMusicImageRequestResponse(DynamicJsonDocument message) {
  String error = message["response"]["error"];
  if (error && error != "null")
  {
    Serial.print("Error on requesting image:" );
    Serial.println(error);
    return;
  }

  remoteImageLength = message["response"]["dataLength"];
  imageWidth = message["response"]["imageWidth"];
  curImageBufferIndex = 0;
  summedYPos = 0;
  overlapBufferLength = 0;
  musicImageRequestId = message["response"]["imageRequestId"];

  Serial.print("Got imageAvailable message. DataLength: ");
  Serial.print(remoteImageLength);
  Serial.print(" image width: ");
  Serial.println(imageWidth);
  ConnectionManager.sendRequest(
    String("getImageSection"),
    String("{\"startIndex\":" + String(curImageBufferIndex) + ", \"sectionLength\":" + String(imageSectionLength) + ", \"imageRequestId\": " + String(musicImageRequestId) + "}"),
    &onImageSectionResponse
  );
}
void onImageSectionResponse(DynamicJsonDocument message) {
  String error = message["response"]["error"];
  if (error && error != "null")
  {
    Serial.print("Error on requesting image section:" );
    Serial.println(error);
    return;
  }

  int startIndex = message["response"]["startIndex"];
  String imageData = message["response"]["data"];
  curImageBufferIndex = startIndex + imageSectionLength;
  //  Serial.print("Got section:");
  //  Serial.print(startIndex);
  //  Serial.print(" - ");
  //  Serial.println(curImageBufferIndex);

  if (curImageBufferIndex < remoteImageLength)
  {
    ConnectionManager.sendRequest(
      String("getImageSection"),
      String("{\"startIndex\":" + String(curImageBufferIndex) + ", \"sectionLength\":" + String(imageSectionLength) + ", \"imageRequestId\": " + String(musicImageRequestId) + "}"),
      &onImageSectionResponse
    );
  } else Serial.println("Finished fetching all parts.");

  const char * text = imageData.c_str();
  size_t outputLength;
  uint8_t* decoded = base64_decode((const unsigned char *)text, strlen(text), &outputLength);

  int maxSectionLength = overlapBufferLength + outputLength;
  int rows = floor(maxSectionLength * 8 / imageWidth);
  int fullSectionLength = rows * imageWidth / 8;

  uint8_t* fullSection = imageOverlapBuffer;
  for (int i = 0; i < fullSectionLength - overlapBufferLength; i++)
  {
    fullSection[overlapBufferLength + i] = decoded[i];
  }

  free(decoded);
  //  Serial.print("convert data: ");
  //  Serial.println(millis() - start);
  //
  //  Serial.print("input length: ");
  //  Serial.print(outputLength);
  //  Serial.print(" maxSectionLength: ");
  //  Serial.print(maxSectionLength);
  //  Serial.print(" rows: ");
  //  Serial.print(rows);
  //  Serial.print(" fullSectionLength: ");
  //  Serial.print(fullSectionLength);
  //  Serial.print(" pre-overlapBufferLength: ");
  //  Serial.print(overlapBufferLength);

  int yStart = summedYPos;
  summedYPos += rows;
  overlapBufferLength = maxSectionLength - fullSectionLength;
  //  Serial.print(" post-overlapBufferLength: ");
  //  Serial.println(overlapBufferLength);

  for (int i = 0; i < overlapBufferLength; i++)
  {
    imageOverlapBuffer[i] = decoded[outputLength - overlapBufferLength + i];
  }

  display.drawImage(fullSection, 0, yStart, imageWidth, rows, true);
}






void onMessage(DynamicJsonDocument message) {
  String packetType = message["type"];
  String error = message["error"];


  Serial.print("[OnMessage] Error: ");
  Serial.println(error);
  Serial.print("[OnMessage] type: ");
  Serial.println(packetType);

  if (packetType == "curState")
  {
    availableMusicCount = 0;
    for (int i = 0; i < sizeof(availableMusic_pageCount) / sizeof(int); i++)
    {
      availableMusic_pageCount[i] = message["data"]["availableMusic"][i]["pages"];
      if (availableMusic_pageCount[i] == 0) break;
      const char* musicName = message["data"]["availableMusic"][i]["name"].as<const char*>();
      availableMusic_names[i] = String(musicName);
      availableMusicCount = i;

      Serial.print("Music available: ");
      Serial.print(availableMusic_names[i]);
      Serial.print(" (");
      Serial.print(availableMusic_pageCount[i]);
      Serial.println(")");
    }
  }
}





void onResponse(DynamicJsonDocument message) {
  String response = message["response"];
  Serial.print("Got message!, response: ");
  Serial.println(response);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Setting up display...");
  pinMode(displayResetPin, OUTPUT);
  display.init();
  display.setRotation(1);
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);

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

  if (Serial.available())
  {
    String ch = Serial.readStringUntil('\n'); // Read until newline
    Serial.println(ch);
    if (ch == "r") {
      Serial.println("rot 0");
      display.setRotation(0);
    } else if (ch == "R") {
      display.setRotation(1);
    } else if (ch == "d") {
      display.display();
    } else if (ch == "D") {
      display.display(true);
    } else if (ch == "f") {
      display.firstPage();
    } else if (ch == "n") {
      display.nextPage();
    } else if (ch == "c") {
      display.fillScreen(GxEPD_WHITE);
    } else if (ch == "m") {
      openPage(MUSIC);
    } else if (ch == "p0") {
      musicPage_curPageIndex = 0;
    } else if (ch == "p1") {
      musicPage_curPageIndex = 1;
    } else if (ch == "p2") {
      musicPage_curPageIndex = 2;
    } 
  }
}


void openMusicPage(int _curMusicIndex) {
  musicPage_curMusicIndex = min(_curMusicIndex, availableMusicCount);
  openPage(MUSIC);
}
void loadMusicImage() {
  ConnectionManager.sendRequest(
    String("requestMusicImage"),
    String("{\"musicName\":\"" + String(availableMusic_names[musicPage_curMusicIndex]) + "\", \"pageIndex\":" + String(musicPage_curPageIndex) + "}"),
    &onMusicImageRequestResponse
  );
}



void openPage(UIPage page) {
  if (page == HOME)
  {
    Serial.println("open HOME");

  } else if (page == MUSIC)
  {
    Serial.println("open MUSIC");
    loadMusicImage();
  }

}
