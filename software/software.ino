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



void displayImage(uint8_t img[], int width, int arrLength) {
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE); // Clear the display

    display.drawBitmap(0, 0, img, width, arrLength * 8 / width, GxEPD_BLACK); // Draw the image
  }
  while (display.nextPage());
}






int remoteImageLength = 0;
int imageWidth = 0;
int curImageBufferIndex = 0;
const int imageSectionLength = 15000;
uint8_t imageOverlapBuffer[imageSectionLength]; // Max buffer length is screen width
int overlapBufferLength = 0;
int summedYPos = 0;
const int displayWidth = 680;



void onImageSectionResponse(DynamicJsonDocument message) {
  int startIndex = message["response"]["startIndex"];
  String imageData = message["response"]["data"];
  curImageBufferIndex = startIndex + imageSectionLength;
  Serial.print("Got section:");
  Serial.print(startIndex);
  Serial.print(" - ");
  Serial.println(curImageBufferIndex);


  if (curImageBufferIndex < remoteImageLength)
  {
    ConnectionManager.sendRequest(
      String("getImageSection"),
      String("{\"startIndex\":" + String(curImageBufferIndex) + ", \"sectionLength\":" + String(imageSectionLength) + "}"),
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

  //  display.writeImage(fullSection, 0, yStart, imageWidth, rows, true);
  //  if (yStart < displayWidth && summedYPos > displayWidth) display.display(true);

  display.drawImage(fullSection, 0, yStart, imageWidth, rows, true);
}






void onMessage(DynamicJsonDocument message) {
  String packetType = message["type"];
  String error = message["error"];

  Serial.print("[OnMessage] Error: ");
  Serial.println(error);
  Serial.print("[OnMessage] type: ");
  Serial.println(packetType);

  if (packetType == "setText")
  {
    const int imageWidth = message["data"]["imgWidth"];
    Serial.print("[OnMessage] width: ");
    Serial.println(imageWidth);
    String imgData = message["data"]["base64"];
    const char * text = imgData.c_str();

    size_t outputLength;
    uint8_t* decoded = base64_decode((const unsigned char *)text, strlen(text), &outputLength);
    Serial.print("Length of decoded message: ");
    Serial.println(outputLength);

    Serial.printf("%.*s", outputLength, decoded);
    for (int i = 0; i < outputLength; i++)
    {
      Serial.println(decoded[i]);
    }
    displayImage(decoded, imageWidth, outputLength);

  } else if (packetType == "imageAvailable")
  {
    remoteImageLength = message["data"]["dataLength"];
    imageWidth = message["data"]["imageWidth"];
    curImageBufferIndex = 0;
    summedYPos = 0;
    overlapBufferLength = 0;

    Serial.print("Got imageAvailable message. DataLength: ");
    Serial.print(remoteImageLength);
    Serial.print(" image width: ");
    Serial.println(imageWidth);
    ConnectionManager.sendRequest(
      String("getImageSection"),
      String("{\"startIndex\":" + String(curImageBufferIndex) + ", \"sectionLength\":" + String(imageSectionLength) + "}"),
      &onImageSectionResponse
    );
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
    }
  }
}
