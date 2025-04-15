#ifndef BACKGROUND_SUBTRACTION_H
#define BACKGROUND_SUBTRACTION_H

#include "common.h"

// Initialize background subtraction parameters
void initBgSubControls(int controlsX, int controlsY, int controlsWidth, int controlsHeight);

// Update background subtraction controls toggle position
void updateBgSubControlsTogglePosition(bool showControls);

// Process background subtraction on current frame
void processBackgroundSubtraction(Mat& frame);

// Draw background subtraction controls on UI
void drawBgSubControls(Mat& uiFrame, bool bgSubtractionActive);

// Draw dual range slider for background subtraction controls
void drawDualRangeSlider(Mat& img, Rect sliderRect, int& minValue, int& maxValue, 
                        int minLimit, int maxLimit, const string& label);

// Handle dual slider interaction
bool handleDualSliderInteraction(int mouseX, int mouseY, Rect sliderRect, 
                                int& minValue, int& maxValue, int minLimit, int maxLimit);

// Find similar detection point
Point findSimilarPoint(const Point& newPoint, const std::vector<Point>& existingPoints);

// Draw detection results on frame
void drawDetectionResults(Mat& frame);

// Save top detection points to file
void saveTopDetectionPoints(int topCount);

#endif // BACKGROUND_SUBTRACTION_H
