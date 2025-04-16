#include "serial.h"
#include "config.h"

serialib cameraSerial;

std::string decToHex(int decimalNumber) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << std::hex << decimalNumber;
    std::string hexString = ss.str();

    std::string result = "";
    for (char c : hexString) {
        result += "0";
        result += c;
    }

    // Pad result to be 8 characters
    std::string paddedResult = std::string(8 - result.length(), '0') + result;
    return paddedResult;
}

bool initializeSerial() {
    // Try to find available serial ports
    const char* serialPorts[] = {"/dev/ttyUSB0", "/dev/ttyACM0", "/dev/ttyS0"};

    for (const char* port : serialPorts) {
        if (cameraSerial.openDevice(port, 9600) == 1) {
            std::cout << "Serial port opened: " << port << std::endl;
            cameraSerial.flushReceiver();

            // Send initial command
            const char* initCmd = "8101044700000000FF";

            // Convert hex string to bytes and send
            int len = strlen(initCmd) / 2;
            unsigned char buffer[len];

            for (int i = 0; i < len; i++) {
                char byteStr[3] = {initCmd[i*2], initCmd[i*2+1], 0};
                buffer[i] = (unsigned char)strtol(byteStr, NULL, 16);
            }

            cameraSerial.writeBytes(buffer, len);
            serialInitialized = true;
            return true;
        }
    }

    std::cerr << "Failed to open any serial port" << std::endl;
    return false;
}

void sendZoomCommand(int level) {
    if (!serialInitialized) {
        if (!initializeSerial()) {
            setLogMessage("Serial error");
            return;
        }
    }

    std::string hexLevel = decToHex(level);
    std::string cmdStr = "81010447" + hexLevel + "FF";

    std::cout << "Sending zoom command: " << cmdStr << std::endl;

    // Convert hex string to bytes and send
    int len = cmdStr.length() / 2;
    unsigned char buffer[len];

    for (int i = 0; i < len; i++) {
        char byteStr[3] = {cmdStr[i*2], cmdStr[i*2+1], 0};
        buffer[i] = (unsigned char)strtol(byteStr, NULL, 16);
    }

    cameraSerial.writeBytes(buffer, len);
}

void sendICRCommand(bool enable) {
    if (!serialInitialized) {
        if (!initializeSerial()) {
            setLogMessage("Serial error");
            return;
        }
    }

    // Command to enable ICR: 81 01 04 01 02 FF
    // Command to disable ICR: 81 01 04 01 03 FF
    std::string cmdStr = enable ? "8101040102FF" : "8101040103FF";

    std::cout << "Sending ICR command: " << cmdStr << " (Enable: " << enable << ")" << std::endl;

    // Convert hex string to bytes and send
    int len = cmdStr.length() / 2;
    unsigned char buffer[len];

    for (int i = 0; i < len; i++) {
        char byteStr[3] = {cmdStr[i*2], cmdStr[i*2+1], 0};
        buffer[i] = (unsigned char)strtol(byteStr, NULL, 16);
    }

    cameraSerial.writeBytes(buffer, len);
    setLogMessage(std::string("ICR Mode: ") + (enable ? "ON" : "OFF"));
}

void sendIRCorrectionCommand(bool enable) {
    if (!serialInitialized) {
        if (!initializeSerial()) {
            setLogMessage("Serial error");
            return;
        }
    }

    // Command to enable IR Correction: 81 01 04 11 01 FF
    // Command to disable IR Correction: 81 01 04 11 00 FF
    std::string cmdStr = enable ? "8101041101FF" : "8101041100FF";

    std::cout << "Sending IR Correction command: " << cmdStr << " (Enable: " << enable << ")" << std::endl;

    // Convert hex string to bytes and send
    int len = cmdStr.length() / 2;
    unsigned char buffer[len];

    for (int i = 0; i < len; i++) {
        char byteStr[3] = {cmdStr[i*2], cmdStr[i*2+1], 0};
        buffer[i] = (unsigned char)strtol(byteStr, NULL, 16);
    }

    cameraSerial.writeBytes(buffer, len);
    setLogMessage(std::string("IR Correction: ") + (enable ? "ON" : "OFF"));
}

void zoomIn() {
    appConfig.loadConfig();
    int ZOOM_LEVEL = appConfig.getInt("ZOOM_LEVEL", 64);
    if (zoomLevel < maxZoomLevel) {
        zoomLevel += ZOOM_LEVEL;
        sendZoomCommand(zoomLevel);
        
        // Calculate zoom multiplier (assuming 12x is max zoom at 16384)
        float zoomMultiplier = (zoomLevel / 16384.0f) * 30.0f;
        
        // Format with 1 decimal place
        std::stringstream stream;
        stream << std::fixed << std::setprecision(1) << zoomMultiplier;
        
        cout << zoomMultiplier << endl;
        setLogMessage("Zoom: " + stream.str() + "x");
    }
}

void zoomOut() {
    appConfig.loadConfig();
    int ZOOM_LEVEL = appConfig.getInt("ZOOM_LEVEL", 64);
    if (zoomLevel > 0) {
        zoomLevel -= ZOOM_LEVEL;
        sendZoomCommand(zoomLevel);
        
        // Calculate zoom multiplier (assuming 12x is max zoom at 16384)
        float zoomMultiplier = (zoomLevel / 16384.0f) * 30.0f;
        
        // Format with 1 decimal place
        std::stringstream stream;
        stream << std::fixed << std::setprecision(1) << zoomMultiplier;
        
        cout << zoomMultiplier << endl;
        setLogMessage("Zoom: " + stream.str() + "x");
    }
}
