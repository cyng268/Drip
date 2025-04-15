#ifndef UI_HELPERS_H
#define UI_HELPERS_H

#include "common.h"

// Draw progress bar
void drawProgressBar(Mat& img, Rect rect, int value, int maxValue, Scalar barColor);

// Create custom arrow images for nav bar
void createArrowImages();

// Update toggle button position
void updateToggleButtonPosition(int windowWidth);

// Draw window control buttons (minimize/close)
void drawWindowControls(Mat& img);

#endif // UI_HELPERS_H
