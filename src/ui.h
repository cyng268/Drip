#ifndef UI_H
#define UI_H

#include "common.h"
#include "background_subtraction.h"
#include "export_dialog.h"
#include "ui_helpers.h"
#include "navigation_bar.h"

// Initialize UI components
void initializeUI(int windowWidth, int windowHeight);

// Mouse callback function 
void mouseCallback(int event, int x, int y, int flags, void* userdata);

void initIR(int x, int y, int width, int height);

void drawIR(Mat& frame, bool bgActive);

#endif // UI_H
