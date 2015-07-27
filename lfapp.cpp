// Main file for laser fence application.
// 07/16/15 LAJ - Document created

#include "stdafx.h"
#include <iostream>
#include <sstream>
#include "lfcam.h"

using namespace std;

int main(int /*argc*/, char** /*argv*/)
{
    cout << "Starting Laser Fence application..." << endl;

    // Initialize camera
    LfGigECam lfcam;
    lfcam.printBuildInfo();
    lfcam.connect(0);
    if (!lfcam.supportsExternalTrigger()) {
        cout << "This camera does not support an external trigger.";
        return -1;
    }
    lfcam.powerOn();
    lfcam.printAllStreamChannelsInfo();
    lfcam.setTriggerMode15();
    lfcam.setGrabTimeout(5000);
    lfcam.setCameraSettings();

    // Start camera
    lfcam.start();
    while (true) {
        // Capture frames
        lfcam.pollForTriggerReady();
        lfcam.retrieveImage();
        Image convertedImage = lfcam.convertImage();
        // Save frames
        ostringstream filename;
        //filename << "lf" << "blah.pgm";
        filename << "blah.png";
        lfcam.saveImage(convertedImage, filename);
        break;
    }

    // Stop camera
    lfcam.stop();
    lfcam.disconnect();

    cout << "Done! Press Enter to exit..." << endl;
    cin.ignore();

    return 0;
}
