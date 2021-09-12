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
        ADC_SAMPLES = 4,
        ADC_CLOCK_DIVIDER = 8,
        MIN_SENSOR_VAL = 10,
        MAX_SENSOR_VAL = 70,
        MAX_JITTER = 15,

        HISTORY_SIZE = 40,

        RANGE = 100,
    };

  private:
    uint16_t m_samples[ADC_SAMPLES] = {0};
    uint16_t m_history[HISTORY_SIZE] = {0};
    int m_pos{0};

  public:
    LightSensor() {
        // populate history with current value
        int32_t average = GetADCAverage();
        for (int i = 0; i < HISTORY_SIZE; ++i) {
            m_history[i] = average;
        }
    }

    uint16_t Get() {
        int32_t average = GetADCAverage();

        // average a bit more
        if ((uint16_t)average > m_history[m_pos] + MAX_JITTER ||
            (uint16_t)average < m_history[m_pos] - MAX_JITTER || average == 0 ||
            average == 100) {
            m_history[m_pos] = average;
        }
        if (m_pos++ == HISTORY_SIZE) {
            m_pos = 0;
        }

        average = 0;
        for (int i = 0; i < HISTORY_SIZE; i++) {
            average += m_history[i];
        }
        average /= HISTORY_SIZE;

        return average;
    }

  private:
    uint16_t GetADCAverage() {
        ReadADC();
        int32_t average = 0;
        for (int i = 0; i < ADC_SAMPLES; i++) {
            average += m_samples[i];
        }
        average /= ADC_SAMPLES;
        average = map(average, MIN_SENSOR_VAL, MAX_SENSOR_VAL, 0, RANGE);
        average = constrain(average, 0, RANGE);
        return average;
    }

    void ReadADC() {
        system_soft_wdt_stop();
        ets_intr_lock();
        noInterrupts();

        for (int i = 0; i < ADC_SAMPLES; i++) {
            system_adc_read_fast(m_samples, ADC_SAMPLES, ADC_CLOCK_DIVIDER);
        }

        interrupts();
        ets_intr_unlock();
        system_soft_wdt_restart();
    }
};