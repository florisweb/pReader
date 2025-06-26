#ifndef DRAWLIB_H // Guard against multiple inclusions
#define DRAWLIB_H

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



void directDrawText(String text, int x, int y, int textColor) {
  gfx.setTextSize(1);
  uint16_t bufferHeight;
  uint8_t* bitmap = gfx.printAndGetBuffer(text, textColor, &bufferHeight);
  directDrawBuffer(bitmap, x, y, BUFFER_WIDTH, bufferHeight);
  delete[] bitmap;
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







// ================ PAGE-GENERAL ================


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



#endif
