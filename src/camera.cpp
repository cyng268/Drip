#include "camera.h"

double calculateFPS(system_clock::time_point& previousFrameTime) {
    auto currentFrameTime = system_clock::now();
    duration<double> elapsed = currentFrameTime - previousFrameTime;
    previousFrameTime = currentFrameTime;
    return 1.0 / elapsed.count();
}

string getCurrentTimeStr() {
    auto now = system_clock::now();
    time_t now_c = system_clock::to_time_t(now);
    struct tm* timeinfo = localtime(&now_c);
    char buffer[80];
    strftime(buffer, 80, "%H:%M:%S", timeinfo);
    stringstream ss;
    ss << buffer;
    return ss.str();
}

string getCurrentDateStr() {
    auto now = system_clock::now();
    time_t now_c = system_clock::to_time_t(now);
    struct tm* timeinfo = localtime(&now_c);
    char buffer[80];
    strftime(buffer, 80, "%Y-%m-%d", timeinfo);
    stringstream ss;
    ss << buffer;
    return ss.str();
}

void cameraConfig(VideoCapture* cap) {
    // Configure camera with optimized settings before opening
    cap->open(0, CAP_V4L2);
    if (!cap->isOpened()) {
        cerr << "ERROR: Unable to open the camera" << endl;
        setLogMessage("Error");
        return;
    }

    // Set camera properties
    cap->set(CAP_PROP_FRAME_WIDTH, WIDTH);
    cap->set(CAP_PROP_FRAME_HEIGHT, HEIGHT);
    cout << "Height:" << CAP_PROP_FRAME_HEIGHT << "\n";
    cout << "Width:" << CAP_PROP_FRAME_WIDTH << "\n";

    // Set additional properties after opening
    cap->set(CAP_PROP_BUFFERSIZE, 0); // Use more buffers
    cap->set(CAP_PROP_FPS, 30); // Request 30 FPS

    // Verify if the settings were applied
    cout << "Camera resolution: " << cap->get(CAP_PROP_FRAME_WIDTH) << "x" << cap->get(CAP_PROP_FRAME_HEIGHT) << endl;
    cout << "Camera FPS: " << cap->get(CAP_PROP_FPS) << endl;
    return;
}
