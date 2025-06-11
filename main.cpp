#include "Engine.h"
#include <iostream>
#include <exception> // Required for std::exception

int main() {
    try {
        Engine engine;
        engine.run();
    } catch (const std::exception& e) {
        std::cerr << "An error occurred during engine execution: " << e.what() << std::endl;
        return 1; // Indicate an error
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1; // Indicate an error
    }

    return 0; // Indicate successful execution
}
