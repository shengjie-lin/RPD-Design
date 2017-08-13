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

// 用于连接目录中的所有指定扩展名的文件路径
void catPath(string& path, string const& searchDirectory, string const& extension);

string getClsSig(const char* const& clsStr);

Rpd::Direction operator~(Rpd::Direction const& direction);

Tooth const& getTooth(const vector<Tooth> (&teeth)[nZones], Rpd::Position const& position);

Tooth& getTooth(vector<Tooth> (&teeth)[nZones], Rpd::Position const& position);

bool isBlockedByMajorConnector(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions);

// 计算沿给定起止牙位的串联曲线
void computeStringCurves(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<float> const& distanceScales, deque<bool> const& keepStartEndPoints, deque<bool> const& considerAnchorDisplacements, bool const& considerDistalPoints, vector<vector<Point>>& curves, float* const& sumOfRadii = nullptr, int* const& nTeeth = nullptr, vector<Point>* const& distalPoints = nullptr);

// 计算沿给定起止牙位的串联曲线族
void computeStringCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, float const& distanceScale, deque<bool> const& keepStartEndPoints, deque<bool> const& considerAnchorDisplacements, bool const& considerDistalPoints, vector<Point>& curve, float* const& sumOfRadii = nullptr, int* const& nTeeth = nullptr, vector<Point>* const& distalPoints = nullptr);

// 给定平滑系数，计算任意三点所构成交角的内切曲线
void computeInscribedCurve(vector<Point> const& cornerPoints, vector<Point>& curve, float const& smoothness = 0.5F, bool const& shouldAppend = true);

// 计算将输入曲线进行平滑后的曲线
void computeSmoothCurve(vector<Point> const& curve, vector<Point>& smoothCurve, bool const& isClosed = false, float const& smoothness = 0.5F);

// 计算将输入曲线进行平滑后的曲线，并可选对始末端的平滑方式做特殊处理
void computePiecewiseSmoothCurve(vector<Point> const& curve, vector<Point>& piecewiseSmoothCurve, bool const& smoothStart = true, bool const& smoothEnd = true);

// 计算沿给定起止牙位的舌侧曲线
void computeLingualCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point>& curve, vector<vector<Point>>& curves, vector<Point>* const& distalPoints = nullptr, const vector<Point>* const& anchorPoints = nullptr);

// 计算沿给定起止牙位的舌侧曲线，限定为单侧
void computeLingualCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point>& curve, vector<vector<Point>>& curves, Point& distalPoint, const vector<Point>* const& anchorPoints = nullptr);

// 计算沿给定起止牙位的近中曲线
void computeMesialCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point>& curve, vector<int>& mesialOrdinals, vector<Point>* const& innerCurve = nullptr);

// 计算沿给定起止牙位的远中曲线
void computeDistalCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point> const& distalPoints, vector<Point>& curve, const vector<int>* const& mesialOrdinals = nullptr, vector<Point>* const& innerCurve = nullptr);

// 计算沿给定起止牙位的内侧曲线
void computeInnerCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, float const& avgRadius, vector<Point>& curve, vector<vector<Point>>& curves, const vector<Point>* const& anchorPoints = nullptr);

// 计算沿给定起止牙位的外侧曲线
void computeOuterCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point>& curve, float* const& avgRadius = nullptr);

// 计算沿给定起止牙位的舌侧对抗曲线
void computeLingualConfrontationCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point>& curve);

// 根据舌侧对抗的实际存在情况，计算给定牙位区段内的所有舌侧对抗曲线
void computeLingualConfrontationCurves(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<vector<Point>>& curves);

// 计算指定点在牙列椭圆中的法向方向
Point2f computeNormalDirection(Point2f const& point, float* const& angle = nullptr);

// 判断大连接体的延伸点
bool shouldAnchor(const vector<Tooth> (&teeth)[nZones], Rpd::Position const& position, Rpd::Direction const& direction);

// 寻求大连接体的延伸范围
void findAnchorPoints(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Rpd::Position>& startEndPositions, const vector<Point>* const& inAnchorPoints = nullptr, vector<Point>* const& outAnchorPoints = nullptr);

// 判断是否末尾牙齿
bool isLastTooth(Rpd::Position const& position);

// 检索全部RPD
bool queryRpds(JNIEnv* const& env, jobject const& ontModel, vector<Rpd*>& rpds);

// 分析底图
void analyzeBaseImage(Mat const& image, vector<Tooth> (&remediedTeeth)[nZones], Mat (&remediedDesignImages)[2], vector<Tooth> (*const& teeth)[nZones] = nullptr, Mat (*const& designImages)[2] = nullptr, Mat* const& baseImage = nullptr);

// 更新设计图
void updateDesign(vector<Tooth> (&teeth)[nZones], vector<Rpd*>& rpds, Mat (&designImages)[2], bool const& isRemedied, bool const& justLoadedImage, bool const& justLoadedRpds);
