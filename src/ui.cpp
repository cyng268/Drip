#include "ui.h"
#include "serial.h"
#include "recording.h"
#include <filesystem>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include "config.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <cstring>

MouseCallbackData mouseData;

void updateBgSubControlsTogglePosition(bool showControls) {
    if (showControls) {
        // Position toggle button next to the controls panel
        toggleBgSubControlsRect = Rect(bgSubControlsRect.x + bgSubControlsRect.width, 
                                      bgSubControlsRect.y, 
                                      40, 40); // Using the larger 40x40 size
    } else {
        // Position toggle button along the left edge of the screen
        toggleBgSubControlsRect = Rect(10, // 10px from left edge
                                      bgSubControlsRect.y, // Same vertical position
                                      40, 40); // Same size
    }
}

// Draw dual range slider
void drawDualRangeSlider(Mat& img, Rect sliderRect, int& minValue, int& maxValue, 
                        int minLimit, int maxLimit, const string& label) {
    // Draw slider background
    rectangle(img, sliderRect, Scalar(40, 40, 40), -1);
    rectangle(img, sliderRect, Scalar(100, 100, 100), 2); // Thicker border
    
    // Draw label with LARGER FONT
    putText(img, label, Point(sliderRect.x, sliderRect.y - 15), // More space above
            FONT_HERSHEY_SIMPLEX, 0.7, TEXT_COLOR, 2); // Increased size and thickness
    
    // Calculate handle positions
    int sliderWidth = sliderRect.width - 30; // Wider margin for larger handles
    int minHandlePos = sliderRect.x + 15 + (minValue - minLimit) * sliderWidth / (maxLimit - minLimit);
    int maxHandlePos = sliderRect.x + 15 + (maxValue - minLimit) * sliderWidth / (maxLimit - minLimit);
    
    // Draw slider track with THICKER LINE
    line(img, Point(sliderRect.x + 15, sliderRect.y + sliderRect.height/2),
         Point(sliderRect.x + sliderRect.width - 15, sliderRect.y + sliderRect.height/2),
         Scalar(150, 150, 150), 3); // Thicker line
    
    // Draw active range with THICKER LINE
    line(img, Point(minHandlePos, sliderRect.y + sliderRect.height/2),
         Point(maxHandlePos, sliderRect.y + sliderRect.height/2),
         BUTTON_COLOR, 6); // Thicker line
    
    // Draw LARGER min handle
    rectangle(img, Rect(minHandlePos - 8, sliderRect.y + 5, 16, sliderRect.height - 10), // Wider handle
              BUTTON_COLOR, -1);
    rectangle(img, Rect(minHandlePos - 8, sliderRect.y + 5, 16, sliderRect.height - 10),
              Scalar(200, 200, 200), 2); // Thicker border
    
    // Draw LARGER max handle
    rectangle(img, Rect(maxHandlePos - 8, sliderRect.y + 5, 16, sliderRect.height - 10), // Wider handle
              BUTTON_COLOR, -1);
    rectangle(img, Rect(maxHandlePos - 8, sliderRect.y + 5, 16, sliderRect.height - 10),
              Scalar(200, 200, 200), 2); // Thicker border
    
    // Display current values with LARGER FONT and more SPACE below
    string valueText = "Min: " + to_string(minValue) + " - Max: " + to_string(maxValue);
    putText(img, valueText, 
            Point(sliderRect.x + sliderRect.width/2 - 80, sliderRect.y + sliderRect.height + 25), // Lower position
            FONT_HERSHEY_SIMPLEX, 0.7, TEXT_COLOR, 2); // Increased size and thickness
}

// Helper to check if mouse is on slider handles and update values accordingly
bool handleDualSliderInteraction(int mouseX, int mouseY, Rect sliderRect, 
                              int& minValue, int& maxValue, int minLimit, int maxLimit) {
    // Calculate handle positions with WIDER MARGINS
    int sliderWidth = sliderRect.width - 30; // Match the drawing function
    int minHandlePos = sliderRect.x + 15 + (minValue - minLimit) * sliderWidth / (maxLimit - minLimit);
    int maxHandlePos = sliderRect.x + 15 + (maxValue - minLimit) * sliderWidth / (maxLimit - minLimit);
    
    // Check if click is on min handle with LARGER HANDLE AREA
    Rect minHandleRect(minHandlePos - 12, sliderRect.y + 5, 24, sliderRect.height - 10); // Wider clickable area
    if (minHandleRect.contains(Point(mouseX, mouseY))) {
        // Calculate new value based on mouse position
        int newValue = minLimit + (mouseX - (sliderRect.x + 15)) * (maxLimit - minLimit) / sliderWidth;
        // Clamp value
        newValue = max(minLimit, min(maxValue - 10, newValue));
        minValue = newValue;
        return true;
    }
    
    // Check if click is on max handle with LARGER HANDLE AREA
    Rect maxHandleRect(maxHandlePos - 12, sliderRect.y + 5, 24, sliderRect.height - 10); // Wider clickable area
    if (maxHandleRect.contains(Point(mouseX, mouseY))) {
        // Calculate new value based on mouse position
        int newValue = minLimit + (mouseX - (sliderRect.x + 15)) * (maxLimit - minLimit) / sliderWidth;
        // Clamp value
        newValue = max(minValue + 10, min(maxLimit, newValue));
        maxValue = newValue;
        return true;
    }
    
    // Check if click is on the slider track to jump
    if (sliderRect.contains(Point(mouseX, mouseY))) {
        // Determine if closer to min or max handle
        if (abs(mouseX - minHandlePos) < abs(mouseX - maxHandlePos)) {
            // Set min handle
            int newValue = minLimit + (mouseX - (sliderRect.x + 15)) * (maxLimit - minLimit) / sliderWidth;
            newValue = max(minLimit, min(maxValue - 10, newValue));
            minValue = newValue;
        } else {
            // Set max handle
            int newValue = minLimit + (mouseX - (sliderRect.x + 15)) * (maxLimit - minLimit) / sliderWidth;
            newValue = max(minValue + 10, min(maxLimit, newValue));
            maxValue = newValue;
        }
        return true;
    }
    
    return false;
}

string openDirectoryBrowser() {
    // If a dialog is already active, don't open another one
    if (directoryDialogActive.load()) {
        setLogMessage("File dialog already open");
        return "";
    }
    
    // Reset result state
    {
        std::lock_guard<std::mutex> lock(directoryResultMutex);
        selectedDirectoryResult = "";
        directoryResultReady = false;
    }
    
    // Set dialog as active
    directoryDialogActive.store(true);
    
    // Create and start the thread for file dialog
    std::thread dialogThread([]() {
        // Create a temporary file to store the result
        string tempFile = "/tmp/dir_selection.txt";
        
        // Run zenity file dialog and save result to temp file
        string cmd = "zenity --file-selection --directory --title=\"Select Export Directory\" > " + tempFile;
        int result = system(cmd.c_str());
        
        string selectedDir = "";
        // Check if command succeeded
        if (result == 0) {
            // Read selected directory from temp file
            ifstream file(tempFile);
            if (file.is_open()) {
                getline(file, selectedDir);
                file.close();
            }
            
            // Make sure the directory path ends with a slash
            if (!selectedDir.empty() && selectedDir.back() != '/') {
                selectedDir += "/";
            }
        }
        
        // Clean up temp file
        remove(tempFile.c_str());
        
        // Store the result
        {
            std::lock_guard<std::mutex> lock(directoryResultMutex);
            selectedDirectoryResult = selectedDir;
            directoryResultReady = true;
        }
        
        // Mark dialog as inactive
        directoryDialogActive.store(false);
    });
    
    // Detach the thread so it runs independently
    dialogThread.detach();
    
    setLogMessage("Directory selection in progress...");
    return "";
}

// Check if a directory selection result is available
void checkDirectorySelection() {
    bool isReady = false;
    string result;
    
    // Check if result is ready
    {
        std::lock_guard<std::mutex> lock(directoryResultMutex);
        if (directoryResultReady) {
            isReady = true;
            result = selectedDirectoryResult;
            directoryResultReady = false; // Reset for next time
        }
    }
    
    // Process the result if ready
    if (isReady && !result.empty()) {
        exportDestDir = result;
        setLogMessage("Directory selected: " + result);
    }
}

void createExportDialog(int windowWidth, int windowHeight) {
    // Create export dialog in the center of the screen
    int dialogWidth = windowWidth * 0.8;
    int dialogHeight = windowHeight * 0.8;
    int dialogX = (windowWidth - dialogWidth) / 2;
    int dialogY = (windowHeight - dialogHeight) / 2;

    exportDialogRect = Rect(dialogX, dialogY, dialogWidth, dialogHeight);

    // Create file list area (right side)
    int fileListWidth = dialogWidth / 2;
    fileListRect = Rect(dialogX + dialogWidth/2, dialogY + 50, fileListWidth, dialogHeight - 100);

    // Create directory selection area (left side)
    dirSelectRect = Rect(dialogX + 20, dialogY + 50, dialogWidth/2 - 40, 40);

    // Create "Keep original files" option
    keepFilesRect = Rect(dialogX + 20, dialogY + dialogHeight - 80, dialogWidth/2 - 40, 30);

    // Create confirm and cancel buttons
    int btnWidth = 120;
    int btnHeight = 40;
    exportConfirmRect = Rect(dialogX + 3/2 - btnWidth - 20,
                           dialogY + dialogHeight - btnHeight - 20,
                           btnWidth, btnHeight);

    exportCancelRect = Rect(dialogX + dialogWidth/2 + 20,
                          dialogY + dialogHeight - btnHeight - 20,
                          btnWidth, btnHeight);

    // Create scroll buttons
    int scrollBtnSize = 30;
    scrollUpRect = Rect(dialogX + dialogWidth - 40, dialogY + 60, scrollBtnSize, scrollBtnSize);
    scrollDownRect = Rect(dialogX + dialogWidth - 40,
                        dialogY + dialogHeight - 60 - scrollBtnSize,
                        scrollBtnSize, scrollBtnSize);
}

void scanDirectory(const string& path, vector<string>& directories) {
    directories.clear();

    // Add parent directory option
    directories.push_back("..");

    DIR *dir;
    struct dirent *ent;
    struct stat st;

    if ((dir = opendir(path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            string name = ent->d_name;
            if (name == "." || name == "..") continue;

            string fullPath = path + "/" + name;
            if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                directories.push_back(name);
            }
        }
        closedir(dir);
    }

    // Sort directories alphabetically
    sort(directories.begin() + 1, directories.end());
}

void scanRecordingDirectory() {
    recordingFiles.clear();
    fileSelection.clear();

    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir("./recordings")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            string filename = ent->d_name;
            // Skip . and .. directories and other non-video files
            if (filename != "." && filename != ".." &&
                (filename.find(".mp4") != string::npos ||
                 filename.find(".avi") != string::npos)) {
                recordingFiles.push_back(filename);
                fileSelection.push_back(false); // Initially not selected
            }
        }
        closedir(dir);

        // Sort recordings alphabetically
        sort(recordingFiles.begin(), recordingFiles.end());
    }

    // Reset scroll position
    scrollOffset = 0;
}

// In ui.cpp, modify the drawExportDialog function to replace directory listing with a browse button

// Update the drawExportDialog function in ui.cpp
// Update the drawExportDialog function in ui.cpp
void drawExportDialog(Mat& img) {
    int dialogWidth = DISPLAY_WIDTH * 0.8;
    int dialogHeight = DISPLAY_HEIGHT * 0.8;
    if (!showExportDialog) return;

    // Draw semi-transparent overlay for the entire screen
    Mat overlay = img.clone();
    rectangle(overlay, Rect(0, 0, img.cols, img.rows), Scalar(30, 30, 30), -1);
    addWeighted(overlay, 0.7, img, 0.3, 0, img);

    // Draw dialog box
    rectangle(img, exportDialogRect, THEME_COLOR, -1);
    rectangle(img, exportDialogRect, Scalar(150, 150, 150), 2);

    // Dialog title
    putText(img, "Export Directory:",
            Point(exportDialogRect.x + 20, exportDialogRect.y + 30),
            FONT_HERSHEY_SIMPLEX, 0.8, TEXT_COLOR, 2);

    // Define scroll button dimensions and spacing
    int scrollBtnWidth = 30;
    int scrollBtnSpacing = 10;

    // Adjust fileListRect to be shorter and narrower to fit scroll buttons within dialog
    fileListRect = Rect(exportDialogRect.x + dialogWidth/2,
                      exportDialogRect.y + 50,
                      dialogWidth/2 - scrollBtnWidth - scrollBtnSpacing - 10, // Make it narrower
                      dialogHeight - 120); // Make it shorter

    // Draw file list area (right side)
    rectangle(img, fileListRect, Scalar(50, 50, 50), -1);
    rectangle(img, fileListRect, Scalar(100, 100, 100), 1);

    putText(img, "Available Files",
            Point(fileListRect.x, fileListRect.y - 10),
            FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.0);

    // Draw files with checkboxes
    int y = fileListRect.y + 25;
    int checkboxSize = 15;
    int maxY = fileListRect.y + fileListRect.height - 30;

    // Store rects for clickable areas
    static vector<Rect> fileCheckboxRects;
    static vector<Rect> fileTextRects;
    fileCheckboxRects.clear();
    fileTextRects.clear();

    // Calculate maximum text width available for filenames
    int checkboxPadding = 20; // Space after checkbox
    int maxTextWidth = fileListRect.width - 10 - checkboxSize - checkboxPadding; // 10 is padding from left

    // Character width approximation for the font
    float charWidth = 9.0; // Approximate width of a character in pixels
    int maxChars = maxTextWidth / charWidth;

    for (size_t i = scrollOffset; i < recordingFiles.size() && y < maxY; i++) {
        // Draw checkbox
        Rect checkboxRect(fileListRect.x + 10, y - checkboxSize/2, checkboxSize, checkboxSize);
        rectangle(img, checkboxRect, TEXT_COLOR, 1);
        fileCheckboxRects.push_back(checkboxRect);

        if (fileSelection[i]) {
            // Draw checkmark
            line(img, Point(checkboxRect.x + 3, checkboxRect.y + checkboxSize/2),
                 Point(checkboxRect.x + checkboxSize/2, checkboxRect.y + checkboxSize - 3),
                 TEXT_COLOR, 2);
            line(img, Point(checkboxRect.x + checkboxSize/2, checkboxRect.y + checkboxSize - 3),
                 Point(checkboxRect.x + checkboxSize - 3, checkboxRect.y + 3),
                 TEXT_COLOR, 2);
        }

        // Get the filename and truncate if too long
        string filename = recordingFiles[i];
        string displayName = filename;

        if (filename.length() > maxChars) {
            // Keep first part and append '...'
            displayName = filename.substr(0, maxChars - 3) + "...";
        }

        // Draw truncated filename
        putText(img, displayName,
                Point(checkboxRect.x + checkboxSize + checkboxPadding, y + 5),
                FONT_HERSHEY_SIMPLEX, 0.5, TEXT_COLOR, 2.0);

        // Create text clickable area (whole row except checkbox)
        Rect textRect(checkboxRect.x + checkboxSize + 5, y - 15,
                    fileListRect.width - checkboxSize - 25, 30);
        fileTextRects.push_back(textRect);

        y += 30;
    }

    // Reposition scroll buttons to be inside the dialog but next to the file list
    scrollUpRect = Rect(fileListRect.x + fileListRect.width + scrollBtnSpacing,
                      fileListRect.y,
                      scrollBtnWidth, scrollBtnWidth);
    scrollDownRect = Rect(fileListRect.x + fileListRect.width + scrollBtnSpacing,
                        fileListRect.y + fileListRect.height - scrollBtnWidth,
                        scrollBtnWidth, scrollBtnWidth);

    // Draw scroll buttons if needed
    if (recordingFiles.size() > maxFilesVisible) {
        // Up button
        rectangle(img, scrollUpRect, Scalar(60, 60, 60), -1);
        rectangle(img, scrollUpRect, Scalar(100, 100, 100), 1);
        // Draw up arrow
        Point p1(scrollUpRect.x + scrollUpRect.width/2, scrollUpRect.y + 5);
        Point p2(scrollUpRect.x + 5, scrollUpRect.y + scrollUpRect.height - 5);
        Point p3(scrollUpRect.x + scrollUpRect.width - 5, scrollUpRect.y + scrollUpRect.height - 5);
        line(img, p1, p2, TEXT_COLOR, 2);
        line(img, p1, p3, TEXT_COLOR, 2);

        // Down button
        rectangle(img, scrollDownRect, Scalar(60, 60, 60), -1);
        rectangle(img, scrollDownRect, Scalar(100, 100, 100), 1);
        // Draw down arrow
        Point p4(scrollDownRect.x + scrollDownRect.width/2, scrollDownRect.y + scrollDownRect.height - 5);
        Point p5(scrollDownRect.x + 5, scrollDownRect.y + 5);
        Point p6(scrollDownRect.x + scrollDownRect.width - 5, scrollDownRect.y + 5);
        line(img, p4, p5, TEXT_COLOR, 2);
        line(img, p4, p6, TEXT_COLOR, 2);
    }

    // Directory selection area (left side)
    int dirAreaX = exportDialogRect.x + 20;
    int dirAreaY = exportDialogRect.y + 100;
    int dirAreaWidth = dialogWidth/2 - 40;

    // Display current path
//    putText(img, "Export Directory:",
//            Point(dirAreaX, dirAreaY - 40),
//            FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2);

    // Create directory path display box
    Rect dirPathRect(dirAreaX, dirAreaY, dirAreaWidth, 30);
    rectangle(img, dirPathRect, Scalar(50, 50, 50), -1);
    rectangle(img, dirPathRect, Scalar(100, 100, 100), 1);

    // Display current export path (trimmed if too long)
    // Calculate character limit for path display
    int pathMaxChars = dirAreaWidth / charWidth - 4; // -2 for padding

    string displayPath = exportDestDir;
    if (displayPath.length() > pathMaxChars) {
        // Keep the first part of the path, then ellipsis, then last part
        size_t totalToKeep = pathMaxChars - 3; // -3 for "..."
        size_t firstPartLength = totalToKeep / 3;
        size_t lastPartLength = totalToKeep - firstPartLength;

        displayPath = displayPath.substr(0, firstPartLength) +
                      "..." +
                      displayPath.substr(displayPath.length() - lastPartLength);
    }

    putText(img, displayPath,
            Point(dirPathRect.x + 10, dirPathRect.y + 20),
            FONT_HERSHEY_SIMPLEX, 0.5, TEXT_COLOR, 2.0);

    // Create Browse button
    Rect browseButtonRect(dirAreaX, dirAreaY + 50, dirAreaWidth, 40);
    rectangle(img, browseButtonRect, Scalar(60, 60, 100), -1);
    rectangle(img, browseButtonRect, Scalar(100, 100, 150), 1);
    putText(img, "Browse...",
            Point(browseButtonRect.x + browseButtonRect.width/2 - 40, browseButtonRect.y + 25),
            FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.0);

    // Update the browseButtonRect in global variables
    dirSelectRect = browseButtonRect;

    // Draw "Keep original files" option
    keepFilesRect = Rect(dirAreaX, exportDialogRect.y + dialogHeight - 80, dirAreaWidth, 30);
    rectangle(img, Rect(keepFilesRect.x, keepFilesRect.y, 20, 20), TEXT_COLOR, 1);

    if (keepOriginalFiles) {
        // Draw checkmark
        line(img, Point(keepFilesRect.x + 3, keepFilesRect.y + 10),
             Point(keepFilesRect.x + 8, keepFilesRect.y + 15),
             TEXT_COLOR, 2);
        line(img, Point(keepFilesRect.x + 8, keepFilesRect.y + 15),
             Point(keepFilesRect.x + 17, keepFilesRect.y + 5),
             TEXT_COLOR, 2);
    }

    putText(img, "Keep original files",
            Point(keepFilesRect.x + 30, keepFilesRect.y + 15),
            FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.0);

    // Reposition buttons to be side by side at bottom
    int btnWidth = 120;
    int btnHeight = 40;
    int btnSpacing = 20;

    // Calculate positions to center both buttons
    int totalBtnWidth = btnWidth * 2 + btnSpacing;
    int startX = exportDialogRect.x + (dialogWidth - totalBtnWidth) / 2;

    // Draw buttons
    exportConfirmRect = Rect(startX, exportDialogRect.y + dialogHeight - btnHeight - 20,
                           btnWidth, btnHeight);
    exportCancelRect = Rect(startX + btnWidth + btnSpacing,
                          exportDialogRect.y + dialogHeight - btnHeight - 20,
                          btnWidth, btnHeight);

    rectangle(img, exportConfirmRect, Scalar(60, 100, 60), -1);
    rectangle(img, exportConfirmRect, Scalar(100, 150, 100), 1);
    putText(img, "Export",
            Point(exportConfirmRect.x + 20, exportConfirmRect.y + 25),
            FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2);

    rectangle(img, exportCancelRect, Scalar(100, 60, 60), -1);
    rectangle(img, exportCancelRect, Scalar(150, 100, 100), 1);
    putText(img, "Cancel",
            Point(exportCancelRect.x + 20, exportCancelRect.y + 25),
            FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2);

    // Store the clickable areas to make them accessible in mouseCallback
    mouseData.fileCheckboxRects = fileCheckboxRects;
    mouseData.fileTextRects = fileTextRects;
}

void performExport() {
    // Make sure destination directory exists
    mkdir(exportDestDir.c_str(), 0777);

    int exportCount = 0;
    for (size_t i = 0; i < recordingFiles.size(); i++) {
        if (fileSelection[i]) {
            string srcPath = "./recordings/" + recordingFiles[i]; // Fix the source path (was "./recording/")
            string destPath = exportDestDir + recordingFiles[i];

            // Copy file to destination using better file handling
            bool copySuccess = false;

            // Open source file in binary mode
            std::ifstream src(srcPath, std::ios::binary);
            if (src) {
                // Open destination file in binary mode
                std::ofstream dst(destPath, std::ios::binary);
                if (dst) {
                    // Get source file size
                    src.seekg(0, std::ios::end);
                    std::streamsize size = src.tellg();
                    src.seekg(0, std::ios::beg);

                    // Allocate buffer
                    const int bufferSize = 4096;
                    char buffer[bufferSize];

                    // Copy file in chunks
                    while (size > 0) {
                        std::streamsize bytesToRead = std::min(static_cast<std::streamsize>(bufferSize), size);
                        src.read(buffer, bytesToRead);
                        dst.write(buffer, src.gcount());
                        size -= src.gcount();
                    }

                    dst.close();
                    copySuccess = true;
                }
                src.close();
            }

            // Check file sizes to confirm copy worked
            struct stat srcStat, dstStat;
            bool sizeCheckOk = false;

            if (stat(srcPath.c_str(), &srcStat) == 0 &&
                stat(destPath.c_str(), &dstStat) == 0) {
                sizeCheckOk = (srcStat.st_size == dstStat.st_size && srcStat.st_size > 0);
            }

            if (copySuccess && sizeCheckOk) {
                exportCount++;

                // Delete original file if not keeping them
                if (!keepOriginalFiles) {
                    if (remove(srcPath.c_str()) == 0) {
                        cout << "Deleted original file: " << srcPath << endl;
                    } else {
                        cerr << "Failed to delete original file: " << srcPath << endl;
                    }
                }
            } else {
                cerr << "Failed to copy file: " << srcPath << " to " << destPath << endl;
                if (copySuccess && !sizeCheckOk) {
                    cerr << "File size mismatch after copy!" << endl;
                }
            }
        }
    }

    if (exportCount > 0) {
        setLogMessage("Exported " + to_string(exportCount) + " files");

        // Refresh the file list (some might have been deleted)
        if (!keepOriginalFiles) {
            scanRecordingDirectory();
        }
    } else {
        setLogMessage("No files selected for export");
    }
}

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

void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    appConfig.loadConfig();
    if (event == EVENT_LBUTTONDOWN) {
        if (toggleNavButtonRect.contains(Point(x, y))) {
            showNavBar = !showNavBar;
            // Update toggle button position is handled in main loop
            return;
        }
        if (toggleBgSubControlsRect.contains(Point(x, y))) {
            showBgSubControls = !showBgSubControls;
            updateBgSubControlsTogglePosition(showBgSubControls); // Update toggle position
            return;
        }
        if (!showExportDialog) {
            // Check if controls toggle button was clicked
            if (toggleBgSubControlsRect.contains(Point(x, y))) {
                showBgSubControls = !showBgSubControls;
                return;
            }

            // Handle slider interaction if controls are visible
            if (showBgSubControls) {
                // Only handle sliders if background subtraction isn't active
                if (!bgSubtractionActive) {
                    if (handleDualSliderInteraction(x, y, bgSubSliderRect, lowerBound, upperBound, minContourArea, maxContourArea)) {
                        return;
                    }
                    
                    // Remove consecutive frames slider handling
                }
            }
        }
        if (videoRect.contains(Point(x, y)) && !showExportDialog && bgSubtractionActive){
            bgSubtractionActive = false;
            return;
        }
        if (videoRect.contains(Point(x, y)) && !showExportDialog) {
            // If not in a dialog and clicked in video area - activate background subtraction
            bgSubtractionActive = true;
            
            // Create a 100x100 box centered at click position
            int boxSize = 100;
            y = y * 720/800;
            bgSubtractionRect = Rect(x - boxSize/2, y - boxSize/2, boxSize, boxSize);
            
            // Reset background subtractor
            backgroundSubtractor = createBackgroundSubtractorMOG2(300, 16, true); // History, variance threshold, detect shadows
            
            // Reset variables
            frameNumber = 0;
            objectOccurrences.clear();
            detectionPoints.clear();
            detectionCounts.clear();
            isTimedOut = false;
            
            // Start the timer
            bgSubStartTime = std::chrono::system_clock::now();
            
            setLogMessage("BG active for 10 seconds");
            return;
        }
        if (showExportDialog) {
            // File checkboxes handling
            bool fileAreaClicked = false;

            // Check if click was on any file checkbox
            for (size_t i = 0; i < mouseData.fileCheckboxRects.size(); i++) {
                if (mouseData.fileCheckboxRects[i].contains(Point(x, y))) {
                    size_t fileIndex = i + scrollOffset;
                    if (fileIndex < fileSelection.size()) {
                        fileSelection[fileIndex] = !fileSelection[fileIndex];
                        fileAreaClicked = true;
                        break;
                    }
                }
            }

            // Check if click was on any file text
            if (!fileAreaClicked) {
                for (size_t i = 0; i < mouseData.fileTextRects.size(); i++) {
                    if (mouseData.fileTextRects[i].contains(Point(x, y))) {
                        size_t fileIndex = i + scrollOffset;
                        if (fileIndex < fileSelection.size()) {
                            fileSelection[fileIndex] = !fileSelection[fileIndex];
                            fileAreaClicked = true;
                            break;
                        }
                    }
                }
            }

            // Check if browse button was clicked
            if (!fileAreaClicked && dirSelectRect.contains(Point(x, y))) {
                string selectedDir = openDirectoryBrowser();
                if (!selectedDir.empty()) {
                    exportDestDir = selectedDir;
                }
            }
            // Check if click was on "Keep original files" checkbox or its text
            else if (!fileAreaClicked &&
                    (Rect(keepFilesRect.x, keepFilesRect.y, 20, 20).contains(Point(x, y)) ||
                     Rect(keepFilesRect.x + 25, keepFilesRect.y, 150, 20).contains(Point(x, y)))) {
                keepOriginalFiles = !keepOriginalFiles;
            }
            // Check if click was on scroll buttons
            else if (!fileAreaClicked && scrollUpRect.contains(Point(x, y)) && scrollOffset > 0) {
                scrollOffset--;
            }
            else if (!fileAreaClicked && scrollDownRect.contains(Point(x, y)) &&
                     scrollOffset < recordingFiles.size() - maxFilesVisible) {
                scrollOffset++;
            }
            // Check if Export button was clicked
            else if (!fileAreaClicked && exportConfirmRect.contains(Point(x, y))) {
                performExport();
                showExportDialog = false;
            }
            // Check if Cancel button was clicked
            else if (!fileAreaClicked && (exportCancelRect.contains(Point(x, y)) ||
                     !exportDialogRect.contains(Point(x, y)))) {
                showExportDialog = false;
                setLogMessage("");
            }
           // Inside EVENT_LBUTTONDOWN
            if (!showExportDialog) 
                return;
        }
        if (minimizeButtonRect.contains(Point(x, y))) {
            // Just minimize the window
            system("xdotool search --name \"Water Dripping Investigation Recording Tools\" windowminimize");
            return;
        }
        else if (closeButtonRect.contains(Point(x, y))) {
            // Close window
            destroyWindow("Water Dripping Investigation Recording Tools");
            return;
        }

        //Check if toggle nav bar button was clicked
        //In the toggle nav bar button section
        

        // If nav bar is not showing, only handle toggle button
        if (!showNavBar) {
            return;
        }

        if (recordButtonRect.contains(Point(x, y))) {
            // Record button clicked
            if (isProcessing) {
                // Don't allow starting a new recording while processing
                setLogMessage("Processing...");
                return;
            }
            isRecording = !isRecording;

            if (isRecording) {
                // Start recording code
                time_t now = time(0);
                char buffer[80];
                strftime(buffer, 80, "%Y%m%d_%H%M%S", localtime(&now));
                tempFilename = "/tmp/" + string(buffer) + "_temp.avi"; // Create temp filename

                recordingStartTime = system_clock::now();
                setLogMessage("Rec started...");
                progressValue = 0;
            } else {
                // Stop recording code
                if (videoWriter.isOpened()) {
                    recordingDurationSeconds = duration<double>(system_clock::now() - recordingStartTime).count();
                    videoWriter.release();
                    setLogMessage("Rec stopped");

                    // Cancel any ongoing processing
                    if (isProcessing && processingThread.joinable()) {
                        isProcessing = false;
                        processingThread.join();
                    }
                    // Start processing in a new thread using the temp file
                    isProcessing = true;
                    if (processingThread.joinable()) {
                        processingThread.join();
                    }
                    processingThread = thread(postProcessVideo, tempFilename, recordingDurationSeconds);
                }
            }
        } else if (exportButtonRect.contains(Point(x, y))) {
            // Export button clicked - show export dialog instead of immediate processing
            if (!isRecording) {
                showExportDialog = true;
                createExportDialog(DISPLAY_WIDTH, DISPLAY_HEIGHT);
                scanRecordingDirectory();
                setLogMessage("Preparing export dialog...");
            } else {
                setLogMessage("Stop rec before exporting");
            }
        }else if (zoomInButtonRect.contains(Point(x, y))) {
            // Zoom in button pressed down
            isZoomInHeld = true;
            lastZoomTime = system_clock::now();
            // Perform initial zoom immediately
            zoomIn();
        } else if (zoomOutButtonRect.contains(Point(x, y))) {
            // Zoom out button pressed down
            isZoomOutHeld = true;
            lastZoomTime = system_clock::now();
            // Perform initial zoom immediately
            zoomOut();
        }
    }
    else if (event == EVENT_LBUTTONUP) {
        // Handle button releases
        isZoomInHeld = false;
        isZoomOutHeld = false;
        isDraggingMinHandle = false;
        isDraggingMaxHandle = false;
    }
    else if (event == EVENT_MOUSEMOVE) {
        // Inside EVENT_MOUSEMOVE section
        // Track slider handles if being dragged
        isDraggingMinHandle = false;
        isDraggingMaxHandle = false;

        if (flags & EVENT_FLAG_LBUTTON) {
            if (showBgSubControls && !bgSubtractionActive) {
                // Calculate handle positions to check which one is being dragged
                int sliderWidth = bgSubSliderRect.width - 30; // Match wider margin
                int minHandlePos = bgSubSliderRect.x + 15 + (lowerBound - 10) * sliderWidth / (maxContourArea - minContourArea);
                int maxHandlePos = bgSubSliderRect.x + 15 + (upperBound - 10) * sliderWidth / (maxContourArea- minContourArea);
                
                Rect minHandleRect(minHandlePos - 12, bgSubSliderRect.y + 5, 24, bgSubSliderRect.height - 10);
                Rect maxHandleRect(maxHandlePos - 12, bgSubSliderRect.y + 5, 24, bgSubSliderRect.height - 10);
                
                // Set dragging flags on initial press
                if (!isDraggingMinHandle && !isDraggingMaxHandle) {
                    if (minHandleRect.contains(Point(x, y))) {
                        isDraggingMinHandle = true;
                    } else if (maxHandleRect.contains(Point(x, y))) {
                        isDraggingMaxHandle = true;
                    }
                }
                
                // Update values based on which handle is being dragged
                if (isDraggingMinHandle) {
                    int newValue = 10 + (x - (bgSubSliderRect.x + 10)) * (maxContourArea - minContourArea) / sliderWidth;
                    lowerBound = max(10, min(upperBound - 10, newValue));
                } else if (isDraggingMaxHandle) {
                    int newValue = 10 + (x - (bgSubSliderRect.x + 10)) * (maxContourArea - minContourArea) / sliderWidth;
                    upperBound = max(lowerBound + 10, min(maxContourArea, newValue));
                }
            }
        }
        // If mouse moved outside the button area while button is held, stop zooming
        if (isZoomInHeld && !zoomInButtonRect.contains(Point(x, y))) {
            isZoomInHeld = false;
        }
        if (isZoomOutHeld && !zoomOutButtonRect.contains(Point(x, y))) {
            isZoomOutHeld = false;
        }

    }
}

void initializeUI(int windowWidth, int windowHeight) {
    // Top bar height (for window controls)
    int topBarHeight = 40;

    // Set navigation bar height to 80px as required
    int bottomBarHeight = 80;

    // Window control buttons (top-right)
    closeButtonRect = Rect(windowWidth - 35, 5, 30, 30);
    minimizeButtonRect = Rect(windowWidth - 70, 5, 30, 30);

    // Bottom navigation bar (full width, 80px height at bottom)
    navBarRect = Rect(0, windowHeight - bottomBarHeight, windowWidth, bottomBarHeight);

    // Main video area (centered, max height 720px)
    int videoHeight = min(720, windowHeight - topBarHeight - bottomBarHeight);
    int yOffset = topBarHeight;  // position video right below top bar
    videoRect = Rect(0, yOffset, windowWidth, videoHeight);

    // Record and export buttons (top)
    recordButtonRect = Rect(windowWidth - 320, 5, 150, 30);
    exportButtonRect = Rect(windowWidth - 160, 5, 150, 30);

    // Bottom bar buttons
    int btnWidth = 120;
    int btnHeight = 30;
    int startX = 10;

    // Position buttons in the navigation bar
    int buttonY = windowHeight - bottomBarHeight + (bottomBarHeight - btnHeight) / 2;

    zoomInButtonRect = Rect(startX, buttonY, btnWidth, btnHeight);
    startX += btnWidth + 10;

    zoomOutButtonRect = Rect(startX, buttonY, btnWidth, btnHeight);
    startX += btnWidth + 10;

    panUpButtonRect = Rect(startX, buttonY, btnWidth, btnHeight);
    startX += btnWidth + 10;

    panDownButtonRect = Rect(startX, buttonY, btnWidth, btnHeight);

    // Status area
    logLabelRect = Rect(10, 5, windowWidth - 330, 30);

    // Update global constant for consistent usage
    NAV_BAR_HEIGHT = bottomBarHeight;

    // Background subtraction controls area - INCREASED SIZE
    int controlsWidth = 400;       // Increased from maxContourArea
    int controlsHeight = 150;      // Increased from 100
    int controlsX = 10;
    int controlsY = topBarHeight + 10;

    bgSubControlsRect = Rect(controlsX, controlsY, controlsWidth, controlsHeight);
    bgSubSliderRect = Rect(controlsX + 20, controlsY + 50, controlsWidth - 40, 60); // Bigger slider

    // Toggle button for background subtraction controls - INCREASED SIZE
    int toggleBtnSize = 40;      // Increased from 30
    toggleBgSubControlsRect = Rect(controlsX + controlsWidth, controlsY, toggleBtnSize, toggleBtnSize);
    // In the initializeUI function, replace the toggleBgSubControlsRect initialization with:
    // Initialize control area first
    bgSubControlsRect = Rect(controlsX, controlsY, controlsWidth, controlsHeight);
    bgSubSliderRect = Rect(controlsX + 20, controlsY + 50, controlsWidth - 40, 60);

    // Then position the toggle button based on initial control visibility
    updateBgSubControlsTogglePosition(showBgSubControls);

    controlsWidth = 400;       
    controlsHeight = 150;      // Reduced from 280 since we don't need space for consecutive frames slider
    controlsX = 10;
    controlsY = topBarHeight + 10;

    bgSubControlsRect = Rect(controlsX, controlsY, controlsWidth, controlsHeight);
    
    // Position the dual range slider for contour area
    bgSubSliderRect = Rect(controlsX + 20, controlsY + 50, controlsWidth - 40, 60);
    
    // Remove the single range slider for consecutive frames
    
    // Then position the toggle button based on initial control visibility
    updateBgSubControlsTogglePosition(showBgSubControls);
}
