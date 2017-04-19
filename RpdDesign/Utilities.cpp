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

const Tooth& getTooth(const vector<Tooth> teeth[nZones], const Rpd::Position& position) { return teeth[position.zone][position.ordinal]; }

Tooth& getTooth(vector<Tooth> teeth[nZones], const Rpd::Position& position) { return const_cast<Tooth&>(getTooth(const_cast<const vector<Tooth>*>(teeth), position)); }

bool isBlockedByMajorConnector(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions) {
	if (positions[0].zone == positions[1].zone) {
		for (auto position = positions[0]; position <= positions[1]; ++position)
			if (getTooth(teeth, position).hasMajorConnector())
				return true;
		return false;
	}
	return isBlockedByMajorConnector(teeth, {Rpd::Position(positions[0].zone, 0), positions[0]}) || isBlockedByMajorConnector(teeth, {Rpd::Position(positions[1].zone, 0), positions[1]});
}

void computeStringCurves(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const vector<float>& distanceScales, const deque<bool>& keepStartEndPoints, const deque<bool>& considerAnchorDisplacements, const bool& considerDistalPoints, vector<vector<Point>>& curves, float* const& sumOfRadii, int* const& nTeeth, vector<Point>* const& distalPoints) {
	auto thisNTeeth = 0;
	if (nTeeth)
		thisNTeeth = *nTeeth;
	float thisSumOfRadii = 0;
	if (sumOfRadii)
		thisSumOfRadii = *sumOfRadii;
	if (distalPoints)
		*distalPoints = vector<Point>(2);
	vector<Point> curve;
	if (positions[0].zone == positions[1].zone) {
		Point lastPoint;
		for (auto position = positions[0]; position <= positions[1]; ++position) {
			++thisNTeeth;
			auto& tooth = getTooth(teeth, position);
			thisSumOfRadii += tooth.getRadius();
			auto& thisPoint = tooth.getAnglePoint(0);
			curve.push_back(position == positions[0] ? thisPoint : (thisPoint + lastPoint) / 2);
			curve.push_back(tooth.getCentroid());
			lastPoint = tooth.getAnglePoint(180);
		}
		curve.push_back(lastPoint);
		if (considerAnchorDisplacements[0]) {
			auto position = --Rpd::Position(positions[0]);
			auto& tooth = getTooth(teeth, position);
			if (positions[0].ordinal) {
				if (tooth.expectMajorConnectorAnchor(Rpd::MESIAL) && !shouldAnchor(teeth, position, Rpd::MESIAL))
					curve[0] = tooth.getAnglePoint(180);
			}
			else if (tooth.expectMajorConnectorAnchor(Rpd::DISTAL) && !shouldAnchor(teeth, position, Rpd::DISTAL))
				curve[0] = tooth.getAnglePoint(0);
		}
		if (considerAnchorDisplacements[1] && !isLastTooth(positions[1])) {
			auto position = ++Rpd::Position(positions[1]);
			auto& tooth = getTooth(teeth, position);
			if (tooth.expectMajorConnectorAnchor(Rpd::DISTAL) && !shouldAnchor(teeth, position, Rpd::DISTAL))
				curve.back() = tooth.getAnglePoint(0);
		}
	}
	else {
		vector<vector<Point>> tmpCurves(2);
		for (auto i = 0; i < 2; ++i)
			computeStringCurve(teeth, {Rpd::Position(positions[i].zone, 0), positions[i]}, 0, {false, false}, {false, considerAnchorDisplacements[i]}, false, tmpCurves[i], &thisSumOfRadii, &thisNTeeth);
		tmpCurves[0][0] = (tmpCurves[0][0] + tmpCurves[1][0]) / 2;
		tmpCurves[1].erase(tmpCurves[1].begin());
		tmpCurves[1].insert(tmpCurves[1].begin(), tmpCurves[0].rbegin(), tmpCurves[0].rend());
		curve = tmpCurves[1];
	}
	if (nTeeth)
		*nTeeth = thisNTeeth;
	if (sumOfRadii)
		*sumOfRadii = thisSumOfRadii;
	auto thisAvgRadius = thisSumOfRadii / thisNTeeth;
	if (considerDistalPoints) {
		if (positions[0].zone != positions[1].zone && isLastTooth(positions[0])) {
			auto tmpPoint = getTooth(teeth, positions[0]).getAnglePoint(180);
			curve.insert(curve.begin(), tmpPoint + roundToPoint(rotate(computeNormalDirection(tmpPoint), -CV_PI / 2) * thisAvgRadius * 0.6F));
			if (distalPoints)
				(*distalPoints)[0] = curve[0];
		}
		if (isLastTooth(positions[1])) {
			auto tmpPoint = getTooth(teeth, positions[1]).getAnglePoint(180);
			curve.push_back(tmpPoint + roundToPoint(rotate(computeNormalDirection(tmpPoint), CV_PI * (positions[1].zone % 2 - 0.5)) * thisAvgRadius * 0.6F));
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
		if (i == 0 && keepStartEndPoints[0]) {
			for (auto j = 0; j < distanceScales.size(); ++j)
				curves[j].insert(curves[j].begin(), curve[0]);
			curve.insert(curve.begin(), curve[0]);
			++i;
		}
		else if (i == curve.size() - 1 && keepStartEndPoints[1]) {
			for (auto j = 0; j < distanceScales.size(); ++j)
				curves[j].push_back(curve.back());
			curve.push_back(curve.back());
			++i;
		}
	}
}

void computeStringCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const float& distanceScale, const deque<bool>& keepStartEndPoints, const deque<bool>& considerAnchorDisplacements, const bool& considerDistalPoints, vector<Point>& curve, float* const& sumOfRadii, int* const& nTeeth, vector<Point>* const& distalPoints) {
	vector<vector<Point>> tmpCurves;
	computeStringCurves(teeth, positions, {distanceScale}, keepStartEndPoints, considerAnchorDisplacements, considerDistalPoints, tmpCurves, sumOfRadii, nTeeth, distalPoints);
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

void findAnchorPoints(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const bool& shouldFindAnchors, const vector<Point>& anchorPoints, vector<Rpd::Position>& startEndPositions, vector<Point>* const& thisAnchorPoints) {
	startEndPositions = positions;
	vector<Point> tmpAnchorPoints;
	if (shouldFindAnchors) {
		tmpAnchorPoints = vector<Point>(2);
		if (startEndPositions[0].zone == startEndPositions[1].zone) {
			if (!shouldAnchor(teeth, startEndPositions[0], Rpd::MESIAL))
				tmpAnchorPoints[0] = getTooth(teeth, startEndPositions[0]++).getAnglePoint(180);
		}
		else if (!shouldAnchor(teeth, startEndPositions[0], Rpd::DISTAL))
			tmpAnchorPoints[0] = getTooth(teeth, startEndPositions[0]--).getAnglePoint(0);
		if (!shouldAnchor(teeth, startEndPositions[1], Rpd::DISTAL))
			tmpAnchorPoints[1] = getTooth(teeth, startEndPositions[1]--).getAnglePoint(0);
	}
	else
		tmpAnchorPoints = anchorPoints;
	if (thisAnchorPoints)
		*thisAnchorPoints = tmpAnchorPoints;
}

void computeLingualCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const bool& shouldFindAnchors, vector<Point>& curve, vector<vector<Point>>& curves, const vector<Point>& anchorPoints, vector<Point>* const& distalPoints) {
	curve.clear();
	vector<Rpd::Position> startEndPositions;
	vector<Point> thisAnchorPoints;
	findAnchorPoints(teeth, positions, shouldFindAnchors, anchorPoints, startEndPositions, &thisAnchorPoints);
	if (startEndPositions[0].zone == startEndPositions[1].zone) {
		auto dbStartPosition = startEndPositions[0];
		while (dbStartPosition <= startEndPositions[1] && !getTooth(teeth, dbStartPosition).hasDentureBase(DentureBase::DOUBLE))
			++dbStartPosition;
		auto considerLast = false;
		for (auto position = startEndPositions[0]; position <= dbStartPosition; ++position) {
			auto isValidPosition = position < dbStartPosition, hasMesialLingualCoverage = false, hasDistalLingualCoverage = false;
			if (isValidPosition) {
				auto& tooth = getTooth(teeth, position);
				hasMesialLingualCoverage = tooth.hasLingualCoverage(Rpd::MESIAL);
				hasDistalLingualCoverage = tooth.hasLingualCoverage(Rpd::DISTAL);
			}
			vector<Point> thisCurve;
			if (considerLast || hasDistalLingualCoverage) {
				auto lastPosition = --Rpd::Position(position);
				computeStringCurve(teeth, {considerLast ? lastPosition : position, hasDistalLingualCoverage ? position : lastPosition}, -1.5F, {true, true}, {false, false}, false, thisCurve);
				computePiecewiseSmoothCurve(thisCurve, thisCurve);
				curves.push_back(thisCurve);
				curve.insert(curve.end(), thisCurve.begin(), thisCurve.end());
			}
			if (isValidPosition && !hasMesialLingualCoverage && !hasDistalLingualCoverage) {
				thisCurve = getTooth(teeth, position).getCurve(180, 0);
				curve.insert(curve.end(), thisCurve.rbegin(), thisCurve.rend());
			}
			considerLast = hasMesialLingualCoverage;
		}
		if (thisAnchorPoints[0] != Point())
			curve.insert(curve.begin(), thisAnchorPoints[0]);
		if (thisAnchorPoints[1] != Point())
			curve.push_back(thisAnchorPoints[1]);
		else if (dbStartPosition <= startEndPositions[1]) {
			vector<Point> dbCurve;
			computeStringCurve(teeth, {dbStartPosition, startEndPositions[1]}, -1.6F, {true, true}, {true, true}, true, dbCurve, nullptr, nullptr, distalPoints);
			computePiecewiseSmoothCurve(dbCurve, dbCurve);
			curve.insert(curve.end(), dbCurve.begin(), dbCurve.end());
		}
	}
	else {
		vector<int> zones = {startEndPositions[0].zone, startEndPositions[1].zone};
		vector<Rpd::Position> startPositions = {Rpd::Position(zones[0], 0), Rpd::Position(zones[1], 0)};
		vector<Tooth> startTeeth = {getTooth(teeth, startPositions[0]), getTooth(teeth, startPositions[1])};
		vector<vector<Point>> tmpCurves(2);
		vector<Point> tmpDistalPoints(2);
		if (distalPoints)
			*distalPoints = vector<Point>(2);
		if (startTeeth[0].hasDentureBase(DentureBase::DOUBLE) && startTeeth[1].hasDentureBase(DentureBase::DOUBLE)) {
			auto dbPositions = startPositions;
			for (auto i = 0; i < 2; ++i) {
				while (getTooth(teeth, ++Rpd::Position(dbPositions[i])).hasDentureBase(DentureBase::DOUBLE))
					++dbPositions[i];
				computeLingualCurve(teeth, {++Rpd::Position(dbPositions[i]), startEndPositions[i]}, false, tmpCurves[i], curves, {Point(), thisAnchorPoints[i]}, &tmpDistalPoints);
				if (distalPoints && tmpDistalPoints[1] != Point())
					(*distalPoints)[i] = tmpDistalPoints[1];
			}
			computeStringCurve(teeth, {dbPositions[0], dbPositions[1]}, -1.6F, {true, true}, {true, true}, true, curve, nullptr, nullptr, &tmpDistalPoints);
			if (distalPoints)
				for (auto i = 0; i < 2; ++i)
					if (tmpDistalPoints[i] != Point())
						(*distalPoints)[i] = tmpDistalPoints[i];
			computePiecewiseSmoothCurve(curve, curve);
			curve.insert(curve.begin(), tmpCurves[0].rbegin(), tmpCurves[0].rend());
			curve.insert(curve.end(), tmpCurves[1].begin(), tmpCurves[1].end());
		}
		else if (startTeeth[0].hasLingualCoverage(Rpd::DISTAL) && startTeeth[1].hasLingualCoverage(Rpd::DISTAL)) {
			for (auto i = 0; i < 2; ++i) {
				computeLingualCurve(teeth, {Rpd::Position(zones[i], 1), startEndPositions[i]}, false, tmpCurves[i], curves, {Point(), thisAnchorPoints[i]}, &tmpDistalPoints);
				if (distalPoints && tmpDistalPoints[1] != Point())
					(*distalPoints)[i] = tmpDistalPoints[1];
			}
			computeStringCurve(teeth, {startPositions[0], startPositions[1]}, -1.5F, {true, true}, {false, false}, false, curve);
			computePiecewiseSmoothCurve(curve, curve);
			curves.push_back(curve);
			curve.insert(curve.begin(), tmpCurves[0].rbegin(), tmpCurves[0].rend());
			curve.insert(curve.end(), tmpCurves[1].begin(), tmpCurves[1].end());
		}
		else {
			for (auto i = 0; i < 2; ++i) {
				computeLingualCurve(teeth, {startPositions[i], startEndPositions[i]}, false, tmpCurves[i], curves, {Point(), thisAnchorPoints[i]}, &tmpDistalPoints);
				if (distalPoints && tmpDistalPoints[1] != Point())
					(*distalPoints)[i] = tmpDistalPoints[1];
			}
			curve.insert(curve.end(), tmpCurves[0].rbegin(), tmpCurves[0].rend());
			curve.insert(curve.end(), tmpCurves[1].begin(), tmpCurves[1].end());
		}
	}
}

void computeLingualCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const bool& shouldFindAnchors, vector<Point>& curve, vector<vector<Point>>& curves, const vector<Point>& anchorPoints, Point& distalPoint) {
	vector<Point> distalPoints;
	computeLingualCurve(teeth, positions, shouldFindAnchors, curve, curves, anchorPoints, &distalPoints);
	if (distalPoints.size())
		distalPoint = distalPoints[1];
}

void computeMesialCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<Point>& curve, vector<Point>* const& innerCurve) {
	auto startPositions = positions;
	for (auto i = 0; i < 2; ++i)
		if (!shouldAnchor(teeth, startPositions[i], Rpd::MESIAL))
			++startPositions[i];
	auto& ordinal = max(startPositions[0].ordinal, startPositions[1].ordinal);
	vector<vector<Point>> curves(2);
	float sumOfRadii = 0;
	auto nTeeth = 0;
	for (auto i = 0; i < 2; ++i)
		computeStringCurve(teeth, {startPositions[i], Rpd::Position(positions[i].zone, ordinal)}, 0, {true, false}, {true, false}, false, curves[i], &sumOfRadii, &nTeeth);
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
	auto endPositions = positions;
	for (auto i = 0; i < 2; ++i)
		if (!shouldAnchor(teeth, endPositions[i], Rpd::DISTAL))
			--endPositions[i];
	auto& ordinal = min(endPositions[0].ordinal, endPositions[1].ordinal);
	vector<vector<Point>> curves(2);
	float sumOfRadii = 0;
	auto nTeeth = 0;
	for (auto i = 0; i < 2; ++i) {
		computeStringCurve(teeth, {Rpd::Position(positions[i].zone, ordinal), endPositions[i]}, 0, {false, false}, {false, true}, false, curves[i], &sumOfRadii, &nTeeth);
		if (distalPoints[i] != Point())
			curves[i].back() = distalPoints[i];
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

void computeInnerCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, const bool& shouldFindAnchors, const float& avgRadius, vector<Point>& curve, vector<vector<Point>>& curves, const vector<Point>& anchorPoints) {
	vector<Rpd::Position> startEndPositions;
	vector<Point> thisAnchorPoints;
	findAnchorPoints(teeth, positions, shouldFindAnchors, anchorPoints, startEndPositions, &thisAnchorPoints);
	if (startEndPositions[0].zone == startEndPositions[1].zone) {
		auto& zone = startEndPositions[0].zone;
		auto& startOrdinal = startEndPositions[0].ordinal;
		auto& endOrdinal = startEndPositions[1].ordinal;
		auto curOrdinal = startOrdinal - 1;
		while (curOrdinal < endOrdinal) {
			auto thisStartOrdinal = ++curOrdinal;
			auto& tooth = getTooth(teeth, Rpd::Position(zone, curOrdinal));
			auto hasLingualConfrontation = tooth.hasLingualConfrontation();
			auto hasSingleDb = tooth.hasDentureBase(DentureBase::SINGLE);
			while (curOrdinal < endOrdinal) {
				auto& thisTooth = getTooth(teeth, Rpd::Position(zone, curOrdinal + 1));
				auto& thisHasLingualConfrontation = thisTooth.hasLingualConfrontation();
				auto& thisHasSingleDb = thisTooth.hasDentureBase(DentureBase::SINGLE);
				if (!hasLingualConfrontation && !hasSingleDb && !thisHasLingualConfrontation && !thisHasSingleDb || hasSingleDb && thisHasSingleDb || hasLingualConfrontation && thisHasLingualConfrontation)
					++curOrdinal;
				else
					break;
			}
			auto isStart = thisStartOrdinal == startOrdinal && shouldFindAnchors, isEnd = curOrdinal == endOrdinal;
			if (hasLingualConfrontation) {
				vector<vector<Point>> tmpCurves;
				computeLingualConfrontationCurves(teeth, {Rpd::Position(zone, thisStartOrdinal), Rpd::Position(zone, curOrdinal)}, tmpCurves);
				if (!isStart) {
					auto& lastTooth = getTooth(teeth, --Rpd::Position(zone, thisStartOrdinal));
					if (lastTooth.hasDentureBase(DentureBase::SINGLE))
						tmpCurves[0].insert(tmpCurves[0].begin(), lastTooth.getAnglePoint(thisStartOrdinal ? 180 : 0));
				}
				else if (thisAnchorPoints[0] != Point())
					tmpCurves[0].insert(tmpCurves[0].begin(), thisAnchorPoints[0]);
				if (!isEnd) {
					auto& nextTooth = getTooth(teeth, Rpd::Position(zone, curOrdinal + 1));
					if (nextTooth.hasDentureBase(DentureBase::SINGLE))
						tmpCurves[0].push_back(nextTooth.getAnglePoint(0));
				}
				else if (thisAnchorPoints[1] != Point())
					tmpCurves[0].push_back(thisAnchorPoints[1]);
				curve.insert(curve.end(), tmpCurves[0].begin(), tmpCurves[0].end());
				curves.push_back(tmpCurves[0]);
			}
			else if (hasSingleDb) {
				vector<Point> tmpCurve;
				computeLingualConfrontationCurve(teeth, {Rpd::Position(zone, thisStartOrdinal), Rpd::Position(zone, curOrdinal)}, tmpCurve);
				if (isStart && thisAnchorPoints[0] != Point())
					tmpCurve.insert(tmpCurve.begin(), thisAnchorPoints[0]);
				if (isEnd && thisAnchorPoints[1] != Point())
					tmpCurve.push_back(thisAnchorPoints[1]);
				curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
			}
			else {
				vector<Point> tmpCurve;
				computeStringCurve(teeth, {Rpd::Position(zone, thisStartOrdinal), Rpd::Position(zone, curOrdinal)}, -1.5F, {true, true}, {true, true}, false, tmpCurve);
				if (!isStart)
					tmpCurve[0] = getTooth(teeth, --Rpd::Position(zone, thisStartOrdinal)).getAnglePoint(thisStartOrdinal ? 180 : 0);
				if (!isEnd)
					tmpCurve.back() = getTooth(teeth, Rpd::Position(zone, curOrdinal + 1)).getAnglePoint(0);
				computePiecewiseSmoothCurve(tmpCurve, tmpCurve, isStart, isEnd);
				curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
				curves.push_back(tmpCurve);
			}
		}
	}
	else {
		vector<vector<Point>> thisCurves(2);
		auto startPositions = startEndPositions;
		auto hasLingualConfrontation = true, hasSingleDb = true, hasNone = true;
		deque<bool> isCoinciding(2);
		for (auto i = 0; i < 2; ++i) {
			auto& tooth = getTooth(teeth, Rpd::Position(startEndPositions[i].zone, 0));
			auto& thisHasLingualConfrontation = tooth.hasLingualConfrontation();
			auto& thisHasSingleDb = tooth.hasDentureBase(DentureBase::SINGLE);
			hasLingualConfrontation &= thisHasLingualConfrontation;
			hasSingleDb &= thisHasSingleDb;
			hasNone &= !thisHasLingualConfrontation && !thisHasSingleDb;
		}
		for (auto i = 0; i < 2; ++i) {
			startPositions[i].ordinal = 0;
			if (hasLingualConfrontation || hasSingleDb || hasNone) {
				while (startPositions[i] < startEndPositions[i]) {
					auto& thisTooth = getTooth(teeth, ++Rpd::Position(startPositions[i]));
					auto& thisHasLingualConfrontation = thisTooth.hasLingualConfrontation();
					auto& thisHasSingleDb = thisTooth.hasDentureBase(DentureBase::SINGLE);
					if (!hasLingualConfrontation && !hasSingleDb && !thisHasLingualConfrontation && !thisHasSingleDb || hasSingleDb && thisHasSingleDb || hasLingualConfrontation && thisHasLingualConfrontation)
						++startPositions[i];
					else
						break;
				}
				computeInnerCurve(teeth, {++Rpd::Position(startPositions[i]), startEndPositions[i]}, false, avgRadius, thisCurves[i], curves, {Point(), thisAnchorPoints[i]});
			}
			else
				computeInnerCurve(teeth, {startPositions[i], startEndPositions[i]}, false, avgRadius, thisCurves[i], curves, {Point(), thisAnchorPoints[i]});
			isCoinciding[i] = startPositions[i] == startEndPositions[i];
		}
		curve.insert(curve.end(), thisCurves[0].rbegin(), thisCurves[0].rend());
		vector<Point> tmpPoints(2), tmpCurve;
		if (hasLingualConfrontation) {
			vector<vector<Point>> tmpCurves;
			computeLingualConfrontationCurves(teeth, startPositions, tmpCurves);
			tmpCurve = tmpCurves[0];
			for (auto i = 0; i < 2; ++i)
				if (!isCoinciding[i]) {
					if (getTooth(teeth, ++Rpd::Position(startPositions[i])).hasDentureBase(DentureBase::SINGLE))
						tmpPoints[i] = thisCurves[i][0];
				}
				else if (thisAnchorPoints[i] != Point())
					tmpPoints[i] = thisAnchorPoints[i];
			if (tmpPoints[0] != Point())
				tmpCurve.insert(tmpCurve.begin(), tmpPoints[0]);
			if (tmpPoints[1] != Point())
				tmpCurve.push_back(tmpPoints[1]);
			curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
			curves.push_back(tmpCurve);
		}
		else if (hasSingleDb) {
			vector<vector<Point>> tmpCurves;
			computeLingualConfrontationCurves(teeth, startPositions, tmpCurves);
			tmpCurve = tmpCurves[0];
			for (auto i = 0; i < 2; ++i)
				if (isCoinciding[i] && thisAnchorPoints[i] != Point())
					tmpPoints[i] = thisAnchorPoints[i];
			if (tmpPoints[0] != Point())
				tmpCurve.insert(tmpCurve.begin(), tmpPoints[0]);
			if (tmpPoints[1] != Point())
				tmpCurve.push_back(tmpPoints[1]);
			curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
		}
		else if (hasNone) {
			computeStringCurve(teeth, startPositions, -1.5F, {true, true}, {true, true}, false, tmpCurve);
			for (auto i = 0; i < 2; ++i)
				if (!isCoinciding[i])
					tmpPoints[i] = getTooth(teeth, ++Rpd::Position(startPositions[i])).getAnglePoint(0);
			if (tmpPoints[0] != Point())
				tmpCurve[0] = tmpPoints[0];
			if (tmpPoints[1] != Point())
				tmpCurve.back() = tmpPoints[1];
			computePiecewiseSmoothCurve(tmpCurve, tmpCurve, isCoinciding[0], isCoinciding[1]);
			curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
			curves.push_back(tmpCurve);
		}
		curve.insert(curve.end(), thisCurves[1].begin(), thisCurves[1].end());
	}
}

void computeOuterCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<Point>& curve, float* const& avgRadius) {
	vector<Rpd::Position> startEndPositions;
	findAnchorPoints(teeth, positions, true, {}, startEndPositions);
	vector<Point> dbCurve1, dbCurve2;
	float sumOfRadii = 0;
	auto nTeeth = 0;
	computeStringCurve(teeth, startEndPositions, -2.25F, {true, true}, {true, true}, false, curve, &sumOfRadii, &nTeeth);
	auto thisAvgRadius = sumOfRadii / nTeeth;
	if (avgRadius)
		*avgRadius = thisAvgRadius;
	auto dbPosition = startEndPositions[0];
	auto isInSameZone = startEndPositions[0].zone == startEndPositions[1].zone;
	isInSameZone ? --dbPosition : ++dbPosition;
	sumOfRadii = nTeeth = 0;
	auto position = dbPosition;
	if (isInSameZone && startEndPositions[0].ordinal > 0)
		while (position.ordinal >= 0) {
			auto& tooth = getTooth(teeth, position);
			if (tooth.hasDentureBase(DentureBase::DOUBLE)) {
				sumOfRadii += tooth.getRadius();
				++nTeeth;
				--position.ordinal;
			}
			else
				break;
		}
	auto flag = position.ordinal < 0;
	if (flag)
		--++position;
	if (!isInSameZone || startEndPositions[0].ordinal == 0 || flag)
		while (!isLastTooth(--Rpd::Position(position))) {
			auto& tooth = getTooth(teeth, position);
			if (tooth.hasDentureBase(DentureBase::DOUBLE)) {
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
		dbCurve1 = {tooth.getAnglePoint(!isInSameZone || startEndPositions[0].ordinal == 0 ? 0 : 180), tooth.getCentroid()};
		auto tmpPoint = dbCurve1[0];
		dbCurve1.insert(dbCurve1.begin(), tmpPoint);
		for (auto i = 1; i < 3; ++i)
			dbCurve1[i] -= roundToPoint(computeNormalDirection(dbCurve1[i]) * thisAvgRadius * 1.6F);
		computeInscribedCurve(dbCurve1, dbCurve1, 1, false);
		dbCurve1.insert(dbCurve1.begin(), tmpPoint);
		if (avgRadius)
			dbCurve1.insert(dbCurve1.begin(), curve[0]);
		curve[0] = dbCurve1.back();
		curve.erase(curve.begin() + 1);
	}
	dbPosition = ++Rpd::Position(startEndPositions[1]);
	sumOfRadii = nTeeth = 0;
	position = dbPosition;
	while (!isLastTooth(--Rpd::Position(position))) {
		auto& tooth = getTooth(teeth, position);
		if (tooth.hasDentureBase(DentureBase::DOUBLE)) {
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
		if (avgRadius)
			dbCurve2.push_back(curve.back());
		curve.back() = dbCurve2[0];
		curve.erase(curve.end() - 2);
	}
	computeSmoothCurve(curve, curve);
	curve.insert(curve.begin(), dbCurve1.begin(), dbCurve1.end());
	curve.insert(curve.end(), dbCurve2.begin(), dbCurve2.end());
}

void computeLingualConfrontationCurve(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<Point>& curve) {
	if (positions[0].zone == positions[1].zone)
		for (auto position = positions[0]; position <= positions[1]; ++position) {
			auto thisCurve = getTooth(teeth, position).getCurve(180, 0);
			curve.insert(curve.end(), thisCurve.rbegin(), thisCurve.rend());
		}
	else {
		vector<vector<Point>> tmpCurves(2);
		for (auto i = 0; i < 2; ++i)
			computeLingualConfrontationCurve(teeth, {Rpd::Position(positions[i].zone, 0), positions[i]}, tmpCurves[i]);
		curve = tmpCurves[1];
		curve.insert(curve.begin(), tmpCurves[0].rbegin(), tmpCurves[0].rend());
	}
}

void computeLingualConfrontationCurves(const vector<Tooth> teeth[nZones], const vector<Rpd::Position>& positions, vector<vector<Point>>& curves) {
	if (positions.size() == 4) {
		computeLingualConfrontationCurves(teeth, {positions[0], positions[1]}, curves);
		computeLingualConfrontationCurves(teeth, {positions[2], positions[3]}, curves);
	}
	else if (positions[0].zone == positions[1].zone) {
		auto& zone = positions[0].zone;
		auto& endOrdinal = positions[1].ordinal;
		auto curOrdinal = positions[0].ordinal - 1;
		while (curOrdinal < endOrdinal) {
			while (curOrdinal < endOrdinal && !getTooth(teeth, Rpd::Position(zone, curOrdinal + 1)).hasLingualConfrontation())
				++curOrdinal;
			if (curOrdinal == endOrdinal)
				break;
			auto startOrdinal = ++curOrdinal;
			while (curOrdinal < endOrdinal && getTooth(teeth, Rpd::Position(zone, curOrdinal + 1)).hasLingualConfrontation())
				++curOrdinal;
			vector<Point> tmpCurve;
			computeLingualConfrontationCurve(teeth, {Rpd::Position(zone, startOrdinal) , Rpd::Position(zone, curOrdinal)}, tmpCurve);
			curves.push_back(tmpCurve);
		}
	}
	else {
		auto lcPositions = positions;
		for (auto i = 0; i < 2; ++i) {
			lcPositions[i].ordinal = -1;
			while (lcPositions[i] < positions[i] && getTooth(teeth, ++Rpd::Position(lcPositions[i])).hasLingualConfrontation())
				++lcPositions[i];
			computeLingualConfrontationCurves(teeth, {++lcPositions[i], positions[i]}, curves);
			--lcPositions[i];
		}
		if (lcPositions[0] < lcPositions[1]) {
			vector<Point> tmpCurve;
			computeLingualConfrontationCurve(teeth, lcPositions, tmpCurve);
			curves.push_back(tmpCurve);
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

bool shouldAnchor(const vector<Tooth> teeth[nZones], const Rpd::Position& position, const Rpd::Direction& direction) {
	auto& tooth = getTooth(teeth, position);
	if (tooth.hasClaspRootOrRest(direction) || tooth.expectDentureBaseAnchor(direction) || tooth.hasLingualConfrontation())
		return true;
	if (direction == Rpd::MESIAL)
		return getTooth(teeth, --Rpd::Position(position)).expectDentureBaseAnchor(position.ordinal ? Rpd::DISTAL : Rpd::MESIAL);
	return !isLastTooth(position) && getTooth(teeth, ++Rpd::Position(position)).expectDentureBaseAnchor(Rpd::MESIAL);
}

bool isLastTooth(const Rpd::Position& position) { return position.ordinal == nTeethPerZone + Tooth::isEighthUsed[position.zone] - 2; }
