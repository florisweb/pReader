#ifndef MUSICPAGE_H // Guard against multiple inclusions
#define MUSICPAGE_H

const int musicImageWidth = 968;

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


void drawMusicPage(int musicIndex, int pageIndex) {
  drawMusicImageFromSD(musicIndex, pageIndex);
  musicPage_drawHeader(musicIndex, pageIndex);
  drawConnectionHeaderIcon();
  display.epd2.refresh(false);
}





#endif
