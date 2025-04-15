#ifndef UI_H
#define UI_H

#include "common.h"

struct MouseCallbackData {
    vector<Rect> fileCheckboxRects;
    vector<Rect> fileTextRects;
};

// Original UI functions
void drawProgressBar(Mat& img, Rect rect, int value, int maxValue, Scalar barColor);
void mouseCallback(int event, int x, int y, int flags, void* userdata);
void initializeUI(int windowWidth, int windowHeight);
void createArrowImages();
void updateToggleButtonPosition(int windowWidth);

// New export dialog functions
void createExportDialog(int windowWidth, int windowHeight);
void scanRecordingDirectory();
void drawExportDialog(Mat& img);
void performExport();
void showDirectoryChooser();
// In ui.h, add this declaration
string openDirectoryBrowser();
void toggleWindowState();
bool isWindowMinimized(const string& windowName);
void checkWindowState();
void checkDirectorySelection();
bool handleDualSliderInteraction(int mouseX, int mouseY, Rect sliderRect, 
    int& minValue, int& maxValue, int minLimit, int maxLimit);
void drawDualRangeSlider(Mat& img, Rect sliderRect, int& minValue, int& maxValue, 
    int minLimit, int maxLimit, const string& label);
void updateBgSubControlsTogglePosition(bool showControls);
 
#endif // UI_H
