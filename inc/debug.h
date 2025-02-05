#pragma once

#include <string>

void debugInit();
void debugDeinit();
void debugLog(std::string text);
void debugFrametiming(float delta_time, int frame_number);
void debugSetSceneProperty(std::string name, std::string content);
void debugSetObjectProperty(std::string name, std::string content);
void debugClearSceneProperty(std::string name);
void debugClearObjectProperty(std::string name);