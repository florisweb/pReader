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





#include <Adafruit_GFX.h>


#define BUFFER_WIDTH 134
#define BUFFER_HEIGHT 64
class MyGFX : public Adafruit_GFX {
  public:
    MyGFX(int16_t w, int16_t h) : Adafruit_GFX(w, h) {
      // Initialize the buffer
      buffer = new uint8_t[w * h / 8];
      memset(buffer, 0, w * h / 8); // Clear the buffer
    }

    ~MyGFX() {
      delete[] buffer; // Clean up the buffer
    }
    void clearBuffer(int textColor) {
      int val = 255;
      if (textColor == 1) val = 0;
      memset(buffer, val, BUFFER_WIDTH * BUFFER_HEIGHT / 8); // Clear the buffer

      if (textColor < 2) return;
      for (int x = 0; x < BUFFER_WIDTH; x++)
      {
        for (int y = 0; y < BUFFER_HEIGHT / 8; y++)
        {
          if (y % 2 == 0)
          {
            buffer[x * BUFFER_HEIGHT / 8 + y] = 0xaa;
          } else buffer[x * BUFFER_HEIGHT / 8 + y] = 0x55;
        }
      }
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
      if ((x >= 0) && (x < WIDTH) && (y >= 0) && (y < HEIGHT)) {
        // Set the pixel in the buffer
        if (color) {
          buffer[x + (y / 8) * WIDTH] |= (1 << (y % 8));
        } else {
          buffer[x + (y / 8) * WIDTH] &= ~(1 << (y % 8));
        }
      }
    }

    void getTextBox(String text, int16_t* tbx, int16_t* tby, uint16_t* tbw, uint16_t* tbh) {
      getTextBounds(text, 0, 0, tbx, tby, tbw, tbh);
    }
    uint8_t* printAndGetBuffer(String text, int textColor, uint16_t* outBufHeight) {
      clearBuffer(textColor);
      int16_t tbx, tby; uint16_t tbw, tbh;

      getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
      *outBufHeight = (tbh + 7) & ~7;

      setTextColor(0);
      if (textColor == 1) setTextColor(1);
      setCursor(tbx, tby); // up = -y

      return getBuffer(*outBufHeight);
    }

    uint8_t* getBuffer(uint16_t outBufHeight) {
      uint8_t* flippedSubBuffer = new uint8_t[BUFFER_WIDTH * BUFFER_HEIGHT / 8];
      for (int x = 0; x < BUFFER_WIDTH; x++) {
        for (int y = 0; y < outBufHeight / 8; y++) {
          flippedSubBuffer[x * outBufHeight / 8 + y] = buffer[y * BUFFER_HEIGHT / 8 + x];
        }
      }

      return flippedSubBuffer;
    }

  private:
    uint8_t* buffer;
};

MyGFX gfx(BUFFER_WIDTH, BUFFER_HEIGHT);

uint8_t WiFiIcon_on[] = {
  0xff, 0x9f, 0xff, 0xcf, 0xfe, 0x67, 0xfb, 0x37, 0xf9, 0x93, 0xfc, 0xdb, 0xee, 0xdb, 0xc6, 0x4b, 0xc6, 0x4b, 0xee, 0xdb, 0xfc, 0xdb, 0xf9, 0x93, 0xfb, 0x37, 0xfe, 0x67, 0xff, 0xcf, 0xff, 0x9f
};
uint8_t WiFiIcon_off[] = {
  0xff, 0x9f, 0xff, 0xd3, 0xfe, 0x61, 0xfb, 0x41, 0xf9, 0x83, 0xfd, 0x07, 0xee, 0x0b, 0xc4, 0x1b, 0xc8, 0x2b, 0xf0, 0x5b, 0xe0, 0xdb, 0xc1, 0x93, 0xc3, 0x37, 0xe6, 0x67, 0xff, 0xcf, 0xff, 0x9f
};








connectionManager ConnectionManager;

const int nextButtonPin = 27;
const int prevButtonPin = 21;
const int okButtonPin = 15;
const int pedalNextPin = 33;
const int pedalPrevPin = 25;
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
  bool changed = importMusicItemSet(doc);
  Serial.print("Loaded config, changed: ");
  Serial.println(changed);
  openPage(HOME);
}

bool importMusicItemSet(DynamicJsonDocument _jsonArr) {
  bool changed = false;
  for (int i = 0; i < sizeof(availableMusic_pageCount) / sizeof(int); i++)
  {
    bool itemChanged = importMusicItem(_jsonArr[i], i);
    if (itemChanged) changed = true;
  }

  int prevAvailableMusicCount = availableMusicCount;

  availableMusicCount = 0;
  for (int i = 0; i < sizeof(availableMusic_pageCount) / sizeof(int); i++)
  {
    if (availableMusic_pageCount[i] == 0) break;
    availableMusicCount = i + 1;
  }

  if (prevAvailableMusicCount != availableMusicCount) return true;
  return changed;
}

bool importMusicItem(DynamicJsonDocument _obj, int i) {
  bool changed = false;
  if (availableMusic_pageCount[i] != _obj["pages"]) changed = true;
  availableMusic_pageCount[i] = _obj["pages"];
  if (availableMusic_pageCount[i] == 0) return changed;
  const char* musicName = _obj["name"].as<const char*>();

  if (availableMusic_itemId[i] != _obj["id"]) changed = true;
  if (availableMusic_learningState[i] != _obj["learningState"]) changed = true;
  if (availableMusic_name[i] != String(musicName)) changed = true;

  availableMusic_itemId[i] = _obj["id"];
  availableMusic_learningState[i] = _obj["learningState"];
  availableMusic_name[i] = String(musicName);

  int newMusicVersion = _obj["musicImageVersion"];
  if (newMusicVersion != availableMusic_musicImageVersion[i] && availableMusic_musicImageVersion[i] != 0) {
    removeMusicFiles(availableMusic_itemId[i]);
    changed = true;
  }
  availableMusic_musicImageVersion[i] = newMusicVersion;
  return changed;
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


void onWiFiConnStateChange(bool conned) {
  Serial.print("Connected?: ");
  Serial.println(conned);
  updateConnectionHeaderIcon();
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



// ================ GENERAL ================
void drawConnectionHeaderIcon() {
  int x = display.width() - 32;
  int y = 16;
  if (ConnectionManager.isConnected()) {
    directDrawBuffer(WiFiIcon_on, x, y, 16, 16);
  } else directDrawBuffer(WiFiIcon_off, x, y, 16, 16);
}

void updateConnectionHeaderIcon() {
  drawConnectionHeaderIcon();
  display.epd2.refresh(false);
  drawConnectionHeaderIcon();
  display.epd2.refresh(false);
}

// ================ MUSICPAGE ================
void drawMusicPage(int musicIndex, int pageIndex) {
  drawMusicImageFromSD(musicIndex, pageIndex);
  musicPage_drawHeader(musicIndex, pageIndex);
  drawConnectionHeaderIcon();
  display.epd2.refresh(false);
}

void drawMusicImageFromSD(int musicIndex, int pageIndex) {
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
}

void musicPage_drawHeader(int musicIndex, int pageIndex) {
  String headerString = (String)availableMusic_name[musicIndex];
  String headerSubString = (String)(pageIndex + 1) + "/" + (String)availableMusic_pageCount[musicIndex];
  gfx.setFont();

  int16_t tx, ty; uint16_t tw, th;
  gfx.getTextBox(headerString, &tx, &ty, &tw, &th);

  int x = display.width() / 2 - tw / 2;
  directDrawText(headerString, x, 4, 0);

  // Page index
  gfx.setFont();
  gfx.getTextBox(headerSubString, &tx, &ty, &tw, &th);
  x = display.width() / 2 - tw / 2;
  int y = display.height() - th - 12;
  directDrawText(headerSubString, x, y, 0);
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

  drawConnectionHeaderIcon();
  drawHomePagePanels();
  display.epd2.refresh(true);
  drawConnectionHeaderIcon();
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
    if (pos[0] > 3) continue;
    drawMusicPreviewPanel(pos[0], pos[1], i);
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
    y = y - y % 8;

    display.setCursor(x, y);
    display.print(headerTitle);
  }
}

void drawMusicPreviewPanel(int _listIndex, int _verticalListIndex, int musicIndex) {
  const int hMargin = homePage_margin - 1;
  const int vMargin = homePage_margin * 3 + 1;
  const int maxWidth = display.width() / horizontalListItems;
  const int maxHeight = (display.height() - homePage_headerHeight - 40) / verticalListItems;

  const int width = maxWidth - hMargin * 2;
  int height = maxHeight - vMargin * 2;
  height = height - height % 8;
  const int previewMargin = 10 - 1; // Border AROUND preview
  const int previewHeight = (width - 2 * previewMargin) * 1.41;

  const int topX = maxWidth * _listIndex + hMargin;
  int topY = maxHeight * _verticalListIndex + vMargin + homePage_headerHeight + vMargin;
  topY = topY - topY % 8; // Make sure it is aligned on full bytes

  bool selected = musicIndex == musicPage_curMusicIndex;
  if (selected)
  {
    fillRect(topX, topY, width, height, 2);
  } else rect(topX, topY, width, height);

  rect(topX + 9, topY + 8, width - 18, height - 32);
  directDrawThumbnail(topX, topY + 4, musicIndex);

  String itemName = availableMusic_name[musicIndex];
  String subText = String(availableMusic_pageCount[musicIndex]) + " pages - ";
  if (availableMusic_pageCount[musicIndex] == 1) subText = "1 page - ";
  subText += learningStates[availableMusic_learningState[musicIndex]];

  uint16_t x = topX + previewMargin;
  uint16_t y = (topY + previewHeight + previewMargin * 2);
  y = y - y % 8;

  int textColor = selected ? 2 : 0;
  gfx.setFont();
  directDrawText(itemName, x, y, textColor);

  gfx.setFont();
  directDrawText(subText, x, y + 8, textColor);
}



void directDrawText(String text, int x, int y, int textColor) {
  gfx.setTextSize(1);
  uint16_t bufferHeight;
  uint8_t* bitmap = gfx.printAndGetBuffer(text, textColor, &bufferHeight);
  directDrawBuffer(bitmap, x, y, BUFFER_WIDTH, bufferHeight);
  delete[] bitmap;
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

  //  display.epd2.writeImage(bytes, display.height() - y - height, x, height, width);
  directDrawBuffer(bytes, x, y, width, height);
  delete[] bytes;
}

void directDrawBuffer(uint8_t* _buffer, int16_t x, int16_t y, int16_t width, int16_t height) {
  // correct -> bitmap rendered from bottom left, upwards in columns (of normal display) | width and height swapped
  display.epd2.writeImage(_buffer, display.height() - y - height, x, height, width);
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
