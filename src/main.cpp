#include <iostream>
#include <string>

#include "application.h"
#include "debug.h"

#include "deserialiser.h"

int main(int argc, char* argv[])
{
    auto tokens = PTDeserialiser::tokenise(demo);

    return 0;
    debugInit();

    debugLog("Hello, Universe!");
    PTApplication app = PTApplication(640, 480);
    try
    {
        app.start();
        debugDeinit();
    }
    catch (const std::exception& e)
    {
        debugLog("FATAL ERROR: " + string(e.what()));
        debugDeinit();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
};