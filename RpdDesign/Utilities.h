#pragma once

#include "Rpd.h"

float degreeToRadian(float const& degree);

float radianToDegree(float const& radian);

template <typename T>
Point2f rotate(Point_<T> const& point, float const& angle) { return Point2f(point.x * cos(angle) - point.y * sin(angle), point.y * cos(angle) + point.x * sin(angle)); }

template <typename T>
Point2f normalize(Point_<T> const& point) { return static_cast<Point2f>(point) / norm(point); }

template <typename T>
Point roundToPoint(Point_<T> const& point) { return Point(round(point.x), round(point.y)); }

void catPath(string& path, string const& searchDirectory, string const& extension);

string getClsSig(const char* const& clsStr);

Rpd::Direction operator~(Rpd::Direction const& direction);

Tooth const& getTooth(const vector<Tooth> (&teeth)[nZones], Rpd::Position const& position);

Tooth& getTooth(vector<Tooth> (&teeth)[nZones], Rpd::Position const& position);

bool isBlockedByMajorConnector(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions);

void computeStringCurves(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<float> const& distanceScales, deque<bool> const& keepStartEndPoints, deque<bool> const& considerAnchorDisplacements, bool const& considerDistalPoints, vector<vector<Point>>& curves, float* const& sumOfRadii = nullptr, int* const& nTeeth = nullptr, vector<Point>* const& distalPoints = nullptr);

void computeStringCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, float const& distanceScale, deque<bool> const& keepStartEndPoints, deque<bool> const& considerAnchorDisplacements, bool const& considerDistalPoints, vector<Point>& curve, float* const& sumOfRadii = nullptr, int* const& nTeeth = nullptr, vector<Point>* const& distalPoints = nullptr);

void computeInscribedCurve(vector<Point> const& cornerPoints, vector<Point>& curve, float const& smoothness = 0.5F, bool const& shouldAppend = true);

void computeSmoothCurve(vector<Point> const& curve, vector<Point>& smoothCurve, bool const& isClosed = false, float const& smoothness = 0.5F);

void computePiecewiseSmoothCurve(vector<Point> const& curve, vector<Point>& piecewiseSmoothCurve, bool const& smoothStart = true, bool const& smoothEnd = true);

void computeLingualCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point>& curve, vector<vector<Point>>& curves, vector<Point>* const& distalPoints = nullptr, const vector<Point>* const& anchorPoints = nullptr);

void computeLingualCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point>& curve, vector<vector<Point>>& curves, Point& distalPoint, const vector<Point>* const& anchorPoints = nullptr);

void computeMesialCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point>& curve, vector<Point>* const& innerCurve = nullptr);

void computeDistalCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point> const& distalPoints, vector<Point>& curve, vector<Point>* const& innerCurve = nullptr);

void computeInnerCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, float const& avgRadius, vector<Point>& curve, vector<vector<Point>>& curves, const vector<Point>* const& anchorPoints = nullptr);

void computeOuterCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point>& curve, float* const& avgRadius = nullptr);

void computeLingualConfrontationCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point>& curve);

void computeLingualConfrontationCurves(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<vector<Point>>& curves);

Point2f computeNormalDirection(Point2f const& point, float* const& angle = nullptr);

bool shouldAnchor(const vector<Tooth> (&teeth)[nZones], Rpd::Position const& position, Rpd::Direction const& direction);

void findAnchorPoints(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Rpd::Position>& startEndPositions, const vector<Point>* const& inAnchorPoints = nullptr, vector<Point>* const& outAnchorPoints = nullptr);

bool isLastTooth(Rpd::Position const& position);

bool queryRpds(JNIEnv* const& env, jobject const& ontModel, vector<Rpd*>& rpds);

void analyzeBaseImage(Mat const& image, vector<Tooth> (&remediedTeeth)[nZones], Mat (&remediedDesignImages)[2], vector<Tooth> (*const& teeth)[nZones] = nullptr, Mat (*const& designImages)[2] = nullptr, Mat* const& baseImage = nullptr);

void updateDesign(vector<Tooth> (&teeth)[nZones], vector<Rpd*>& rpds, Mat (&designImages)[2], bool const& justLoadedImage, bool const& justLoadedRpds);
