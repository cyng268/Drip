# Drip - Water Dripping Investigation Recording Tools

A C++ application for the Raspberry Pi 5 (64-bit) to capture real-time air conditioner dripping.

## Features

- 🔄 **Real-time droplet detection** 
- 🎥 **Video recording** 
- 🔍 **Pan and zoom controls** 
- 📡 **ICR controls** 

## System Requirements

- Raspberry Pi 5 (64-bit OS)
- USB camera
- X11 display server

## Installation

### 1. Required Dependencies

Install OpenCV for C++:
```bash
sudo apt install libopencv-dev
```

Install xdotool for window control:
```bash
sudo apt-get install xdotool
```

Install CodeBlocks IDE:
```bash
sudo apt install codeblocks
```

### 2. Setup X11 Display Server

This application requires X11 display server:
```bash
sudo raspi-config
```
Navigate to: Advanced Options > Wayland > Select X11 > Reboot

### 3. Camera Setup

Connect your USB camera to the Pi and verify the connection:
```bash
v4l2-ctl --list-devices
```
The camera should be detected as `/dev/video0`.

### 4. Project Configuration

1. Clone the repository
2. Open CodeBlocks and select "Open an existing project"
3. Navigate to and select `./Drip/Drip.cbp`
4. Set up the default C++ compiler if prompted
5. Click "Build and Run" to compile and start the application

## Software Components

- **Background Subtraction** - Drip detection
- **Export Dialog** - recording management and export functionality
- **Navigation Bar** - User interface controls
- **Recording** - Video capture and processing
- **Serial Interface** - Zooming and ICR control

## Configuration

The application behavior can be customized through the `config.ini` file. Adjust parameters before running the application.

## Recording and Analysis Export

Detection results are automatically saved with timestamps in CSV format, including:
- Drop occurence count
- Detection timestamps
- Position data

## Development Resources

For more information on using OpenCV with C++ on Raspberry Pi:
https://qengineering.eu/opencv-c-examples-on-raspberry-pi.html

## Credits

Serial communication powered by [serialib](https://github.com/imabot2/serialib)
