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
			QMessageBox::warning(nullptr, "", QString(("matToQImage() - Mat type not handled :  " + to_string(inputMat.type())).data()));
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

Rpd::Direction operator~(const Rpd::Direction& direction) { return direction == Rpd::MESIAL ? Rpd::DISTAL : Rpd::MESIAL; }

bool isDentureBase(const RpdAsLingualBlockage::LingualBlockage& lingualBlockage) { return lingualBlockage == RpdAsLingualBlockage::DENTURE_BASE || lingualBlockage == RpdAsLingualBlockage::DENTURE_BASE_TAIL; }

const Tooth& getTooth(const vector<Tooth> teeth[nZones], const Rpd::Position& position) { return teeth[position.zone][position.ordinal]; }

Tooth& getTooth(vector<Tooth> teeth[nZones], const Rpd::Position& position) { return const_cast<Tooth&>(getTooth(const_cast<const vector<Tooth>*>(teeth), position)); }

bool findLingualBlockage(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions) {
	if (positions[0].zone == positions[1].zone) {
		for (auto position = positions[0]; position <= positions[1]; ++position)
			if (getTooth(teeth, position).getLingualBlockage())
				return true;
		return false;
	}
	return findLingualBlockage(teeth, {Rpd::Position(positions[0].zone, 0), positions[0]}) || findLingualBlockage(teeth, {Rpd::Position(positions[1].zone, 0), positions[1]});
}

void computeStringCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<Point>& curve, float& sumOfRadii, int& nTeeth) {
	curve.clear();
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
		}
	}
	else {
		vector<vector<Point>> tmpCurves(2);
		for (auto i = 0; i < 2; ++i)
			computeStringCurve(teeth, {Rpd::Position(positions[i].zone, 0), positions[i]}, tmpCurves[i], sumOfRadii, nTeeth);
		tmpCurves[0][0] = (tmpCurves[0][0] + tmpCurves[1][0]) / 2;
		tmpCurves[1].erase(tmpCurves[1].begin());
		curve.insert(curve.end(), tmpCurves[0].rbegin(), tmpCurves[0].rend());
		curve.insert(curve.end(), tmpCurves[1].begin(), tmpCurves[1].end());
	}
}

void computeStringCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<Point>& curve, float& avgRadius) {
	float sumOfRadii = 0;
	auto nTeeth = 0;
	computeStringCurve(teeth, positions, curve, sumOfRadii, nTeeth);
	avgRadius = sumOfRadii / nTeeth;
}

void computeStringCurves(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const vector<float>& distanceScales, const bool& keepStartEndPoints, const bool& considerDistalPoints, vector<vector<Point>>& curves, float* const& avgRadius, vector<Point>* const& distalPoints) {
	vector<Point> curve;
	float thisAvgRadius;
	computeStringCurve(teeth, positions, curve, thisAvgRadius);
	if (avgRadius)
		*avgRadius = thisAvgRadius;
	if (considerDistalPoints) {
		if (distalPoints)
			*distalPoints = vector<Point>(2);
		if (positions[0].zone != positions[1].zone) {
			auto& tooth = getTooth(teeth, positions[0]);
			if (tooth.getLingualBlockage() == RpdAsLingualBlockage::DENTURE_BASE_TAIL) {
				auto distalPoint = tooth.getAnglePoint(180);
				curve.insert(curve.begin(), distalPoint + roundToPoint(rotate(computeNormalDirection(distalPoint), -CV_PI / 2) * thisAvgRadius * 0.6F));
				if (distalPoints)
					(*distalPoints)[0] = curve[0];
			}
		}
		auto& tooth = getTooth(teeth, positions[1]);
		if (tooth.getLingualBlockage() == RpdAsLingualBlockage::DENTURE_BASE_TAIL) {
			auto distalPoint = tooth.getAnglePoint(180);
			curve.push_back(distalPoint + roundToPoint(rotate(computeNormalDirection(distalPoint), CV_PI * (positions[1].zone % 2 - 0.5)) * thisAvgRadius * 0.6F));
			if (distalPoints)
				(*distalPoints)[1] = curve.back();
		}
	}
	curves.clear();
	for (auto i = 0; i < distanceScales.size(); ++i)
		curves.push_back(curve);
	for (auto i = 0; i < curve.size(); ++i) {
		auto delta = computeNormalDirection(curve[i]) * thisAvgRadius;
		for (auto j = 0; j < distanceScales.size(); ++j)
			curves[j][i] += roundToPoint(delta * distanceScales[j]);
		if (keepStartEndPoints)
			if (i == 0) {
				for (auto j = 0; j < distanceScales.size(); ++j)
					curves[j].insert(curves[j].begin(), curve[0]);
				curve.insert(curve.begin(), curve[0]);
				++i;
			}
			else if (i == curve.size() - 1) {
				for (auto j = 0; j < distanceScales.size(); ++j)
					curves[j].push_back(curve.back());
				curve.push_back(curve.back());
				++i;
			}
	}
}

void computeStringCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const float& distanceScale, const bool& keepStartEndPoints, const bool& considerDistalPoints, vector<Point>& curve, float* const& avgRadius, vector<Point>* const& distalPoints) {
	vector<vector<Point>> tmpCurves;
	computeStringCurves(teeth, positions, {distanceScale}, keepStartEndPoints, considerDistalPoints, tmpCurves, avgRadius, distalPoints);
	curve = tmpCurves[0];
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
	auto isValidCurve = EllipticCurve(cornerPoints[1] + roundToPoint(normalize(d1 + d2) * radius / sin(theta / 2)), Size(radius, radius), radianToDegree(sinTheta > 0 ? atan2(d2.x, -d2.y) : atan2(d1.x, -d1.y)), 180 - radianToDegree(theta), sinTheta > 0).getCurve(thisCurve);
	if (!shouldAppend)
		curve.clear();
	if (isValidCurve)
		curve.insert(curve.end(), thisCurve.begin(), thisCurve.end());
	else
		curve.push_back(cornerPoints[1]);
}

void computeSmoothCurve(const vector<Point>& curve, vector<Point>& smoothCurve, const bool& isClosed, const float& smoothness) {
	vector<Point> tmpCurve;
	for (auto point = curve.begin(); point < curve.end(); ++point) {
		auto isFirst = point == curve.begin(), isLast = point == curve.end() - 1;
		if (isClosed || !(isFirst || isLast))
			computeInscribedCurve({isFirst ? curve.back() : *(point - 1), *point, isLast ? curve[0] : *(point + 1)}, tmpCurve, smoothness);
		else
			tmpCurve.insert(tmpCurve.end(), *point);
	}
	smoothCurve = tmpCurve;
}

void computePiecewiseSmoothCurve(const vector<Point>& curve, vector<Point>& piecewiseSmoothCurve, const bool& shouldSmoothStart, const bool& shouldSmoothEnd) {
	vector<vector<Point>> smoothCurves(3);
	smoothCurves[1] = vector<Point>{curve.begin() + 2, curve.end() - 2};
	if (shouldSmoothStart) {
		computeInscribedCurve(vector<Point>{curve.begin(), curve.begin() + 3}, smoothCurves[0], 1);
		smoothCurves[0].insert(smoothCurves[0].begin(), curve[0]);
		smoothCurves[1].insert(smoothCurves[1].begin(), smoothCurves[0].back());
	}
	else
		smoothCurves[1].insert(smoothCurves[1].begin(), curve.begin(), curve.begin() + 2);
	if (shouldSmoothEnd) {
		computeInscribedCurve(vector<Point>{curve.end() - 3, curve.end()}, smoothCurves[2], 1);
		smoothCurves[2].push_back(curve.back());
		smoothCurves[1].push_back(smoothCurves[2][0]);
	}
	else
		smoothCurves[1].insert(smoothCurves[1].end(), curve.end() - 2, curve.end());
	computeSmoothCurve(smoothCurves[1], smoothCurves[1]);
	piecewiseSmoothCurve.clear();
	for (auto i = 0; i < 3; ++i)
		piecewiseSmoothCurve.insert(piecewiseSmoothCurve.end(), smoothCurves[i].begin(), smoothCurves[i].end());
}

void computeLingualCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<Point>& curve, vector<vector<Point>>& curves, vector<Point>* const& distalPoints) {
	curve.clear();
	if (positions[0].zone == positions[1].zone) {
		auto dbStartPosition = positions[0];
		while (dbStartPosition <= positions[1] && !isDentureBase(getTooth(teeth, dbStartPosition).getLingualBlockage()))
			++dbStartPosition;
		auto shouldConsiderLast = false;
		for (auto position = positions[0]; position <= dbStartPosition; ++position) {
			vector<Point> thisCurve;
			auto lingualBlockage = position < dbStartPosition ? getTooth(teeth, position).getLingualBlockage() : RpdAsLingualBlockage::NONE;
			auto hasClaspDistalRest = lingualBlockage == RpdAsLingualBlockage::ARMED_DISTAL_REST;
			if (shouldConsiderLast || hasClaspDistalRest) {
				auto lastPosition = --Rpd::Position(position);
				computeStringCurve(teeth, {shouldConsiderLast ? lastPosition : position, hasClaspDistalRest ? position : lastPosition}, -1.6F, true, false, thisCurve);
				computePiecewiseSmoothCurve(thisCurve, thisCurve);
				curves.push_back(thisCurve);
				curve.insert(curve.end(), thisCurve.begin(), thisCurve.end());
			}
			if (lingualBlockage == RpdAsLingualBlockage::MAJOR_CONNECTOR) {
				thisCurve = getTooth(teeth, position).getCurve(180, 0);
				curve.insert(curve.end(), thisCurve.rbegin(), thisCurve.rend());
			}
			shouldConsiderLast = lingualBlockage == RpdAsLingualBlockage::ARMED_MESIAL_REST;
		}
		if (dbStartPosition <= positions[1]) {
			vector<Point> dbCurve;
			computeStringCurve(teeth, {dbStartPosition, positions[1]}, -1.6F, true, true, dbCurve, nullptr, distalPoints);
			computePiecewiseSmoothCurve(dbCurve, dbCurve);
			curve.insert(curve.end(), dbCurve.begin(), dbCurve.end());
		}
	}
	else {
		vector<int> zones = {positions[0].zone, positions[1].zone};
		vector<Rpd::Position> startPositions = {Rpd::Position(zones[0], 0), Rpd::Position(zones[1], 0)};
		vector<RpdAsLingualBlockage::LingualBlockage> startLingualBlockages = {getTooth(teeth, startPositions[0]).getLingualBlockage() , getTooth(teeth, startPositions[1]).getLingualBlockage()};
		vector<Point*> thisDistalPoints;
		if (distalPoints) {
			*distalPoints = vector<Point>(2);
			for (auto i = 0; i < 2; ++i)
				thisDistalPoints.push_back(&(*distalPoints)[i]);
		}
		else
			thisDistalPoints = {nullptr, nullptr};
		vector<vector<Point>> tmpCurves(2);
		if (isDentureBase(startLingualBlockages[0]) && isDentureBase(startLingualBlockages[1])) {
			auto dbPositions = startPositions;
			for (auto i = 0; i < 2; ++i) {
				while (isDentureBase(getTooth(teeth, ++Rpd::Position(dbPositions[i])).getLingualBlockage()))
					++dbPositions[i];
				computeLingualCurve(teeth, {++Rpd::Position(dbPositions[i]), positions[i]}, tmpCurves[i], curves, thisDistalPoints[i]);
			}
			vector<Point> tmpDistalPoints;
			computeStringCurve(teeth, {dbPositions[0], dbPositions[1]}, -1.6F, true, true, curve, nullptr, &tmpDistalPoints);
			computePiecewiseSmoothCurve(curve, curve);
			curve.insert(curve.begin(), tmpCurves[0].rbegin(), tmpCurves[0].rend());
			curve.insert(curve.end(), tmpCurves[1].begin(), tmpCurves[1].end());
			for (auto i = 0; i < 2; ++i)
				if (tmpDistalPoints[i] != Point())
					(*distalPoints)[i] = tmpDistalPoints[i];
		}
		else if (startLingualBlockages[0] == RpdAsLingualBlockage::ARMED_DISTAL_REST && startLingualBlockages[1] == RpdAsLingualBlockage::ARMED_DISTAL_REST) {
			for (auto i = 0; i < 2; ++i)
				computeLingualCurve(teeth, {Rpd::Position(zones[i], 1), positions[i]}, tmpCurves[i], curves, thisDistalPoints[i]);
			computeStringCurve(teeth, {startPositions[0], startPositions[1]}, -1.6F, true, false, curve);
			computePiecewiseSmoothCurve(curve, curve);
			curves.push_back(curve);
			curve.insert(curve.begin(), tmpCurves[0].rbegin(), tmpCurves[0].rend());
			curve.insert(curve.end(), tmpCurves[1].begin(), tmpCurves[1].end());
		}
		else {
			for (auto i = 0; i < 2; ++i)
				computeLingualCurve(teeth, {startPositions[i], positions[i]}, tmpCurves[i], curves, thisDistalPoints[i]);
			curve.insert(curve.end(), tmpCurves[0].rbegin(), tmpCurves[0].rend());
			curve.insert(curve.end(), tmpCurves[1].begin(), tmpCurves[1].end());
		}
	}
}

void computeLingualCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<Point>& curve, vector<vector<Point>>& curves, Point* const& distalPoint) {
	vector<Point> distalPoints;
	computeLingualCurve(teeth, positions, curve, curves, &distalPoints);
	if (distalPoints.size())
		*distalPoint = distalPoints[1];
}

void computeMesialCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<Point>& curve, vector<Point>* const& innerCurve) {
	auto& ordinal = max(positions[0].ordinal, positions[1].ordinal);
	vector<vector<Point>> curves(2);
	float sumOfRadii = 0;
	auto nTeeth = 0;
	for (auto i = 0; i < 2; ++i) {
		computeStringCurve(teeth, {positions[i], Rpd::Position(positions[i].zone, ordinal)}, curves[i], sumOfRadii, nTeeth);
		curves[i].insert(curves[i].begin(), curves[i][0]);
	}
	auto avgRadius = sumOfRadii / nTeeth;
	for (auto i = 0; i < 2; ++i)
		for (auto point = curves[i].begin() + 1; point < curves[i].end(); ++point)
			*point -= roundToPoint(computeNormalDirection(*point) * avgRadius * 2.4F);
	if (innerCurve) {
		innerCurve->clear();
		innerCurve->push_back(*(curves[0].end() - 2));
		innerCurve->push_back((curves[0].back() + curves[1].back()) / 2);
		innerCurve->push_back(*(curves[1].end() - 2));
	}
	curve = vector<Point>{curves[0].begin(), curves[0].end() - 2};
	curve.push_back((*(curves[0].end() - 2) + *(curves[1].end() - 2)) / 2);
	curve.insert(curve.end(), curves[1].rbegin() + 2, curves[1].rend());
	computeSmoothCurve(curve, curve);
}

void computeDistalCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const vector<Point>& distalPoints, vector<Point>& curve, vector<Point>* const& innerCurve) {
	auto& ordinal = min(positions[0].ordinal, positions[1].ordinal);
	vector<vector<Point>> curves(2);
	float sumOfRadii = 0;
	auto nTeeth = 0;
	for (auto i = 0; i < 2; ++i) {
		computeStringCurve(teeth, {Rpd::Position(positions[i].zone, ordinal), positions[i]}, curves[i], sumOfRadii, nTeeth);
		if (getTooth(teeth, positions[i]).getLingualBlockage() == RpdAsLingualBlockage::DENTURE_BASE_TAIL)
			curves[i].push_back(distalPoints[i]);
		curves[i].push_back(curves[i].back());
	}
	auto avgRadius = sumOfRadii / nTeeth;
	for (auto i = 0; i < 2; ++i)
		for (auto point = curves[i].begin(); point < curves[i].end() - 1; ++point)
			*point -= roundToPoint(computeNormalDirection(*point) * avgRadius * 2.4F);
	if (innerCurve) {
		innerCurve->push_back((innerCurve->back() + curves[0][1]) / 2);
		innerCurve->push_back(curves[0][1]);
		innerCurve->push_back((curves[0][0] + curves[1][0]) / 2);
		innerCurve->push_back(curves[1][1]);
		innerCurve->push_back((curves[1][1] + (*innerCurve)[0]) / 2);
		computeSmoothCurve(*innerCurve, *innerCurve, true);
	}
	curve = vector<Point>{curves[0].rbegin(), curves[0].rend() - 2};
	curve.push_back((curves[0][1] + curves[1][1]) / 2);
	curve.insert(curve.end(), curves[1].begin() + 2, curves[1].end());
	computeSmoothCurve(curve, curve);
}

void computeInnerCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const float& avgRadius, const bool hasLingualConfrontations[nZones][nTeethPerZone], vector<Point>& curve, const bool& isSelfCall) {
	if (positions[0].zone == positions[1].zone) {
		auto& zone = positions[0].zone;
		auto& startOrdinal = positions[0].ordinal;
		auto& endOrdinal = positions[1].ordinal;
		auto curOrdinal = startOrdinal - 1;
		while (curOrdinal < endOrdinal) {
			auto thisStartOrdinal = curOrdinal + 1;
			while (curOrdinal < endOrdinal && !hasLingualConfrontations[zone][curOrdinal + 1])
				++curOrdinal;
			if (curOrdinal >= thisStartOrdinal) {
				vector<Point> tmpCurve;
				computeStringCurve(teeth, {Rpd::Position(zone, thisStartOrdinal), Rpd::Position(zone, curOrdinal)}, -1.5F, true, false, tmpCurve);
				auto shouldSmoothStart = thisStartOrdinal == startOrdinal && !isSelfCall, shouldSmoothEnd = curOrdinal == endOrdinal;
				if (!shouldSmoothStart)
					if (thisStartOrdinal)
						tmpCurve[0] = getTooth(teeth, Rpd::Position(zone, thisStartOrdinal - 1)).getAnglePoint(180);
					else
						tmpCurve[0] = getTooth(teeth, Rpd::Position(zone + 1 - zone % 2 * 2, 0)).getAnglePoint(0);
				if (!shouldSmoothEnd)
					tmpCurve.back() = getTooth(teeth, Rpd::Position(zone, curOrdinal + 1)).getAnglePoint(0);
				computePiecewiseSmoothCurve(tmpCurve, tmpCurve, shouldSmoothStart, shouldSmoothEnd);
				curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
				if (curOrdinal == endOrdinal)
					break;
				thisStartOrdinal = curOrdinal + 1;
			}
			while (curOrdinal < endOrdinal && hasLingualConfrontations[zone][curOrdinal + 1])
				++curOrdinal;
			vector<vector<Point>> tmpCurves;
			computeLingualConfrontationCurves(teeth, {Rpd::Position(zone, thisStartOrdinal), Rpd::Position(zone, curOrdinal)}, hasLingualConfrontations, tmpCurves);
			curve.insert(curve.end(), tmpCurves[0].begin(), tmpCurves[0].end());
		}
	}
	else {
		vector<vector<Point>> curves(2);
		vector<Rpd::Position> startEndPositions;
		auto hasLingualConfrontation = hasLingualConfrontations[positions[0].zone][0] == hasLingualConfrontations[positions[1].zone][0] ? hasLingualConfrontations[positions[0].zone][0] : -1;
		for (auto i = 0; i < 2; ++i) {
			auto& zone = positions[i].zone;
			if (hasLingualConfrontation < 0)
				computeInnerCurve(teeth, {Rpd::Position(zone, 0), positions[i]}, avgRadius, hasLingualConfrontations, curves[i], true);
			else {
				startEndPositions.push_back(Rpd::Position(zone, 0));
				while (startEndPositions[i] < positions[i] && static_cast<int>(hasLingualConfrontations[zone][startEndPositions[i].ordinal + 1]) == hasLingualConfrontation)
					++startEndPositions[i];
				computeInnerCurve(teeth, {++Rpd::Position(startEndPositions[i]), positions[i]}, avgRadius, hasLingualConfrontations, curves[i], true);
			}
		}
		curve.insert(curve.end(), curves[0].rbegin(), curves[0].rend());
		if (hasLingualConfrontation > 0) {
			vector<vector<Point>> tmpCurves;
			computeLingualConfrontationCurves(teeth, startEndPositions, hasLingualConfrontations, tmpCurves);
			curve.insert(curve.end(), tmpCurves[0].begin(), tmpCurves[0].end());
		}
		else if (hasLingualConfrontation == 0) {
			vector<Point> tmpCurve;
			computeStringCurve(teeth, startEndPositions, -1.5F, true, false, tmpCurve);
			for (auto i = 0; i < 2; ++i)
				if (startEndPositions[i] < positions[i])
					tmpCurve[i ? tmpCurve.size() - 1 : 0] = getTooth(teeth, ++Rpd::Position(startEndPositions[i])).getAnglePoint(0);
			computeSmoothCurve(tmpCurve, tmpCurve);
			curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
		}
		curve.insert(curve.end(), curves[1].begin(), curves[1].end());
	}
}

void computeOuterCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<Point>& curve, float* const& avgRadius) {
	vector<Point> dbCurve1, dbCurve2;
	float thisAvgRadius;
	computeStringCurve(teeth, positions, -2.25F, true, false, curve, &thisAvgRadius);
	if (avgRadius)
		*avgRadius = thisAvgRadius;
	auto dbPosition = positions[0];
	auto isInSameZone = positions[0].zone == positions[1].zone;
	if (!isInSameZone)
		++dbPosition;
	else if (positions[0].ordinal == 0)
		dbPosition.zone += 1 - positions[0].zone % 2 * 2;
	else
		--dbPosition;
	float sumOfRadii = 0;
	auto nTeeth = 0;
	auto position = dbPosition;
	if (isInSameZone && positions[0].ordinal > 0)
		while (position.ordinal >= 0) {
			auto& tooth = getTooth(teeth, position);
			if (isDentureBase(tooth.getLingualBlockage())) {
				sumOfRadii += tooth.getRadius();
				++nTeeth;
				--position;
			}
			else
				break;
		}
	auto flag = position.ordinal < 0;
	if (flag) {
		position.zone += 1 - positions[0].zone % 2 * 2;
		position.ordinal = 0;
	}
	if (!isInSameZone || positions[0].ordinal == 0 || flag)
		while (position.ordinal < nTeethPerZone) {
			auto& tooth = getTooth(teeth, position);
			if (isDentureBase(tooth.getLingualBlockage())) {
				sumOfRadii += tooth.getRadius();
				++nTeeth;
				++position;
			}
			else
				break;
		}
	if (nTeeth > 0) {
		thisAvgRadius = sumOfRadii / nTeeth;
		auto& tooth = getTooth(teeth, dbPosition);
		dbCurve1 = {tooth.getAnglePoint(!isInSameZone || positions[0].ordinal == 0 ? 0 : 180), tooth.getCentroid()};
		auto tmpPoint = dbCurve1[0];
		dbCurve1.insert(dbCurve1.begin(), tmpPoint);
		for (auto i = 1; i < 3; ++i)
			dbCurve1[i] -= roundToPoint(computeNormalDirection(dbCurve1[i]) * thisAvgRadius * 1.6F);
		computeInscribedCurve(dbCurve1, dbCurve1, 1, false);
		dbCurve1.insert(dbCurve1.begin(), tmpPoint);
		curve[0] = dbCurve1.back();
		curve.erase(curve.begin() + 1);
	}
	dbPosition = ++Rpd::Position(positions[1]);
	sumOfRadii = nTeeth = 0;
	position = dbPosition;
	while (position.ordinal < nTeethPerZone) {
		auto& tooth = getTooth(teeth, position);
		if (isDentureBase(tooth.getLingualBlockage())) {
			sumOfRadii += tooth.getRadius();
			++nTeeth;
			++position;
		}
		else
			break;
	}
	if (nTeeth > 0) {
		thisAvgRadius = sumOfRadii / nTeeth;
		auto& tooth = getTooth(teeth, dbPosition);
		dbCurve2 = {tooth.getCentroid(), tooth.getAnglePoint(0)};
		auto tmpPoint = dbCurve2.back();
		dbCurve2.push_back(tmpPoint);
		for (auto i = 0; i < 2; ++i)
			dbCurve2[i] -= roundToPoint(computeNormalDirection(dbCurve2[i]) * thisAvgRadius * 1.6F);
		computeInscribedCurve(dbCurve2, dbCurve2, 1, false);
		dbCurve2.push_back(tmpPoint);
		curve.back() = dbCurve2[0];
		curve.erase(curve.end() - 2);
	}
	computeSmoothCurve(curve, curve);
	curve.insert(curve.begin(), dbCurve1.begin(), dbCurve1.end());
	curve.insert(curve.end(), dbCurve2.begin(), dbCurve2.end());
}

void computeLingualConfrontationCurves(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const bool hasLingualConfrontations[nZones][nTeethPerZone], vector<vector<Point>>& curves) {
	if (positions.size() == 4) {
		computeLingualConfrontationCurves(teeth, {positions[0], positions[1]}, hasLingualConfrontations, curves);
		computeLingualConfrontationCurves(teeth, {positions[2], positions[3]}, hasLingualConfrontations, curves);
	}
	else if (positions[0].zone == positions[1].zone) {
		auto& zone = positions[0].zone;
		auto& endOrdinal = positions[1].ordinal;
		auto curOrdinal = positions[0].ordinal - 1;
		while (curOrdinal < endOrdinal) {
			while (curOrdinal < endOrdinal && !hasLingualConfrontations[zone][curOrdinal + 1])
				++curOrdinal;
			if (curOrdinal == endOrdinal)
				break;
			auto startOrdinal = curOrdinal + 1;
			while (curOrdinal < endOrdinal && hasLingualConfrontations[zone][curOrdinal + 1])
				++curOrdinal;
			vector<Point> curve;
			for (auto position = Rpd::Position(zone, startOrdinal); position <= Rpd::Position(zone, curOrdinal); ++position) {
				auto thisCurve = getTooth(teeth, position).getCurve(180, 0);
				curve.insert(curve.end(), thisCurve.rbegin(), thisCurve.rend());
			}
			curves.push_back(curve);
		}
	}
	else {
		vector<vector<Point>> tmpCurves;
		for (auto i = 0; i < 2; ++i) {
			auto& zone = positions[i].zone;
			auto curOrdinal = -1;
			while (curOrdinal < positions[i].ordinal && hasLingualConfrontations[zone][curOrdinal + 1])
				++curOrdinal;
			computeLingualConfrontationCurves(teeth, {Rpd::Position(zone, 0), Rpd::Position(zone, curOrdinal++)}, hasLingualConfrontations, tmpCurves);
			computeLingualConfrontationCurves(teeth, {Rpd::Position(zone, curOrdinal), positions[i]}, hasLingualConfrontations, curves);
		}
		if (tmpCurves.size()) {
			if (tmpCurves.size() == 2) {
				reverse(tmpCurves[0].begin(), tmpCurves[0].end());
				tmpCurves[0].insert(tmpCurves[0].end(), tmpCurves[1].begin(), tmpCurves[1].end());
			}
			curves.push_back(tmpCurves[0]);
		}
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

vector<RpdAsLingualBlockage::LingualBlockage> tipDirectionsToLingualBlockages(const vector<Rpd::Direction>& tipDirections) {
	vector<RpdAsLingualBlockage::LingualBlockage> lingualBlockages;
	for (auto tipDirection = tipDirections.begin(); tipDirection < tipDirections.end(); ++tipDirection)
		lingualBlockages.push_back(*tipDirection == Rpd::MESIAL ? RpdAsLingualBlockage::ARMED_DISTAL_REST : RpdAsLingualBlockage::ARMED_MESIAL_REST);
	return lingualBlockages;
}
