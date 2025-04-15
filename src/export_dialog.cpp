#include "export_dialog.h"
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>

MouseCallbackData mouseData;

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
    vector<Rect> fileCheckboxRects;
    vector<Rect> fileTextRects;
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
            string srcPath = "./recordings/" + recordingFiles[i];
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
