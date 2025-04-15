#ifndef NAVIGATION_BAR_H
#define NAVIGATION_BAR_H

#include "common.h"

// Draw navigation bar
void drawNavigationBar(Mat& img, int windowWidth, bool isRecording, bool isProcessing, 
                     bool isZoomInHeld, bool isZoomOutHeld);

// Initialize navigation bar
void initNavigationBar(int windowWidth, int windowHeight);

#endif // NAVIGATION_BAR_H
