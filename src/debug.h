#pragma once

#include <string>

void debugInit();
void debugDeinit();
void debugLog(std::string text);
void debugFrametiming(float delta_time, int frame_number);