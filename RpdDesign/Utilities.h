#pragma once

#include "Rpd.h"

float degreeToRadian(const float& degree);

float radianToDegree(const float& radian);

template <typename T>
Point2f rotate(const Point_<T>& point, const float& angle) { return Point2f(point.x * cos(angle) - point.y * sin(angle), point.y * cos(angle) + point.x * sin(angle)); }

template <typename T>
Point2f normalize(const Point_<T>& point) { return static_cast<Point2f>(point) / norm(point); }

template <typename T>
Point roundToPoint(const Point_<T>& point) { return Point(round(point.x), round(point.y)); }

void catPath(string& path, const string& searchDirectory, const string& extension);

string getClsSig(const char* const& clsStr);

Rpd::Direction operator~(const Rpd::Direction& direction);

const Tooth& getTooth(const vector<Tooth> teeth[nZones], const Rpd::Position& position);

Tooth& getTooth(vector<Tooth> teeth[nZones], const Rpd::Position& position);

bool isBlockedByMajorConnector(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions);

void computeStringCurves(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const vector<float>& distanceScales, const deque<bool>& keepStartEndPoints, const deque<bool>& considerAnchorDisplacements, const bool& considerDistalPoints, vector<vector<Point>>& curves, float* const& sumOfRadii = nullptr, int* const& nTeeth = nullptr, vector<Point>* const& distalPoints = nullptr);

void computeStringCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const float& distanceScale, const deque<bool>& keepStartEndPoints, const deque<bool>& considerAnchorDisplacements, const bool& considerDistalPoints, vector<Point>& curve, float* const& sumOfRadii = nullptr, int* const& nTeeth = nullptr, vector<Point>* const& distalPoints = nullptr);

void computeInscribedCurve(const vector<Point>& cornerPoints, vector<Point>& curve, const float& smoothness = 0.5F, const bool& shouldAppend = true);

void computeSmoothCurve(const vector<Point>& curve, vector<Point>& smoothCurve, const bool& isClosed = false, const float& smoothness = 0.5F);

void computePiecewiseSmoothCurve(const vector<Point>& curve, vector<Point>& piecewiseSmoothCurve, const bool& smoothStart = true, const bool& smoothEnd = true);

void computeLingualCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const bool& shouldFindAnchors, vector<Point>& curve, vector<vector<Point>>& curves, const vector<Point>& anchorPoints = {}, vector<Point>* const& distalPoints = nullptr);

void computeLingualCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const bool& shouldFindAnchors, vector<Point>& curve, vector<vector<Point>>& curves, const vector<Point>& anchorPoints, Point& distalPoint);

void computeMesialCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<Point>& curve, vector<Point>* const& innerCurve = nullptr);

void computeDistalCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const vector<Point>& distalPoints, vector<Point>& curve, vector<Point>* const& innerCurve = nullptr);

void computeInnerCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const bool& shouldFindAnchors, const float& avgRadius, vector<Point>& curve, vector<vector<Point>>& curves, const vector<Point>& anchorPoints = {});

void computeOuterCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<Point>& curve, float* const& avgRadius = nullptr);

void computeLingualConfrontationCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<Point>& curve);

void computeLingualConfrontationCurves(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<vector<Point>>& curves);

Point2f computeNormalDirection(const Point2f& point, float* const& angle = nullptr);

bool shouldAnchor(const vector<Tooth> teeth[nZones], const Rpd::Position& position, const Rpd::Direction& direction);

void findAnchorPoints(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const bool& shouldFindAnchors, const vector<Point>& anchorPoints, vector<Rpd::Position>& startEndPositions, vector<Point>* const& thisAnchorPoints = nullptr);

bool isLastTooth(const Rpd::Position& position);

bool queryRpds(JNIEnv* const& env, const jobject& ontModel, vector<Rpd*>& rpds);

bool analyzeBaseImage(const Mat& image, vector<Tooth> remediedTeeth[nZones], Mat remediedDesignImages[2], vector<Tooth> teeth[nZones] = nullptr, Mat designImages[2] = nullptr, Mat* const& baseImage = nullptr);

void updateDesign(vector<Tooth> teeth[nZones], vector<Rpd*>& rpds, Mat designImages[2], const bool& justLoadedImage, const bool& justLoadedRpds);
