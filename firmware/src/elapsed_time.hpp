#pragma once
#include <Arduino.h>  // for millis()

class ElapsedTime {
  private:
    unsigned long m_millis{0};

  public:
    ElapsedTime() { Reset(); }
    void Reset() { m_millis = millis(); }
    int Ms() { return millis() - m_millis; }

    static void Delay(const int ms) {
        ElapsedTime delayTime;
        while (delayTime.Ms() < ms) {
            delay(ms);
        }
    }
};
