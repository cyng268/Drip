#ifndef SERIAL_H
#define SERIAL_H

#include "common.h"
#include "serialib.h"

extern serialib cameraSerial;

// Initialize serial connection
bool initializeSerial();

// Send zoom command
void sendZoomCommand(int level);

// Function to convert decimal to hex string
std::string decToHex(int decimalNumber);

// Zoom functions
void zoomIn();
void zoomOut();

#endif // SERIAL_H
