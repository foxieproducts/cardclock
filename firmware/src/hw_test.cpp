#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <user_interface.h>

#include "button.hpp"
#include "display.hpp"
#include "rtc.hpp"

#define APSSID "foxie"
#define APPSK "schwarz12345"  // this is a spaceballs joke
#define FIRMWARE_LOCATION "https://foxieproducts.com/firmware/cardclock"

void ShowTestStatus();
void DownloadFirmware();

Button g_btnUp(PIN_BTN_UP, INPUT_PULLUP);
Button g_btnDown(PIN_BTN_DOWN, INPUT);
Button g_btnLeft(PIN_BTN_LEFT, INPUT_PULLUP);
Button g_btnRight(PIN_BTN_RIGHT, INPUT_PULLUP);

Display g_display;
Rtc g_rtc;
ESP8266WiFiMulti g_WiFiMulti;

struct TestResults {
    bool up{false};
    bool down{false};
    bool left{false};
    bool right{false};
    bool rtc{false};
    bool Done() { return up && down && left && right && rtc; }
} g_results;

void setup() {
    // hold RIGHT button on boot to erase WiFi settings
    if (digitalRead(PIN_BTN_RIGHT) == LOW) {
        pinMode(LED_BUILTIN, OUTPUT);
        digitalWrite(LED_BUILTIN, LOW);
        ESP.eraseConfig();
        delay(500);
        digitalWrite(LED_BUILTIN, HIGH);
    }

    WiFi.mode(WIFI_STA);
    g_WiFiMulti.addAP(APSSID, APPSK);

    g_btnUp.config.handlerFunc = [&](const Button::Event_e evt) {
        if (evt == Button::PRESS) {
            g_results.up = true;
        }
    };

    g_btnDown.config.handlerFunc = [&](const Button::Event_e evt) {
        if (evt == Button::PRESS) {
            g_results.down = true;
        }
    };

    g_btnLeft.config.handlerFunc = [&](const Button::Event_e evt) {
        if (evt == Button::PRESS) {
            g_results.left = true;

            Rtc_Pcf8563 tempRtc;
            tempRtc.setTime(13, 37, 42);
        }
    };

    g_btnRight.config.handlerFunc = [&](const Button::Event_e evt) {
        if (evt == Button::PRESS) {
            g_results.right = true;
        }
    };
}

void loop() {
    if (g_results.Done()) {
        DownloadFirmware();
        return;
    }

    Button::Update();

    g_rtc.Update();
    // this time is set when the left button is pressed
    if (g_rtc.Hour() == 13 && g_rtc.Minute() == 37 && g_rtc.Second() >= 42 &&
        g_rtc.Second() <= 43) {
        g_results.rtc = true;
    }

    ShowTestStatus();

    if (g_results.Done()) {
        g_display.Clear(BLACK, true);
        g_display.DrawText(1, "DNLD", ORANGE);
        g_display.Show();
    }

    delay(10);
}

void ShowTestStatus() {
    static uint8_t colorWheelPos = 128;
    g_display.Clear(
        Display::ScaleBrightness(Display::ColorWheel(colorWheelPos++), 0.5f),
        true);

    // g_display.DrawText(0, String(g_display.GetBrightness()), WHITE);
    g_display.DrawText(0, String(g_rtc.Second()), WHITE);

    g_display.DrawPixel(17 * 1 + 14, g_results.up ? GREEN : WHITE);
    g_display.DrawPixel(17 * 3 + 14, g_results.down ? GREEN : WHITE);
    g_display.DrawPixel(17 * 2 + 13, g_results.left ? GREEN : WHITE);
    g_display.DrawPixel(17 * 2 + 15, g_results.right ? GREEN : WHITE);
    g_display.DrawPixel(17 * 2 + 14, g_results.rtc ? GREEN : WHITE);

    g_display.Update();
}

void FWInstallComplete() {
    g_display.Clear();
    g_display.DrawText(1, "FLSH", ORANGE);
    g_display.Show();
    g_rtc.SetClockToZero();
}

void FWInstallProgress(int cur, int total) {
    g_display.DrawPixel(FIRST_HOUR_LED + map(cur, 0, total, 0, 23), BLUE);
    g_display.Clear();
    g_display.DrawText(3, String(map(cur, 0, total, 0, 100)) + "%", PURPLE);
    g_display.Show();
}

void FWInstallError(int err) {
    g_display.DrawTextScrolling(ESPhttpUpdate.getLastErrorString(), RED, 25);
}

void DownloadFirmware() {
    if (g_WiFiMulti.run() == WL_CONNECTED) {
        WiFiClientSecure client;
        client.setInsecure();

        ESPhttpUpdate.onEnd(FWInstallComplete);
        ESPhttpUpdate.onProgress(FWInstallProgress);
        ESPhttpUpdate.onError(FWInstallError);
        ESPhttpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        ESPhttpUpdate.rebootOnUpdate(true);

        // if successful, this will reboot before returning
        ESPhttpUpdate.update(client, FIRMWARE_LOCATION);

        // if we get here, we failed
        g_display.DrawText(1, "FAIL", 0);
        g_display.Show();
        while (true) {
            yield();
        }
    }
}
