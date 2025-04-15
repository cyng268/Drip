#include "common.h"
#include "camera.h"
#include "ui.h"
#include "serial.h"
#include "recording.h"
#include "config.h"

Config appConfig;
// Export dialog variables
bool showExportDialog = false;
Rect exportDialogRect;
string exportDestDir = "./recordings/";
vector<string> recordingFiles;
vector<bool> fileSelection;
bool keepOriginalFiles = true;
int scrollOffset = 0;
const int maxFilesVisible = 10;
Rect fileListRect;
Rect dirSelectRect;
Rect keepFilesRect;
Rect exportConfirmRect;
Rect exportCancelRect;
Rect scrollUpRect;
Rect scrollDownRect;

queue<Mat> frameQueue;
mutex queueMutex;
condition_variable frameCondition;
atomic<bool> recordingThreadActive(false);
thread recordingThread;
double recordingDurationSeconds = 0.0;

// Define global variables
bool isRecording = false;
VideoWriter videoWriter;
string filename;
string tempFilename;
bool isFirstFrame = true;
Size frameSize;
deque<double> fpsHistory;
const int FPS_HISTORY_SIZE = 30;
system_clock::time_point recordingStartTime;
int WIDTH = 1280;
int HEIGHT = 720;
int DISPLAY_WIDTH = 1280;
int DISPLAY_HEIGHT = 800;

bool isDraggingConsFramesHandle = false;
int minConsFrames = 1;           // Minimum value for consecutive frames
int maxConsFrames = 10;          // Maximum value for consecutive frames

// Thread-related variables
thread processingThread;
atomic<bool> isProcessing(false);
mutex frameMutex;

//file explorer
atomic<bool> directoryDialogActive(false);
mutex directoryResultMutex;
string selectedDirectoryResult = "";
bool directoryResultReady = false;

// Zoom control variables
int zoomLevel = 0;
int maxZoomLevel = 0x8000;
Rect zoomInButtonRect;
Rect zoomOutButtonRect;
bool serialInitialized = false;

// Button holding state variables
bool isZoomInHeld = false;
bool isZoomOutHeld = false;
system_clock::time_point lastZoomTime;
int ZOOM_DELAY_MS = 100;

// Background subtraction parameters
bool showBgSubControls = true;  // Flag to show/hide the contour range slider
int lowerBound = 0;        // Min contour area value (default 50)
int upperBound = 200;       // Max contour area value (default 200)
int minContourArea = 0; // Minimum contour area for detection
int maxContourArea = 300; // Maximum contour area for detection
Rect bgSubControlsRect;         // Area to place the controls
Rect bgSubSliderRect;           // Slider area
Rect toggleBgSubControlsRect;   // Button to toggle control visibility

bool isDraggingMinHandle = false;
bool isDraggingMaxHandle = false;

// Display options
bool showFPS = false;
bool showNavBar = true;  // Navigation bar visibility flag
Rect navBarRect;          // Navigation bar area
Rect toggleNavButtonRect; // Button to toggle navigation bar
Mat upArrowImage;         // Custom up arrow image
Mat downArrowImage;       // Custom down arrow image

// Theme colors
// Theme colors - change to green theme
Scalar THEME_COLOR = Scalar(20, 60, 20);         // Dark green background
Scalar PROGRESS_BAR_COLOR = Scalar(0, 200, 0);   // Bright green progress
Scalar TEXT_COLOR = Scalar(220, 220, 220);       // Light gray text
Scalar BUTTON_COLOR = Scalar(0, 204, 0);      // Medium green buttons
Scalar BUTTON_TEXT_COLOR = Scalar(240, 240, 240); // Slightly off-white text
Scalar HIGHLIGHT_COLOR = Scalar(40, 120, 40);    // Lighter green for highlights

// UI layout parameters
int UI_FONT_SIZE = 1;
int PADDING = 10;
int BTN_HEIGHT = 60;
int NAV_BAR_HEIGHT = 80; // Height of the navigation bar
int NAV_BUTTON_SIZE = 40; // Size of the navigation toggle button

// UI components
Rect videoRect;
Rect recordButtonRect;
Rect progressBarRect;
Rect logLabelRect;
Rect exportButtonRect;
Rect logoRect;
// Pan control variables
Rect panUpButtonRect;
Rect panDownButtonRect;

// Window control buttons
Rect minimizeButtonRect;
Rect closeButtonRect;

// Progress bar variables
atomic<int> progressValue(0);
const int progressMax = 100;

// Log message
string logMessage = "";
mutex logMutex;

int isFullscreen = true;

bool bgSubtractionActive = false;
Rect bgSubtractionRect;
Ptr<BackgroundSubtractorMOG2> backgroundSubtractor;
std::map<std::string, std::vector<std::map<int, float>>> objectOccurrences;
int frameNumber = 0;
std::vector<cv::Point> detectionPoints;
std::map<cv::Point, int, PointCompare> detectionCounts;

std::chrono::time_point<std::chrono::system_clock> bgSubStartTime;
const int BG_SUB_TIMEOUT_SECONDS = 10;
bool isTimedOut = false;

std::string getTimestampString() {
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&nowTime), "%Y%m%d_%H%M%S");
    return ss.str();
}

// Function to safely update log message
void setLogMessage(const string& message) {
    lock_guard<mutex> lock(logMutex);
    logMessage = message;
}

// Function to safely get log message
string getLogMessage() {
    lock_guard<mutex> lock(logMutex);
    return logMessage;
}

// Add this function before drawDetectionResults
Point findSimilarPoint(const Point& newPoint, const std::vector<Point>& existingPoints) {
    for (const auto& point : existingPoints) {
        int xDiff = abs(point.x - newPoint.x);
        int yDiff = abs(point.y - newPoint.y);
        
        // If both x and y are within 1 pixel, consider it the same point
        if (xDiff <= 1 && yDiff <= 1) {
            return point;
        }
    }
    return Point(-1, -1); // No similar point found
}
// Function to create custom arrow images
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

// Update toggle button position based on navigation bar visibility
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

// Helper function to draw detection results
// Replace the entire drawDetectionResults function with this version
void drawDetectionResults(Mat& frame) {
    // Draw the detection points and their counts
    for (const auto& point : detectionPoints) {
        if (detectionCounts.find(point) != detectionCounts.end()) {
            int count = detectionCounts[point];
            
            // MODIFIED: Only check total count, not consecutive frames
            if (count > 5){
                // Draw a small red box around the detection point
                rectangle(frame, 
                         Rect(point.x - 5, point.y - 5, 10, 10), 
                         Scalar(0, 0, 255), 1);
                
                // Draw total count only
                std::string countText = to_string(count);
                putText(frame, countText, 
                        Point(point.x + 7, point.y - 3), 
                        FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2);
            }
        }
    }
}

// Function to save top detection points to a file
void saveTopDetectionPoints(int topCount) {
    // Convert map to vector for sorting
    std::vector<std::pair<Point, int>> countVector;
    for (const auto& pair : detectionCounts) {
        if (pair.second > 1) { // Only consider points with more than 5 occurrences
            countVector.push_back(pair);
        }
    }
    
    // Sort by count (descending)
    std::sort(countVector.begin(), countVector.end(), 
              [](const std::pair<Point, int>& a, const std::pair<Point, int>& b) {
                  return a.second > b.second;
              });
    
    // Create filename with timestamp
    std::string timestamp = getTimestampString();
    std::string filename = "./drip_detect/detection_results_" + timestamp + ".csv";
    
    // Open file for writing
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }
    
    // Write header
    outFile << "Rank,X,Y,Count" << std::endl;
    
    // Write top points
    int count = 0;
    for (const auto& pair : countVector) {
        if (count >= topCount) break;
        
        outFile << count + 1 << "," 
                << pair.first.x << "," 
                << pair.first.y << "," 
                << pair.second << std::endl;
        
        count++;
    }
    
    outFile.close();
    std::cout << "Saved top " << count << " detection points to " << filename << std::endl;
    
    // Also save a summary to a log file that appends results
    std::ofstream logFile("./drip_detect/detection_log.csv", std::ios::app);
    if (logFile.is_open()) {
        // Write a single summary line with timestamp and top detection
        if (!countVector.empty()) {
            logFile << timestamp << "," 
                    << countVector[0].first.x << "," 
                    << countVector[0].first.y << "," 
                    << countVector[0].second;
            
            // Add total detections count
            int totalDetections = 0;
            for (const auto& pair : countVector) {
                totalDetections += pair.second;
            }
            logFile << "," << totalDetections;
            
            logFile << std::endl;
        }
        logFile.close();
    }
}

// Updated background subtraction function
void processBackgroundSubtraction(Mat& frame) {
    if (!bgSubtractionActive || bgSubtractionRect.width <= 0 || bgSubtractionRect.height <= 0) {
        return;
    }
    
    // Check for timeout
    auto currentTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed = currentTime - bgSubStartTime;
    int remainingSeconds = BG_SUB_TIMEOUT_SECONDS - static_cast<int>(elapsed.count());
    
    // If time is up and not already processed
    if (remainingSeconds <= 0 && !isTimedOut) {
        isTimedOut = true;
        
        // Save the top 10 detection points to a file
        saveTopDetectionPoints(10);
        
        // Don't disable bg subtraction so user can still see results
        // but stop processing new frames
        setLogMessage("BG Results saved.");
        bgSubtractionActive = false;
        return;
    }
    
    // If already timed out, just show the results but don't process new frames
    if (isTimedOut) {
        // Just draw the results but don't process
        drawDetectionResults(frame);
        return;
    }
    
    // Ensure the rectangle is within bounds
    Rect safeRect = bgSubtractionRect & Rect(0, 0, frame.cols, frame.rows);
    if (safeRect.width <= 0 || safeRect.height <= 0) {
        return;
    }
    
    // Extract the ROI (Region of Interest)
    Mat roi = frame(safeRect);
    
    // Apply background subtraction
    Mat foregroundMask;
    backgroundSubtractor->apply(roi, foregroundMask);
    
    // Thresholding to get binary mask
    threshold(foregroundMask, foregroundMask, 250, 255, THRESH_BINARY);
    
    // Morphological operations to remove noise
    Mat kernel;
    erode(foregroundMask, foregroundMask, kernel, Point(-1,-1), 1);
    dilate(foregroundMask, foregroundMask, kernel, Point(-1,-1), 2);
    
    // Find contours
    vector<vector<Point>> contours;
    findContours(foregroundMask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    
    // Display contours count and remaining time
    putText(frame, "Contours: " + to_string(contours.size()) + " | Time left: " + to_string(remainingSeconds) + "s", 
            Point(safeRect.x, safeRect.y - 10), 
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 1);
    
    // Process contours
    for (const auto& contour : contours) {
        double area = contourArea(contour);
        
        // Filter contours by area (similar to Python code)
        if (area > lowerBound && area < upperBound) {
            Rect boundingBox = boundingRect(contour);
            
            // Adjust coordinates to frame coordinates
            int x = boundingBox.x + safeRect.x;
            int y = boundingBox.y + safeRect.y;
            
            // Create unique object ID based on position
            string objectId = "object_" + to_string(x) + "_" + to_string(y);
            
            // Store occurrence
            if (objectOccurrences.find(objectId) == objectOccurrences.end()) {
                std::map<int, float> frameData;
                frameData[frameNumber] = area;
                objectOccurrences[objectId] = {frameData};
            } else {
                std::map<int, float> frameData;
                frameData[frameNumber] = area;
                objectOccurrences[objectId].push_back(frameData);
            }
            
            // Draw rectangle on the original frame (adjusted to frame coordinates)
            rectangle(frame, 
                     Rect(x - 4, y - 4, boundingBox.width + 8, boundingBox.height + 8), 
                     Scalar(0, 0, 255), 1);
            
            // Draw label
            putText(frame, "drop", Point(x, y - 10), 
                    FONT_HERSHEY_SIMPLEX, 0.3, Scalar(0, 255, 0), 1);
            
            // Store detection point
            Point detectionPoint(x, y);

            // Check if this point is similar to any existing point
            Point existingPoint = findSimilarPoint(detectionPoint, detectionPoints);

            if (existingPoint.x == -1) {
                // No similar point found, add new point
                detectionCounts[detectionPoint] = 1;
                detectionPoints.push_back(detectionPoint);
            } else {
                // Similar point found, update existing point
                detectionCounts[existingPoint]++;
            }
        }
    }
    
    // Draw the detection results
    drawDetectionResults(frame);
    
    // Draw background subtraction ROI rectangle
    rectangle(frame, safeRect, Scalar(0, 255, 0), 2);

    // Increment frame number
    frameNumber++;
}



int main(int argc, char** argv) {

// Load configuration
    appConfig.loadConfig();

    // Apply configuration settings
    DISPLAY_WIDTH = appConfig.getInt("DISPLAY_WIDTH", 1280);
    DISPLAY_HEIGHT = appConfig.getInt("DISPLAY_HEIGHT", 800);
    WIDTH = appConfig.getInt("CAMERA_WIDTH", 1280);
    HEIGHT = appConfig.getInt("CAMERA_HEIGHT", 720);
    exportDestDir = appConfig.getString("EXPORT_DEST_DIR", "./recordings/");
    keepOriginalFiles = appConfig.getBool("KEEP_ORIGINAL_FILES", true);
    showFPS = appConfig.getBool("SHOW_FPS", false);
    showNavBar = appConfig.getBool("SHOW_NAV_BAR", true);
    bool useFullscreen = appConfig.getBool("FULL_SCREEN", true);
    lowerBound = appConfig.getInt("LOWERBOUND", 50);
    upperBound = appConfig.getInt("UPPERBOUND", 200);
    minContourArea = appConfig.getInt("MIN_CONTOUR_AREA", 0);
    maxContourArea = appConfig.getInt("MAX_CONTOUR_AREA", 300);
    showBgSubControls = appConfig.getBool("SHOW_BG_SUB_CONTROLS", true);
    // Create a window with a specific size
    int windowWidth = DISPLAY_WIDTH;
    int windowHeight = DISPLAY_HEIGHT;

    // Create a window and set it to fullscreen consistently
    namedWindow("Water Dripping Investigation Recording Tools", WINDOW_GUI_NORMAL);
    resizeWindow("Water Dripping Investigation Recording Tools", windowWidth, windowHeight);
    // Add this to your global variables

    if (useFullscreen) {
        // Enter true fullscreen mode (no window decorations)
        setWindowProperty("Water Dripping Investigation Recording Tools", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
    } else {
        destroyWindow("Water Dripping Investigation Recording Tools");
        namedWindow("Water Dripping Investigation Recording Tools", WINDOW_GUI_NORMAL);
        resizeWindow("Water Dripping Investigation Recording Tools", windowWidth, windowHeight);
    }

    setMouseCallback("Water Dripping Investigation Recording Tools", mouseCallback, NULL);

    // Initialize UI component positions
    initializeUI(windowWidth, windowHeight);

    // Create custom arrow images
    createArrowImages();

    // Update toggle button position
    updateToggleButtonPosition(windowWidth);

    // Configure camera
    VideoCapture cap;
    cameraConfig(&cap);

    Mat frame;
    // In main.cpp, modify this line:
    Mat uiFrame(DISPLAY_HEIGHT, DISPLAY_WIDTH, CV_8UC3, THEME_COLOR);
    cout << "Camera opened successfully. Press ESC to exit." << endl;
    setLogMessage("");

    bool videoWriterInitialized = false;
    system_clock::time_point previousFrameTime = system_clock::now();
    double currentFPS = 0.0;

    // Load record and stop images/icons
    Mat recIcon(BTN_HEIGHT, BTN_HEIGHT, CV_8UC3, Scalar(0, 200, 0));  // Green
    Mat stopIcon(BTN_HEIGHT, BTN_HEIGHT, CV_8UC3, Scalar(200, 0, 0)); // Red

    // Create a simple logo
    Mat logoImg(logoRect.height, logoRect.width, CV_8UC3, THEME_COLOR);
    putText(logoImg, "LOGO", Point(logoImg.cols/2 - 50, logoImg.rows/2 + 10),
            FONT_HERSHEY_SIMPLEX, 1.0, TEXT_COLOR, 2);

    // Create overlay for navigation bar
    Mat navBarOverlay(NAV_BAR_HEIGHT, windowWidth, CV_8UC3, Scalar(40, 40, 40));

    while (true) {
        if (getWindowProperty("Water Dripping Investigation Recording Tools", WND_PROP_VISIBLE) < 1) {
            cout << "Window closed, exiting..." << endl;
            break;
        }

        bool frameRead = cap.read(frame);
        if (!frameRead || frame.empty()) {
            cerr << "ERROR: Unable to grab from the camera" << endl;
            setLogMessage("Error");
            break;
        }
        if (frameRead && !frame.empty()) {
            // Process background subtraction if active
            processBackgroundSubtraction(frame);
            
            // Your existing code to resize and display the frame...
        }
        // Calculate FPS
        currentFPS = calculateFPS(previousFrameTime);

        // Update FPS history
        fpsHistory.push_back(currentFPS);
        if (fpsHistory.size() > FPS_HISTORY_SIZE) {
            fpsHistory.pop_front();
        }

        // Calculate average FPS from history
        double avgFPS = 0;
        for (const auto& fps : fpsHistory) {
            avgFPS += fps;
        }
        avgFPS = avgFPS / fpsHistory.size();

        // Store frame size on first successful capture
        if (isFirstFrame) {
            frameSize = frame.size();
            isFirstFrame = false;
            cout << "Actual frame size: " << frameSize.width << "x" << frameSize.height << endl;
        }

        // Initialize VideoWriter if recording is requested and not yet initialized
        if (isRecording && !videoWriterInitialized) {
            // Check if we have valid frame dimensions
            if (frameSize.width > 0 && frameSize.height > 0) {
                int codec = VideoWriter::fourcc('M', 'J', 'P', 'G');
                double fps = 30.0; // Target FPS for raw recording

                // Use temp filename for direct recording to file
                videoWriter.open(tempFilename, codec, fps, frameSize, true);

                if (!videoWriter.isOpened()) {
                    cerr << "ERROR: Could not open the output video file for write" << endl;
                    isRecording = false;
                    setLogMessage("Error");
                } else {
                    videoWriterInitialized = true;
                    cout << "Started recording to " << tempFilename << endl;
                    setLogMessage("Recording...");
                }
            } else {
                cerr << "ERROR: Invalid frame dimensions: " << frameSize.width << "x" << frameSize.height << endl;
                isRecording = false;
                setLogMessage("Error");
            }
        }

        // Create a full screen frame from camera input
        resize(frame, uiFrame, Size(windowWidth, windowHeight));

        // Display date, time and FPS on the video
        string dateStr = getCurrentDateStr();
        string timeStr = getCurrentTimeStr();
        string displayStr = dateStr + " " + timeStr;
        if (showFPS) {
            displayStr += " FPS: " + to_string(int(avgFPS));
        }
        putText(uiFrame, displayStr, Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.7, TEXT_COLOR, 2);

        // Show recording indicator in top-right corner if recording
        if (isRecording) {
            // Position the recording indicator to avoid conflict with minimize/close buttons
            // Move it to the left side of the minimize button with some padding
            int recIndicatorX = minimizeButtonRect.x - 140; // Leave space for text
            Rect recIndicator(recIndicatorX, 10, 20, 20);
            circle(uiFrame, Point(recIndicator.x + 10, recIndicator.y + 10), 10, Scalar(0, 0, 255), -1);

            // Show recording time
            auto currentTime = system_clock::now();
            duration<double> elapsedSecs = currentTime - recordingStartTime;
            int mins = static_cast<int>(elapsedSecs.count()) / 60;
            int secs = static_cast<int>(elapsedSecs.count()) % 60;
            char timeBuffer[10];
            sprintf(timeBuffer, "%02d:%02d", mins, secs);

            // Display recording time next to the red dot
            Point timePos(recIndicator.x + 25, recIndicator.y + 15);
            putText(uiFrame, timeBuffer, timePos, FONT_HERSHEY_SIMPLEX, UI_FONT_SIZE, Scalar(255, 255, 255), 2);
        }

        // Draw navigation bar if visible
        if (showNavBar) {
            // Create semi-transparent navigation bar at the bottom
            Mat bottomBar = uiFrame(navBarRect).clone();
            Mat overlay(bottomBar.rows, bottomBar.cols, bottomBar.type(), Scalar(20, 60, 20));
            double alpha = 0.7; // Transparency level (0.0 = fully transparent, 1.0 = opaque)
            addWeighted(overlay, alpha, bottomBar, 1.0 - alpha, 0.0, bottomBar);
            bottomBar.copyTo(uiFrame(navBarRect));

            // Draw record/stop button on the navigation bar
            int btnWidth = 140;
            int btnHeight = NAV_BAR_HEIGHT - 20;
            int btnSpacing = 20;
            int startX = PADDING;

            // Record button
            Rect recBtnRect(startX, navBarRect.y + 10, btnWidth, btnHeight);
            rectangle(uiFrame, recBtnRect, isRecording ? Scalar(60, 0, 0) : BUTTON_COLOR, -1);
            rectangle(uiFrame, recBtnRect, Scalar(100, 100, 100), 1);
            putText(uiFrame, isRecording ? "Stop" : "Record",
                    Point(recBtnRect.x + 10, recBtnRect.y + btnHeight/2 + 5),
                    FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.2);

            // Export button
            startX += btnWidth + btnSpacing;
            Rect exportBtnRect(startX, navBarRect.y + 10, btnWidth, btnHeight);
            Scalar exportBtnColor = isProcessing ? Scalar(30, 30, 30) : BUTTON_COLOR;
            rectangle(uiFrame, exportBtnRect, exportBtnColor, -1);
            rectangle(uiFrame, exportBtnRect, Scalar(100, 100, 100), 1);
            putText(uiFrame, "Export", Point(exportBtnRect.x + 10, exportBtnRect.y + btnHeight/2 + 5),
                    FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.2);

            // Zoom In button
            startX += btnWidth + btnSpacing;
            Rect zoomInBtnRect(startX, navBarRect.y + 10, btnWidth, btnHeight);
            rectangle(uiFrame, zoomInBtnRect, isZoomInHeld ? Scalar(100, 200, 100) : BUTTON_COLOR, -1);
            rectangle(uiFrame, zoomInBtnRect, Scalar(100, 100, 100), 1);
            putText(uiFrame, "Zoom +", Point(zoomInBtnRect.x + 10, zoomInBtnRect.y + btnHeight/2 + 5),
                    FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.2);

            // Zoom Out button
            startX += btnWidth + btnSpacing;
            Rect zoomOutBtnRect(startX, navBarRect.y + 10, btnWidth, btnHeight);
            rectangle(uiFrame, zoomOutBtnRect, isZoomOutHeld ? Scalar(100, 200, 100) : BUTTON_COLOR, -1);
            rectangle(uiFrame, zoomOutBtnRect, Scalar(100, 100, 100), 1);
            putText(uiFrame, "Zoom -", Point(zoomOutBtnRect.x + 10, zoomOutBtnRect.y + btnHeight/2 + 5),
                    FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.2);

            startX += btnWidth + btnSpacing;
            Rect panUpBtnRect(startX, navBarRect.y + 10, btnWidth, btnHeight);
            rectangle(uiFrame, panUpBtnRect, BUTTON_COLOR , -1);
            rectangle(uiFrame, panUpBtnRect, Scalar(100, 150, 100), 1);
            putText(uiFrame, "Pan Up", Point(panUpBtnRect.x + 10, panUpBtnRect.y + btnHeight/2 + 5),
                    FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.2);

            // Pan Down button
            startX += btnWidth + btnSpacing;
            Rect panDownBtnRect(startX, navBarRect.y + 10, btnWidth, btnHeight);
            rectangle(uiFrame, panDownBtnRect, BUTTON_COLOR, -1);
            rectangle(uiFrame, panDownBtnRect, Scalar(100, 150, 100), 1);
            putText(uiFrame, "Pan Down", Point(panDownBtnRect.x + 10, panDownBtnRect.y + btnHeight/2 + 5),
                    FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.2);

            // Update button rectangles for mouse callback
            panUpButtonRect = panUpBtnRect;
            panDownButtonRect = panDownBtnRect;

            // Status display
            startX += btnWidth + btnSpacing * 2;
            Rect statusRect(startX, navBarRect.y + 10, windowWidth - startX - PADDING, btnHeight);
            rectangle(uiFrame, statusRect, Scalar(40, 40, 40), -1);

            // Show status/log message
            putText(uiFrame, getLogMessage(), Point(statusRect.x + 10, statusRect.y + btnHeight/2 + 5),
                    FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 2.2);

            // Update button rectangles for mouse callback
            recordButtonRect = recBtnRect;
            exportButtonRect = exportBtnRect;
            zoomInButtonRect = zoomInBtnRect;
            zoomOutButtonRect = zoomOutBtnRect;

            // Show progress bar if processing
            // if (isProcessing) {
            //     Rect progressRect(PADDING, navBarRect.y - 15, windowWidth - PADDING*2, 10);
            //     drawProgressBar(uiFrame, progressRect, progressValue, progressMax, PROGRESS_BAR_COLOR);
            // }
        }

        // Update toggle button position and draw it
        updateToggleButtonPosition(windowWidth);

        if (showNavBar) {
            // Draw down arrow (to hide navigation bar)
            downArrowImage.copyTo(uiFrame(toggleNavButtonRect));
        } else {
            // Draw up arrow (to show navigation bar)
            upArrowImage.copyTo(uiFrame(toggleNavButtonRect));
        }

        // Check for held zoom buttons and perform continuous zooming
        auto currentTime = system_clock::now();
        duration<double, std::milli> elapsed = currentTime - lastZoomTime;

        if ((isZoomInHeld || isZoomOutHeld) && elapsed.count() >= ZOOM_DELAY_MS) {
            if (isZoomInHeld) {
                zoomIn();
            }
            else if (isZoomOutHeld) {
                zoomOut();
            }
            lastZoomTime = currentTime;
        }

        // If recording, write frame directly to temp file
        if (isRecording && videoWriterInitialized) {
            try {
                // Add overlays to the frame before saving
                Mat frameWithOverlay = frame.clone();

                // Add date, time and FPS in a single line
                string dateStr = getCurrentDateStr();
                string timeStr = getCurrentTimeStr();
                string displayStr = dateStr + " " + timeStr;
                if (showFPS) {
                    displayStr += " FPS: " + to_string(int(avgFPS));
                }

                putText(frameWithOverlay, displayStr, Point(10, 30),
                        FONT_HERSHEY_SIMPLEX, 0.7, TEXT_COLOR, 2);

                videoWriter.write(frameWithOverlay);
            } catch (const cv::Exception& e) {
                cerr << "ERROR: Exception while writing video: " << e.what() << endl;
                videoWriter.release();
                isRecording = false;
                videoWriterInitialized = false;
                setLogMessage("Error");
            }
        } else {
            // Reset videoWriterInitialized when not recording
            videoWriterInitialized = false;
        }
        if (showExportDialog) {
            drawExportDialog(uiFrame);
        }
        rectangle(uiFrame, closeButtonRect, Scalar(100, 40, 40), -1);
        rectangle(uiFrame, closeButtonRect, Scalar(160, 80, 80), 2);
        rectangle(uiFrame, minimizeButtonRect, Scalar(40, 100, 40), -1);
        rectangle(uiFrame, minimizeButtonRect, Scalar(80, 160, 80), 2);
        if (useFullscreen) {
            // Enter true fullscreen mode (no window decorations)
            setWindowProperty("Water Dripping Investigation Recording Tools", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
            isFullscreen = true;  // Set the initial state
        } else {
            // Normal window
            isFullscreen = false;
        }

        // Draw close button
        rectangle(uiFrame, closeButtonRect, Scalar(100, 40, 40), -1);
        rectangle(uiFrame, closeButtonRect, Scalar(160, 80, 80), 2);
        // Draw X icon
        line(uiFrame,
            Point(closeButtonRect.x + closeButtonRect.width/4, closeButtonRect.y + closeButtonRect.height/4),
            Point(closeButtonRect.x + 3*closeButtonRect.width/4, closeButtonRect.y + 3*closeButtonRect.height/4),
            Scalar(220, 220, 220), 2);
        line(uiFrame,
            Point(closeButtonRect.x + 3*closeButtonRect.width/4, closeButtonRect.y + closeButtonRect.height/4),
            Point(closeButtonRect.x + closeButtonRect.width/4, closeButtonRect.y + 3*closeButtonRect.height/4),
            Scalar(220, 220, 220), 2);

        if (isFullscreen) {
            // Draw minimize icon (horizontal line)
            line(uiFrame,
                 Point(minimizeButtonRect.x + minimizeButtonRect.width/4, minimizeButtonRect.y + minimizeButtonRect.height/2),
                 Point(minimizeButtonRect.x + 3*minimizeButtonRect.width/4, minimizeButtonRect.y + minimizeButtonRect.height/2),
                 Scalar(220, 220, 220), 2);
        } else {
            // Draw maximize icon (plus sign)
            line(uiFrame,
                 Point(minimizeButtonRect.x + minimizeButtonRect.width/4, minimizeButtonRect.y + minimizeButtonRect.height/2),
                 Point(minimizeButtonRect.x + 3*minimizeButtonRect.width/4, minimizeButtonRect.y + minimizeButtonRect.height/2),
                 Scalar(220, 220, 220), 2);
            line(uiFrame,
                 Point(minimizeButtonRect.x + minimizeButtonRect.width/2, minimizeButtonRect.y + minimizeButtonRect.height/4),
                 Point(minimizeButtonRect.x + minimizeButtonRect.width/2, minimizeButtonRect.y + 3*minimizeButtonRect.height/4),
                 Scalar(220, 220, 220), 2);
        }
        checkDirectorySelection();
        // Draw toggle button for controls
        rectangle(uiFrame, toggleBgSubControlsRect, Scalar(100, 150, 100), 2); // Thicker border

        // In the main loop, when drawing the background subtraction controls:

        // First check if we should draw the controls
        // In main.cpp, in the main loop where you're drawing the background subtraction controls
    if (showBgSubControls) {
        // Draw the controls as before
        rectangle(uiFrame, bgSubControlsRect, Scalar(30, 40, 30, 180), -1);  
        rectangle(uiFrame, bgSubControlsRect, Scalar(100, 150, 100), 2);
        
        // Draw dual range slider for contour area
        drawDualRangeSlider(uiFrame, bgSubSliderRect, lowerBound, upperBound, 
                        minContourArea, maxContourArea, "Contour Area Range");
        
        if (bgSubtractionActive) {
            // Overlay if controls are disabled
            rectangle(uiFrame, bgSubControlsRect, Scalar(40, 40, 40, 150), -1);
            putText(uiFrame, "Controls disabled during active detection",
                    Point(bgSubControlsRect.x + 20, bgSubControlsRect.y + bgSubControlsRect.height/2),
                    FONT_HERSHEY_SIMPLEX, 0.5, Scalar(200, 200, 200), 2);
        }
    }

        // Always draw the toggle button
        rectangle(uiFrame, toggleBgSubControlsRect, Scalar(60, 80, 60), -1);
        rectangle(uiFrame, toggleBgSubControlsRect, Scalar(100, 150, 100), 2);

        // Draw the appropriate icon
        if (showBgSubControls) {
            // Draw minus sign
            line(uiFrame, 
                Point(toggleBgSubControlsRect.x + 10, toggleBgSubControlsRect.y + toggleBgSubControlsRect.height/2),
                Point(toggleBgSubControlsRect.x + toggleBgSubControlsRect.width - 10, toggleBgSubControlsRect.y + toggleBgSubControlsRect.height/2),
                Scalar(220, 220, 220), 3);
        } else {
            // Draw plus sign
            line(uiFrame, 
                Point(toggleBgSubControlsRect.x + 10, toggleBgSubControlsRect.y + toggleBgSubControlsRect.height/2),
                Point(toggleBgSubControlsRect.x + toggleBgSubControlsRect.width - 10, toggleBgSubControlsRect.y + toggleBgSubControlsRect.height/2),
                Scalar(220, 220, 220), 3);
            line(uiFrame,
                Point(toggleBgSubControlsRect.x + toggleBgSubControlsRect.width/2, toggleBgSubControlsRect.y + 10),
                Point(toggleBgSubControlsRect.x + toggleBgSubControlsRect.width/2, toggleBgSubControlsRect.y + toggleBgSubControlsRect.height - 10),
                Scalar(220, 220, 220), 3);
        }

        imshow("Water Dripping Investigation Recording Tools", uiFrame);

        // Check for key pressf
        int key = waitKey(1);
        if (key == 27) // ESC key
            break;
        else if (key == 'f' || key == 'F')  // Toggle FPS display
            showFPS = !showFPS;

        else if (key == 'n' || key == 'N')  // Toggle navigation bar
            showNavBar = !showNavBar;
        else if (key == 'b' || key == 'B') {  // Cancel background subtraction
            bgSubtractionActive = false;
            setLogMessage("BG canceled");
        }
    }

    // Clean up
    if (isRecording && videoWriter.isOpened()) {
        videoWriter.release();
        cout << "Stopped recording and saved to " << tempFilename << endl;
    }

    // Cancel any ongoing processing
    if (isProcessing) {
        isProcessing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }

    if (serialInitialized) {
        cameraSerial.closeDevice();
    }
    // Save any changed configuration

    cout << "Closing the camera" << endl;
    cap.release();
    destroyAllWindows();
    cout << "Bye!" << endl;
    return 0;
}
