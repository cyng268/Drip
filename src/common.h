#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include <string>
#include <ctime>
#include <chrono>
#include <deque>
#include <iomanip>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <condition_variable>
#include "config.h"
#include <fstream>
#include <algorithm>

using namespace cv;
using namespace std;
using namespace chrono;

// Define a Point comparison operator for the map
struct PointCompare {
    bool operator() (const cv::Point& a, const cv::Point& b) const {
        return (a.x < b.x) || (a.x == b.x && a.y < b.y);
    }
};

extern Rect consFramesSliderRect;        // Slider area for consecutive frames
extern bool isDraggingConsFramesHandle;
extern int minConsFrames;           // Minimum value for consecutive frames
extern int maxConsFrames;          // Maximum value for consecutive frames

extern bool showBgSubControls;  // Flag to show/hide the contour range slider
extern int minContourArea;        // Min contour area value (default 50)
extern int maxContourArea;       // Max contour area value (default 200)
extern int lowerBound;        // Min contour area value (default 50)
extern int upperBound;        // Min contour area value (default 50)
extern Rect bgSubControlsRect;         // Area to place the controls
extern Rect bgSubSliderRect;           // Slider area
extern Rect toggleBgSubControlsRect;   // Button to toggle control visibility
extern bool isDraggingMinHandle;
extern bool isDraggingMaxHandle;

extern bool bgSubtractionActive;
extern Rect bgSubtractionRect;
extern Ptr<BackgroundSubtractorMOG2> backgroundSubtractor;
extern std::map<std::string, std::vector<std::map<int, float>>> objectOccurrences;
extern int frameNumber;
extern std::vector<cv::Point> detectionPoints;
extern std::map<cv::Point, int, PointCompare> detectionCounts;
extern int REQUIRED_CONSECUTIVE_FRAMES;  // Configurable: number of consecutive frames required
extern std::map<cv::Point, int, PointCompare> consecutiveDetections;  // Track consecutive detections
extern std::map<cv::Point, int, PointCompare> lastFrameDetected;      // Track when point was last detected

extern std::chrono::time_point<std::chrono::system_clock> bgSubStartTime;
extern const int BG_SUB_TIMEOUT_SECONDS;
extern bool isTimedOut;

extern queue<Mat> frameQueue;
extern mutex queueMutex;
extern condition_variable frameCondition;
extern atomic<bool> recordingThreadActive;
extern thread recordingThread;
extern double recordingDurationSeconds;


extern Config appConfig;
// Export dialog variables
extern bool showExportDialog;
extern Rect exportDialogRect;
extern string exportDestDir;
extern vector<string> recordingFiles;
extern vector<bool> fileSelection;
extern bool keepOriginalFiles;
extern int scrollOffset;
extern const int maxFilesVisible;
extern Rect fileListRect;
extern Rect dirSelectRect;
extern Rect keepFilesRect;
extern Rect exportConfirmRect;
extern Rect exportCancelRect;
extern Rect scrollUpRect;
extern Rect scrollDownRect;

// Global variables
extern bool isRecording;
extern VideoWriter videoWriter;
extern string filename;
extern string tempFilename;
extern bool isFirstFrame;
extern Size frameSize;
extern deque<double> fpsHistory;
extern const int FPS_HISTORY_SIZE;
extern system_clock::time_point recordingStartTime;
extern int WIDTH;
extern int HEIGHT;
extern int DISPLAY_WIDTH;
extern int DISPLAY_HEIGHT;

// Thread-related variables
extern thread processingThread;
extern atomic<bool> isProcessing;
extern mutex frameMutex;

extern atomic<bool> directoryDialogActive;
extern mutex directoryResultMutex;
extern string selectedDirectoryResult;
extern bool directoryResultReady;

// Zoom control variables
extern int zoomLevel;
extern int maxZoomLevel;
extern Rect zoomInButtonRect;
extern Rect zoomOutButtonRect;
extern bool serialInitialized;

// Button holding state variables
extern bool isZoomInHeld;
extern bool isZoomOutHeld;
extern system_clock::time_point lastZoomTime;
extern int ZOOM_DELAY_MS;

// Display options
extern bool showFPS;

// Theme colors
extern Scalar THEME_COLOR;
extern Scalar PROGRESS_BAR_COLOR;
extern Scalar TEXT_COLOR;
extern Scalar BUTTON_COLOR;
extern Scalar BUTTON_TEXT_COLOR;
extern Scalar HIGHLIGHT_COLOR;

// UI layout parameters
extern int UI_FONT_SIZE;
extern int PADDING;
extern int BTN_HEIGHT;
extern int NAV_BAR_HEIGHT;
extern int NAV_BUTTON_SIZE;

// Custom arrow images
extern Mat upArrowImage;
extern Mat downArrowImage;
extern bool showNavBar;
// UI components
extern Rect videoRect;
extern Rect recordButtonRect;
extern Rect progressBarRect;
extern Rect logLabelRect;
extern Rect exportButtonRect;
extern Rect logoRect;
extern Rect zoomInButtonRect;
extern Rect zoomOutButtonRect;
extern Rect navBarRect;
extern Rect toggleNavButtonRect;
extern Rect panUpButtonRect;
extern Rect panDownButtonRect;

extern int isFullscreen;
// Window control buttons
extern Rect minimizeButtonRect;
extern Rect closeButtonRect;

// New UI variables
extern bool hideUI;  // To track UI visibility
// Progress bar variables
extern atomic<int> progressValue;
extern const int progressMax;

// Log message
extern string logMessage;
extern mutex logMutex;

// Function to safely update log message
void setLogMessage(const string& message);

// Function to safely get log message
string getLogMessage();

#endif // COMMON_H
