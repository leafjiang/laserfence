#include "lfalg.h"
#define ROWS 512
#define COLS 612
#define CAPTURE_IMAGES

LfAlg::LfAlg()
{
    counter = 0;
    image1 = new unsigned char*[ROWS];
    image2 = new unsigned char*[ROWS];
    for (int I = 0; I < ROWS; I++) {
        image1[I] = new unsigned char[COLS];
        image2[I] = new unsigned char[COLS];
    }
}

int LfAlg::loadMask(int r1, int c1, int r2, int c2)
{
    row1 = r1;
    col1 = c1;
    row2 = r2;
    col2 = c2;
    return 0;
}

int LfAlg::firstImage(unsigned char *data, int rows, int cols)
{
    cerr << "first image rows = " << rows << ", cols = " << cols << endl;
    sumFirstImage = 0;
    for (int I=row1; I<=row2; I++)
        for (int J=col1; J<=col2; J++)
            sumFirstImage += data[I*cols+J];
#ifdef CAPTURE_IMAGES
    for (int I=0; I<rows; I++)
        for (int J=0; J<cols; J++)
            image1[I][J] = data[I*cols+J];
#endif
    return 0;
}

int LfAlg::secondImage(unsigned char *data, int rows, int cols)
{
    cerr << "second image rows = " << rows << ", cols = " << cols << endl;
    sumSecondImage = 0;
    for (int I=row1; I<=row2; I++)
        for (int J=col1; J<=col2; J++)
            sumSecondImage += data[I*cols+J];
#ifdef CAPTURE_IMAGES
    for (int I=0; I<rows; I++)
        for (int J=0; J<cols; J++)
            image2[I][J] = data[I*cols+J];
#endif
    // Increment counter
    counter++;
    return 0;
}

string LfAlg::computePassFail()
{
    string result;          // string which will contain the result
    ostringstream convert;  // stream used for the conversion
    convert << counter << ' ' << sumFirstImage << ' '
            << sumSecondImage;
    result = convert.str();
    return result;
}

void LfAlg::saveImagesToDisk()
{
    // Save image1 and image2 to disk as pgm files
    // Can use imagemagick convert to get them into other formats:
    // convert image1.pgm image1.jpg
    // To compare two images -- shows any small difference
    // compare image1.pgm image2.pgm compare.pgm
    // Difference:
    // composite image1.pgm image2.pgm -compose difference diff.pgm
    FILE *f = fopen("image1.pgm", "wb");
    int height = ROWS;
    int width = COLS;
    fprintf(f, "P5\n%i %i 255\n", width, height);
    for (int y=0; y<height; y++)
      for (int x=0; x<width; x++)
        fputc(image1[y][x], f);   // 0 .. 255
    fclose(f);

    f = fopen("image2.pgm", "wb");
    fprintf(f, "P5\n%i %i 255\n", width, height);
    for (int y=0; y<height; y++)
      for (int x=0; x<width; x++)
        fputc(image2[y][x], f);   // 0 .. 255
    fclose(f);

    ofstream image1csv("image1.csv");
    for (int I=0; I<ROWS; I++) {
        for (int J=0; J<COLS; J++)
            image1csv << int(image1[I][J]) << ',';
        image1csv << endl;
    }
    image1csv.close();

    ofstream image2csv("image2.csv");
    for (int I=0; I<ROWS; I++) {
        for (int J=0; J<COLS; J++)
            image2csv << int(image2[I][J]) << ',';
        image2csv << endl;
    }
    image2csv.close();
}



