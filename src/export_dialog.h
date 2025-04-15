#ifndef EXPORT_DIALOG_H
#define EXPORT_DIALOG_H

#include "common.h"

// Initialize export dialog with window dimensions
void createExportDialog(int windowWidth, int windowHeight);

// Draw export dialog on UI frame
void drawExportDialog(Mat& img);

// Scan available recording files
void scanRecordingDirectory();

// Open directory browser dialog
string openDirectoryBrowser();

// Check if directory selection has completed
void checkDirectorySelection();

// Perform export operation
void performExport();

// MouseCallbackData structure for handling export UI interaction
struct MouseCallbackData {
    vector<Rect> fileCheckboxRects;
    vector<Rect> fileTextRects;
};

extern MouseCallbackData mouseData;

#endif // EXPORT_DIALOG_H
