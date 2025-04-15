#include "ui_helpers.h"

void drawProgressBar(Mat& img, Rect rect, int value, int maxValue, Scalar barColor) {
    // Draw progress bar background
    rectangle(img, rect, THEME_COLOR, -1);
    rectangle(img, rect, Scalar(100, 100, 100), 1);

    // Calculate fill width
    int fillWidth = value * rect.width / maxValue;
    if (fillWidth > 0) {
        Rect fillRect(rect.x, rect.y, fillWidth, rect.height);
        rectangle(img, fillRect, barColor, -1);
    }
}

void createArrowImages() {
    // Create up arrow image
    upArrowImage = Mat(NAV_BUTTON_SIZE, NAV_BUTTON_SIZE, CV_8UC3, Scalar(60, 60, 60));

    // Draw the up arrow shape
    Point points[3] = {
        Point(NAV_BUTTON_SIZE/2, 5),
        Point(5, NAV_BUTTON_SIZE-10),
        Point(NAV_BUTTON_SIZE-5, NAV_BUTTON_SIZE-10)
    };
    fillConvexPoly(upArrowImage, points, 3, Scalar(200, 200, 200));

    // Add a border to the button
    rectangle(upArrowImage, Rect(0, 0, NAV_BUTTON_SIZE, NAV_BUTTON_SIZE), Scalar(100, 100, 100), 1);

    // Create down arrow image (just flip the up arrow)
    downArrowImage = Mat(NAV_BUTTON_SIZE, NAV_BUTTON_SIZE, CV_8UC3, Scalar(60, 60, 60));

    // Draw the down arrow shape
    Point pointsDown[3] = {
        Point(NAV_BUTTON_SIZE/2, NAV_BUTTON_SIZE-5),
        Point(5, 10),
        Point(NAV_BUTTON_SIZE-5, 10)
    };
    fillConvexPoly(downArrowImage, pointsDown, 3, Scalar(200, 200, 200));

    // Add a border to the button
    rectangle(downArrowImage, Rect(0, 0, NAV_BUTTON_SIZE, NAV_BUTTON_SIZE), Scalar(100, 100, 100), 1);
}

void updateToggleButtonPosition(int windowWidth) {
    if (showNavBar) {
        // Position button at the top of the nav bar
        toggleNavButtonRect = Rect(windowWidth/2 - NAV_BUTTON_SIZE/2,
                                 navBarRect.y - NAV_BUTTON_SIZE,
                                 NAV_BUTTON_SIZE, NAV_BUTTON_SIZE);
    } else {
        // Position button at the bottom of the screen
        toggleNavButtonRect = Rect(windowWidth/2 - NAV_BUTTON_SIZE/2,
                                 navBarRect.y + navBarRect.height - NAV_BUTTON_SIZE,
                                 NAV_BUTTON_SIZE, NAV_BUTTON_SIZE);
    }
}

void drawWindowControls(Mat& img) {
    // Draw close button
    rectangle(img, closeButtonRect, Scalar(100, 40, 40), -1);
    rectangle(img, closeButtonRect, Scalar(160, 80, 80), 2);
    
    // Draw X icon
    line(img,
        Point(closeButtonRect.x + closeButtonRect.width/4, closeButtonRect.y + closeButtonRect.height/4),
        Point(closeButtonRect.x + 3*closeButtonRect.width/4, closeButtonRect.y + 3*closeButtonRect.height/4),
        Scalar(220, 220, 220), 2);
    line(img,
        Point(closeButtonRect.x + 3*closeButtonRect.width/4, closeButtonRect.y + closeButtonRect.height/4),
        Point(closeButtonRect.x + closeButtonRect.width/4, closeButtonRect.y + 3*closeButtonRect.height/4),
        Scalar(220, 220, 220), 2);

    // Draw minimize button and icon
    rectangle(img, minimizeButtonRect, Scalar(40, 100, 40), -1);
    rectangle(img, minimizeButtonRect, Scalar(80, 160, 80), 2);
    
    // Draw minimize icon (horizontal line)
    line(img,
         Point(minimizeButtonRect.x + minimizeButtonRect.width/4, minimizeButtonRect.y + minimizeButtonRect.height/2),
         Point(minimizeButtonRect.x + 3*minimizeButtonRect.width/4, minimizeButtonRect.y + minimizeButtonRect.height/2),
         Scalar(220, 220, 220), 2);
}

