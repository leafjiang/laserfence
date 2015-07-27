// Camera object for laser fence application.
// 07/16/15 LAJ -- Document created

#ifndef LFCAM_H
#define LFCAM_H

#include "stdafx.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <iomanip>
#include "FlyCapture2.h"

using namespace FlyCapture2;
using namespace std;

class LfCam {
protected:
    Error error;
    BusManager busMgr;
    unsigned int numCameras;
    PGRGuid *guids;  // Array of camera identifiers
    Image rawImage;
    Camera *pcam;
public:
    LfCam();
    ~LfCam();
    // Debug info
    void printBuildInfo();
    void printError( Error error );
    // Start camera
    int connect(int cameraIndex);
    int start();
    // Run camera
    int retrieveImage();
    Image convertImage();
    int saveImage(Image &img, ostringstream &filename);
    // Stop camera
    int stop();
    int disconnect();
    // Trigger functions
    bool checkSoftwareTriggerPresence();
    bool pollForTriggerReady();
    bool fireSoftwareTrigger();
    int powerOn();
    bool supportsExternalTrigger();
    int setTriggerMode15();
    int setGrabTimeout(int ms); // 5000 ms
    int setTriggerModeOff();
};

class LfUsbCam: public LfCam {
public:
    LfUsbCam();
    void printCameraInfo();
};

class LfGigECam: public LfCam {
public:
    LfGigECam();
    void printCameraInfo(CameraInfo *);
    void printAllStreamChannelsInfo();
    void printStreamChannelInfo(GigEStreamChannel*);
    void setCameraSettings();
};

#endif

