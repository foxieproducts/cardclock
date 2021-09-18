#pragma once
#include <Arduino.h>         // for map()
#include <stdint.h>          // for uint16_t and others
#include <user_interface.h>  // for ESP-specific API calls

#include "elapsed_time.hpp"

/**
 * This class solves two problems:
 * 1. It uses the ESP API to read multiple samples from the ADC.
 *    The main reason this is used instead of analogRead() is because
 *    analogRead() is unstable on the ESP8266 and crashes often (???)
 * 2. It provides averaging so that light changes happen gradually,
 *    which is also useful to eliminate LED-lamp-PWM-flicker
 * */
class LightSensor {
  public:
    enum {
        ADC_SAMPLES = 8,
        ADC_CLOCK_DIVIDER = 8,
        MIN_SENSOR_VAL = 10,  // affected by side lighting from the display LEDs
                              // below this level
        MAX_SENSOR_VAL = 100,
        JITTER = 3,

        HISTORY_SIZE = 80,
    };

  private:
    uint16_t* m_samples{nullptr};
    uint16_t* m_history{nullptr};
    size_t m_historyPos{0};

    ElapsedTime m_timeSinceBrightnessSet;
    float m_curBrightness{0.0f};

  public:
    LightSensor() {
        m_samples = (uint16_t*)zalloc(ADC_SAMPLES * sizeof(uint16_t));
        m_history = (uint16_t*)zalloc(HISTORY_SIZE * sizeof(uint16_t));

        for (size_t i = 0; i < HISTORY_SIZE; ++i) {
            m_history[i] = GetCurrentADCValue();
        }
        m_curBrightness = GetHistoryMean();
    }

    size_t Get() {
        m_history[m_historyPos] = GetCurrentADCValue();
        if (m_historyPos++ == HISTORY_SIZE) {
            m_historyPos = 0;
        }

        uint16_t mean = GetHistoryMean();
        if (m_timeSinceBrightnessSet.Ms() > 40) {
            if (mean < m_curBrightness - JITTER ||
                mean > m_curBrightness + JITTER) {
                // move quickly toward current brightness
                m_curBrightness = (mean + m_curBrightness) / 2;
            } else {
                // move slowly toward current brightness
                if (m_curBrightness >= MAX_SENSOR_VAL - JITTER) {
                    // increase the mean slightly so we can get to
                    // MAX_SENSOR_VAL
                    mean++;
                }
                const float val = (99.0f * m_curBrightness) + mean;
                m_curBrightness = val / 100.0f;
            }
            m_timeSinceBrightnessSet.Reset();
        }

        return m_curBrightness;
    }

  private:
    size_t GetHistoryMean() {
        size_t average = 0;
        for (size_t i = 0; i < HISTORY_SIZE; i++) {
            average += m_history[i];
        }

        return average / HISTORY_SIZE;
    }

    // get a smoothed, bounded value from the sensor
    size_t GetCurrentADCValue() {
        system_soft_wdt_stop();
        ets_intr_lock();
        noInterrupts();

        system_adc_read_fast(m_samples, ADC_SAMPLES, ADC_CLOCK_DIVIDER);

        interrupts();
        ets_intr_unlock();
        system_soft_wdt_restart();

        size_t mean = 0;
        for (size_t i = 0; i < ADC_SAMPLES; i++) {
            mean += m_samples[i];
        }
        return constrain((mean / ADC_SAMPLES), MIN_SENSOR_VAL, MAX_SENSOR_VAL);
    }
};