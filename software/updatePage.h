#ifndef UPDATEPAGE_H // Guard against multiple inclusions
#define UPDATEPAGE_H

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


#endif
