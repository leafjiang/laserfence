#include <fstream>

using namespace std;

int main(int argc, char *argv[])
{
    ofstream image1csv("image1.csv");         //Opening file to print info to
    //Morison_File << "Time Force(N/m)" << endl;          //Headings for file
    /*
    for (t = 0; t <= 20; t++) {
      u = sin(omega * t);
      du = cos(omega * t);
      F = (0.5 * rho * C_d * D * u * fabs(u)) + rho * Area * C_m * du;

      cout << "t = " << t << "\t\tF = " << F << endl;
      Morison_File << t;                                 //Printing to file
      Morison_File << F;
    }
     Morison_File.close();

     for (int I=0; I<ROWS; I++) {
        for (int J=0; J<COLS; J++)
            image1csv << to_string(image1[I][J]) << ',';
        image1csv << endl;
    }
    */

    unsigned char a = 123;
    unsigned char b = 13;
    unsigned char c = 9;

    image1csv << int(a) << ',' << int(b) << endl;
    image1csv << int(c) << ',' << endl;

    image1csv.close();
}
