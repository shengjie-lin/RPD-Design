#pragma once

#include <QImage>

using namespace std;
using namespace cv;

Mat qImage2Mat(const QImage& inputImage);

QImage mat2QImage(const Mat& inputMat);

Mat qPixmap2Mat(const QPixmap& inputPixmap);

QPixmap mat2QPixmap(const Mat& inputMat);

Size qSize2Size(const QSize& size);

QSize size2QSize(const Size& size);

template <typename T>
T degree2Radian(T degree) { return degree / 180 * CV_PI; }

template <typename T>
T radian2Degree(T radian) { return radian / CV_PI * 180; }

template <typename T>
Point2f rotate(const Point_<T>& point, float angle) { return Point2f(point.x * cos(angle) - point.y * sin(angle), point.y * cos(angle) + point.x * sin(angle)); }

void concatenatePath(string& path, const string& searchDirectory, const string& extension);

string getClsSig(const char* clsStr);

Point2f computeNormalDirection(const Point2f& point, float* angle = nullptr);

vector<Point> computeSmoothCurve(const vector<Point> curve, float maxRadius);

template <typename T>
Point2f normalize(const Point_<T>& point) { return static_cast<Point2f>(point) / norm(point); }

template <typename T>
int roundToInt(T num) { return round(num); }

template <typename T>
Point roundToInt(const Point_<T>& point) { return Point(round(point.x), round(point.y)); }

template <typename T>
Size roundToInt(const Size_<T>& size) { return Size(round(size.width), round(size.height)); }

extern const int lineThicknessOfLevel[];

extern const string jenaLibPath;

extern RotatedRect teethEllipse;
