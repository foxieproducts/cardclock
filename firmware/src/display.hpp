#pragma once
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>  // for delay()
#include <map>        // for std::map
#include <vector>     // for std::vector
#include "light_sensor.hpp"

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
};

class Display {
  private:
    Adafruit_NeoPixel m_leds{TOTAL_LEDS, LEDS_PIN, NEO_GRB + NEO_KHZ800};
    LightSensor m_lightSensor;

  public:
    enum {
        WIDTH = 17,
        HEIGHT = 5,
        ROUND_LEDS = 24,
        TOTAL_LEDS = (WIDTH * HEIGHT) + ROUND_LEDS,

        OFF_COLOR = 0x000000,
        LEDS_PIN = 15,
    };

    Display() {
        m_leds.begin();
        m_leds.setBrightness(LightSensor::MIN_BRIGHTNESS);
    }

    Adafruit_NeoPixel& GetLEDs() { return m_leds; };

    void Show() { m_leds.show(); }
    void Clear(int color = OFF_COLOR, const bool includeRoundLEDs = false) {
        int num = WIDTH * HEIGHT + (includeRoundLEDs ? ROUND_LEDS : 0);
        for (int i = 0; i < num; ++i) {
            m_leds.setPixelColor(i, color);
        }
    }

    void DrawTextScrolling(String text, int color, int delayMs = 25) {
        const auto length = DrawText(0, text, color);
        for (int i = WIDTH; i > -length; --i) {
            Clear();
            DrawText(i, text, color);
            Show();
            delay(delayMs);
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

    uint16_t GetBrightness() { return m_lightSensor.Get(); }
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
    const std::map<uint8_t, std::vector<uint8_t>> CHARS = {
        // clang-format off

        { 'A', { 
           0, 1, 0,
           1, 0, 1,
           1, 0, 1,
           1, 1, 1,
           1, 0, 1,
        } },

        { 'B', { 
           1, 1, 0,
           1, 0, 1,
           1, 1, 0,
           1, 0, 1,
           1, 1, 0,
        } },

        { 'C', { 
           0, 1, 1,
           1, 0, 0,
           1, 0, 0,
           1, 0, 0,
           0, 1, 1,
        } },

        { 'D', { 
           1, 1, 0,
           1, 0, 1,
           1, 0, 1,
           1, 0, 1,
           1, 1, 0,
        } },

        { 'E', { 
           1, 1, 1,
           1, 0, 0,
           1, 1, 1,
           1, 0, 0,
           1, 1, 1,
        } },

        { 'F', { 
           1, 1, 1,
           1, 0, 0,
           1, 1, 1,
           1, 0, 0,
           1, 0, 0,
        } },

        { 'G', { 
           0, 1, 1,
           1, 0, 0,
           1, 1, 1,
           1, 0, 1,
           0, 1, 1,
        } },

        { 'H', { 
           1, 0, 1,
           1, 0, 1,
           1, 1, 1,
           1, 0, 1,
           1, 0, 1,
        } },

        { 'I', { 
           1, 1, 1,
           0, 1, 0,
           0, 1, 0,
           0, 1, 0,
           1, 1, 1,
        } },

        { 'J', { 
           0, 0, 1,
           0, 0, 1,
           0, 0, 1,
           0, 0, 1,
           1, 1, 0,
        } },

        { 'K', { 
           1, 0, 1,
           1, 0, 1,
           1, 1, 0,
           1, 0, 1,
           1, 0, 1,
        } },

        { 'L', { 
           1, 0, 0,
           1, 0, 0,
           1, 0, 0,
           1, 0, 0,
           1, 1, 1,
        } },

        { 'M', { 
           1, 0, 0, 0, 1,
           1, 1, 0, 1, 1,
           1, 0, 1, 0, 1,
           1, 0, 0, 0, 1,
           1, 0, 0, 0, 1,
        } },

        { 'N', { 
           1, 0, 0, 1,
           1, 1, 0, 1,
           1, 0, 1, 1,
           1, 0, 0, 1,
           1, 0, 0, 1,
        } },

        { 'O', { 
           0, 1, 0,
           1, 0, 1,
           1, 0, 1,
           1, 0, 1,
           0, 1, 0,
        } },

        { 'P', { 
           1, 1, 0,
           1, 0, 1,
           1, 1, 0,
           1, 0, 0,
           1, 0, 0,
        } },

        { 'Q', { 
           0, 1, 0,
           1, 0, 1,
           1, 0, 1,
           1, 0, 1,
           0, 1, 1,
        } },

        { 'R', { 
           1, 1, 0,
           1, 0, 1,
           1, 1, 0,
           1, 0, 1,
           1, 0, 1,
        } },

        { 'S', { 
           0, 1, 1,
           1, 0, 0,
           0, 1, 0,
           0, 0, 1,
           1, 1, 0,
        } },

        { 'T', { 
           1, 1, 1,
           0, 1, 0,
           0, 1, 0,
           0, 1, 0,
           0, 1, 0,
        } },

        { 'U', { 
           1, 0, 1,
           1, 0, 1,
           1, 0, 1,
           1, 0, 1,
           0, 1, 1,
        } },

        { 'V', { 
           1, 0, 1,
           1, 0, 1,
           1, 0, 1,
           1, 0, 1,
           0, 1, 0,
        } },

        { 'W', { 
           1, 0, 0, 0, 1,
           1, 0, 0, 0, 1,
           1, 0, 1, 0, 1,
           1, 0, 1, 0, 1,
           0, 1, 0, 1, 0,
        } },

        { 'X', { 
           1, 0, 1,
           1, 0, 1,
           0, 1, 0,
           1, 0, 1,
           1, 0, 1,
        } },

        { 'Y', { 
           1, 0, 1,
           1, 0, 1,
           0, 1, 0,
           0, 1, 0,
           0, 1, 0,
        } },

        { 'Z', { 
           1, 1, 1,
           0, 0, 1,
           0, 1, 0,
           1, 0, 0,
           1, 1, 1,
        } },

        { '/', { 
           0, 0, 1,
           0, 0, 1,
           0, 1, 0,
           1, 0, 0,
           1, 0, 0,
        } },

        { '\\', { 
           1, 0, 0,
           1, 0, 0,
           0, 1, 0,
           0, 0, 1,
           0, 0, 1,
        } },

        { '!', { 
           1,
           1,
           1,
           0,
           1,
        } },

        { '@', { 
           0, 1, 0,
           1, 1, 1,
           1, 1, 1,
           1, 0, 0,
           0, 1, 1,
        } },

        { '#', { 
           0, 1, 0, 1, 0,
           1, 1, 1, 1, 1,
           0, 1, 0, 1, 0,
           1, 1, 1, 1, 1,
           0, 1, 0, 1, 0,
        } },

        { '$', { 
           0, 1, 1,
           1, 1, 0,
           0, 1, 0,
           0, 1, 1,
           1, 1, 0,
        } },

        { '%', { 
           1, 0, 0,
           0, 0, 1,
           0, 1, 0,
           1, 0, 0,
           0, 0, 1,
        } },

        { '^', { 
           0, 1, 0,
           1, 0, 1,
           0, 0, 0,
           0, 0, 0,
           0, 0, 0,
        } },

        { '&', { 
           0, 1, 0,
           1, 0, 0,
           0, 1, 0,
           1, 0, 1,
           1, 1, 1,
        } },

        { '*', { 
           1, 0, 1,
           0, 1, 0,
           1, 1, 1,
           0, 1, 0,
           1, 0, 1,
        } },

        { '(', { 
           0, 1,
           1, 0,
           1, 0,
           1, 0,
           0, 1,
        } },

        { ')', { 
           1, 0,
           0, 1,
           0, 1,
           0, 1,
           1, 0,
        } },

        { '-', { 
           0, 0, 0,
           0, 0, 0,
           1, 1, 1,
           0, 0, 0,
           0, 0, 0,
        } },

        { '_', { 
           0, 0, 0,
           0, 0, 0,
           0, 0, 0,
           0, 0, 0,
           1, 1, 1,
        } },

        { '+', { 
           0, 0, 0,
           0, 1, 0,
           1, 1, 1,
           0, 1, 0,
           0, 0, 0,
        } },

        { '=', { 
           0, 0, 0,
           1, 1, 1,
           0, 0, 0,
           1, 1, 1,
           0, 0, 0,
        } },

        { ',', { 
           0, 0,
           0, 0,
           0, 0,
           0, 1,
           1, 0,
        } },

        { '.', { 
           0,
           0,
           0,
           0,
           1,
        } },

        { '<', { 
           0, 0, 1,
           0, 1, 0,
           1, 0, 0,
           0, 1, 0,
           0, 0, 1,
        } },

        { '>', { 
           1, 0, 0,
           0, 1, 0,
           0, 0, 1,
           0, 1, 0,
           1, 0, 0,
        } },

        { ';', { 
           0, 0,
           0, 1,
           0, 0,
           0, 1,
           1, 0,
        } },

        { ':', { 
           0,
           1,
           0,
           1,
           0,
        } },

        { '\'', { 
           1,
           1,
           0,
           0,
           0,
        } },

        { '"', { 
           1, 0, 1,
           1, 0, 1,
           0, 0, 0,
           0, 0, 0,
           0, 0, 0,
        } },

        { '?', { 
           1, 1, 0,
           0, 0, 1,
           0, 1, 0,
           0, 0, 0,
           0, 1, 0,
        } },

        { ' ', { 
           0, 0, 0,
           0, 0, 0,
           0, 0, 0,
           0, 0, 0,
           0, 0, 0,
        } },

        { '0', { 
           0, 1, 0,
           1, 0, 1,
           1, 0, 1,
           1, 0, 1,
           0, 1, 0,
        } },

        { '1', { 
           0, 1, 0,
           1, 1, 0,
           0, 1, 0,
           0, 1, 0,
           0, 1, 0,
        } },

        { '2', { 
           1, 1, 0,
           0, 0, 1,
           0, 1, 0,
           1, 0, 0,
           1, 1, 1,
        } },
   
        { '3', { 
           1, 1, 0,
           0, 0, 1,
           0, 1, 0,
           0, 0, 1,
           1, 1, 0,
        } },
     
        { '4', { 
           1, 0, 1,
           1, 0, 1,
           0, 1, 1,
           0, 0, 1,
           0, 0, 1,
        } },
     
        { '5', { 
           1, 1, 1,
           1, 0, 0,
           1, 1, 1,
           0, 0, 1,
           1, 1, 0,
        } },
     
        { '6', { 
           0, 1, 1,
           1, 0, 0,
           1, 1, 0,
           1, 0, 1,
           0, 1, 0,
        } },
     
        { '7', { 
           1, 1, 1,
           0, 0, 1,
           0, 1, 0,
           0, 1, 0,
           0, 1, 0,
        } },
     
        { '8', { 
           0, 1, 0,
           1, 0, 1,
           0, 1, 0,
           1, 0, 1,
           0, 1, 0,
        } },
     
        { '9', { 
           0, 1, 0,
           1, 0, 1,
           0, 1, 1,
           0, 0, 1,
           1, 1, 0,
        } },

        // clang-format on
    };
};