#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

#include <QImage>

#include "EllipticCurve.h"

using namespace std;

Mat qImage2Mat(const QImage& inputImage);

QImage mat2QImage(const Mat& inputMat);

Mat qPixmap2Mat(const QPixmap& inputPixmap);

QPixmap mat2QPixmap(const Mat& inputMat);

Size qSize2Size(const QSize& size);

QSize size2QSize(const Size& size);

template <typename T>
T degree2Radian(T degree) { return degree / 180 * M_PI; }

template <typename T>
T radian2Degree(T radian) { return radian / M_PI * 180; }

template <typename T>
Point2f rotate(const Point_<T>& point, float angle) { return Point2f(point.x * cos(angle) - point.y * sin(angle), point.y * cos(angle) + point.x * sin(angle)); }

void concatenatePath(string& path, const string& searchDirectory, const string& extension);

string getClsSig(const char* clsStr);

extern const int lineThicknessOfLevel[];

extern RotatedRect teethEllipse;

Point2f computeNormalDirection(const Point2f& point, float* angle);

void computeInscribedCircle(const vector<Point>& points, float maxRadius, EllipticCurve& ellipticCurve, vector<Point>& tangentPoints);

void drawSmoothCurve(Mat& designImage, const vector<Point> curve, float maxRadius, const Scalar& color, int thickness, LineTypes lineType = LINE_AA);

template <typename T>
Point2f normalize(const Point_<T>& point) { return static_cast<Point2f>(point) / norm(point); }

template <typename T>
int roundToInt(T num) { return round(num); }

template <typename T>
Point roundToInt(const Point_<T>& point) { return Point(round(point.x), round(point.y)); }

template <typename T>
Size roundToInt(const Size_<T>& size) { return Size(round(size.width), round(size.height)); }
