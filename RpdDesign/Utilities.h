#pragma once

#include <QImage>

#include "Rpd.h"

QImage matToQImage(const Mat& inputMat);

QPixmap matToQPixmap(const Mat& inputMat);

Size qSizeToSize(const QSize& size);

QSize sizeToQSize(const Size& size);

float degreeToRadian(const float& degree);

float radianToDegree(const float& radian);

template <typename T>
Point2f rotate(const Point_<T>& point, const float& angle) { return Point2f(point.x * cos(angle) - point.y * sin(angle), point.y * cos(angle) + point.x * sin(angle)); }

template <typename T>
Point2f normalize(const Point_<T>& point) { return static_cast<Point2f>(point) / norm(point); }

template <typename T>
Point roundToInt(const Point_<T>& point) { return Point(round(point.x), round(point.y)); }

template <typename T>
Size roundToInt(const Size_<T>& size) { return Size(round(size.width), round(size.height)); }

void catPath(string& path, const string& searchDirectory, const string& extension);

string getClsSig(const char*const& clsStr);

Tooth& getTooth(vector<Tooth> teeth[nZones], const Rpd::Position& position);

const Tooth& getTooth(const vector<Tooth> teeth[nZones], const Rpd::Position& position);

const Tooth& getTooth(const vector<Tooth> teeth[nZones], const RpdAsMajorConnector::Anchor& anchor, const int& shift = 0, const bool& shouldMirror = false);

const Point& getPoint(const vector<Tooth> teeth[nZones], const RpdAsMajorConnector::Anchor& anchor, const int& shift = 0, const bool& shouldMirror = false);

void computeStringCurve(const vector<Tooth> teeth[nZones], const Rpd::Position& startPosition, const Rpd::Position& endPosition, vector<Point>& curve, float& avgRadius, bool*const& hasLingualBlockage = nullptr);

Point2f computeNormalDirection(const Point2f& point, float*const& angle = nullptr);

void computeInscribedCurve(const vector<Point>& cornerPoints, vector<Point>& curve, const float& maxRadius, const bool& shouldAppend = true);

void computeSmoothCurve(const vector<Point> curve, vector<Point>& smoothCurve, const bool& isClosed = false, const float& maxRadius = INFINITY);
