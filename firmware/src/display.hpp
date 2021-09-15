#pragma once
#include <Adafruit_NeoPixel.h>  // for communication with WS2812B LEDs
#include <map>                  // for std::map
#include <vector>               // for std::vector
#include "characters.hpp"
#include "elapsed_time.hpp"
#include "light_sensor.hpp"
#include "settings.hpp"

enum Colors {
    BLACK = 0x000000,
    GRAY = 0x7F7F7F,
    WHITE = 0xFFFFFF,
    RED = 0xFF0000,
    GREEN = 0x00FF00,
    BLUE = 0x0000FF,
    ORANGE = 0xFFA500,
    PURPLE = 0x8000FF,

    DARK_RED = 0x7F0000,
    DARK_GREEN = 0x007F00,
    DARK_BLUE = 0x00007F,
    DARK_GRAY = 0x3F3F3F,
};

enum Display_e {
    WIDTH = 17,
    HEIGHT = 5,
    ROUND_LEDS = 24,
    FIRST_HOUR_LED = WIDTH * HEIGHT,
    FIRST_MINUTE_LED = FIRST_HOUR_LED + 12,
    TOTAL_LEDS = (WIDTH * HEIGHT) + ROUND_LEDS,

    MIN_BRIGHTNESS = 4,
    MAX_BRIGHTNESS = 70,
    SCROLL_DELAY_HORIZONTAL_MS = 10,
    SCROLL_DELAY_VERTICAL_MS = 35,

    LEDS_PIN = 15,
};

class Display {
  private:
    // Adafruit_NeoPixel::setBrightness() is destructive to the pixel
    // data, pixel read operations at low brightness behave poorly. Having this
    // extra buffer solves this problem, which allows better horizontal
    // scrolling in all brightness conditions
    class PixelsWithBuffer : public Adafruit_NeoPixel {
        using Adafruit_NeoPixel::Adafruit_NeoPixel;
        uint32_t m_pixels[TOTAL_LEDS] = {0};

      public:
        void setPixelColor(uint16_t n, uint32_t c) {
            m_pixels[n] = c;
            ((Adafruit_NeoPixel*)this)->setPixelColor(n, c);
        }
        uint32_t getPixelColor(uint16_t n) { return m_pixels[n]; }
    };

    Settings& m_settings;

    PixelsWithBuffer m_leds{TOTAL_LEDS, LEDS_PIN, NEO_GRB + NEO_KHZ800};
    LightSensor m_lightSensor;
    int m_currentBrightness{0};

  public:
    Display(Settings& settings) : m_settings(settings) {
        m_leds.begin();
        // make sure the blue LED on the ESP-12F is off
        pinMode(LED_BUILTIN, OUTPUT);
        digitalWrite(LED_BUILTIN, HIGH);
    }

    Adafruit_NeoPixel& GetLEDs() { return m_leds; };

    void Update() {
        m_currentBrightness = m_lightSensor.Get();
        SetBrightness(map(m_currentBrightness, 0, LightSensor::RANGE,
                          m_settings["min_brightness"] | MIN_BRIGHTNESS,
                          m_settings["max_brightness"] | MAX_BRIGHTNESS));
        Show();
    }

    void Show() { m_leds.show(); }

    void Clear(int color = BLACK, const bool includeRoundLEDs = false) {
        int num = WIDTH * HEIGHT + (includeRoundLEDs ? ROUND_LEDS : 0);
        for (int i = 0; i < num; ++i) {
            m_leds.setPixelColor(i, color);
        }
    }

    void ClearRoundLEDs(int color = BLACK) {
        for (int i = FIRST_HOUR_LED; i < TOTAL_LEDS; ++i) {
            m_leds.setPixelColor(i, color);
        }
    }

    void DrawPixel(const int num, const int color) {
        m_leds.setPixelColor(num, color);
    }

    int DrawText(int x, String text, int color) {
        text.toUpperCase();
        int textWidth = 0;

        for (auto character : text) {
            const int charWidth = DrawChar(x, character, color);
            textWidth += charWidth;
            x += charWidth;
        }
        return textWidth;
    }

    void DrawTextScrolling(String text, int color, int delayMs = 75) {
        const auto length = DrawText(0, text, color);

        for (int i = WIDTH; i > WIDTH - length; --i) {
            Clear();
            DrawText(i, text, color);
            Show();
            ElapsedTime::Delay(delayMs);
        }

        ElapsedTime::Delay(delayMs * 4);
    }

    void DrawTextCentered(String text, int color) {
        int pos = 9 - (text.length() * 2);
        DrawText(pos, text, color);
    }

    void ScrollHorizontal(const int num,
                          const int direction,
                          const int delayMs = SCROLL_DELAY_HORIZONTAL_MS) {
        for (int i = 0; i < num; ++i) {
            MoveHorizontal(direction);
            Update();
            ElapsedTime::Delay(delayMs);
        }
        ElapsedTime::Delay(delayMs);
    }

    void ScrollVertical(const int num,
                        const int direction,
                        const int delayMs = SCROLL_DELAY_VERTICAL_MS) {
        for (int i = 0; i < num; ++i) {
            MoveVertical(direction);
            Update();
            ElapsedTime::Delay(delayMs);
        }
        ElapsedTime::Delay(delayMs);
    }

    void DrawMinuteLED(const int minute, const int color) {
        DrawPixel(FIRST_MINUTE_LED + GetMinuteLED(minute), color);
    }
    void DrawHourLED(const int hour, const int color) {
        DrawPixel(FIRST_HOUR_LED + hour - 1, color);
    }

    void DrawSecondLEDs(const int minute, const int color) {
        DrawPixel(FIRST_HOUR_LED + GetSecondLED(minute), color);
        DrawPixel(FIRST_MINUTE_LED + GetMinuteLED(minute), color);
    }

    int GetMinuteLED(const int minute) {
        return minute / 5;  // represent 60 seconds with 12 LEDs, 60 / 12 = 5
    }

    int GetSecondLED(const int minute) {
        return minute >= 5 ? GetMinuteLED(minute) - 1 : 11;
    }

    int DrawChar(const int x, char character, int color) {
        if (CHARS.find(character) == CHARS.end()) {
            character = '?';
        }

        const std::vector<uint8_t>& charData = CHARS.at(character);
        const int charWidth = charData.size() / HEIGHT;
        const uint8_t* data = &charData[0];

        for (int row = 0; row < HEIGHT; ++row) {
            int column = x;
            for (; column < x + charWidth; ++column) {
                const int led = column + row * WIDTH;
                if (column >= 0 && column < WIDTH && *data) {
                    m_leds.setPixelColor(led, color);
                }
                data++;

                // TODO: Add a way for the calling function to change the color
                // as it is drawn
            }
        }
        return charWidth + 1;
    }

    static uint32_t ColorWheel(uint8_t pos) {
        pos = 255 - pos;
        if (pos < 85) {
            return Adafruit_NeoPixel::Color(255 - pos * 3, 0, pos * 3);
        }

        if (pos < 170) {
            pos -= 85;
            return Adafruit_NeoPixel::Color(0, pos * 3, 255 - pos * 3);
        }

        pos -= 170;
        return Adafruit_NeoPixel::Color(pos * 3, 255 - pos * 3, 0);
    }

    uint16_t GetBrightness() { return m_currentBrightness; }
    void SetBrightness(const uint8_t brightness) {
        m_leds.setBrightness(brightness);
    }

    static int ScaleBrightness(const int color, const float brightness) {
        const float r = ((color & 0xFF0000) >> 16) * brightness;
        const float g = ((color & 0x00FF00) >> 8) * brightness;
        const float b = (color & 0x0000FF) * brightness;
        return Adafruit_NeoPixel::Color(r, g, b);
    }

  private:
    void MovePixel(int fromCol, int fromRow, int toCol, int toRow) {
        int color = m_leds.getPixelColor(fromRow * WIDTH + fromCol);

        if (toCol >= 0 && toCol < WIDTH && toRow >= 0 && toRow < HEIGHT) {
            DrawPixel(toRow * WIDTH + toCol, color);
            DrawPixel(fromRow * WIDTH + fromCol, BLACK);
        }
    }

    void MoveHorizontal(int num) {
        for (int row = 0; row < HEIGHT; ++row) {
            if (num < 0) {
                for (int col = 1; col < WIDTH; ++col) {
                    MovePixel(col, row, col + num, row);
                }
            } else {
                for (int col = WIDTH - 2; col >= 0; --col) {
                    MovePixel(col, row, col + num, row);
                }
            }
        }
    }

    void MoveVertical(int num) {
        if (num < 0) {
            for (int row = 1; row < HEIGHT; ++row) {
                for (int col = 0; col < WIDTH; ++col) {
                    MovePixel(col, row, col, row + num);
                }
            }
        } else {
            for (int row = HEIGHT - 2; row >= 0; --row) {
                for (int col = 0; col < WIDTH; ++col) {
                    MovePixel(col, row, col, row + num);
                }
            }
        }
    }

  public:
};
