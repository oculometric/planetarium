#include "input.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <linux/joystick.h>

using namespace std;

// PTController::PTController()
// {
//     PTController(0);
// }

// PTController::PTController(uint8_t index)
// {
//     device_handle = open((string("/dev/input/js") + to_string(index)).c_str(), O_RDONLY | O_NONBLOCK);
// }

// PTControllerEvent PTController::poll()
// {
//     PTControllerEvent event{ };

//     if (!isValid()) return event;

//     read

//     return PTControllerEvent();
// }

// bool PTController::isValid()
// {
//     return device_handle != -1;
// }
