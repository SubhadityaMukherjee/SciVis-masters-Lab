#!/bin/bash
g++ -std=c++17 -lopencv_videoio -lopencv_imgcodecs -lopencv_core -lopencv_imgproc -lopencv_features2d -lopencv_flann -lopencv_video -lopencv_calib3d -lopencv_highgui -o lic lic.cpp
./lic
xdg-open line_integrated.ppm
