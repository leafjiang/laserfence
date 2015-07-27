// Camera object for laser fence application.
// 07/16/15 LAJ -- Document created

#include "lfcam.h"

LfCam::LfCam(void)
{
    cout << "LfCam::LfCam begin..." << endl;

    // Since this application saves images in the current folder
    // we must ensure that we have permission to write to this folder.
    // If we do not have permission, fail right away.
    FILE* tempFile = fopen("test.txt", "w+");
    if (tempFile == NULL)
    {
        cout << "Failed to create file in current folder.  Please check permissions." << endl;
        throw runtime_error("No permission to write to current folder.");
    }
    fclose(tempFile);
    remove("test.txt");
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
    cout << "LfCam::printBuildInfo" << endl;

    FC2Version fc2Version;
    Utilities::GetLibraryVersion( &fc2Version );

    ostringstream version;
    version << "FlyCapture2 library version: " << fc2Version.major << "." << fc2Version.minor << "." << fc2Version.type << "." << fc2Version.build;
    cout << version.str() << endl;

    ostringstream timeStamp;
    timeStamp <<"Application build date: " << __DATE__ << " " << __TIME__;
    cout << timeStamp.str() << endl << endl;
}

void LfCam::printError( Error error )
{
    error.PrintErrorTrace();
}

int LfCam::connect(int cameraIndex)
{
    cout << "Connecting to camera..." << endl;

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
    cout << "Start capturing images..." << endl;

    // Start capturing images
    error = pcam->StartCapture();
    if (error != PGRERROR_OK)
    {
        printError( error );
        return -1;
    }
    return 0;
}

int LfCam::retrieveImage()
{
    // Retrieve an image
    error = pcam->RetrieveBuffer( &rawImage );
    if (error != PGRERROR_OK)
    {
        printError( error );
    }

    cout << "Grabbed image" << endl;

    return 0;
}

Image LfCam::convertImage()
{
    // Create a converted image
    Image convertedImage;

    // Convert the raw image
    error = rawImage.Convert( PIXEL_FORMAT_MONO8, &convertedImage );
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
    cout << "LfCam::powerOn" << endl;

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
        cout << "Camera does not support external trigger! Exiting..." << endl;
        return false;
    }

    return true;
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
        cout << endl;
        cout << "Error polling for trigger ready!" << endl;
        return -1;
    }

    return 0;
}

int LfCam::setGrabTimeout(int ms)
{
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
    cout << endl;
    cout << "Finished grabbing images" << endl;
    return 0;
}



LfUsbCam::LfUsbCam()
{
    cout << "LfUsbCam::LfUsbCam begin..." << endl;

    error = busMgr.GetNumOfCameras(&numCameras);
    if (error != PGRERROR_OK)
    {
        printError( error );
        throw runtime_error("Error getting number of cameras");
    }
    cout << "Number of cameras detected: " << numCameras << endl;
    if ( numCameras < 1 )
    {
        cout << "Insufficient number of cameras... exiting" << endl;
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

    cout << endl;
    cout << "*** CAMERA INFORMATION ***" << endl;
    cout << "Serial number -" << pCamInfo->serialNumber << endl;
    cout << "Camera model - " << pCamInfo->modelName << endl;
    cout << "Camera vendor - " << pCamInfo->vendorName << endl;
    cout << "Sensor - " << pCamInfo->sensorInfo << endl;
    cout << "Resolution - " << pCamInfo->sensorResolution << endl;
    cout << "Firmware version - " << pCamInfo->firmwareVersion << endl;
    cout << "Firmware build time - " << pCamInfo->firmwareBuildTime << endl << endl;
}




LfGigECam::LfGigECam()
{
    cout << "LfGigECam::LfGigECam begin..." << endl;

    CameraInfo camInfo[10];
    unsigned int numCamInfo = 10;
    error = BusManager::DiscoverGigECameras( camInfo, &numCamInfo );
    if (error != PGRERROR_OK)
    {
        printError( error );
        throw runtime_error("DiscoverGigECameras failed.");
    }

    cout << "Number of cameras discovered: " << numCamInfo << endl;

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

    cout << "Number of cameras enumerated: " << numCameras << endl;

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
    cout << "LfGigECam::printCameraInfo" << endl;

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

    cout << endl;
    cout << "*** GigE CAMERA INFORMATION ***" << endl;
    cout << "Serial number - " << pCamInfo->serialNumber << endl;
    cout << "Camera model - " << pCamInfo->modelName << endl;
    cout << "Camera vendor - " << pCamInfo->vendorName << endl;
    cout << "Sensor - " << pCamInfo->sensorInfo << endl;
    cout << "Resolution - " << pCamInfo->sensorResolution << endl;
    cout << "Firmware version - " << pCamInfo->firmwareVersion << endl;
    cout << "Firmware build time - " << pCamInfo->firmwareBuildTime << endl;
    cout << "GigE version - " << pCamInfo->gigEMajorVersion << "." << pCamInfo->gigEMinorVersion << endl;
    cout << "User defined name - " << pCamInfo->userDefinedName << endl;
    cout << "XML URL 1 - " << pCamInfo->xmlURL1 << endl;
    cout << "XML URL 2 - " << pCamInfo->xmlURL2 << endl;
    cout << "MAC address - " << macAddress.str() << endl;
    cout << "IP address - " << ipAddress.str() << endl;
    cout << "Subnet mask - " << subnetMask.str() << endl;
    cout << "Default gateway - " << defaultGateway.str() << endl << endl;
}

void LfGigECam::printAllStreamChannelsInfo()
{
    cout << "LfGigECam::printAllStreamChannelsInfo" << endl;

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

        cout << "Printing stream channel information for channel " << i << endl;
        printStreamChannelInfo( &streamChannel );
    }
}

void LfGigECam::printStreamChannelInfo( GigEStreamChannel* pStreamChannel )
{
    cout << "LfGigECam::printStreamChannelInfo" << endl;

    //char ipAddress[32];
    ostringstream ipAddress;
    ipAddress << (unsigned int)pStreamChannel->destinationIpAddress.octets[0] << "." <<
        (unsigned int)pStreamChannel->destinationIpAddress.octets[1] << "." <<
        (unsigned int)pStreamChannel->destinationIpAddress.octets[2] << "." <<
        (unsigned int)pStreamChannel->destinationIpAddress.octets[3];

    cout << "Network interface: " << pStreamChannel->networkInterfaceIndex << endl;
    cout << "Host Port: " << pStreamChannel->hostPort << endl;
    cout << "Do not fragment bit: " << ( pStreamChannel->doNotFragment ? "Enabled" : "Disabled") << endl;
    cout << "Packet size: " << pStreamChannel->packetSize << endl;
    cout << "Inter packet delay: " << pStreamChannel->interPacketDelay << endl;
    cout << "Destination IP address: " << ipAddress.str() << endl;
    cout << "Source port (on camera): " << pStreamChannel->sourcePort << endl << endl;
}

void LfGigECam::setCameraSettings()
{
    cout << "Querying GigE image setting information..." << endl;

    GigEImageSettingsInfo imageSettingsInfo;
    error = ((GigECamera *)pcam)->GetGigEImageSettingsInfo( &imageSettingsInfo );
    if (error != PGRERROR_OK)
    {
        printError( error );
        throw runtime_error("Error getting GigE image settings info");
    }

    GigEImageSettings imageSettings;
    imageSettings.offsetX = 0;
    imageSettings.offsetY = 0;
    imageSettings.height = imageSettingsInfo.maxHeight;
    imageSettings.width = imageSettingsInfo.maxWidth;
    imageSettings.pixelFormat = PIXEL_FORMAT_MONO8;

    cout << "Setting GigE image settings..." << endl;

    error = ((GigECamera *)pcam)->SetGigEImageSettings( &imageSettings );
    if (error != PGRERROR_OK)
    {
        printError( error );
        throw runtime_error("Error setting GigE image settings");
    }

}
