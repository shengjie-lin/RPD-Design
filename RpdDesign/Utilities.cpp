#include <Windows.h>
#include <opencv2/imgproc.hpp>
#include <QMessageBox>

#include "Utilities.h"
#include "EllipticCurve.h"
#include "Tooth.h"

QImage matToQImage(const Mat& inputMat) {
	switch (inputMat.type()) {
			// 8-bit, 4-channel
		case CV_8UC4:
			return QImage(inputMat.data, inputMat.cols, inputMat.rows, inputMat.step, QImage::Format_ARGB32);
			// 8-bit, 3-channel
		case CV_8UC3:
			return QImage(inputMat.data, inputMat.cols, inputMat.rows, inputMat.step, QImage::Format_RGB888).rgbSwapped();
			// 8-bit, 1-channel
		case CV_8UC1: {
			static auto isColorTableReady = false;
			static QVector<QRgb> colorTable(256);
			if (!isColorTableReady) {
				for (auto i = 0; i < 256; ++i)
					colorTable[i] = qRgb(i, i, i);
				isColorTableReady = true;
			}
			QImage image(inputMat.data, inputMat.cols, inputMat.rows, inputMat.step, QImage::Format_Indexed8);
			image.setColorTable(colorTable);
			return image;
		}
		default:
			QMessageBox::information(nullptr, "", QString(("matToQImage() - Mat type not handled :  " + to_string(inputMat.type())).data()));
			return QImage();
	}
}

QPixmap matToQPixmap(const Mat& inputMat) { return QPixmap::fromImage(matToQImage(inputMat)); }

Size qSizeToSize(const QSize& size) { return Size(size.width(), size.height()); }

QSize sizeToQSize(const Size& size) { return QSize(size.width, size.height); }

float degreeToRadian(const float& degree) { return degree / 180 * CV_PI; }

float radianToDegree(const float& radian) { return radian / CV_PI * 180; }

void catPath(string& path, const string& searchDirectory, const string& extension) {
	auto searchPattern = searchDirectory + extension;
	WIN32_FIND_DATA findData;
	auto hFind = FindFirstFile(searchPattern.c_str(), &findData);
	do
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			path.append(searchDirectory + findData.cFileName + ';');
	while (FindNextFile(hFind, &findData));
	FindClose(hFind);
}

string getClsSig(const char* const& clsStr) { return 'L' + string(clsStr) + ';'; }

const Tooth& getTooth(const vector<Tooth> teeth[nZones], const Rpd::Position& position) { return teeth[position.zone][position.ordinal]; }

Tooth& getTooth(vector<Tooth> teeth[nZones], const Rpd::Position& position) { return const_cast<Tooth&>(getTooth(const_cast<const vector<Tooth>*>(teeth), position)); }

void computeStringCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<Point>& curve, float& avgRadius, bool* const& isBlockedByMajorConnector) {
	float sumOfRadii = 0;
	auto nTeeth = 0;
	computeStringCurve(teeth, positions, curve, sumOfRadii, nTeeth, isBlockedByMajorConnector);
	avgRadius = sumOfRadii / nTeeth;
}

void computeStringCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<Point>& curve, float& sumOfRadii, int& nTeeth, bool* const& isBlockedByMajorConnector) {
	curve.clear();
	if (isBlockedByMajorConnector)
		*isBlockedByMajorConnector = false;
	if (positions[0].zone == positions[1].zone) {
		Point lastPoint;
		for (auto position = positions[0]; position <= positions[1]; ++position) {
			++nTeeth;
			auto& tooth = getTooth(teeth, position);
			sumOfRadii += tooth.getRadius();
			auto& thisPoint = tooth.getAnglePoint(0);
			curve.push_back(position == positions[0] ? thisPoint : (thisPoint + lastPoint) / 2);
			curve.push_back(tooth.getCentroid());
			lastPoint = tooth.getAnglePoint(180);
			if (position == positions[1])
				curve.push_back(lastPoint);
			if (isBlockedByMajorConnector && !*isBlockedByMajorConnector)
				*isBlockedByMajorConnector = tooth.getLingualBlockage() == RpdAsLingualBlockage::MAJOR_CONNECTOR;
		}
	}
	else {
		vector<Point> curve1, curve2;
		computeStringCurve(teeth, {Rpd::Position(positions[0].zone, 0), positions[0]}, curve1, sumOfRadii, nTeeth, isBlockedByMajorConnector);
		computeStringCurve(teeth, {Rpd::Position(positions[1].zone, 0), positions[1]}, curve2, sumOfRadii, nTeeth, isBlockedByMajorConnector && !*isBlockedByMajorConnector ? isBlockedByMajorConnector : nullptr);
		curve1[0] = (curve1[0] + curve2[0]) / 2;
		curve2.erase(curve2.begin());
		curve.insert(curve.end(), curve1.rbegin(), curve1.rend());
		curve.insert(curve.end(), curve2.begin(), curve2.end());
	}
}

void computeLingualCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<Point>& curve, vector<vector<Point>>& curves) {
	curve.clear();
	auto shouldConsiderLast = false;
	for (auto position = positions[0]; position <= positions[1]; ++position) {
		auto& tooth = getTooth(teeth, position);
		vector<Point> thisCurve;
		if (shouldConsiderLast || tooth.getLingualBlockage() == RpdAsLingualBlockage::CLASP_DISTAL_REST) {
			auto lastPosition = --Rpd::Position(position);
			float avgRadius;
			computeStringCurve(teeth, {shouldConsiderLast ? lastPosition : position, tooth.getLingualBlockage() == RpdAsLingualBlockage::CLASP_DISTAL_REST ? position : lastPosition}, thisCurve, avgRadius);
			thisCurve.insert(thisCurve.begin(), thisCurve[0]);
			thisCurve.push_back(thisCurve.back());
			for (auto i = 1; i < thisCurve.size() - 1; ++i)
				thisCurve[i] -= roundToInt(computeNormalDirection(thisCurve[i]) * avgRadius * 1.5);
			computeSmoothCurve(thisCurve, thisCurve, false, 1);
		}
		curves.push_back(thisCurve);
		curve.insert(curve.end(), thisCurve.begin(), thisCurve.end());
		if (tooth.getLingualBlockage() == RpdAsLingualBlockage::MAJOR_CONNECTOR) {
			thisCurve = tooth.getCurve(180, 0);
			curve.insert(curve.end(), thisCurve.rbegin(), thisCurve.rend());
		}
		shouldConsiderLast = tooth.getLingualBlockage() == RpdAsLingualBlockage::CLASP_MESIAL_REST;
	}
}

Point2f computeNormalDirection(const Point2f& point, float* const& angle) {
	auto direction = point - teethEllipse.center;
	auto thisAngle = atan2(direction.y, direction.x) - degreeToRadian(teethEllipse.angle);
	if (thisAngle < -CV_PI)
		thisAngle += CV_2PI;
	if (angle)
		*angle = thisAngle;
	auto normalDirection = Point2f(pow(teethEllipse.size.height, 2) * cos(thisAngle), pow(teethEllipse.size.width, 2) * sin(thisAngle));
	return normalDirection / norm(normalDirection);
}

void computeInscribedCurve(const vector<Point>& cornerPoints, vector<Point>& curve, const float& smoothness, const bool& shouldAppend) {
	Point2f v1 = cornerPoints[0] - cornerPoints[1], v2 = cornerPoints[2] - cornerPoints[1];
	auto l1 = norm(v1), l2 = norm(v2);
	auto d1 = v1 / l1, d2 = v2 / l2;
	auto sinTheta = d1.cross(d2);
	auto theta = asin(abs(sinTheta));
	if (d1.dot(d2) < 0)
		theta = CV_PI - theta;
	auto radius = static_cast<float>(min({l1, l2}) * tan(theta / 2) * smoothness);
	vector<Point> thisCurve;
	auto isValidCurve = EllipticCurve(cornerPoints[1] + roundToInt(normalize(d1 + d2) * radius / sin(theta / 2)), roundToInt(Size(radius, radius)), radianToDegree(sinTheta > 0 ? atan2(d2.x, -d2.y) : atan2(d1.x, -d1.y)), 180 - radianToDegree(theta), sinTheta > 0).getCurve(thisCurve);
	if (!shouldAppend)
		curve.clear();
	if (isValidCurve)
		curve.insert(curve.end(), thisCurve.begin(), thisCurve.end());
	else
		curve.push_back(cornerPoints[1]);
}

void computeSmoothCurve(const vector<Point> curve, vector<Point>& smoothCurve, const bool& isClosed, const float& smoothness) {
	smoothCurve.clear();
	for (auto point = curve.begin(); point < curve.end(); ++point) {
		if (point == curve.begin())
			if (isClosed)
				computeInscribedCurve({curve.back(),*point, *(point + 1)}, smoothCurve, smoothness);
			else
				smoothCurve.insert(smoothCurve.end(), *point);
		else if (point == curve.end() - 1)
			if (isClosed)
				computeInscribedCurve({smoothCurve.back(), *point, curve[0]}, smoothCurve, smoothness);
			else
				smoothCurve.insert(smoothCurve.end(), *point);
		else
			computeInscribedCurve({smoothCurve.back(),*point, *(point + 1)}, smoothCurve, smoothness);
	}
}

vector<RpdAsLingualBlockage::LingualBlockage> tipDirectionsToLingualBlockages(const vector<Rpd::Direction> tipDirections) {
	vector<RpdAsLingualBlockage::LingualBlockage> lingualBlockages;
	for (auto tipDirection = tipDirections.begin(); tipDirection < tipDirections.end(); ++tipDirection)
		lingualBlockages.push_back(*tipDirection == Rpd::MESIAL ? RpdAsLingualBlockage::CLASP_DISTAL_REST : RpdAsLingualBlockage::CLASP_MESIAL_REST);
	return lingualBlockages;
}
