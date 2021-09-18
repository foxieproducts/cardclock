#pragma once
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "elapsed_time.hpp"

enum {
    MAX_SETTINGS_SIZE = 1024,
};

class Settings : public DynamicJsonDocument {
  private:
    String m_filename;
    bool m_loaded{false};

  public:
    Settings(const String filename = "/config.json")
        : DynamicJsonDocument(MAX_SETTINGS_SIZE), m_filename(filename) {
        LittleFS.begin();
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

    bool Save(bool force = false) {
        if (IsSameAsSavedFile() && !force) {
            return true;
        }

        LittleFS.remove(m_filename);
        File file = LittleFS.open(m_filename, "w+");
        if (!file) {
            return false;
        }

        const bool success = serializeJson(*this, file) > 0;
        file.close();

        return success;
    }
    bool IsLoaded() { return m_loaded; }

  private:
    bool IsSameAsSavedFile() {
        File currentFile = LittleFS.open(m_filename, "r");
        if (currentFile.size() <= MAX_SETTINGS_SIZE) {
            DynamicJsonDocument currentDoc(MAX_SETTINGS_SIZE);
            DeserializationError err = deserializeJson(currentDoc, currentFile);
            currentFile.close();

            if (!err && currentDoc == *this) {
                return true;  // file is the same
            }
        }
        return false;  // file is different or non-existent/parsable
    }
};