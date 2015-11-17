#include "lfcam.h"

LfCam::LfCam(void)
{
    cerr << "LfCam::LfCam begin..." << endl;

    // Since this application saves images in the current folder
    // we must ensure that we have permission to write to this folder.
    // If we do not have permission, fail right away.
    FILE* tempFile = fopen("test.txt", "w+");
    if (tempFile == NULL)
    {
        cerr << "Failed to create file in current folder.  Please check permissions." << endl;
        throw runtime_error("No permission to write to current folder.");
    }
    fclose(tempFile);
    remove("test.txt");

    // Initial size of image memory buffer
    N = 0;
}

LfCam::~LfCam(void)
{
    // Deallocate the memory that was reserved for the camera IDs
    if (guids) {
        delete[] guids;
    }
}

void LfCam::printBuildInfo()
{
    cerr << "LfCam::printBuildInfo" << endl;

    FC2Version fc2Version;
    Utilities::GetLibraryVersion( &fc2Version );

    ostringstream version;
    version << "FlyCapture2 library version: " << fc2Version.major << "." << fc2Version.minor << "." << fc2Version.type << "." << fc2Version.build;
    cerr << version.str() << endl;

    ostringstream timeStamp;
    timeStamp <<"Application build date: " << __DATE__ << " " << __TIME__;
    cerr << timeStamp.str() << endl << endl;
}

void LfCam::printError( Error error )
{
    error.PrintErrorTrace();
}

int LfCam::connect(int cameraIndex)
{
    cerr << "Connecting to camera..." << endl;

    // Connect to a camera
    error = pcam->Connect(&(guids[cameraIndex]));
    if (error != PGRERROR_OK)
    {
        printError( error );
        return -1;
    }
    return 0;
}

int LfCam::start()
{
    cerr << "Start capturing images..." << endl;

    // Start capturing images
    error = pcam->StartCapture();
    if (error != PGRERROR_OK)
    {
        printError( error );
        return -1;
    }
    return 0;
}

int LfCam::retrieveImage(int I)
{
    // Retrieve an image
    error = pcam->RetrieveBuffer( &rawImages[I] );
    if (error != PGRERROR_OK)
    {
        printError( error );
    }

    cerr << "Grabbed image" << endl;

    return 0;
}

Image LfCam::convertImage(Image &rawImg)
{
    // Create a converted image
    Image convertedImage;

    // Convert the raw image
    error = rawImg.Convert( PIXEL_FORMAT_MONO8, &convertedImage );
    if (error != PGRERROR_OK)
    {
        printError( error );
        throw runtime_error("Can't convert raw image");
    }

    return convertedImage;
}

int LfCam::saveImage(Image &img, ostringstream &filename)
{
    // Create a unique filename
    //ostringstream filename;
    //filename << "FlyCapture2Test-" << camInfo.serialNumber << "-" << imageCnt << ".pgm";

    // Save the image. If a file format is not passed in, then the file
    // extension is parsed to attempt to determine the file format.
    error = img.Save( filename.str().c_str() );
    //error = img.Save( filename->str().c_str() );
    if (error != PGRERROR_OK)
    {
        printError( error );
        return -1;
    }

    return 0;
}

int LfCam::stop()
{
    // Stop capturing images
    error = pcam->StopCapture();
    if (error != PGRERROR_OK)
    {
        printError( error );
        return -1;
    }
    return 0;
}

int LfCam::disconnect()
{
    // Disconnect the camera
    error = pcam->Disconnect();
    if (error != PGRERROR_OK)
    {
        printError( error );
        return -1;
    }
    return 0;
}


bool LfCam::checkSoftwareTriggerPresence()
{
    const unsigned int k_triggerInq = 0x530;
    unsigned int regVal = 0;

    error = pcam->ReadRegister( k_triggerInq, &regVal );

    if (error != PGRERROR_OK)
    {
        printError( error );
        return false;
    }

    if( ( regVal & 0x10000 ) != 0x10000 )
    {
        return false;
    }

    return true;
}

bool LfCam::pollForTriggerReady()
{
    const unsigned int k_softwareTrigger = 0x62C;
    unsigned int regVal = 0;

    do
    {
        error = pcam->ReadRegister( k_softwareTrigger, &regVal );
        if (error != PGRERROR_OK)
        {
            printError( error );
            return false;
        }

    } while ( (regVal >> 31) != 0 );

    return true;
}

bool LfCam::fireSoftwareTrigger()
{
    const unsigned int k_softwareTrigger = 0x62C;
    const unsigned int k_fireVal = 0x80000000;

    error = pcam->WriteRegister( k_softwareTrigger, k_fireVal );
    if (error != PGRERROR_OK)
    {
        printError( error );
        return false;
    }

    return true;
}

int LfCam::powerOn()
{
    cerr << "LfCam::powerOn" << endl;

    // Power on the camera
    const unsigned int k_cameraPower = 0x610;
    const unsigned int k_powerVal = 0x80000000;
    error  = pcam->WriteRegister( k_cameraPower, k_powerVal );
    if (error != PGRERROR_OK)
    {
        printError( error );
        return -1;
    }

    const unsigned int millisecondsToSleep = 100;
    unsigned int regVal = 0;
    unsigned int retries = 10;

    // Wait for camera to complete power-up
    do
    {
#if defined(WIN32) || defined(WIN64)
        Sleep(millisecondsToSleep);
#else
        usleep(millisecondsToSleep * 1000);
#endif
        error = pcam->ReadRegister(k_cameraPower, &regVal);
        if (error == PGRERROR_TIMEOUT)
        {
            // ignore timeout errors, camera may not be responding to
            // register reads during power-up
        }
        else if (error != PGRERROR_OK)
        {
            printError( error );
            return -1;
        }

        retries--;
    } while ((regVal & k_powerVal) == 0 && retries > 0);

    // Check for timeout errors after retrying
    if (error == PGRERROR_TIMEOUT)
    {
        printError( error );
        return -1;
    }

    return 0;
}

bool LfCam::supportsExternalTrigger()
{
    // Check for external trigger support
    TriggerModeInfo triggerModeInfo;
    error = pcam->GetTriggerModeInfo( &triggerModeInfo );
    if (error != PGRERROR_OK)
    {
        printError( error );
        return false;
    }

    if ( triggerModeInfo.present != true )
    {
        cerr << "Camera does not support external trigger! Exiting..." << endl;
        return false;
    }

    return true;
}

int LfCam::setTriggerMode(int modenum)
{
    // Standard external trigger mode
    cerr << "LfCam::setTriggerMode3()" << endl;

    // Get current trigger settings
    cerr << "Get current trigger settings" << endl;
    TriggerMode triggerMode;
    error = pcam->GetTriggerMode( &triggerMode );
    if (error != PGRERROR_OK)
    {
        printError( error );
        return -1;
    }
    cerr << " onOff = " << triggerMode.onOff << endl;
    cerr << " mode = " << triggerMode.mode << endl;
    cerr << " parameter = " << triggerMode.parameter << endl;
    cerr << " polarity = " << triggerMode.polarity << endl;
    cerr << " source = " << triggerMode.source << endl;

    // Set camera to trigger mode modenum (usually 0 or 15)
    triggerMode.onOff = true;
    triggerMode.mode = modenum;
    triggerMode.parameter = 2;   // Capture 2 frames per trigger event (for mode 15)
    triggerMode.polarity = 1;   // Trigger on rising edge (1=rising edge?)

    // A source of 7 means software trigger
    //triggerMode.source = 7;

    // Triggering the camera externally using source 0.
    triggerMode.source = 0; // GPIO trigger pin 0

    cerr << "Set trigger settings" << endl;
    cerr << " onOff = " << triggerMode.onOff << endl;
    cerr << " mode = " << triggerMode.mode << endl;
    cerr << " parameter = " << triggerMode.parameter << endl;
    cerr << " polarity = " << triggerMode.polarity << endl;
    cerr << " source = " << triggerMode.source << endl;
    error = pcam->SetTriggerMode( &triggerMode );
    if (error != PGRERROR_OK)
    {
        printError( error );
        return -1;
    }

    // Poll to ensure camera is ready
    cerr << "Polling camera for trigger ready" << endl;
    bool retVal = pollForTriggerReady();
    if( !retVal )
    {
        cerr << endl;
        cerr << "Error polling for trigger ready!" << endl;
        return -1;
    }

    cerr << "Trigger is ready!" << endl;

    return 0;
}


int LfCam::setTriggerMode15()
{
    // Get current trigger settings
    TriggerMode triggerMode;
    error = pcam->GetTriggerMode( &triggerMode );
    if (error != PGRERROR_OK)
    {
        printError( error );
        return -1;
    }

    // Set camera to trigger mode 15, multishot
    triggerMode.onOff = true;
    triggerMode.mode = 15;
    triggerMode.parameter = 2;  // Capture 2 frames per trigger event
    triggerMode.polarity = 1;   // Trigger on rising edge (1=rising edge?)

    // A source of 7 means software trigger
    //triggerMode.source = 7;

    // Triggering the camera externally using source 0.
    triggerMode.source = 0;

    error = pcam->SetTriggerMode( &triggerMode );
    if (error != PGRERROR_OK)
    {
        printError( error );
        return -1;
    }

    // Poll to ensure camera is ready
    bool retVal = pollForTriggerReady();
    if( !retVal )
    {
        cerr << endl;
        cerr << "Error polling for trigger ready!" << endl;
        return -1;
    }

    return 0;
}

int LfCam::setGrabTimeout(int ms)
{
    // Time in milliseconds that RetrieveBuffer() and WaitForBufferEvent()
    // will wait for an image before timing out and returning.
    // enum GrabTimeout { TIMEOUT_NONE = 0, TIMEOUT_INFINITE = -1, TIME- OUT_UNSPECIFIED = -2, GRAB_TIMEOUT_FORCE_32BITS = FULL_32BIT- _VALUE }

    // Get the camera configuration
    FC2Config config;
    error = pcam->GetConfiguration( &config );
    if (error != PGRERROR_OK)
    {
        printError( error );
        return -1;
    }

    // Set the grab timeout to 5 seconds
    config.grabTimeout = ms;

    // Set the camera configuration
    error = pcam->SetConfiguration( &config );
    if (error != PGRERROR_OK)
    {
        printError( error );
        return -1;
    }

    return 0;
}

int LfCam::setTriggerModeOff()
{
    // Get current trigger settings
    TriggerMode triggerMode;
    error = pcam->GetTriggerMode( &triggerMode );
    if (error != PGRERROR_OK)
    {
        printError( error );
        return -1;
    }

    // Turn trigger mode off.
    triggerMode.onOff = false;
    error = pcam->SetTriggerMode( &triggerMode );
    if (error != PGRERROR_OK)
    {
        printError( error );
        return -1;
    }
    cerr << endl;
    cerr << "Finished grabbing images" << endl;
    return 0;
}

int LfCam::setImageBufferSize(int n)
{
    if (n > 1) {
        rawImages = new Image[n];
        N = n;
        return 0;
    }

    return -1;
}

int LfCam::writeImageBufferToDisk()
{
    for (int I=0; I<N; I++) {
        Image convertedImage = convertImage(rawImages[I]);
        // Save frames
        ostringstream filename;
        filename << "snapshot_" << I << ".png";
        saveImage(convertedImage, filename);
    }
    // To draw a transparent rectangle over the image
    // from the command line, use ImageMagick
    // convert input.jpg -strokewidth 0 -fill "rgba( 255, 215, 0 , 0.5 )" -draw "rectangle 66,50 200,150 " output.jpg
}


LfUsbCam::LfUsbCam()
{
    cerr << "LfUsbCam::LfUsbCam begin..." << endl;

    error = busMgr.GetNumOfCameras(&numCameras);
    if (error != PGRERROR_OK)
    {
        printError( error );
        throw runtime_error("Error getting number of cameras");
    }
    cerr << "Number of cameras detected: " << numCameras << endl;
    if ( numCameras < 1 )
    {
        cerr << "Insufficient number of cameras... exiting" << endl;
        throw runtime_error("No cameras detected.");
    }

    // Create new array of camera IDs
    guids = new PGRGuid[numCameras];

    // Assign values to array of camera IDs
    for (unsigned int i=0; i < numCameras; i++)
    {
        error = busMgr.GetCameraFromIndex(i, &(guids[i]));
        if (error != PGRERROR_OK)
        {
            printError( error );
            throw runtime_error("Could not get camera from index");
        }
    }

    pcam = new Camera();
}

void LfUsbCam::printCameraInfo()
{
    // Get the camera information
    CameraInfo camInfo;
    error = pcam->GetCameraInfo(&camInfo);
    if (error != PGRERROR_OK)
    {
        printError( error );
        throw runtime_error("Can't get camera info");
    }

    CameraInfo* pCamInfo = &camInfo;

    cerr << endl;
    cerr << "*** CAMERA INFORMATION ***" << endl;
    cerr << "Serial number -" << pCamInfo->serialNumber << endl;
    cerr << "Camera model - " << pCamInfo->modelName << endl;
    cerr << "Camera vendor - " << pCamInfo->vendorName << endl;
    cerr << "Sensor - " << pCamInfo->sensorInfo << endl;
    cerr << "Resolution - " << pCamInfo->sensorResolution << endl;
    cerr << "Firmware version - " << pCamInfo->firmwareVersion << endl;
    cerr << "Firmware build time - " << pCamInfo->firmwareBuildTime << endl << endl;
}




LfGigECam::LfGigECam()
{
    cerr << "LfGigECam::LfGigECam begin..." << endl;

    CameraInfo camInfo[10];
    unsigned int numCamInfo = 10;
    error = BusManager::DiscoverGigECameras( camInfo, &numCamInfo );
    if (error != PGRERROR_OK)
    {
        printError( error );
        throw runtime_error("DiscoverGigECameras failed.");
    }

    cerr << "Number of cameras discovered: " << numCamInfo << endl;

    for (unsigned int i=0; i < numCamInfo; i++)
    {
        printCameraInfo( &camInfo[i] );
    }

    error = busMgr.GetNumOfCameras(&numCameras);
    if (error != PGRERROR_OK)
    {
        printError( error );
        throw runtime_error("GetNumOfCameras failed.");
    }

    cerr << "Number of cameras enumerated: " << numCameras << endl;

    // Create new array of camera IDs
    guids = new PGRGuid[numCameras];

    for (unsigned int i=0; i < numCameras; i++)
    {
        error = busMgr.GetCameraFromIndex(i, &(guids[i]));
        if (error != PGRERROR_OK)
        {
            printError( error );
            throw runtime_error("GetCameraFomIndex failed.");
        }

        InterfaceType interfaceType;
        error = busMgr.GetInterfaceTypeFromGuid( &(guids[i]), &interfaceType );
        if ( error != PGRERROR_OK )
        {
            printError( error );
            throw runtime_error("GetInterfaceTypeFromGuid failed.");
        }
    }

    GigECamera *pgigecam = new GigECamera();
    pcam = (Camera *) pgigecam;
}

void LfGigECam::printCameraInfo(CameraInfo *pCamInfo)
{
    cerr << "LfGigECam::printCameraInfo" << endl;

    ostringstream macAddress;
    macAddress << hex << setw(2) << setfill('0') << (unsigned int)pCamInfo->macAddress.octets[0] << ":" <<
        hex << setw(2) << setfill('0') << (unsigned int)pCamInfo->macAddress.octets[1] << ":" <<
        hex << setw(2) << setfill('0') << (unsigned int)pCamInfo->macAddress.octets[2] << ":" <<
        hex << setw(2) << setfill('0') << (unsigned int)pCamInfo->macAddress.octets[3] << ":" <<
        hex << setw(2) << setfill('0') << (unsigned int)pCamInfo->macAddress.octets[4] << ":" <<
        hex << setw(2) << setfill('0') << (unsigned int)pCamInfo->macAddress.octets[5];

    ostringstream ipAddress;
    ipAddress << (unsigned int)pCamInfo->ipAddress.octets[0] << "." <<
        (unsigned int)pCamInfo->ipAddress.octets[1] << "." <<
        (unsigned int)pCamInfo->ipAddress.octets[2] << "." <<
        (unsigned int)pCamInfo->ipAddress.octets[3];

    ostringstream subnetMask;
    subnetMask << (unsigned int)pCamInfo->subnetMask.octets[0] << "." <<
        (unsigned int)pCamInfo->subnetMask.octets[1] << "." <<
        (unsigned int)pCamInfo->subnetMask.octets[2] << "." <<
        (unsigned int)pCamInfo->subnetMask.octets[3];

    ostringstream defaultGateway;
    defaultGateway << (unsigned int)pCamInfo->defaultGateway.octets[0] << "." <<
        (unsigned int)pCamInfo->defaultGateway.octets[1] << "." <<
        (unsigned int)pCamInfo->defaultGateway.octets[2] << "." <<
        (unsigned int)pCamInfo->defaultGateway.octets[3];

    cerr << endl;
    cerr << "*** GigE CAMERA INFORMATION ***" << endl;
    cerr << "Serial number - " << pCamInfo->serialNumber << endl;
    cerr << "Camera model - " << pCamInfo->modelName << endl;
    cerr << "Camera vendor - " << pCamInfo->vendorName << endl;
    cerr << "Sensor - " << pCamInfo->sensorInfo << endl;
    cerr << "Resolution - " << pCamInfo->sensorResolution << endl;
    cerr << "Firmware version - " << pCamInfo->firmwareVersion << endl;
    cerr << "Firmware build time - " << pCamInfo->firmwareBuildTime << endl;
    cerr << "GigE version - " << pCamInfo->gigEMajorVersion << "." << pCamInfo->gigEMinorVersion << endl;
    cerr << "User defined name - " << pCamInfo->userDefinedName << endl;
    cerr << "XML URL 1 - " << pCamInfo->xmlURL1 << endl;
    cerr << "XML URL 2 - " << pCamInfo->xmlURL2 << endl;
    cerr << "MAC address - " << macAddress.str() << endl;
    cerr << "IP address - " << ipAddress.str() << endl;
    cerr << "Subnet mask - " << subnetMask.str() << endl;
    cerr << "Default gateway - " << defaultGateway.str() << endl << endl;
}

void LfGigECam::printAllStreamChannelsInfo()
{
    cerr << "LfGigECam::printAllStreamChannelsInfo" << endl;

    unsigned int numStreamChannels = 0;
    error = ((GigECamera *)pcam)->GetNumStreamChannels( &numStreamChannels );
    if (error != PGRERROR_OK)
    {
        printError( error );
        throw runtime_error("Error getting number of GigE stream channels");
    }

    for (unsigned int i=0; i < numStreamChannels; i++)
    {
        GigEStreamChannel streamChannel;
        error = ((GigECamera *)pcam)->GetGigEStreamChannelInfo( i, &streamChannel );
        if (error != PGRERROR_OK)
        {
            printError( error );
            throw runtime_error("Error getting GigE stream channel info");
        }

        streamChannel.destinationIpAddress.octets[0] = 224;
        streamChannel.destinationIpAddress.octets[1] = 0;
        streamChannel.destinationIpAddress.octets[2] = 0;
        streamChannel.destinationIpAddress.octets[3] = 1;

        error = ((GigECamera *)pcam)->SetGigEStreamChannelInfo( i, &streamChannel );
        if (error != PGRERROR_OK)
        {
            printError( error );
            throw runtime_error("Error setting GigE stream channel info");
        }

        cerr << "Printing stream channel information for channel " << i << endl;
        printStreamChannelInfo( &streamChannel );
    }
}

void LfGigECam::printStreamChannelInfo( GigEStreamChannel* pStreamChannel )
{
    cerr << "LfGigECam::printStreamChannelInfo" << endl;

    //char ipAddress[32];
    ostringstream ipAddress;
    ipAddress << (unsigned int)pStreamChannel->destinationIpAddress.octets[0] << "." <<
        (unsigned int)pStreamChannel->destinationIpAddress.octets[1] << "." <<
        (unsigned int)pStreamChannel->destinationIpAddress.octets[2] << "." <<
        (unsigned int)pStreamChannel->destinationIpAddress.octets[3];

    cerr << "Network interface: " << pStreamChannel->networkInterfaceIndex << endl;
    cerr << "Host Port: " << pStreamChannel->hostPort << endl;
    cerr << "Do not fragment bit: " << ( pStreamChannel->doNotFragment ? "Enabled" : "Disabled") << endl;
    cerr << "Packet size: " << pStreamChannel->packetSize << endl;
    cerr << "Inter packet delay: " << pStreamChannel->interPacketDelay << endl;
    cerr << "Destination IP address: " << ipAddress.str() << endl;
    cerr << "Source port (on camera): " << pStreamChannel->sourcePort << endl << endl;
}

void LfGigECam::setCameraSettings()
{
    cerr << "Querying GigE camera mode..." << endl;

    Mode mode;
    error = ((GigECamera *)pcam)->GetGigEImagingMode ( &mode );
    if (error != PGRERROR_OK)
    {
        printError( error );
        throw runtime_error("Error querying GigE camera mode");
    }
    cerr << "Mode = " << mode << endl;

    mode = MODE_6;  // 612 x 512
    cerr << "Setting GigE camera to mode " << mode << endl;
    error = ((GigECamera *)pcam)->SetGigEImagingMode ( mode );
    if (error != PGRERROR_OK)
    {
        printError( error );
        throw runtime_error("Error setting GigE camera mode");
    }

    cerr << "Querying GigE image setting information..." << endl;

    GigEImageSettingsInfo imageSettingsInfo;
    error = ((GigECamera *)pcam)->GetGigEImageSettingsInfo( &imageSettingsInfo );
    if (error != PGRERROR_OK)
    {
        printError( error );
        throw runtime_error("Error getting GigE image settings info");
    }

    cerr << "maxWidth = " << imageSettingsInfo.maxWidth << endl;
    cerr << "maxHeight = " << imageSettingsInfo.maxHeight << endl;
    cerr << "offsetHStepSize = " << imageSettingsInfo.offsetHStepSize << endl;
    cerr << "offsetVStepSize = " << imageSettingsInfo.offsetVStepSize << endl;
    cerr << "imageHStepSize = " << imageSettingsInfo.imageHStepSize << endl;
    cerr << "imageVStepSize = " << imageSettingsInfo.imageVStepSize << endl;
    cerr << "pixelFormatBitField = " << imageSettingsInfo.pixelFormatBitField << endl;
    cerr << "vendorPixelFormatBitField = " << imageSettingsInfo.vendorPixelFormatBitField << endl;

    cerr << "Querying GigE image settings ..." << endl;

    GigEImageSettings imageSettings;
    error = ((GigECamera *)pcam)->GetGigEImageSettings( &imageSettings );
    if (error != PGRERROR_OK)
    {
        printError( error );
        throw runtime_error("Error getting GigE image settings");
    }
    cerr << "offsetX = " << imageSettings.offsetX << endl;
    cerr << "offsetY = " << imageSettings.offsetY << endl;
    cerr << "height = " << imageSettings.height << endl;
    cerr << "width = " << imageSettings.width << endl;
    cerr << "pixelFormat = " << imageSettings.pixelFormat << endl;

    cerr << "Setting GigE image settings..." << endl;

    imageSettings.offsetX = 0;
    imageSettings.offsetY = 0;
    imageSettings.height = imageSettingsInfo.maxHeight;
    imageSettings.width = imageSettingsInfo.maxWidth;
    imageSettings.pixelFormat = PIXEL_FORMAT_MONO8;

    error = ((GigECamera *)pcam)->SetGigEImageSettings( &imageSettings );
    if (error != PGRERROR_OK)
    {
        printError( error );
        throw runtime_error("Error setting GigE image settings");
    }
    /*
    // Set the camera exposure
    EmbeddedImageInfo embeddedInfo;
    error = ((GigECamera *)pcam)->GetEmbeddedImageInfo( &embeddedInfo );
    if ( error != PGRERROR_OK ) {
        printError( error );
        throw runtime_error( "Error reading embedded image info");
    }
    cerr << "EmbeddedImageInfo: (on/off)" << endl;
    cerr << "  timestamp = " << embeddedInfo.timestamp.onOff << endl;
    cerr << "  gain = " << embeddedInfo.gain.onOff << endl;
    cerr << "  shutter = " << embeddedInfo.shutter.onOff << endl;
    cerr << "  brightness " << embeddedInfo.brightness.onOff << endl;
    cerr << "  exposure = " << embeddedInfo.exposure.onOff << endl;
    cerr << "  whiteBalance = " << embeddedInfo.whiteBalance.onOff << endl;
    cerr << "  frameCounter = " << embeddedInfo.frameCounter.onOff << endl;
    cerr << "  strobePattern = " << embeddedInfo.strobePattern.onOff << endl;
    cerr << "  GPIOPinState = " << embeddedInfo.GPIOPinState.onOff << endl;
    cerr << "  ROIPosition = " << embeddedInfo.ROIPosition.onOff << endl;

    cerr << "EmbeddedImageInfo: (available)" << endl;
    cerr << "  timestamp = " << embeddedInfo.timestamp.available << endl;
    cerr << "  gain = " << embeddedInfo.gain.available << endl;
    cerr << "  shutter = " << embeddedInfo.shutter.available << endl;
    cerr << "  brightness " << embeddedInfo.brightness.available << endl;
    cerr << "  exposure = " << embeddedInfo.exposure.available << endl;
    cerr << "  whiteBalance = " << embeddedInfo.whiteBalance.available << endl;
    cerr << "  frameCounter = " << embeddedInfo.frameCounter.available << endl;
    cerr << "  strobePattern = " << embeddedInfo.strobePattern.available << endl;
    cerr << "  GPIOPinState = " << embeddedInfo.GPIOPinState.available << endl;
    cerr << "  ROIPosition = " << embeddedInfo.ROIPosition.available << endl;

    error = ((GigECamera *)pcam)->SetEmbeddedImageInfo( &embeddedInfo );
    if ( error != PGRERROR_OK ) {
        printError( error );
        throw runtime_error( "Error writing embedded image info");
    }
    */
}


