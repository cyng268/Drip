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

// // Call this function to zoom in
// void zoomIn();

// // Call this function to zoom out
// void zoomOut();

#endif // UI_H
