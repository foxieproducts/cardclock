#pragma once
#include <Arduino.h>  // for millis()
#include <stdint.h>   // for size_t

class ElapsedTime {
  private:
    unsigned long m_millis{0};

  public:
    ElapsedTime() { Reset(); }
    void Reset() { m_millis = millis(); }
    size_t Ms() { return millis() - m_millis; }

    static void Delay(const size_t ms) {
        ElapsedTime delayTime;
        while (delayTime.Ms() < ms) {
            delay(ms);
        }
    }
};
