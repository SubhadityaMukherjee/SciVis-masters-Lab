#!/bin/bash
g++ -std=c++17 -o pre-integration pre-integration.cpp
./pre-integration
xdg-open pre-integrated.ppm
