#ifndef CAMERA_H
#define CAMERA_H

#include "common.h"

// Configure the camera
void cameraConfig(VideoCapture* cap);

// Calculate current FPS
double calculateFPS(system_clock::time_point& previousFrameTime);

// Get current time as string
string getCurrentTimeStr();

// Get current date as string
string getCurrentDateStr();

#endif // CAMERA_H
