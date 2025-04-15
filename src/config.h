#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

// Configuration class to handle loading and saving settings
class Config {
private:
    string configFilePath;
    map<string, string> settings;

public:
//    Config(const string& filePath = "/home/kng/Drip/config.ini") : configFilePath(filePath) {
    Config(const string& filePath = "./config.ini") : configFilePath(filePath) {
        loadConfig();
    }

    bool loadConfig() {
        settings.clear();
        ifstream configFile(configFilePath);

        if (!configFile.is_open()) {
            // If file doesn't exist, create a default config
            createDefaultConfig();
            return false;
        }

        string line;
        while (getline(configFile, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }

            size_t delimiterPos = line.find('=');
            if (delimiterPos != string::npos) {
                string key = line.substr(0, delimiterPos);
                string value = line.substr(delimiterPos + 1);

                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                settings[key] = value;
            }
        }

        configFile.close();
        return true;
    }

    bool saveConfig() {
        ofstream configFile(configFilePath);
        if (!configFile.is_open()) {
            cerr << "Error: Could not open config file for writing: " << configFilePath << endl;
            return false;
        }

        configFile << "# Water Dripping Investigation Recording Tools Configuration\n";
        configFile << "# Automatically generated - you can edit this file\n\n";

        for (const auto& setting : settings) {
            configFile << setting.first << " = " << setting.second << "\n";
        }

        configFile.close();
        return true;
    }

    void createDefaultConfig() {
        // Set default values
        settings["DISPLAY_WIDTH"] = "1280";
        settings["DISPLAY_HEIGHT"] = "800";
        settings["CAMERA_WIDTH"] = "1280";
        settings["CAMERA_HEIGHT"] = "720";
        settings["EXPORT_DEST_DIR"] = "./recordings/";
        settings["KEEP_ORIGINAL_FILES"] = "true";
        settings["RECORDING_FPS"] = "30.0";
        settings["SHOW_FPS"] = "false";
        settings["SHOW_NAV_BAR"] = "true";
        settings["ZOOM_LEVEL"] = "64";
        settings["FULL_SCREEN"] = "true";
        settings["UPPERBOUND"] = "200";
        settings["LOWERBOUND"] = "0";
        settings["MIN_CONTOUR_AREA"] = "0";
        settings["MAX_CONTOUR_AREA"] = "300";
        settings["SHOW_BG_SUB_CONTROLS"] = "true";
        settings["CONSECUTIVE_FRAMES"] = "3";

        // Save the default configuration
        saveConfig();

        cout << "Created default configuration file at: " << configFilePath << endl;
    }

    // Get value as string
    string getString(const string& key, const string& defaultValue = "") const {
        auto it = settings.find(key);
        if (it != settings.end()) {
            return it->second;
        }
        return defaultValue;
    }

    // Get value as int
    int getInt(const string& key, int defaultValue = 0) const {
        auto it = settings.find(key);
        if (it != settings.end()) {
            try {
                return stoi(it->second);
            } catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    // Get value as double
    double getDouble(const string& key, double defaultValue = 0.0) const {
        auto it = settings.find(key);
        if (it != settings.end()) {
            try {
                return stod(it->second);
            } catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    // Get value as bool
    bool getBool(const string& key, bool defaultValue = false) const {
        auto it = settings.find(key);
        if (it != settings.end()) {
            string value = it->second;
            // Convert to lowercase for case-insensitive comparison
            transform(value.begin(), value.end(), value.begin(), ::tolower);
            if (value == "true" || value == "yes" || value == "1") {
                return true;
            } else if (value == "false" || value == "no" || value == "0") {
                return false;
            }
        }
        return defaultValue;
    }
};

#endif // CONFIG_H
