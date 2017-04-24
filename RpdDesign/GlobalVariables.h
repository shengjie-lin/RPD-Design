#pragma once

#include <opencv2/core/types.hpp>

using namespace std;
using namespace cv;

const int lineThicknessOfLevel[]{1, 4, 7}, nTeethPerZone = 8, nZones = 4;

const string jenaLibPath = "D:/Utilities/apache-jena-3.2.0/lib/";

extern bool remedyImage;

extern RotatedRect teethEllipse;

extern RotatedRect remediedTeethEllipse;
