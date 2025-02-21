#include <iostream>
#include <string>

#include "application.h"
#include "debug.h"

using namespace std;

int main(int argc, char* argv[])
{
    debugInit();

    debugLog("Hello, Universe!");
    PTApplication app = PTApplication(640, 480);
    try
    {
        app.start();
    }
    catch (const std::exception& e)
    {
        debugLog("FATAL ERROR: " + string(e.what()));
        debugDeinit();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
};