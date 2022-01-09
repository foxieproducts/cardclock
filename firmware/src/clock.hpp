#pragma once
#include <stdint.h>  // for uint16_t and others

#include "elapsed_time.hpp"
#include "menu.hpp"
#include "rtc.hpp"

class Clock : public Menu {
  private:
    enum ConfigMode_e {
        CONF_MODE_NORMAL,
        CONF_MODE_COLORWHEEL,
        CONF_MODE_ANIM,
    };

    enum AnimationMode_e {
        ANIM_MODE_NORMAL,
        ANIM_MODE_SHIMMER,
        ANIM_MODE_RAINBOW,
        ANIM_MODE_MARQUEE,
        ANIM_MODE_MARQUEE_RAINBOW,
        ANIM_MODE_BINARY,
        ANIM_MODE_BINARY_SHIMMER,
        TOTAL_ANIM_MODES,
    };

    enum {
        MARQUEE_DELAY_MS = 125,
    };

    Rtc& m_rtc;

    int m_animMode{(int)ANIM_MODE_NORMAL};
    bool m_shouldSaveSettings{false};

    ElapsedTime m_waitingToSaveSettings;
    ElapsedTime m_sinceLastAnimation;
    ElapsedTime m_sinceStartedConfigMode;
    ElapsedTime m_marqueeMovement;

    uint8_t m_colorWheelPos{0};
    uint32_t m_currentColor{0};
    ConfigMode_e m_configMode{CONF_MODE_NORMAL};
    String m_configMessage;

    int m_marqueePos{0};

  public:
    Clock(Display& display, Rtc& rtc, Settings& settings)
        : Menu(display, settings), m_rtc(rtc) {
        LoadSettings();
    }

    virtual void Update() {
        uint32_t color = Display::ColorWheel(m_colorWheelPos);
        if (m_display.GetBrightness() < MIN_BRIGHTNESS_DEFAULT) {
            // if the user wants things really dim, we can do that. but we still
            // need the config menu and time menus to work properly, so we dim
            // via this method
            color = Display::ScaleBrightness(color, 0.6f);
        }

        m_currentColor = color;

        if (m_sinceStartedConfigMode.Ms() > 2000) {
            if (m_configMode == CONF_MODE_ANIM) {
                m_display.ScrollHorizontal(WIDTH, SCROLL_LEFT);
            }
            m_configMode = CONF_MODE_NORMAL;
        }

        switch (m_configMode) {
            case CONF_MODE_ANIM:
                m_display.Clear();
                m_display.DrawText(
                    1 - (m_sinceStartedConfigMode.Ms() / MARQUEE_DELAY_MS),
                    m_configMessage, m_currentColor);
                break;

            case CONF_MODE_COLORWHEEL:  // fall through
            case CONF_MODE_NORMAL:
                switch (m_animMode) {
                    case ANIM_MODE_NORMAL:
                    case ANIM_MODE_SHIMMER:
                    case ANIM_MODE_RAINBOW:
                        DrawNormal();
                        break;

                    case ANIM_MODE_MARQUEE:
                    case ANIM_MODE_MARQUEE_RAINBOW:
                        DrawMarquee();
                        break;

                    case ANIM_MODE_BINARY:
                    case ANIM_MODE_BINARY_SHIMMER:
                        DrawBinary();
                        break;

                    default:
                        break;
                }
                break;

            default:
                break;
        }

        DrawAnalog(color);

        CheckIfWaitingToSaveSettings();
    }

    virtual bool Up(const Button::Event_e evt) {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            switch (m_configMode) {
                case CONF_MODE_NORMAL:  // fall through
                case CONF_MODE_COLORWHEEL:
                    m_configMode = CONF_MODE_COLORWHEEL;
                    m_colorWheelPos -= 4;
                    m_settings[F("COLR")] = m_colorWheelPos;
                    break;

                case CONF_MODE_ANIM:
                    if (--m_animMode == -1) {
                        m_animMode = TOTAL_ANIM_MODES - 1;
                        m_settings[F("MODE")] = m_animMode;
                    }
                    SetMode();
                    break;

                default:
                    break;
            }

            m_sinceStartedConfigMode.Reset();
            PrepareToSaveSettings();
        }
        return true;
    }

    virtual bool Down(const Button::Event_e evt) {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            switch (m_configMode) {
                case CONF_MODE_NORMAL:  // fall through
                case CONF_MODE_ANIM:
                    m_configMode = CONF_MODE_ANIM;
                    if (++m_animMode == TOTAL_ANIM_MODES) {
                        m_animMode = ANIM_MODE_NORMAL;
                        m_settings[F("MODE")] = m_animMode;
                    }
                    SetMode();
                    break;

                case CONF_MODE_COLORWHEEL:
                    m_colorWheelPos += 4;
                    m_settings[F("COLR")] = m_colorWheelPos;
                    break;

                default:
                    break;
            }

            m_sinceStartedConfigMode.Reset();
            PrepareToSaveSettings();
        }
        return true;
    }

    virtual void Activate() override { SetMode(false); }
    virtual void Hide() override { m_display.GetPixels().clearColorOverride(); }

    virtual bool ShouldTimeout() override { return false; }

  private:
    void SetMode(const bool showMessage = true) {
        using namespace std::placeholders;
        m_display.GetPixels().clearColorOverride();
        m_marqueePos = WIDTH;

        String message;
        switch (m_animMode) {
            case ANIM_MODE_NORMAL:
                m_configMessage = F("NORMAL");
                break;

            case ANIM_MODE_SHIMMER:
                m_display.GetPixels().setColorOverride(
                    std::bind(&Clock::AddShimmerEffect, this, _1, _2));
                m_configMessage = F("SHIMMER");
                break;

            case ANIM_MODE_RAINBOW:
                m_display.GetPixels().setColorOverride(
                    std::bind(&Clock::AddRainbowEffect, this, _1, _2));
                m_configMessage = F("RAINBOW");
                break;

            case ANIM_MODE_MARQUEE:
                m_configMessage = F("MARQUEE");
                break;

            case ANIM_MODE_MARQUEE_RAINBOW:
                m_display.GetPixels().setColorOverride(
                    std::bind(&Clock::AddRainbowEffect, this, _1, _2));
                m_configMessage = F("MARQUEE");
                break;

            case ANIM_MODE_BINARY:
                m_configMessage = F("BINARY");
                break;

            case ANIM_MODE_BINARY_SHIMMER:
                m_display.GetPixels().setColorOverride(
                    std::bind(&Clock::AddShimmerEffect, this, _1, _2));
                m_configMessage = F("BIN SHIM");
                break;

            default:
                m_configMessage = F("MODE ") + String(m_animMode);
                break;
        }

        if (!showMessage) {
            m_configMode = CONF_MODE_NORMAL;
        }
    }

    void DrawNormal() {
        DrawClockDigits(m_currentColor);
        DrawSeparator(8, m_currentColor);
        DrawWiFiStatus();
    }

    void DrawClockDigits(const uint32_t color) {
        m_display.Clear();

        char text[10];
        if (m_settings[F("24HR")] == F("ON")) {
            sprintf(text, "%02d", m_rtc.Hour());
        } else {
            sprintf(text, "%2d", m_rtc.Hour());
        }
        m_display.DrawText(0, text, color);
        sprintf(text, "%02d", m_rtc.Minute());
        m_display.DrawText(10, text, color);
    }

    void DrawMarquee() {
        m_display.Clear();
        char text[20];
        if (m_settings[F("24HR")] == F("ON")) {
            sprintf(text, "%02d:%02d:%02d", m_rtc.Hour(), m_rtc.Minute(),
                    m_rtc.Second());
        } else {
            sprintf(text, "%2d:%02d:%02d", m_rtc.Hour(), m_rtc.Minute(),
                    m_rtc.Second());
        }
        int length = m_display.DrawText(m_marqueePos, text, m_currentColor) + 1;
        if (m_marqueeMovement.Ms() > MARQUEE_DELAY_MS) {
            m_marqueeMovement.Reset();
            if (--m_marqueePos <= -(length * 1.5)) {
                m_marqueePos = WIDTH;
            }
        }
    }

    void DrawBinary() {
        m_display.Clear();
        DrawBinaryDigit(1, m_rtc.Hour());
        DrawSeparator(5, m_currentColor);
        DrawBinaryDigit(7, m_rtc.Minute());
        DrawSeparator(11, m_currentColor);
        DrawBinaryDigit(13, m_rtc.Second());
        DrawWiFiStatus();
    }

    void DrawBinaryDigit(const int x, const uint8_t value) {
        uint32_t onCol = m_currentColor;
        uint32_t offCol = Display::ScaleBrightness(m_currentColor, 0.2f);
        // 6th bit
        m_display.DrawPixel(x + 0, 0, value & 0b0010'0000 ? onCol : offCol);
        m_display.DrawPixel(x + 0, 1, value & 0b0010'0000 ? onCol : offCol);
        m_display.DrawPixel(x + 0, 2, value & 0b0010'0000 ? onCol : offCol);
        m_display.DrawPixel(x + 0, 3, value & 0b0010'0000 ? onCol : offCol);
        m_display.DrawPixel(x + 0, 4, value & 0b0010'0000 ? onCol : offCol);
        // 5th bit
        m_display.DrawPixel(x + 1, 0, value & 0b0001'0000 ? onCol : offCol);
        m_display.DrawPixel(x + 2, 0, value & 0b0001'0000 ? onCol : offCol);
        // 4th bit
        m_display.DrawPixel(x + 1, 1, value & 0b0000'1000 ? onCol : offCol);
        m_display.DrawPixel(x + 2, 1, value & 0b0000'1000 ? onCol : offCol);
        // 3rd bit
        m_display.DrawPixel(x + 1, 2, value & 0b0000'0100 ? onCol : offCol);
        m_display.DrawPixel(x + 2, 2, value & 0b0000'0100 ? onCol : offCol);
        // 2nd bit
        m_display.DrawPixel(x + 1, 3, value & 0b0000'0010 ? onCol : offCol);
        m_display.DrawPixel(x + 2, 3, value & 0b0000'0010 ? onCol : offCol);
        // 1st bit
        m_display.DrawPixel(x + 1, 4, value & 0b0000'0001 ? onCol : offCol);
        m_display.DrawPixel(x + 2, 4, value & 0b0000'0001 ? onCol : offCol);
    }

    void DrawAnalog(uint32_t color) {
        if (m_configMode == CONF_MODE_COLORWHEEL) {
            m_display.DrawColorWheel(m_colorWheelPos);
        } else {
            m_display.ClearRoundLEDs(
                m_settings[F("CLKB")] == F("ON") ? DARK_GRAY : BLACK);

            uint32_t secondColor =
                Display::ScaleBrightness(m_currentColor, 0.6f);
            uint32_t hourAndMinuteColor = m_currentColor;

            if (m_display.GetBrightness() < MIN_BRIGHTNESS_DEFAULT) {
                // the red LEDs are the only ones still visible at low
                // brightness with most smart LEDs, so since we can't be sure
                // that the currentColor is anything brighter than DARK_GRAY
                // (the analog clock background - CLKB), we can just inverse
                // the clock face, lighting up every LED that isn't the current
                // hour/minute/second position
                m_display.ClearRoundLEDs(DARK_GRAY);

                // now, turn off the LEDs to indicate time
                secondColor = BLACK;
                hourAndMinuteColor = BLACK;
            }

            // the forceColor parameter is used here for the secondHand so that
            // it doesn't flicker
            m_display.DrawSecondLEDs(m_rtc.Second(), secondColor, true);
            m_display.DrawHourLED(m_rtc.Hour12(), hourAndMinuteColor);
            m_display.DrawMinuteLED(m_rtc.Minute(), hourAndMinuteColor);
        }
    }

    void DrawSeparator(const int x, uint32_t color) {
        const float ms = m_rtc.Millis() / 1000.0f;
        const int transitionColor = Display::ScaleBrightness(
            m_currentColor,
            0.2f + (m_rtc.Second() % 2 ? ms : 1.0f - ms) * 0.8f);

        // force the pixels to be drawn in the edited color
        m_display.DrawPixel(x, 1, transitionColor, true);
        m_display.DrawPixel(x, 3, transitionColor, true);
    }

    void AddShimmerEffect(const uint16_t pixelNum, uint32_t& color) {
        static uint8_t jump = 0;
        static int pixelsChanged = 0;
        static float multiplier = 1.0f;

        // stop, it's
        const float SHIMMER_TIME = 600.0f;

        if (m_sinceLastAnimation.Ms() > SHIMMER_TIME) {
            m_sinceLastAnimation.Reset();
            jump--;
        }

        if (pixelNum == 0) {
            pixelsChanged = 0;
        }

        if (color != BLACK && pixelNum < WIDTH * HEIGHT) {
            if ((++pixelsChanged + jump) % 7 == 0) {
                multiplier = m_sinceLastAnimation.Ms() / SHIMMER_TIME;
                if (m_sinceLastAnimation.Ms() < SHIMMER_TIME / 2) {
                    multiplier = 1.0f - multiplier;
                }

                color =
                    Display::ScaleBrightness(color, 0.1f + (multiplier * 0.7f));
            }
        }
    }

    void AddRainbowEffect(const uint16_t pixelNum, uint32_t& color) {
        static uint8_t baseColor = 128;
        static uint8_t curColor = 0;

        if (m_sinceLastAnimation.Ms() > 50) {
            m_sinceLastAnimation.Reset();
            baseColor--;
        }

        if (pixelNum == 0) {
            curColor = 0;
        }

        // DARK_GRAY is the color of the analog clock ring
        if (color != BLACK && color != DARK_GRAY) {
            curColor += 4;
            m_currentColor = color = Display::ColorWheel(baseColor + curColor);
        }
    }

    void DrawWiFiStatus() {
        const bool isWiFiEnabled =
            m_settings.containsKey("WIFI") && m_settings[F("WIFI")] != F("OFF");
        const bool isWiFiLEDStatusEnabled =
            m_settings.containsKey("WLED") && m_settings[F("WLED")] != F("OFF");
        if (isWiFiEnabled && isWiFiLEDStatusEnabled) {
            if (WiFi.isConnected()) {
                if (m_animMode < ANIM_MODE_BINARY) {
                    // it's fitting that 42 is exactly the right place for
                    // the WiFi status LED, and for that, we'll call
                    // setPixelColor directly.
                    m_display.GetPixels().setPixelColor(42, m_currentColor);
                } else {
                    m_display.DrawPixel(5, 2, m_currentColor);
                    m_display.DrawPixel(11, 2, m_currentColor);
                }
            } else {
                if ((m_rtc.Millis() > 300 && m_rtc.Millis() < 400) ||
                    (m_rtc.Millis() > 500 && m_rtc.Millis() < 600)) {
                    uint32_t heartBeatColor =
                        Display::ScaleBrightness(m_currentColor, 0.5f);
                    if (m_animMode < ANIM_MODE_BINARY) {
                        m_display.GetPixels().setPixelColor(42, heartBeatColor);
                    } else {
                        m_display.DrawPixel(5, 2, heartBeatColor);
                        m_display.DrawPixel(11, 2, heartBeatColor);
                    }
                }
            }
        }
    }

    void PrepareToSaveSettings() {
        m_shouldSaveSettings = true;
        m_waitingToSaveSettings.Reset();
    }

    void CheckIfWaitingToSaveSettings() {
        if (m_shouldSaveSettings && m_waitingToSaveSettings.Ms() >= 2000) {
            // wait until 2 seconds after changing the color or mode to save
            // settings, since the user can quickly change either one and we
            // want to save flash write cycles
            m_settings.Save();
            m_shouldSaveSettings = false;
        }
    }

    void LoadSettings() {
        if (!m_settings.containsKey(F("WLED"))) {
            m_settings[F("WLED")] = F("ON");
        }

        if (!m_settings.containsKey(F("COLR"))) {
            m_settings[F("COLR")] = m_colorWheelPos;
        } else {
            m_colorWheelPos = m_settings[F("COLR")].as<uint32_t>();
        }

        if (!m_settings.containsKey(F("MODE"))) {
            m_settings[F("MODE")] = m_animMode;
        } else {
            m_animMode = m_settings[F("MODE")].as<int>();
            if (m_animMode >= TOTAL_ANIM_MODES) {
                m_animMode = ANIM_MODE_NORMAL;
            }
        }

        if (!m_settings.containsKey(F("CLKB"))) {
            m_settings[F("CLKB")] = F("ON");
        }
    }
};