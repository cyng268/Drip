#include "navigation_bar.h"
#include "ui_helpers.h"

void initNavigationBar(int windowWidth, int windowHeight) {
    // Bottom navigation bar (full width, 80px height at bottom)
    navBarRect = Rect(0, windowHeight - NAV_BAR_HEIGHT, windowWidth, NAV_BAR_HEIGHT);
    
    // Position buttons in the navigation bar
    int btnWidth = 140;
    int btnHeight = NAV_BAR_HEIGHT - 20;
    int btnSpacing = 20;
    int startX = PADDING;
    int buttonY = windowHeight - NAV_BAR_HEIGHT + (NAV_BAR_HEIGHT - btnHeight) / 2;

    // Record button
    recordButtonRect = Rect(startX, buttonY, btnWidth, btnHeight);
    startX += btnWidth + btnSpacing;

    // Export button
    exportButtonRect = Rect(startX, buttonY, btnWidth, btnHeight);
    startX += btnWidth + btnSpacing;

    // Zoom In button
    zoomInButtonRect = Rect(startX, buttonY, btnWidth, btnHeight);
    startX += btnWidth + btnSpacing;

    // Zoom Out button
    zoomOutButtonRect = Rect(startX, buttonY, btnWidth, btnHeight);
    startX += btnWidth + btnSpacing;

    // Pan Up button
    panUpButtonRect = Rect(startX, buttonY, btnWidth, btnHeight);
    startX += btnWidth + btnSpacing;

    // Pan Down button
    panDownButtonRect = Rect(startX, buttonY, btnWidth, btnHeight);
}

void drawNavigationBar(Mat& img, int windowWidth, bool isRecording, bool isProcessing, 
                     bool isZoomInHeld, bool isZoomOutHeld) {
    if (!showNavBar) {
        return;
    }
    
    // Create semi-transparent navigation bar at the bottom
    Mat bottomBar = img(navBarRect).clone();
    Mat overlay(bottomBar.rows, bottomBar.cols, bottomBar.type(), Scalar(20, 60, 20));
    double alpha = 0.7; // Transparency level (0.0 = fully transparent, 1.0 = opaque)
    addWeighted(overlay, alpha, bottomBar, 1.0 - alpha, 0.0, bottomBar);
    bottomBar.copyTo(img(navBarRect));

    // Draw record/stop button
    rectangle(img, recordButtonRect, isRecording ? Scalar(60, 0, 0) : BUTTON_COLOR, -1);
    rectangle(img, recordButtonRect, Scalar(100, 100, 100), 1);
    putText(img, isRecording ? "Stop" : "Record",
            Point(recordButtonRect.x + 10, recordButtonRect.y + recordButtonRect.height/2 + 5),
            FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.2);

    // Export button
    Scalar exportBtnColor = isProcessing ? Scalar(30, 30, 30) : BUTTON_COLOR;
    rectangle(img, exportButtonRect, exportBtnColor, -1);
    rectangle(img, exportButtonRect, Scalar(100, 100, 100), 1);
    putText(img, "Export", 
            Point(exportButtonRect.x + 10, exportButtonRect.y + exportButtonRect.height/2 + 5),
            FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.2);

    // Zoom In button
    rectangle(img, zoomInButtonRect, isZoomInHeld ? Scalar(100, 200, 100) : BUTTON_COLOR, -1);
    rectangle(img, zoomInButtonRect, Scalar(100, 100, 100), 1);
    putText(img, "Zoom +", 
            Point(zoomInButtonRect.x + 10, zoomInButtonRect.y + zoomInButtonRect.height/2 + 5),
            FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.2);

    // Zoom Out button
    rectangle(img, zoomOutButtonRect, isZoomOutHeld ? Scalar(100, 200, 100) : BUTTON_COLOR, -1);
    rectangle(img, zoomOutButtonRect, Scalar(100, 100, 100), 1);
    putText(img, "Zoom -", 
            Point(zoomOutButtonRect.x + 10, zoomOutButtonRect.y + zoomOutButtonRect.height/2 + 5),
            FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.2);

    // Pan Up button
    rectangle(img, panUpButtonRect, BUTTON_COLOR, -1);
    rectangle(img, panUpButtonRect, Scalar(100, 150, 100), 1);
    putText(img, "Pan Up", 
            Point(panUpButtonRect.x + 10, panUpButtonRect.y + panUpButtonRect.height/2 + 5),
            FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.2);

    // Pan Down button
    rectangle(img, panDownButtonRect, BUTTON_COLOR, -1);
    rectangle(img, panDownButtonRect, Scalar(100, 150, 100), 1);
    putText(img, "Pan Down", 
            Point(panDownButtonRect.x + 10, panDownButtonRect.y + panDownButtonRect.height/2 + 5),
            FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.2);

    // Status display
    int statusX = panDownButtonRect.x + panDownButtonRect.width + 20;
    Rect statusRect(statusX, navBarRect.y + 10, windowWidth - statusX - PADDING, panDownButtonRect.height);
    rectangle(img, statusRect, Scalar(40, 40, 40), -1);

    // Show status/log message
    putText(img, getLogMessage(), 
            Point(statusRect.x + 10, statusRect.y + statusRect.height/2 + 5),
            FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.2);
}
