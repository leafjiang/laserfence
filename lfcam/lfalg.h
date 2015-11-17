// Algorithm object for laser fence application.
// 07/28/15 LAJ -- Document created

#ifndef LFALG_H
#define LFALG_H

#include "stdafx.h"
#include <iostream>
#include <sstream>
#include <fstream>
//#include <stdexcept>
//#include <unistd.h>
//#include <iomanip>

using namespace std;

class LfAlg {
public:
    unsigned char **image1;
    unsigned char **image2;
    int counter;
    string result;
    int row1, col1, row2, col2;
    int sumFirstImage, sumSecondImage;
public:
    LfAlg();
    int loadMask(int, int, int, int);
    int firstImage(unsigned char *, int, int);
    int secondImage(unsigned char *, int, int);
    string computePassFail();
    void saveImagesToDisk();
};

#endif

