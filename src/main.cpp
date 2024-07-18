#include <iostream>
#include <string>
#include "application.h"

int main(int argc, char* argv[])
{
    std::cout << "Hello, Universe!" << std::endl;
    PTApplication app = PTApplication(640, 480);
    try
    {
        app.start();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
};