#pragma once
#include <Adafruit_NeoPixel.h>  // for communication with WS2812B LEDs
#include <functional>           // for std::function
#include <map>                  // for std::map
#include <vector>               // for std::vector

#include "button.hpp"
#include "elapsed_time.hpp"
#include "light_sensor.hpp"
#include "settings.hpp"

enum Colors_e {
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
    DARK_ORANGE = 0xFF8C00,
    DARK_PURPLE = 0x301934,
    DARK_GRAY = 0x3F3F3F,
};

enum SpecialChars_e {
    CHAR_UP_ARROW = 100,
    CHAR_DOWN_ARROW = 101,
    CHAR_RIGHT_ARROW = 102,
    CHAR_LEFT_ARROW = 103,
};

enum ScrollDirection_e {
    SCROLL_UP = -1,
    SCROLL_DOWN = 1,
    SCROLL_RIGHT = 1,
    SCROLL_LEFT = -1,
};

enum Display_e {
    WIDTH = 17,
    HEIGHT = 5,
    ROUND_LEDS = 24,
    LED_MATRIX_TOTAL_LEDS = WIDTH * HEIGHT,
    FIRST_HOUR_LED = LED_MATRIX_TOTAL_LEDS,
    FIRST_MINUTE_LED = FIRST_HOUR_LED + 12,
    TOTAL_LEDS = (WIDTH * HEIGHT) + ROUND_LEDS,

    MIN_BRIGHTNESS = 4,
    MIN_BRIGHTNESS_DEFAULT = 8,
    MAX_BRIGHTNESS = 150,
    MAX_BRIGHTNESS_DEFAULT = 70,
    SCROLLING_TEXT_MS = 50,
    SCROLL_DELAY_HORIZONTAL_MS = 10,
    SCROLL_DELAY_VERTICAL_MS = 20,
    FRAMES_PER_SECOND = 30,
    LIGHT_SENSOR_UPDATE_MS = 2,
    LEDS_PIN = 15,
};

class Display {
  private:
    // Adafruit_NeoPixel::setBrightness() is destructive to the pixel
    // data, pixel read operations at low brightness behave poorly. Having this
    // extra buffer solves this problem, which allows better horizontal
    // scrolling in all brightness conditions. In addition, provides the ability
    // to intercept setPixelColor calls to change the color of any pixels.
    // the override functionality is used by the rainbow and shimmer effects.
    class PixelsWithBuffer : public Adafruit_NeoPixel {
        using Adafruit_NeoPixel::Adafruit_NeoPixel;
        uint32_t m_pixels[TOTAL_LEDS] = {0};
        std::function<void(uint16_t num, uint32_t&)> m_colorOverrideFunc;

      public:
        void setPixelColor(const uint16_t num,
                           uint32_t color,
                           bool forceColor = false) {
            if (num >= TOTAL_LEDS) {
                return;
            }

            if (m_colorOverrideFunc && !forceColor) {
                m_colorOverrideFunc(num, color);
            }

            m_pixels[num] = color;
            ((Adafruit_NeoPixel*)this)->setPixelColor(num, color);
        }
        void restorePixels() {
            for (size_t i = 0; i < TOTAL_LEDS; ++i) {
                ((Adafruit_NeoPixel*)this)->setPixelColor(i, m_pixels[i]);
            }
        }
        void setColorOverride(
            std::function<void(const uint16_t num, uint32_t&)> func) {
            m_colorOverrideFunc = func;
        }
        void clearColorOverride() { m_colorOverrideFunc = nullptr; }
        uint32_t getPixelColor(uint16_t num) { return m_pixels[num]; }
    };

    Settings& m_settings;

    PixelsWithBuffer m_pixels{TOTAL_LEDS, LEDS_PIN, NEO_GRB + NEO_KHZ800};
    LightSensor m_lightSensor;
    size_t m_currentBrightness{0};
    size_t m_lastBrightness{0};
    ElapsedTime m_sinceLastShow;
    ElapsedTime m_sinceLastLightSensorUpdate;

  public:
    Display(Settings& settings) : m_settings(settings) {
        m_pixels.begin();

        // make sure the blue LED on the ESP-12F is off
        pinMode(LED_BUILTIN, OUTPUT);
        digitalWrite(LED_BUILTIN, HIGH);

        if (!settings.containsKey(F("MINB"))) {
            settings[F("MINB")] = String(MIN_BRIGHTNESS_DEFAULT);
        }
        if (!settings.containsKey(F("MAXB"))) {
            settings[F("MAXB")] = String(MAX_BRIGHTNESS_DEFAULT);
        }
    }

    PixelsWithBuffer& GetPixels() { return m_pixels; };

    void Update(const bool force = false) {
        if (m_sinceLastLightSensorUpdate.Ms() > LIGHT_SENSOR_UPDATE_MS) {
            m_sinceLastLightSensorUpdate.Reset();
            m_currentBrightness = m_lightSensor.Get();
        }

        if (m_sinceLastShow.Ms() > (1000 / FRAMES_PER_SECOND) || force) {
            m_sinceLastShow.Reset();

            int minBrightness = m_settings[F("MINB")].as<int>();
            int maxBrightness = m_settings[F("MAXB")].as<int>();

            if (m_currentBrightness != m_lastBrightness) {
                m_lastBrightness = m_currentBrightness;
                SetBrightness(map(
                    m_currentBrightness, LightSensor::MIN_SENSOR_VAL,
                    LightSensor::MAX_SENSOR_VAL, minBrightness, maxBrightness));
            }
            Show();
        }
    }

    void Show() {
        system_soft_wdt_stop();
        ets_intr_lock();
        noInterrupts();

        m_pixels.show();

        interrupts();
        ets_intr_unlock();
        system_soft_wdt_restart();
    }

    void Clear(const uint32_t color = BLACK,
               const bool includeRoundLEDs = false) {
        const size_t numToClear =
            WIDTH * HEIGHT + (includeRoundLEDs ? ROUND_LEDS : 0);
        for (size_t i = 0; i < numToClear; ++i) {
            m_pixels.setPixelColor(i, color);
        }
    }

    void ClearRoundLEDs(const uint32_t color = BLACK) {
        for (size_t i = FIRST_HOUR_LED; i < TOTAL_LEDS; ++i) {
            m_pixels.setPixelColor(i, color);
        }
    }

    // if forceColor == true, the colorOverrideFunc (used by shimmer/rainbow
    // effects) will not be called for this pixel and thus cannot change its
    // color. otherwise, the current colorOverrideFunc can change any pixel
    // at will. fun!
    void DrawPixel(const int x,
                   const int y,
                   const uint32_t color,
                   const bool forceColor = false) {
        m_pixels.setPixelColor(y * WIDTH + x, color, forceColor);
    }

    // if pos == 0 then LED is at the 1:00 position
    void DrawInsideRingPixel(const int pos,
                             const uint32_t color,
                             const bool forceColor = false) {
        m_pixels.setPixelColor(FIRST_HOUR_LED + pos, color, forceColor);
    }

    // if pos == 0 then the LED is in the 12:00 position
    void DrawOutsideRingPixel(const int pos,
                              const uint32_t color,
                              const bool forceColor = false) {
        m_pixels.setPixelColor(FIRST_MINUTE_LED + pos, color, forceColor);
    }

    int DrawText(int x, String text, uint32_t color) {
        text.toUpperCase();
        int textWidth = 0;

        for (auto character : text) {
            const int charWidth = DrawChar(x, character, color);
            textWidth += charWidth;
            x += charWidth;
        }
        return textWidth;
    }

    void DrawTextCentered(const String& text, const uint32_t color) {
        int x = 9 - (text.length() *
                     2);  // chars are usually 3 pixels wide with a 1 pixel
                          // space. this is not completely accurate for a few
                          // narrow characters, such as the colon
        DrawText(x, text, color);
    }

    void DrawTextScrolling(const String& text,
                           const uint32_t color,
                           const size_t delayMs = SCROLLING_TEXT_MS) {
        const auto length = DrawText(0, text, color);

        ElapsedTime waitToScroll;
        for (int i = WIDTH; i > -length;) {
            Clear();
            DrawText(i, text, color);
            // pressing a button will speed up a long blocking scrolling
            // message
            waitToScroll.Reset();
            size_t adjustedDelay = delayMs;
            while (waitToScroll.Ms() < adjustedDelay) {
                Update(true);
                adjustedDelay = Button::AreAnyButtonsPressed() != -1
                                    ? delayMs / 3
                                    : delayMs;
                yield();
            }
            --i;
        }
    }

    void ScrollHorizontal(const int numColumns,
                          const int direction,
                          const size_t delayMs = SCROLL_DELAY_HORIZONTAL_MS) {
        for (int i = 0; i < numColumns; ++i) {
            MoveHorizontal(direction);
            Show();  // don't wait for FPS update
            ElapsedTime::Delay(delayMs);
        }
    }

    void ScrollVertical(const int numRows,
                        const int direction,
                        const size_t delayMs = SCROLL_DELAY_VERTICAL_MS) {
        for (int i = 0; i < numRows; ++i) {
            MoveVertical(direction);
            Show();  // don't wait for FPS update
            ElapsedTime::Delay(delayMs);
        }
    }

    void DrawMinuteLED(const int minute, const uint32_t color) {
        // represent 60 seconds using 12 LEDs, 60 / 12 = 5
        DrawOutsideRingPixel(minute / 5, color);
    }
    void DrawHourLED(const int hour, const uint32_t color) {
        DrawInsideRingPixel(hour - 1, color);
    }

    void DrawSecondLEDs(const int second,
                        const uint32_t color,
                        const bool forceColor = false) {
        DrawInsideRingPixel(second < 5 ? 11 : (second / 5) - 1, color,
                            forceColor);
        DrawOutsideRingPixel(second / 5, color, forceColor);
    }

    int DrawChar(const int x, char character, const uint32_t color) {
        std::vector<uint8_t> charData;
// clang-format off
        // --------------- 
        // characters are implemented in code as a list of if/else statements
        // ---------------
        #include "characters.hpp" 
        
        // show a ? for unknown characters
        if (charData.empty()) {
            charData = {
                1, 1, 0,
                0, 0, 1,
                0, 1, 0,
                0, 0, 0,
                0, 1, 0,
            };
        }

        // ---------------
        // clang-format on

        const int charWidth = charData.size() / HEIGHT;
        const uint8_t* data = &charData[0];
        for (int row = 0; row < HEIGHT; ++row) {
            for (int column = x; column < x + charWidth; ++column) {
                if (column >= 0 && column < WIDTH && *data) {
                    DrawPixel(column, row, color);
                }
                data++;
            }
        }

        return charWidth + 1;
    }

    void DrawColorWheel(const uint8_t bottomPixelWheelPos) {
        uint8_t wheelPos = bottomPixelWheelPos - 128;
        for (size_t i = 0; i < 12; ++i) {
            uint32_t color = ColorWheel(wheelPos);
            wheelPos += 255 / 12;
            DrawInsideRingPixel(i == 0 ? 11 : i - 1, color);
            DrawOutsideRingPixel(i, color);
        }
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
        m_pixels.restorePixels();
        m_pixels.setBrightness(brightness);
        m_lastBrightness = m_currentBrightness = brightness;
    }
    bool IsAtMinimumBrightness() {
        return GetBrightness() == LightSensor::MIN_SENSOR_VAL;
    }

    static int ScaleBrightness(const uint32_t color, const float brightness) {
        const float r = ((color & 0xFF0000) >> 16) * brightness;
        const float g = ((color & 0x00FF00) >> 8) * brightness;
        const float b = (color & 0x0000FF) * brightness;
        return Adafruit_NeoPixel::Color(r, g, b);
    }

  private:
    void MovePixel(const int fromCol,
                   const int fromRow,
                   const int toCol,
                   const int toRow) {
        uint32_t color = m_pixels.getPixelColor(fromRow * WIDTH + fromCol);

        if (toCol >= 0 && toCol < WIDTH && toRow >= 0 && toRow < HEIGHT) {
            DrawPixel(toCol, toRow, color);
            DrawPixel(fromCol, fromRow, BLACK);
        }
    }

    void MoveHorizontal(const int num) {
        for (int row = 0; row < HEIGHT; ++row) {
            if (num < 0) {
                for (int column = 1; column < WIDTH; ++column) {
                    MovePixel(column, row, column + num, row);
                }
            } else {
                for (int column = WIDTH - 2; column >= 0; --column) {
                    MovePixel(column, row, column + num, row);
                }
            }
        }
    }

    void MoveVertical(const int num) {
        if (num < 0) {
            for (int row = 1; row < HEIGHT; ++row) {
                for (int column = 0; column < WIDTH; ++column) {
                    MovePixel(column, row, column, row + num);
                }
            }
        } else {
            for (int row = HEIGHT - 2; row >= 0; --row) {
                for (int column = 0; column < WIDTH; ++column) {
                    MovePixel(column, row, column, row + num);
                }
            }
        }
    }
};
