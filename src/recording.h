#ifndef RECORDING_H
#define RECORDING_H

#include "common.h"

// Function to post-process video to match actual FPS
void postProcessVideo(const string& inputFilename, double actualFPS);

#endif // RECORDING_H
