#ifndef DATAMANAGER_H // Guard against multiple inclusions
#define DATAMANAGER_H


// Download section
int curDownloadMusicIndex = 0;
int curDownloadPageIndex = 0;
int musicImageRequestId = 0;
int remoteImageLength = 0;

int curImageBufferIndex = 0;
const int rowsPerSection = 90;
int imageSectionLength = 0;

void (*onDownloadFinishPointer)();




File file;
void setupSD() {
  Serial.println("Initializing SD card...");
  if (!SD.begin(SSDCS)) {
    Serial.println("SD initialization failed!");
    while (1);
  }
  Serial.println("SD initialization done.");
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

bool removeFile(String path) {
  if (SD.exists(path) == 0) return false;
  Serial.print("Removing: ");
  Serial.println(path);
  SD.remove(path);
  return true;
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
  } else onDownloadFinishPointer();
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



void downloadMusicImage(int musicIndex, int pageIndex, void _onDownloadFinish()) {
  onDownloadFinishPointer = _onDownloadFinish;
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




void downloadUndownloadedItems(void _onDownloadFinish(), void _openPage(UIPage _page)) {
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
      if (curPage != UPDATING_CATALOGUE) _openPage(UPDATING_CATALOGUE);

      if (!downloaded) downloadMusicImage(m, p, _onDownloadFinish);
      if (thumbnailDown) return;
      ConnectionManager.sendRequest(
        String("getThumbnail"),
        String("{\"musicId\":" + String(availableMusic_itemId[m]) + "}"),
        &onThumbnailResponse
      ); // Needs return to homepage / exit catalogue
      return;
    }
  }
  if (curPage != MUSIC) _openPage(HOME);
}



#endif
