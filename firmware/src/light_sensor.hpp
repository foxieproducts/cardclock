#pragma once
#include <Arduino.h>         // for map()
#include <stdint.h>          // for uint16_t and others
#include <user_interface.h>  // for ESP-specific API calls

/**
 * This class uses the ESP API to read multiple samples from the ADC.
 * The main reason this is used instead of analogRead() is because
 * analogRead() is unstable on the ESP8266 and crashes often (???)
 * */
class LightSensor {
  public:
    enum {
        ADC_SAMPLES = 8,
        ADC_CLOCK_DIVIDER = 8,

        MIN_BRIGHTNESS = 10,
        MAX_BRIGHTNESS = 150,
        MAX_JITTER = 3,
    };

  private:
    uint16_t m_samples[ADC_SAMPLES] = {0};
    uint16_t m_cur = 0;

  public:
    LightSensor() { Get(); }

    uint16_t Get() {
        ReadADC();

        uint32_t average = 0;
        for (int i = 0; i < ADC_SAMPLES; i++) {
            average += m_samples[i];
        }

        return Constrain(average / ADC_SAMPLES);
    }

  private:
    uint16_t Constrain(uint16_t value) {
        value = map(value, 10, 600, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
        if (value < m_cur - MAX_JITTER || value > m_cur + MAX_JITTER) {
            m_cur = constrain(value, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
        }

        if (m_cur <= MIN_BRIGHTNESS + MAX_JITTER) {
            m_cur = MIN_BRIGHTNESS;
        } else if (m_cur >= MAX_BRIGHTNESS - MAX_JITTER) {
            m_cur = MAX_BRIGHTNESS;
        }

        return m_cur;
    }

    void ReadADC() {
        for (int i = 0; i < ADC_SAMPLES; i++) {
            system_adc_read_fast(m_samples, ADC_SAMPLES, ADC_CLOCK_DIVIDER);
        }
    }

    // This doesn't seem to be necessary
    void ReadADCWithoutInterrupts() {
        system_soft_wdt_stop();
        ets_intr_lock();
        noInterrupts();

        ReadADC();

        interrupts();
        ets_intr_unlock();
        system_soft_wdt_restart();
    }
};