#include "recording.h"
#include <cstdio>
#include <fstream>
#include <filesystem>

// In recording.cpp, modify the postProcessVideo function:

void postProcessVideo(const string& inputFilename, double recordingDurationSeconds) {
    // Check if input file exists
    if (access(inputFilename.c_str(), F_OK) != 0) {
        cerr << "ERROR: Input file does not exist: " << inputFilename << endl;
        setLogMessage("Error: File not found");
        isProcessing = false;
        return;
    }

    // Generate output filename
    string outputFilename = "./recordings/" + inputFilename.substr(5, 14) + ".avi";
    
    // Create directory if it doesn't exist
    filesystem::create_directories("./recordings/");
    
    // Get frame count using FFmpeg
    string frameCountCmd = "ffprobe -v error -count_frames -select_streams v:0 "
                         "-show_entries stream=nb_read_frames -of default=nokey=1:noprint_wrappers=1 "
                         "\"" + inputFilename + "\"";
    
    FILE* fpipeCount = popen(frameCountCmd.c_str(), "r");
    if (!fpipeCount) {
        cerr << "Error running FFprobe for frame count" << endl;
        setLogMessage("Error analyzing video");
        isProcessing = false;
        return;
    }
    
    char buffer[128];
    string frameCountStr = "";
    while (fgets(buffer, sizeof(buffer), fpipeCount) != NULL) {
        frameCountStr += buffer;
    }
    pclose(fpipeCount);
    
    // Remove trailing newline
    frameCountStr.erase(std::remove(frameCountStr.begin(), frameCountStr.end(), '\n'), 
                     frameCountStr.end());
    
    int totalFrames = 0;
    try {
        totalFrames = stoi(frameCountStr);
    } catch (...) {
        cerr << "Error converting frame count: " << frameCountStr << endl;
        totalFrames = 0;
    }
    
    double exactFPS = 30.0; // Default
    if (totalFrames > 0 && recordingDurationSeconds > 0) {
        exactFPS = totalFrames / recordingDurationSeconds;
        cout << "Calculated exact FPS: " << exactFPS << " (" << totalFrames 
             << " frames / " << recordingDurationSeconds << " seconds)" << endl;
    } else {
        cout << "Using default FPS: " << exactFPS << endl;
    }
    
    setLogMessage("Processing...");
    progressValue = 0; // Reset progress
    
    // Create a progress file path
    string progressFile = "/tmp/ffmpeg_progress.txt";
    
    // Use the exact FPS for accurate timing and add progress output
    string command = "ffmpeg -i \"" + inputFilename + "\" -c copy -r " + 
          to_string(exactFPS) + " -progress \"" + progressFile + "\" -hide_banner -loglevel error \"" + 
          outputFilename + "\" -y";
    
    cout << "FFmpeg command: " << command << endl;
    
    // Start FFmpeg process
    FILE* pipe = popen(command.c_str(), "r");
    
    if (!pipe) {
        cerr << "Error starting FFmpeg process" << endl;
        setLogMessage("Error starting process");
        isProcessing = false;
        return;
    }
    
    // Monitor progress while FFmpeg is running
    string line;
    int currentFrame = 0;
    bool processCompleted = false;
    
    while (isProcessing && !processCompleted) {
        // Sleep briefly to avoid hogging CPU
        this_thread::sleep_for(chrono::milliseconds(200));
        
        // Check if process is still running
        int result = system(("ps -p $(pgrep -f \"" + command + "\") > /dev/null 2>&1").c_str());
        if (result != 0) {
            // Process has completed
            processCompleted = true;
        }
        
        // Open and read the progress file
        ifstream progressStream(progressFile);
        if (progressStream) {
            // Read the last occurrence of frame= to get the most recent update
            string lastFrameLine;
            while (getline(progressStream, line)) {
                if (line.find("frame=") == 0) {
                    lastFrameLine = line;
                }
            }
            
            // Process the last frame line if found
            if (!lastFrameLine.empty()) {
                try {
                    currentFrame = stoi(lastFrameLine.substr(6));
                    if (totalFrames > 0) {
                        // Calculate progress percentage
                        progressValue = static_cast<int>((static_cast<double>(currentFrame) / totalFrames) * 100);
                        cout << "Current frame: " << currentFrame << " / " << totalFrames << endl;
                        // Cap at 99% until fully complete
                        if (progressValue >= 100) progressValue = 99;
                        
                        cout << "Progress: " << progressValue << "%" << endl;
                    }
                } catch (...) {
                    // Handle conversion errors
                }
            }
            progressStream.close();
        }
    }
    
    // Only check status after process completes
    if (processCompleted) {
        // Get the exit status
        int status = system(("ffprobe -v error \"" + outputFilename + "\" > /dev/null 2>&1").c_str());
        
        if (status == 0) {
            // Success
            progressValue = 100;
            setLogMessage("Saved to file");
            
            // Remove the temporary files
            remove(inputFilename.c_str());
            remove(progressFile.c_str());
        } else {
            setLogMessage("Error processing video");
        }
    }
    
    isProcessing = false;
}