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


connectionManager ConnectionManager;

const int nextButtonPin = 27;
const int prevButtonPin = 21;
const int okButtonPin = 15;
const int pedalNextPin = 25;
const int pedalPrevPin = 33;
const int BlueLEDPin = 2;

const int DisplayCS = 17; // SPI chip select


const int SSDCS = 26; // SPI chip select
File file;


const int displayResetPin = 16;
GxEPD2_BW < GxEPD2_1330_GDEM133T91, GxEPD2_1330_GDEM133T91::HEIGHT / 2 > // /2 makes it two pages
display(GxEPD2_1330_GDEM133T91(/*CS=*/ DisplayCS, /*DC=*/ 5, /*RST=*/ displayResetPin, /*BUSY=*/ 4)); // GDEM133T91 960x680, SSD1677, (FPC-7701 REV.B)


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

const int musicImageWidth = 968;

// Download section
int curDownloadMusicIndex = 0;
int curDownloadPageIndex = 0;
int musicImageRequestId = 0;
int remoteImageLength = 0;

int curImageBufferIndex = 0;
const int rowsPerSection = 90;
int imageSectionLength = 0;
bool openMusicPageOnDownloadFinish = false;

void downloadMusicImage(int musicIndex, int pageIndex) {
  curDownloadMusicIndex = musicIndex;
  curDownloadPageIndex = pageIndex;
  Serial.print("Downloading: ");
  Serial.print(curDownloadMusicIndex);
  Serial.print(" - ");
  Serial.println(curDownloadPageIndex);

  String musicFileName = "/" + (String)availableMusic_itemId[curDownloadMusicIndex] + "_[" + (String)curDownloadPageIndex + "].txt";
  file = SD.open(musicFileName, FILE_WRITE);
  if (file) {
    Serial.print("Creating/clearing ");
    Serial.print(musicFileName);
    file.print("");
    file.close();
    Serial.println("-> done.");
  } else {
    Serial.print("error opening");
    Serial.println(musicFileName);
  }

  ConnectionManager.sendRequest(
    String("requestMusicImage"),
    String("{\"musicId\":" + String(availableMusic_itemId[curDownloadMusicIndex]) + ", \"pageIndex\":" + String(curDownloadPageIndex) + "}"),
    &onMusicImageRequestResponse
  );
}

void onMusicImageRequestResponse(DynamicJsonDocument message) {
  String error = message["response"]["error"];
  if (error && error != "null")
  {
    Serial.print("Error on requesting image:" );
    Serial.println(error);
    return;
  }

  remoteImageLength = message["response"]["dataLength"];
  int imageWidth = message["response"]["imageWidth"];
  curImageBufferIndex = 0;
  musicImageRequestId = message["response"]["imageRequestId"];
  imageSectionLength = imageWidth / 8 * rowsPerSection;

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
    // TODO: REMOVE FILE AND STOP DOWNLOADING / RESTART FROM SCRATCH
    return;
  }

  int startIndex = message["response"]["startIndex"];
  String imageData = message["response"]["data"];
  curImageBufferIndex = startIndex + imageSectionLength;

  String musicFileName = "/" + (String)availableMusic_itemId[curDownloadMusicIndex] + "_[" + (String)curDownloadPageIndex + "].txt";
  file = SD.open(musicFileName, FILE_WRITE);
  if (file) {
    file.seek(file.size());
    file.print(imageData);
    file.close();
  } else {
    // if the file didn't open, print an error:
    Serial.print("error opening");
    Serial.println(musicFileName);
    // TODO: REMOVE FILE AND STOP DOWNLOADING / RESTART FROM SCRATCH
  }

  if (curImageBufferIndex < remoteImageLength)
  {
    ConnectionManager.sendRequest(
      String("getImageSection"),
      String("{\"startIndex\":" + String(curImageBufferIndex) + ", \"sectionLength\":" + String(imageSectionLength) + ", \"imageRequestId\": " + String(musicImageRequestId) + "}"),
      &onImageSectionResponse
    );
  } else onDownloadFinish();
}

void onDownloadFinish() {
  Serial.println("Finished downloading!");
  if (openMusicPageOnDownloadFinish)
  {
    openMusicPageOnDownloadFinish = false;
    openPage(MUSIC);
  }
  downloadUndownloadedItems();
}



void downloadUndownloadedItems() {
  for (int m = 0; m < availableMusicCount; m++)
  {
    int maxPages = availableMusic_pageCount[m];
    for (int p = 0; p < maxPages; p++)
    {
      bool downloaded = musicImageDownloaded(m, p);
      bool thumbnailDown = thumbnailDownloaded(m);

      Serial.print("downloaded?:");
      Serial.print(m);
      Serial.print("_[");
      Serial.print(p);
      Serial.print("]: ");
      Serial.println(downloaded);
      if (downloaded && thumbnailDown) continue;
      if (curPage != UPDATING_CATALOGUE) openPage(UPDATING_CATALOGUE);

      if (!downloaded) downloadMusicImage(m, p);
      if (thumbnailDown) return;
      ConnectionManager.sendRequest(
        String("getThumbnail"),
        String("{\"musicId\":" + String(availableMusic_itemId[m]) + "}"),
        &onThumbnailResponse
      ); // Needs return to homepage / exit catalogue
      return;
    }
  }
  if (curPage != MUSIC) openPage(HOME);
}

void writeConfigFile() {
  String configString = "[";
  for (int i = 0; i < availableMusicCount; i++)
  {
    if (i != 0) configString += ", ";
    String musicString =
      "{\"id\":" + String(availableMusic_itemId[i]) +
      ",\"name\":\"" + availableMusic_name[i] + "\"" +
      ",\"pages\":" + String(availableMusic_pageCount[i]) +
      ",\"musicImageVersion\":" + String(availableMusic_musicImageVersion[i]) +
      ",\"learningState\":" + String(availableMusic_learningState[i]) +
      "}";
    configString += musicString;
  }
  configString += "]";

  file = SD.open("/musicItems.json", FILE_WRITE);
  if (file) {
    Serial.print("Writing config: ");
    Serial.println(configString);
    file.print(configString);
    file.close();
  } else {
    Serial.println("Error writing config.");
  }
}

void loadConfigFile() {
  file = SD.open("/musicItems.json");
  if (!file)
  {
    Serial.println("Error reading config.");
    return;
  }

  char text[file.size()];
  while (file.available()) {
    int bytesRead = file.readBytes(text, file.size()); // Read bytes into buffer
  }
  file.close();

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, text);
  importMusicItemSet(doc);
  openPage(HOME);
}

void importMusicItemSet(DynamicJsonDocument _jsonArr) {
  for (int i = 0; i < sizeof(availableMusic_pageCount) / sizeof(int); i++)
  {
    importMusicItem(_jsonArr[i], i);
  }

  availableMusicCount = 0;
  for (int i = 0; i < sizeof(availableMusic_pageCount) / sizeof(int); i++)
  {
    if (availableMusic_pageCount[i] == 0) break;
    availableMusicCount = i + 1;
  }
}

void importMusicItem(DynamicJsonDocument _obj, int i) {
  availableMusic_pageCount[i] = _obj["pages"];
  if (availableMusic_pageCount[i] == 0) return;
  availableMusic_itemId[i] = _obj["id"];
  availableMusic_learningState[i] = _obj["learningState"];

  const char* musicName = _obj["name"].as<const char*>();
  availableMusic_name[i] = String(musicName);

  int newMusicVersion = _obj["musicImageVersion"];
  if (newMusicVersion != availableMusic_musicImageVersion[i] && availableMusic_musicImageVersion[i] != 0) removeMusicFiles(availableMusic_itemId[i]);
  availableMusic_musicImageVersion[i] = newMusicVersion;
}



void removeMusicFiles(int musicId) {
  Serial.print("Remove files of: ");
  Serial.println(musicId);

  removeFile("/" + (String)musicId + "_[THUMB].txt");
  int pageIndex = 0;
  while (removeFile("/" + (String)musicId + "_[" + pageIndex + "].txt"))
  {
    pageIndex++;
  }
}

bool removeFile(String path) {
  if (SD.exists(path) == 0) return false;
  Serial.print("Removing: ");
  Serial.println(path);
  SD.remove(path);
  return true;
}





const int thumbnailWidth = 184;
const int thumbnailHeight = 130;

void onThumbnailResponse(DynamicJsonDocument message) {
  String error = message["responsed"]["error"];
  if (error && error != "null")
  {
    Serial.print("Error on requesting thumbnail:" );
    Serial.println(error);
    return;
  }

  int musicId = message["response"]["musicId"];
  int imageWidth = message["response"]["width"];
  String imgData = message["response"]["data"];

  String musicFileName = "/" + (String)musicId + "_[THUMB].txt";
  file = SD.open(musicFileName, FILE_WRITE);
  if (file) {
    Serial.print("Writing thumbnail ");
    Serial.print(musicId);
    file.print(imgData);
    file.close();
    Serial.println("-> done.");
  } else {
    Serial.print("error opening");
    Serial.println(musicId);
  }
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
    importMusicItemSet(message["data"]["availableMusic"]);
    writeConfigFile();
    downloadUndownloadedItems();
  }
}





std::array<int, 2> getHomePagePanelPos(int _musicIndex) {
  int learningStateCounts = 0;
  int ownLearningState = min(availableMusic_learningState[_musicIndex], 2);
  for (int i = 0; i < _musicIndex; i++)
  {
    int learningState = min(availableMusic_learningState[i], 2);
    if (learningState != ownLearningState) continue;
    learningStateCounts++;
  }
  return {learningStateCounts, ownLearningState};
}

int getHomePagePanelCountInRow(int _row) {
  int learningStateCounts = 0;
  for (int i = 0; i < availableMusicCount; i++)
  {
    int learningState = min(availableMusic_learningState[i], 2);
    if (learningState != _row) continue;
    learningStateCounts++;
  }
  return learningStateCounts;
}


int getHomePagePanelMusicIndexAtPos(int x, int y) {
  for (int i = 0; i < availableMusicCount; i++)
  {
    std::array<int, 2> pos = getHomePagePanelPos(i);
    if (pos[0] != x || pos[1] != y) continue;
    return i;
  }
  return -1;
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

  Serial.println("Initializing SD card...");


  if (!SD.begin(SSDCS)) {
    Serial.println("SD initialization failed!");
    while (1);
  }

  Serial.println("SD initialization done.");

  loadConfigFile();

  Serial.println("Setting up WiFi...");
  delay(200);

  ConnectionManager.defineEventDocs("[]");
  ConnectionManager.defineAccessPointDocs("["
                                          "{"
                                          "\"type\": \"setText\","
                                          "\"description\": \"Sets text\""
                                          "}"
                                          "]");
  ConnectionManager.setup(&onMessage);
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

  bool pedalNextState = digitalRead(pedalNextPin);
  bool pedalPrevState = digitalRead(pedalPrevPin);

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
    } else if (ch == "h") {
      openPage(HOME);
    } else if (ch == "up") {
      openPage(UPDATING_CATALOGUE);
    } else if (ch == "next") {
      next();
    } else if (ch == "prev") {
      prev();
    } else if (ch == "ok") {
      ok();
    } else if (ch == "lm") {
      drawMusicImageFromSSD(musicPage_curMusicIndex, musicPage_curPageIndex);
    } else if (ch == "readBuff") {
      Serial.println("buff !== 255:");
      for (int i = 0; i < 40800; i++) // 40800
      {
        int val = (int)display._buffer[i];
        if (val == 255) continue;
        Serial.print((String)val + ",");
      }
    } else if (ch == "re") {
      display.epd2.refresh(false);
    } else  if (ch == "RE") {
      display.epd2.refresh(true);
    } else if (ch == "down") {
      downloadUndownloadedItems();
    } else if (ch == "mem") {
      Serial.print("Free mem: ");
      Serial.println(ESP.getFreeHeap());
    } else if (ch == "reboot") {
      _rebootESP();
    } else if (ch == "drawTest") {
      updateUpdaterPage();
    } else if (ch == "DH") {
      drawHomePagePanels();
    }
  }
}

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


void openMusicPage(int _curMusicIndex) {
  musicPage_curMusicIndex = min(_curMusicIndex, availableMusicCount - 1);
  openPage(MUSIC);
}




// ================ MUSICPAGE ================


void drawMusicImageFromSSD(int musicIndex, int pageIndex) {
  String musicFileName = "/" + (String)availableMusic_itemId[musicIndex] + "_[" + (String)pageIndex + "].txt";
  Serial.print("Start drawing: ");
  Serial.println(musicFileName);
  file = SD.open(musicFileName);
  if (!file) {
    Serial.print("error opening");
    Serial.println(musicFileName);
    return;
  }

  int summedYPos = 0;
  const int blockLen = musicImageWidth * 5; // 7
  char text[blockLen];
  while (file.available()) {
    int bytesRead = file.readBytes(text, blockLen); // Read bytes into buffer
    size_t outputLength;
    uint8_t* decoded = base64_decode((const unsigned char *)text, bytesRead, &outputLength);
    for (int i = 0; i < outputLength; i++) decoded[i] = ~decoded[i]; // Invert image color

    int rows = floor(outputLength * 8 / musicImageWidth);
    display.epd2.writeImage(decoded, 0, summedYPos, musicImageWidth, rows);
    free(decoded);
    Serial.println(summedYPos);
    summedYPos += rows;
  }

  file.close();

  display.epd2.refresh(false); // FULL refresh when loading - speed not important
  Serial.println("Finished drawing.");
}



// ================ HOMEPAGE ================
const int homePage_headerHeight = 60;
const int homePage_margin = 10;
int horizontalListItems = 4;
int verticalListItems = 3;
void drawHomePage() {
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    drawHomePageHeader();
    drawHeaders();
  }
  while (display.nextPage());

  drawHomePagePanels();
  //  drawHomePagePanelThumbnails();
  display.epd2.refresh(true);
  drawHomePagePanels();
  display.epd2.refresh(true);
}


void homePage_selectMusicItem(int musicItemIndex) {
  Serial.print("Select ");
  Serial.print(musicItemIndex);
  Serial.print(" - prev: ");
  Serial.println(musicPage_curMusicIndex);

  int prevIndex = musicPage_curMusicIndex;
  musicPage_curMusicIndex = musicItemIndex;


  std::array<int, 2> pos0 = getHomePagePanelPos(prevIndex);
  drawMusicPreviewPanel(pos0[0], pos0[1], prevIndex);
  std::array<int, 2> pos1 = getHomePagePanelPos(musicPage_curMusicIndex);
  drawMusicPreviewPanel(pos1[0], pos1[1], musicPage_curMusicIndex);
  display.epd2.refresh(true);

  std::array<int, 2> pos2 = getHomePagePanelPos(prevIndex);
  drawMusicPreviewPanel(pos2[0], pos2[1], prevIndex);
  std::array<int, 2> pos3 = getHomePagePanelPos(musicPage_curMusicIndex);
  drawMusicPreviewPanel(pos3[0], pos3[1], musicPage_curMusicIndex);
  display.epd2.refresh(true);



  //
  //
  //  int prevIndex = musicPage_curMusicIndex;
  //  musicPage_curMusicIndex = musicItemIndex;
  //
  //  int learningStateCounts[] = {0, 0, 0};
  //  int preLearningState = 0;
  //  int preListIndex = 0;
  //  for (int i = 0; i < availableMusicCount; i++)
  //  {
  //    int learningState = min(availableMusic_learningState[i], 2);
  //    if (i == prevIndex)
  //    {
  //      preLearningState = learningState;
  //      preListIndex = learningStateCounts[learningState];
  //      break;
  //    }
  //
  //    learningStateCounts[learningState]++;
  //  }
  //
  //  const int hMargin = homePage_margin;
  //  const int vMargin = homePage_margin * 3;
  //  const int maxWidth = display.width() / horizontalListItems;
  //  const int maxHeight = (display.height() - homePage_headerHeight - 40) / verticalListItems;
  //
  //  const int width = maxWidth - hMargin * 2;
  //  const int height = maxHeight - vMargin * 2;
  //
  //  const int topX = maxWidth * preListIndex + hMargin;
  //  const int topY = maxHeight * preLearningState + vMargin + homePage_headerHeight + vMargin;
  //
  //  Serial.print("width: ");
  //  Serial.print(width);
  //  Serial.print(" height: ");
  //  Serial.println(height);
  //
  //
  //  //
  //  //  drawHomePageHeader();
  //  //  drawHeaders();
  //
  //  //  display.display(true);
  //
  //  display.fillRect(topX, topY, width, height, GxEPD_WHITE);
  //  drawHomePagePanels();
  //  display.epd2.refresh(true);
  //  display.fillRect(topX, topY, width, height, GxEPD_WHITE);
  //  drawHomePagePanels();
  //  display.epd2.refresh(true);
  //
  //  //  for (int i = 0; i < 10; i++) display.epd2.refresh(true);
  //
  //  //  drawHomePagePanelThumbnails();
  //  //  display.epd2.refresh(true);
}

//void homePage_selectMusicItem(int musicItemIndex) {
//  Serial.print("Select ");
//  Serial.print(musicItemIndex);
//  Serial.print(" - prev: ");
//  Serial.println(musicPage_curMusicIndex);
//
//  int prevIndex = musicPage_curMusicIndex;
//  musicPage_curMusicIndex = musicItemIndex;
//
//  int learningStateCounts[] = {0, 0, 0};
//  int preLearningState = 0;
//  int preListIndex = 0;
//  for (int i = 0; i < availableMusicCount; i++)
//  {
//    int learningState = min(availableMusic_learningState[i], 2);
//    if (i == prevIndex)
//    {
//      preLearningState = learningState;
//      preListIndex = learningStateCounts[learningState];
//      break;
//    }
//
//    learningStateCounts[learningState]++;
//  }
//
//  const int hMargin = homePage_margin;
//  const int vMargin = homePage_margin * 3;
//  const int maxWidth = display.width() / horizontalListItems;
//  const int maxHeight = (display.height() - homePage_headerHeight - 40) / verticalListItems;
//
//  const int width = maxWidth - hMargin * 2;
//  const int height = maxHeight - vMargin * 2;
//
//  const int topX = maxWidth * preListIndex + hMargin;
//  const int topY = maxHeight * preLearningState + vMargin + homePage_headerHeight + vMargin;
//
//  display.fillRect(topX, topY, width, height, GxEPD_WHITE);
//
//  drawHomePageHeader();
//  drawHeaders();
//  drawHomePagePanels();
//  display.display(true);
//
//  for (int i = 0; i < 10; i++) display.epd2.refresh(true);
//
//  drawHomePagePanelThumbnails();
//  display.epd2.refresh(true);
//}


void drawHomePageHeader() {
  String headerName = "Piano Reader";
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSansBoldOblique18pt7b);
  display.getTextBounds(headerName, 0, 0, &tbx, &tby, &tbw, &tbh);

  uint16_t x = homePage_margin - tbx;
  uint16_t y = ((homePage_headerHeight - tbh) / 2) - tby;

  display.setCursor(x, y);
  display.print(headerName);
  display.fillRect(homePage_margin, homePage_headerHeight - 1, display.width() - homePage_margin * 2, 1, GxEPD_BLACK);
}


void drawHomePagePanels() {
  for (int i = 0; i < availableMusicCount; i++)
  {
    std::array<int, 2> pos = getHomePagePanelPos(i);
    if (pos[0] > 3) continue;
    drawMusicPreviewPanel(pos[0], pos[1], i);
  }
}

void drawHomePagePanelThumbnails() {
  for (int i = 0; i < availableMusicCount; i++)
  {
    std::array<int, 2> pos = getHomePagePanelPos(i);
    if (pos[0] > 3) continue;
    drawMusicPreviewThumbnail(pos[0], pos[1], i);
  }
}


void drawHeaders() {
  const int vOffset = 40;
  const int maxHeight = (display.height() - homePage_headerHeight - 40) / verticalListItems;
  for (int i = 0; i < verticalListItems; i++)
  {
    display.setFont(&FreeSansOblique9pt7b);
    display.setTextColor(GxEPD_BLACK);
    String headerTitle = learningStates[i];
    if (i == 2) headerTitle = "Finished & Rest";

    int16_t tbx, tby; uint16_t tbw, tbh;
    display.getTextBounds(headerTitle, 0, 0, &tbx, &tby, &tbw, &tbh);

    uint16_t x = homePage_margin - tbx;
    uint16_t y = homePage_margin + homePage_headerHeight + vOffset + maxHeight * i;

    display.setCursor(x, y);
    display.print(headerTitle);
  }
}


void drawMusicPreviewPanel(int _listIndex, int _verticalListIndex, int _musicItemIndex) {
  directDrawPanel(_listIndex, _verticalListIndex, _musicItemIndex);
  //
  //  bool selected = _musicItemIndex == musicPage_curMusicIndex;
  //  String curName = availableMusic_name[_musicItemIndex];
  //  String subText = String(availableMusic_pageCount[_musicItemIndex]) + " pages - ";
  //  if (availableMusic_pageCount[_musicItemIndex] == 1) subText = "1 page - ";
  //  subText += learningStates[availableMusic_learningState[_musicItemIndex]];
  //
  //  const int hMargin = homePage_margin - 1;
  //  const int vMargin = homePage_margin * 3 + 1;
  //  const int maxWidth = display.width() / horizontalListItems;
  //  const int maxHeight = (display.height() - homePage_headerHeight - 40) / verticalListItems;
  //
  //  const int width = maxWidth - hMargin * 2;
  //  const int height = maxHeight - vMargin * 2;
  //  const int previewMargin = 10 - 1; // Border AROUND preview
  //  const int previewHeight = (width - 2 * previewMargin) * 1.41;
  //
  //  const int topX = maxWidth * _listIndex + hMargin;
  //  const int topY = maxHeight * _verticalListIndex + vMargin + homePage_headerHeight + vMargin;
  //
  //  if (selected)
  //  {
  //    display.fillRect(topX, topY, width, height, GxEPD_BLACK);
  //    display.fillRect(topX + previewMargin, topY + previewMargin, (width - 2 * previewMargin), previewHeight, GxEPD_WHITE);
  //  } else {
  //    display.drawRect(topX, topY, width, height, GxEPD_BLACK);
  //    display.drawRect(topX + previewMargin, topY + previewMargin, (width - 2 * previewMargin), previewHeight, GxEPD_BLACK);
  //  }
  //
  //  display.setFont();
  //  if (selected) {
  //    display.setTextColor(GxEPD_WHITE);
  //  } else display.setTextColor(GxEPD_BLACK);
  //
  //  int16_t tbx, tby; uint16_t tbw, tbh;
  //  display.getTextBounds(curName, 0, 0, &tbx, &tby, &tbw, &tbh);
  //
  //  uint16_t x = -tbx + topX + previewMargin;
  //  uint16_t y = (((height - previewHeight) - tbh) / 2) - tby + topY + previewHeight;
  //  display.setCursor(x, y);
  //  display.print(curName);
  //
  //  display.setCursor(x, y + tbh * 1.5);
  //  display.print(subText);
}

void drawMusicPreviewThumbnail(int _listIndex, int _verticalListIndex, int _musicItemIndex) {
  String musicFileName = "/" + (String)availableMusic_itemId[_musicItemIndex] + "_[THUMB].txt";
  file = SD.open(musicFileName);
  if (!file) {
    Serial.print("error opening (thumbnail draw) ");
    Serial.println(musicFileName);
    return;
  }
  const int previewMargin = 10;
  const int hMargin = homePage_margin;
  const int vMargin = homePage_margin * 3;

  const int maxWidth = display.width() / horizontalListItems;
  const int maxHeight = (display.height() - homePage_headerHeight - 40) / verticalListItems;
  const int topX = maxWidth * _listIndex + hMargin + previewMargin;
  const int topY = maxHeight * _verticalListIndex + vMargin + homePage_headerHeight + vMargin + previewMargin;


  const int fileSize = file.size();
  char text[fileSize];
  while (file.available()) {
    int bytesRead = file.readBytes(text, fileSize); // Read bytes into buffer

    size_t outputLength;
    uint8_t* decoded = base64_decode((const unsigned char *)text, bytesRead, &outputLength);
    for (int i = 0; i < outputLength; i++) decoded[i] = ~decoded[i]; // Invert image color

    // Different coordinate system due to direct RAM write
    display.epd2.writeImage(decoded, display.height() - topY - thumbnailWidth, topX, thumbnailWidth, thumbnailHeight);
    free(decoded);
  }
  file.close();
}




void directDrawPanel(int _listIndex, int _verticalListIndex, int musicIndex) {
  const int hMargin = homePage_margin - 1;
  const int vMargin = homePage_margin * 3 + 1;
  const int maxWidth = display.width() / horizontalListItems;
  const int maxHeight = (display.height() - homePage_headerHeight - 40) / verticalListItems;

  const int width = maxWidth - hMargin * 2;
  const int height = maxHeight - vMargin * 2;
  const int previewMargin = 10 - 1; // Border AROUND preview
  const int previewHeight = (width - 2 * previewMargin) * 1.41;

  const int topX = maxWidth * _listIndex + hMargin;
  const int topY = maxHeight * _verticalListIndex + vMargin + homePage_headerHeight + vMargin;

  bool selected = musicIndex == musicPage_curMusicIndex;
  if (selected)
  {
    fillRect(topX, topY, width, height, 2); // GxEPD_BLACK
  } else rect(topX, topY, width, height);

  rect(topX + 9, topY + 8, width - 18, height - 40);
  directDrawThumbnail(topX, topY, musicIndex);
}

void directDrawThumbnail(int topX, int topY, int musicIndex) {
  uint8_t* bytes = getThumbnailBuffer(musicIndex);
  display.epd2.writeImage(bytes, display.height() - topY - 8 - thumbnailWidth, topX + 11, thumbnailWidth, thumbnailHeight);
  delete[] bytes;
}

void fillRect(int16_t x, int16_t y, int16_t width, int16_t height, int color) {
  uint8_t* bytes = createPxBuffer(width, height, color);
  display.epd2.writeImage(bytes, display.height() - y - height, x, height, width);
  delete[] bytes;
}

void rect(int16_t x, int16_t y, int16_t width, int16_t height) {
  int curIndex = 0;
  int trueHeight = ceil(height / 8) * 8;

  uint8_t* bytes = new uint8_t[trueHeight / 8 * width];
  for (int x = 0; x < width; x++)
  {
    for (int y = 0; y < trueHeight; y += 8)
    {
      if (x == 0 || x == width - 1)
      {
        bytes[curIndex] = 0x00;
      } else {
        if (y == 0)
        {
          bytes[curIndex] = 0x7f;
        } else if (y == trueHeight - 8) {
          bytes[curIndex] = 0xfe;
        } else {
          bytes[curIndex] = 0xff;
        }
      }
      curIndex++;
    }
  }

  display.epd2.writeImage(bytes, display.height() - y - height, x, height, width);
  delete[] bytes;
}


uint8_t* createPxBuffer(int16_t width, int16_t height, int color) {
  int val = 0xff;
  if (color == GxEPD_BLACK) val = 0x00;

  int curIndex = 0;
  int trueHeight = ceil(height / 8) * 8;
  Serial.print("True height: ");
  Serial.println(trueHeight);

  uint8_t* bytes = new uint8_t[trueHeight / 8 * width];

  for (int y = 0; y < width; y++)
  {
    for (int x = 0; x < trueHeight; x += 8)
    {
      if (color != 2)
      {
        bytes[curIndex] = val;
      } else {
        if (y % 2 == 0)
        {
          bytes[curIndex] = 0xaa;
        } else bytes[curIndex] = 0x55;

      }
      curIndex++;
    }
  }
  return bytes;
}


uint8_t* getThumbnailBuffer(int _musicItemIndex) {
  String musicFileName = "/" + (String)availableMusic_itemId[_musicItemIndex] + "_[THUMB].txt";
  file = SD.open(musicFileName);
  uint8_t* bytes = new uint8_t[thumbnailWidth / 8 * thumbnailHeight];
  if (!file) {
    Serial.print("error opening (thumbnail draw) ");
    Serial.println(musicFileName);
    return bytes;
  }

  const int fileSize = file.size();
  char text[fileSize];
  while (file.available()) {
    int bytesRead = file.readBytes(text, fileSize); // Read bytes into buffer

    size_t outputLength;
    uint8_t* decoded = base64_decode((const unsigned char *)text, bytesRead, &outputLength);
    for (int i = 0; i < outputLength; i++) bytes[i] = ~decoded[i]; // Invert image color
    free(decoded);
  }
  file.close();

  return bytes;
}




// ================ UPDATE PAGE ================
void drawUpdatePage() {
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeSansOblique9pt7b);
    display.setTextColor(GxEPD_BLACK);

    String updateText = "Updating catalogue...";
    int16_t tbx, tby; uint16_t tbw, tbh;
    display.getTextBounds(updateText, 0, 0, &tbx, &tby, &tbw, &tbh);

    uint16_t x = display.width() / 2 - tbw / 2 - tbx;
    uint16_t y = display.height() / 2 - tbh / 2 - tby;
    display.setCursor(x, y);
    display.print(updateText);
  }
  while (display.nextPage());
}

void updateUpdaterPage() {
  Serial.print("UPD");
  directDrawPanel(1, 1, 0);
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
      drawMusicImageFromSSD(musicPage_curMusicIndex, musicPage_curPageIndex);
    } else {
      Serial.println("Not downloaded, downloading now...");
      downloadMusicImage(musicPage_curMusicIndex, musicPage_curPageIndex);
      openMusicPageOnDownloadFinish = true;
    }
  } else if (page == UPDATING_CATALOGUE) {
    Serial.println("open Update page");
    drawUpdatePage();
  }
}







bool fileDownloaded(String fileName) {
  if (SD.exists(fileName) == 0) return false;

  // Check if there is any data in the file
  file = SD.open(fileName);
  int fileSize = file.size();
  file.close();
  return fileSize > 0;
}

bool thumbnailDownloaded(int musicIndex) {
  String musicFileName = "/" + (String)availableMusic_itemId[musicIndex] + "_[THUMB].txt";
  return fileDownloaded(musicFileName);
}

bool musicImageDownloaded(int musicIndex, int pageIndex) {
  String musicFileName = "/" + (String)availableMusic_itemId[musicIndex] + "_[" + (String)pageIndex + "].txt";
  return fileDownloaded(musicFileName);
}
