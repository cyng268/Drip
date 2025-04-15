#include "background_subtraction.h"
#include <fstream>
#include <algorithm>

void initBgSubControls(int controlsX, int controlsY, int controlsWidth, int controlsHeight) {
    bgSubControlsRect = Rect(controlsX, controlsY, controlsWidth, controlsHeight);
    bgSubSliderRect = Rect(controlsX + 20, controlsY + 50, controlsWidth - 40, 60);
    
    // Position toggle button based on initial control visibility
    updateBgSubControlsTogglePosition(showBgSubControls);
}

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

std::string getTimestampString() {
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&nowTime), "%Y%m%d_%H%M%S");
    return ss.str();
}

void saveTopDetectionPoints(int topCount) {
    // Convert map to vector for sorting
    std::vector<std::pair<Point, int>> countVector;
    for (const auto& pair : detectionCounts) {
        if (pair.second > 1) { // Only consider points with more than 1 occurrence
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
    
    // Create directory if it doesn't exist
    std::filesystem::create_directories("./drip_detect/");
    
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
        
        // Filter contours by area
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

void drawBgSubControls(Mat& uiFrame, bool bgSubtractionActive) {
    if (showBgSubControls) {
        // Draw the controls
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
}
