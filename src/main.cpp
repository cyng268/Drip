#include "common.h"
#include "camera.h"
#include "ui.h"
#include "serial.h"
#include "recording.h"
#include "config.h"
#include "background_subtraction.h"
#include "export_dialog.h"
#include "ui_helpers.h"
#include "navigation_bar.h"

// Global variables that need to be in main
Config appConfig;
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

// Thread-related variables
thread processingThread;
atomic<bool> isProcessing(false);
mutex frameMutex;

// Directory dialog variables
atomic<bool> directoryDialogActive(false);
mutex directoryResultMutex;
string selectedDirectoryResult = "";
bool directoryResultReady = false;

// Zoom control variables
int zoomLevel = 0;
int maxZoomLevel = 0x4000;
Rect zoomInButtonRect;
Rect zoomOutButtonRect;
bool serialInitialized = false;

// Button holding state variables
bool isZoomInHeld = false;
bool isZoomOutHeld = false;
system_clock::time_point lastZoomTime;
int ZOOM_DELAY_MS = 100;

// Background subtraction parameters
bool showBgSubControls = true;
int lowerBound = 0;
int upperBound = 200;
int minContourArea = 0;
int maxContourArea = 300;
Rect bgSubControlsRect;
Rect bgSubSliderRect;
Rect toggleBgSubControlsRect;

// New control variables for ICR and IR Correction
bool icrModeEnabled = false;
bool irCorrectionEnabled = false;
Rect icrButtonRect;
Rect irCorrectionButtonRect;

bool isDraggingMinHandle = false;
bool isDraggingMaxHandle = false;

// Display options
bool showFPS = false;
bool showNavBar = true;
Rect navBarRect;
Rect toggleNavButtonRect;
Mat upArrowImage;
Mat downArrowImage;

// Theme colors
Scalar THEME_COLOR = Scalar(20, 60, 20);
Scalar PROGRESS_BAR_COLOR = Scalar(0, 200, 0);
Scalar TEXT_COLOR = Scalar(220, 220, 220);
Scalar BUTTON_COLOR = Scalar(0, 204, 0);
Scalar BUTTON_TEXT_COLOR = Scalar(240, 240, 240);
Scalar HIGHLIGHT_COLOR = Scalar(40, 120, 40);

// UI layout parameters
int UI_FONT_SIZE = 1;
int PADDING = 10;
int BTN_HEIGHT = 60;
int NAV_BAR_HEIGHT = 80;
int NAV_BUTTON_SIZE = 40;

// UI components
Rect videoRect;
Rect recordButtonRect;
Rect progressBarRect;
Rect logLabelRect;
Rect exportButtonRect;
Rect logoRect;
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
    initIR(bgSubControlsRect.x, bgSubControlsRect.y, bgSubControlsRect.width, bgSubControlsRect.height);
    // Create custom arrow images
    createArrowImages();

    // Update toggle button position
    updateToggleButtonPosition(windowWidth);

    // Configure camera
    VideoCapture cap;
    cameraConfig(&cap);

    Mat frame;
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
            drawNavigationBar(uiFrame, windowWidth, isRecording, isProcessing, isZoomInHeld, isZoomOutHeld);
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
        
        // Draw window controls
        drawWindowControls(uiFrame);
        
        if (useFullscreen) {
            // Enter true fullscreen mode (no window decorations)
            setWindowProperty("Water Dripping Investigation Recording Tools", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
            isFullscreen = true;
        } else {
            // Normal window
            isFullscreen = false;
        }

        checkDirectorySelection();
        
        // Draw background subtraction controls
        drawBgSubControls(uiFrame, bgSubtractionActive);
        drawIR(uiFrame, bgSubtractionActive);

        imshow("Water Dripping Investigation Recording Tools", uiFrame);

        // Check for key press
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

    cout << "Closing the camera" << endl;
    cap.release();
    destroyAllWindows();
    cout << "Bye!" << endl;
    return 0;
}
