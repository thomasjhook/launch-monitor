#include "logger.hpp"
#include "camera.hpp"
#include "radar.hpp"
#include "trigger.hpp"


int main() {

    Logger::info("Starting DIY Launch Monitor...");
    Logger::info("Initializing components...");
    initCamera();
    initRadar();
    initTrigger();
    Logger::info("Components initialized.");

    // TODO main loop
    return 0;
}