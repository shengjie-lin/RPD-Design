#pragma once

#include <QImage>

using namespace std;
using namespace cv;

Mat qImageToMat(const QImage& inputImage);

QImage matToQImage(const Mat& inputMat);

Mat qPixmapToMat(const QPixmap& inputPixmap);

QPixmap matToQPixmap(const Mat& inputMat);

Size qSizeToSize(const QSize& size);

QSize sizeToQSize(const Size& size);

float degreeToRadian(float degree);

float radianToDegree(float radian);

template <typename T>
Point2f rotate(const Point_<T>& point, float angle) { return Point2f(point.x * cos(angle) - point.y * sin(angle), point.y * cos(angle) + point.x * sin(angle)); }

template <typename T>
Point2f normalize(const Point_<T>& point) { return static_cast<Point2f>(point) / norm(point); }

template <typename T>
int roundToInt(T num) { return round(num); }

template <typename T>
Point roundToInt(const Point_<T>& point) { return Point(round(point.x), round(point.y)); }

template <typename T>
Size roundToInt(const Size_<T>& size) { return Size(round(size.width), round(size.height)); }

void catPath(string& path, const string& searchDirectory, const string& extension);

string getClsSig(const char* clsStr);

Point2f computeNormalDirection(const Point2f& point, float* angle = nullptr);

void computeIncribedCurve(const vector<Point>& cornerPoints, float maxRadius, vector<Point>& curve, bool shouldAppend = true);

vector<Point> computeSmoothCurve(const vector<Point> curve, bool isClosed = false, float maxRadius = INFINITY);

extern const string jenaLibPath;

extern const int lineThicknessOfLevel[];

extern RotatedRect teethEllipse;
