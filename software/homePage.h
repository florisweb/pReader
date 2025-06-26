#ifndef HOMEPAGE_H // Guard against multiple inclusions
#define HOMEPAGE_H

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

const int homePage_headerHeight = 60;
const int homePage_margin = 10;
int horizontalListItems = 4;
int verticalListItems = 3;




void directDrawThumbnail(int topX, int topY, int musicIndex) {
  uint8_t* bytes = getThumbnailBuffer(musicIndex);
  display.epd2.writeImage(bytes, display.height() - topY - 8 - thumbnailWidth, topX + 11, thumbnailWidth, thumbnailHeight);
  delete[] bytes;
}





void drawHomePageHeader() {
  String headerName = "Piano Reader";
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSansBoldOblique18pt7b);
  display.getTextBounds(headerName, 0, 0, &tbx, &tby, &tbw, &tbh);

  uint16_t x = homePage_margin - tbx + 34; // 34 = offset for icon
  uint16_t y = ((homePage_headerHeight - tbh) / 2) - tby;

  display.setCursor(x, y);
  display.print(headerName);
  display.fillRect(homePage_margin, homePage_headerHeight - 1, display.width() - homePage_margin * 2, 1, GxEPD_BLACK);
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




void drawHomePage() {
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    drawHomePageHeader();
    drawHeaders();
  }
  while (display.nextPage());

  int iconX = 10;
  int iconY = 2;

  directDrawBuffer(PReaderIcon, iconX, iconY, 32, 48);
  drawConnectionHeaderIcon();
  drawHomePagePanels();
  display.epd2.refresh(true);
  
  directDrawBuffer(PReaderIcon, iconX, iconY, 32, 48);
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





#endif
