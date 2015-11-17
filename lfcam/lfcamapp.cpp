// Main file for laser fence application.
//
// Example command line usage:
// stdbuf -oL lfcamapp 210 10 240 500 -n 10 | cat
// stdbuf -oL lfcamapp 210 10 240 500 | (python lfmainapp.py)
// where
// stdbuf -o0 turns off buffering completely for stdout
// stdbuf -oL sets buffering to line buffering for stdout
//
// 07/16/15 LAJ - Document created

#include "stdafx.h"
#include <sys/time.h>
#include <iostream>
#include <sstream>
#include <boost/program_options.hpp>
#include "lfcam.h"
#include "lfalg.h"

using namespace std;
namespace po = boost::program_options;

long int gettime()
{
    // Returns the time in ms since epoch for the local time zone
    // To convert this result back to human-readable format, use
    // the web converter at
    // www.timestampconvert.com
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    return ms;
}

int main(int argc, char** argv)
{
    // Variables to hold command line argument values
    int n;
    int row1, col1, row2, col2;

    // Parse command line arguments
    try {
        // Define all options
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("row1", po::value<int>(&row1), "Mask: row1")
            ("col1", po::value<int>(&col1), "Mask: col1")
            ("row2", po::value<int>(&row2), "Mask: row2")
            ("col2", po::value<int>(&col2), "Mask: col2")
            ("capture,n", po::value<int>(&n)->default_value(0),
                "Capture n frames")
        ;

        // Select which options are "positional options"
        po::positional_options_description p;
        p.add("row1", 1)
        .add("col1", 1)
        .add("row2", 1)
        .add("col2", 1);

        // Parse command line arguments
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                options(desc).positional(p).run(), vm);
        po::notify(vm);

        // Print the options and their values
        cerr << "row1 " << row1 << endl;
        cerr << "col1 " << col1 << endl;
        cerr << "row2 " << row2 << endl;
        cerr << "col2 " << col2 << endl;
        cerr << "Capture " << n <<
            " frames, save them to disk, then quit program." << endl;
        if (n%2==1) {  // Is n odd?
            cerr << "Sorry, n must be even." << endl;
            return 1;
        }
    } catch (std::exception& e) {
        cerr << e.what() << "\n";
        return 1;
    }

    cerr << "Application started on " << gettime() << " ms since epoch" << endl;

    // Start camera application
    //extern const char *gitversion;
    //cerr << "Laser Fence camera application, version " << gitversion << endl;
    //printf("Version: %s\n", VERSION);
    cerr << "Version: " << VERSION << endl;

    // Initialize camera
    LfGigECam lfcam;
    lfcam.printBuildInfo();
    lfcam.connect(0);
    if (!lfcam.supportsExternalTrigger()) {
        cerr << "This camera does not support an external trigger.";
        return -1;
    }
    lfcam.powerOn();
    lfcam.printAllStreamChannelsInfo();
    lfcam.setTriggerMode(0);
    lfcam.setGrabTimeout(-1); // -1 = infinity, never timeout
    lfcam.setCameraSettings();
    lfcam.setImageBufferSize(n==0 ? 1 : n);

    // Initialize algorithm object
    LfAlg lfalg;
    lfalg.loadMask(row1, col1, row2, col2);

    // Start camera
    lfcam.start();
    string result;
    if (n == 0) {
        // Production mode.  Infinite loop
        while(true) {
            // Capture frames
            lfcam.pollForTriggerReady();
            // Save first image to computer memory buffer
            lfcam.retrieveImage(0);
            lfalg.firstImage(lfcam.rawImages[0].GetData(),
                    lfcam.rawImages[0].GetRows(),
                    lfcam.rawImages[0].GetCols());
            // Save second image to computer memory buffer
            lfcam.retrieveImage(0);
            lfalg.secondImage(lfcam.rawImages[0].GetData(),
                    lfcam.rawImages[0].GetRows(),
                    lfcam.rawImages[0].GetCols());
            // Compute pass/fail algorithm
            result = lfalg.computePassFail();
            // Send result of computation to stdout
            cout << result << ' ' << gettime() << endl;  // e.g., result = "125 123 456\n" = frameCount firstImgVal secondImgVal
        }
    } else {
        // Debug mode. Capture n images, then quit program.
        for (int I=0; I<n; I+=2) {
            // Capture frames
            lfcam.pollForTriggerReady();
            // Save first image to computer memory buffer
            lfcam.retrieveImage(I);
            lfalg.firstImage(lfcam.rawImages[I].GetData(),
                    lfcam.rawImages[0].GetRows(),
                    lfcam.rawImages[0].GetCols());
            // Save second image to computer memory buffer
            lfcam.retrieveImage(I+1);
            lfalg.secondImage(lfcam.rawImages[I+1].GetData(),
                    lfcam.rawImages[0].GetRows(),
                    lfcam.rawImages[0].GetCols());
            // Compute pass/fail algorithm
            result = lfalg.computePassFail();
            // Debug
            lfalg.saveImagesToDisk();
            // Send result of computation to stdout
            cout << result << ' ' << gettime() << endl;  // e.g., result = "125 123 456\n" = frameCount firstImgVal secondImgVal
        }
        // Write images in computer memory to disk
        lfcam.writeImageBufferToDisk();
   }

    // Stop camera
    lfcam.stop();
    lfcam.disconnect();

    cerr << "Done!" << endl;
    //cin.ignore();

    return 0;
}



