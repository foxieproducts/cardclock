#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
enum {
    MAX_SETTINGS_SIZE = 1024,
};

class Settings : public DynamicJsonDocument {
  private:
    String m_filename{"/config.json"};

    bool m_loaded{false};

  public:
    Settings() : DynamicJsonDocument(MAX_SETTINGS_SIZE) {
        LittleFS.begin();
        Load();
    }

    Settings(const String filename) : Settings() {
        m_filename = filename;
        Load();
    }

    bool Load() {
        File file = LittleFS.open(m_filename, "r");
        m_loaded = false;
        if (file.size() > MAX_SETTINGS_SIZE) {
            return false;
        }

        DeserializationError err = deserializeJson(*this, file);
        file.close();

        m_loaded = err ? false : true;
        return m_loaded;
    }

    bool Save() {
        LittleFS.remove(m_filename);
        File file = LittleFS.open(m_filename, "w+");
        if (!file) {
            return false;
        }

        const bool success = serializeJson(*this, file) > 0;
        file.close();

        return success;
    }

    int AsInt(const char* key) {
        const String val = getMember(key);
        return val.toInt();
    }

    bool IsLoaded() { return m_loaded; }
};