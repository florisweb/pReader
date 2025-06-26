#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBoldOblique18pt7b.h>
#include <Fonts/FreeSansOblique9pt7b.h>

#include <array>
#include "connectionManager.h"
extern "C" {
#include "crypto/base64.h"
}
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "images.h"


const int thumbnailWidth = 184;
const int thumbnailHeight = 130;


connectionManager ConnectionManager;

// ===== PIN DEFINITIONS =====
const int nextButtonPin = 27;
const int prevButtonPin = 21;
const int okButtonPin = 15;
const int pedalNextPin = 33;
const int pedalPrevPin = 25;
const int BlueLEDPin = 2;

const int SSDCS = 26; // SPI chip select

// Display
const int DisplayCS = 17; // SPI chip select
const int displayResetPin = 16;




void(* _rebootESP) (void) = 0; // create a standard reset function

enum UIPage {
  HOME,
  MUSIC,
  UPDATING_CATALOGUE
};

int musicPage_curMusicIndex = 0;
int musicPage_curPageIndex = 0;
UIPage curPage = HOME;
UIPage prevPage = HOME;

const String learningStates[] = {"Learning", "Wishlist", "On Hold", "Finished", "Canceled"};
int availableMusicCount = 0;
String availableMusic_name[20];
int availableMusic_itemId[20];
int availableMusic_pageCount[20];
int availableMusic_musicImageVersion[20];
int availableMusic_learningState[20];




#include "dataManager.h"

GxEPD2_BW < GxEPD2_1330_GDEM133T91, GxEPD2_1330_GDEM133T91::HEIGHT / 2 > // /2 makes it two pages
display(GxEPD2_1330_GDEM133T91(/*CS=*/ DisplayCS, /*DC=*/ 5, /*RST=*/ displayResetPin, /*BUSY=*/ 4)); // GDEM133T91 960x680, SSD1677, (FPC-7701 REV.B)

#include "drawLib.h" // Include AFTER display-definition

#include "updatePage.h"
#include "homePage.h"
#include "musicPage.h"





// ===== Event Handlers =====

void onWiFiConnStateChange(bool conned) {
  Serial.print("Connected?: ");
  Serial.println(conned);
  updateConnectionHeaderIcon();
}

bool openMusicPageOnDownloadFinish = false;
void onDownloadFinish() {
  Serial.println("Finished downloading!");
  if (openMusicPageOnDownloadFinish)
  {
    openMusicPageOnDownloadFinish = false;
    openPage(MUSIC);
  }
  downloadUndownloadedItems(&onDownloadFinish, &openPage);
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
    bool changed = importMusicItemSet(message["data"]["availableMusic"]);
    Serial.print("Got new config, changed: ");
    Serial.println(changed);
    writeConfigFile();
    downloadUndownloadedItems(&onDownloadFinish,&openPage);
  }
}

void onResponse(DynamicJsonDocument message) {
  String response = message["response"];
  Serial.print("Got message!, response: ");
  Serial.println(response);
}






void setup() {
  Serial.begin(115200);

  Serial.println("========================================================================================================================================================================================================================================================================");
  delay(100);
  Serial.println("");
  Serial.println("Setting up display...");

  pinMode(displayResetPin, OUTPUT);
  display.init();
  display.setRotation(1);
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);

  Serial.println("Setting up buttons and pedal...");
  pinMode(nextButtonPin, INPUT);
  pinMode(prevButtonPin, INPUT);
  pinMode(okButtonPin, INPUT);
  pinMode(pedalNextPin, INPUT);
  pinMode(pedalPrevPin, INPUT);

  pinMode(BlueLEDPin, OUTPUT);

  setupSD();
  loadConfigFile();
  openPage(HOME);

  Serial.println("Setting up WiFi...");
  delay(200);

  ConnectionManager.defineEventDocs("[]");
  ConnectionManager.defineAccessPointDocs("["
                                          "{"
                                          "\"type\": \"setText\","
                                          "\"description\": \"Sets text\""
                                          "}"
                                          "]");
  ConnectionManager.setup(&onMessage, &onWiFiConnStateChange);
  ConnectionManager.setServerLocation("206.83.41.24", 8081);
}


bool prevNextButtonState = false;
bool prevPrevButtonState = false;
bool prevOkButtonState = false;
bool prevPedalNextState = false;
bool prevPedalPrevState = false;
void loop() {
  ConnectionManager.loop();

  bool nextButtonState = digitalRead(nextButtonPin);
  bool prevButtonState = digitalRead(prevButtonPin);
  bool okButtonState = digitalRead(okButtonPin);

  bool pedalNextState = analogRead(pedalNextPin) > 3500;
  bool pedalPrevState = analogRead(pedalPrevPin) > 3500;

  int pedalNextPinVal = analogRead(pedalNextPin);
  int pedalPrevPinVal = analogRead(pedalPrevPin);

  if (curImageBufferIndex >= remoteImageLength) {
    // Not Downloading an image
    if (
      (nextButtonState && nextButtonState != prevNextButtonState) ||
      (pedalNextState && pedalNextState != prevPedalNextState)
    ) {
      digitalWrite(BlueLEDPin, HIGH);
      next();
      digitalWrite(BlueLEDPin, LOW);
    } else if (
      (prevButtonState && prevButtonState != prevPrevButtonState) ||
      (pedalPrevState && pedalPrevState != prevPedalPrevState)
    ) {
      digitalWrite(BlueLEDPin, HIGH);
      prev();
      digitalWrite(BlueLEDPin, LOW);
    } else if (okButtonState && okButtonState != prevOkButtonState)
    {
      digitalWrite(BlueLEDPin, HIGH);
      ok();
      digitalWrite(BlueLEDPin, LOW);
    }
  }

  prevNextButtonState = nextButtonState;
  prevPrevButtonState = prevButtonState;
  prevOkButtonState = okButtonState;
  prevPedalNextState = pedalNextState;
  prevPedalPrevState = pedalPrevState;


  if (Serial.available())
  {
    String ch = Serial.readStringUntil('\n'); // Read until newline
    Serial.println(ch);
    if (ch == "mem") {
      Serial.print("Free mem: ");
      Serial.println(ESP.getFreeHeap());
    } else if (ch == "reboot") {
      _rebootESP();
    }
  }
}


// ===== CONTROLS =====
void next() {
  if (curPage == HOME)
  {
    if (availableMusicCount == 0) return; // No point in moving - prevent a loop
    std::array<int, 2> curPos = getHomePagePanelPos(musicPage_curMusicIndex);
    Serial.print("CurPos: ");
    Serial.print(curPos[0]);
    Serial.print(" - ");
    Serial.println(curPos[1]);
    curPos[0]++;
    int newIndex = getHomePagePanelMusicIndexAtPos(curPos[0], curPos[1]);
    while (newIndex == -1)
    {
      curPos[1]++;
      if (curPos[1] > 2) curPos[1] = 0;
      curPos[0] = 0;
      newIndex = getHomePagePanelMusicIndexAtPos(curPos[0], curPos[1]);
    }
    Serial.print("NewPos: ");
    Serial.print(curPos[0]);
    Serial.print(" - ");
    Serial.println(curPos[1]);

    homePage_selectMusicItem(newIndex);
  } else if (curPage == MUSIC) {
    musicPage_curPageIndex++;
    if (musicPage_curPageIndex >= availableMusic_pageCount[musicPage_curMusicIndex])
    {
      musicPage_curPageIndex = 0;
    }
    Serial.print("new pageIndex: ");
    Serial.println(musicPage_curPageIndex);
    openPage(MUSIC);
  }
}

void prev() {
  if (curPage == HOME)
  {
    if (availableMusicCount == 0) return; // No point in moving - prevent a loop
    std::array<int, 2> curPos = getHomePagePanelPos(musicPage_curMusicIndex);
    Serial.print("CurPos: ");
    Serial.print(curPos[0]);
    Serial.print(" - ");
    Serial.println(curPos[1]);

    curPos[0]--;
    int newIndex = getHomePagePanelMusicIndexAtPos(curPos[0], curPos[1]);
    while (newIndex == -1)
    {
      curPos[1]--;
      if (curPos[1] < 0) curPos[1] = 2;
      curPos[0] = getHomePagePanelCountInRow(curPos[1]) - 1;
      newIndex = getHomePagePanelMusicIndexAtPos(curPos[0], curPos[1]);
    }
    Serial.print("NewPos: ");
    Serial.print(curPos[0]);
    Serial.print(" - ");
    Serial.println(curPos[1]);

    homePage_selectMusicItem(newIndex);
  } else if (curPage == MUSIC) {
    musicPage_curPageIndex--;
    if (musicPage_curPageIndex < 0)
    {
      musicPage_curPageIndex = availableMusic_pageCount[musicPage_curMusicIndex] - 1;
    }
    openPage(MUSIC);
  }
}

void ok() {
  if (curPage == HOME)
  {
    if (musicPage_curMusicIndex == -1) return;
    musicPage_curPageIndex = 0;
    openMusicPage(musicPage_curMusicIndex);
  } else if (curPage == MUSIC) {
    openPage(HOME);
  }
}



// ================ PAGE ================
void openMusicPage(int _curMusicIndex) {
  musicPage_curMusicIndex = min(_curMusicIndex, availableMusicCount - 1);
  openPage(MUSIC);
}

void openPage(UIPage page) {
  prevPage = curPage;
  curPage = page;
  if (page == HOME)
  {
    Serial.println("open HOME");
    drawHomePage();
  } else if (page == MUSIC)
  {
    Serial.println("open MUSIC");
    bool downloaded = musicImageDownloaded(musicPage_curMusicIndex, musicPage_curPageIndex);
    Serial.print("downloaded?:");
    Serial.println(downloaded);
    if (downloaded)
    {
      drawMusicPage(musicPage_curMusicIndex, musicPage_curPageIndex);
    } else {
      Serial.println("Not downloaded, downloading now...");
      downloadMusicImage(musicPage_curMusicIndex, musicPage_curPageIndex, &onDownloadFinish);
      openMusicPageOnDownloadFinish = true;
    }
  } else if (page == UPDATING_CATALOGUE) {
    Serial.println("open Update page");
    drawUpdatePage();
  }
}
