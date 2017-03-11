#pragma once

using namespace std;
using namespace cv;

const int nZones = 4;

const int nTeethPerZone = 7;

const int lineThicknessOfLevel[]{1, 4, 7};

const string jenaLibPath = "D:/Utilities/apache-jena-3.2.0/lib/";

extern RotatedRect teethEllipse;
