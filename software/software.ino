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


const char* ssid = "";
const char* password = "";
const String deviceId = "";
const String deviceKey = "";

connectionManager ConnectionManager;

const int nextButtonPin = 27;
const int prevButtonPin = 21;
const int okButtonPin = 15;
const int BlueLEDPin = 2;

const int DisplayCS = 17; // SPI chip select


const int SSDCS = 26; // SPI chip select
File file;


const int displayResetPin = 16;
GxEPD2_BW < GxEPD2_1330_GDEM133T91, GxEPD2_1330_GDEM133T91::HEIGHT / 2 > // /2 makes it two pages
display(GxEPD2_1330_GDEM133T91(/*CS=*/ DisplayCS, /*DC=*/ 5, /*RST=*/ displayResetPin, /*BUSY=*/ 4)); // GDEM133T91 960x680, SSD1677, (FPC-7701 REV.B)



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




const int thumbnailWidth = 184;
const int thumbnailHeight = 130; // 181


void onThumbnailResponse(DynamicJsonDocument message) {
  String error = message["response"]["error"];
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
    availableMusicCount = 0;
    for (int i = 0; i < sizeof(availableMusic_pageCount) / sizeof(int); i++)
    {
      availableMusic_pageCount[i] = message["data"]["availableMusic"][i]["pages"];
      if (availableMusic_pageCount[i] == 0) break;
      availableMusic_itemId[i] = message["data"]["availableMusic"][i]["id"];
      availableMusic_learningState[i] = message["data"]["availableMusic"][i]["learningState"];
      const char* musicName = message["data"]["availableMusic"][i]["name"].as<const char*>();
      availableMusic_name[i] = String(musicName);
      availableMusicCount = i + 1;
    }
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

  Serial.println("");
  Serial.println("");
  Serial.println("Setting up display...");

  pinMode(displayResetPin, OUTPUT);
  display.init();
  display.setRotation(1);
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);

  pinMode(nextButtonPin, INPUT);
  pinMode(prevButtonPin, INPUT);
  pinMode(okButtonPin, INPUT);
  pinMode(BlueLEDPin, OUTPUT);
  Serial.println("Initializing SD card...");

  if (!SD.begin(SSDCS)) {
    Serial.println("SD initialization failed!");
    while (1);
  }

  Serial.println("SD initialization done.");

  Serial.println("Setting up WiFi...");
  delay(200);

  ConnectionManager.defineEventDocs("[]");
  ConnectionManager.defineAccessPointDocs("["
                                          "{"
                                          "\"type\": \"setText\","
                                          "\"description\": \"Sets text\""
                                          "}"
                                          "]");
  ConnectionManager.setup(ssid, password, deviceId, deviceKey, &onMessage);
  ConnectionManager.setServerLocation("206.83.41.24", 8081);
}


bool prevNextButtonState = false;
bool prevPrevButtonState = false;
bool prevOkButtonState = false;
void loop() {
  ConnectionManager.loop();

  bool nextButtonState = digitalRead(nextButtonPin);
  bool prevButtonState = digitalRead(prevButtonPin);
  bool okButtonState = digitalRead(okButtonPin);

  if (curImageBufferIndex >= remoteImageLength) {
    // Not Loading an image
    if (nextButtonState && nextButtonState != prevNextButtonState)
    {
      digitalWrite(BlueLEDPin, HIGH);
      next();
      digitalWrite(BlueLEDPin, LOW);
    } else if (prevButtonState && prevButtonState != prevPrevButtonState)
    {
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
    drawHomePagePanels();
  }
  while (display.nextPage());

  drawHomePagePanelThumbnails();
  display.epd2.refresh(true);
}


void homePage_selectMusicItem(int musicItemIndex) {
  Serial.print("Select ");
  Serial.print(musicItemIndex);
  Serial.print(" - prev: ");
  Serial.println(musicPage_curMusicIndex);

  int prevIndex = musicPage_curMusicIndex;
  musicPage_curMusicIndex = musicItemIndex;

  int learningStateCounts[] = {0, 0, 0};
  int preLearningState = 0;
  int preListIndex = 0;
  for (int i = 0; i < availableMusicCount; i++)
  {
    int learningState = min(availableMusic_learningState[i], 2);
    if (i == prevIndex)
    {
      preLearningState = learningState;
      preListIndex = learningStateCounts[learningState];
      break;
    }

    learningStateCounts[learningState]++;
  }

  const int hMargin = homePage_margin;
  const int vMargin = homePage_margin * 3;
  const int maxWidth = display.width() / horizontalListItems;
  const int maxHeight = (display.height() - homePage_headerHeight - 40) / verticalListItems;

  const int width = maxWidth - hMargin * 2;
  const int height = maxHeight - vMargin * 2;

  const int topX = maxWidth * preListIndex + hMargin;
  const int topY = maxHeight * preLearningState + vMargin + homePage_headerHeight + vMargin;

  display.fillRect(topX, topY, width, height, GxEPD_WHITE);

  drawHomePageHeader();
  drawHeaders();
  drawHomePagePanels();
  display.display(true);

  for (int i = 0; i < 10; i++) display.epd2.refresh(true);

  drawHomePagePanelThumbnails();
  display.epd2.refresh(true);
}


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
    drawMusicPreviewPanel(pos[0], pos[1], i);
  }
}

void drawHomePagePanelThumbnails() {
  for (int i = 0; i < availableMusicCount; i++)
  {
    std::array<int, 2> pos = getHomePagePanelPos(i);
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
  bool selected = _musicItemIndex == musicPage_curMusicIndex;
  String curName = availableMusic_name[_musicItemIndex];
  String subText = String(availableMusic_pageCount[_musicItemIndex]) + " pages - ";
  if (availableMusic_pageCount[_musicItemIndex] == 1) subText = "1 page - ";
  subText += learningStates[availableMusic_learningState[_musicItemIndex]];

  const int hMargin = homePage_margin;
  const int vMargin = homePage_margin * 3;
  const int maxWidth = display.width() / horizontalListItems;
  const int maxHeight = (display.height() - homePage_headerHeight - 40) / verticalListItems;

  const int width = maxWidth - hMargin * 2;
  const int height = maxHeight - vMargin * 2;
  const int previewMargin = 10 - 1; // Border AROUND preview
  const int previewHeight = (width - 2 * previewMargin) * 1.41;

  const int topX = maxWidth * _listIndex + hMargin;
  const int topY = maxHeight * _verticalListIndex + vMargin + homePage_headerHeight + vMargin;

  if (selected)
  {
    display.fillRect(topX, topY, width, height, GxEPD_BLACK);
    display.fillRect(topX + previewMargin, topY + previewMargin, (width - 2 * previewMargin), previewHeight, GxEPD_WHITE);
  } else {
    display.drawRect(topX, topY, width, height, GxEPD_BLACK);
    display.drawRect(topX + previewMargin, topY + previewMargin, (width - 2 * previewMargin), previewHeight, GxEPD_BLACK);
  }

  display.setFont();
  if (selected) {
    display.setTextColor(GxEPD_WHITE);
  } else display.setTextColor(GxEPD_BLACK);

  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds(curName, 0, 0, &tbx, &tby, &tbw, &tbh);

  uint16_t x = -tbx + topX + previewMargin;
  uint16_t y = (((height - previewHeight) - tbh) / 2) - tby + topY + previewHeight;
  display.setCursor(x, y);
  display.print(curName);

  display.setCursor(x, y + tbh * 1.5);
  display.print(subText);
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









bool thumbnailDownloaded(int musicIndex) {
  String musicFileName = "/" + (String)availableMusic_itemId[musicIndex] + "_[THUMB].txt";
  if (SD.exists(musicFileName) == 0) return false;

  // Check if there is anything in the file
  file = SD.open(musicFileName);
  int fileSize = file.size();
  file.close();
  return fileSize > 0;
}


bool musicImageDownloaded(int musicIndex, int pageIndex) {
  String musicFileName = "/" + (String)availableMusic_itemId[musicIndex] + "_[" + (String)pageIndex + "].txt";
  if (SD.exists(musicFileName) == 0) return false;

  // Check if there is anything in the file
  file = SD.open(musicFileName);
  int fileSize = file.size();
  file.close();
  return fileSize > 0;
}
