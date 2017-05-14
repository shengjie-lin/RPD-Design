#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <opencv2/imgproc.hpp>

#include "Utilities.h"
#include "EllipticCurve.h"
#include "Tooth.h"

float degreeToRadian(float const& degree) { return degree / 180 * CV_PI; }

float radianToDegree(float const& radian) { return radian / CV_PI * 180; }

void catPath(string& path, string const& searchDirectory, string const& extension) {
	auto const& searchPattern = searchDirectory + extension;
	WIN32_FIND_DATA findData;
	auto const& hFind = FindFirstFile(searchPattern.c_str(), &findData);
	do
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			path.append(searchDirectory + findData.cFileName + ';');
	while (FindNextFile(hFind, &findData));
	FindClose(hFind);
}

string getClsSig(const char* const& clsStr) { return 'L' + string(clsStr) + ';'; }

Rpd::Direction operator~(Rpd::Direction const& direction) { return direction == Rpd::MESIAL ? Rpd::DISTAL : Rpd::MESIAL; }

Tooth const& getTooth(const vector<Tooth> (&teeth)[nZones], Rpd::Position const& position) { return teeth[position.zone][position.ordinal]; }

Tooth& getTooth(vector<Tooth> (&teeth)[nZones], Rpd::Position const& position) { return const_cast<Tooth&>(getTooth(const_cast<const vector<Tooth>(&)[nZones]>(teeth), position)); }

bool isBlockedByMajorConnector(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions) {
	if (positions[0].zone == positions[1].zone) {
		for (auto position = positions[0]; position <= positions[1]; ++position)
			if (getTooth(teeth, position).hasMajorConnector())
				return true;
		return false;
	}
	return isBlockedByMajorConnector(teeth, {Rpd::Position(positions[0].zone, 0), positions[0]}) || isBlockedByMajorConnector(teeth, {Rpd::Position(positions[1].zone, 0), positions[1]});
}

void computeStringCurves(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<float> const& distanceScales, deque<bool> const& keepStartEndPoints, deque<bool> const& considerAnchorDisplacements, bool const& considerDistalPoints, vector<vector<Point>>& curves, float* const& sumOfRadii, int* const& nTeeth, vector<Point>* const& distalPoints) {
	auto thisNTeeth = 0;
	if (nTeeth)
		thisNTeeth = *nTeeth;
	float thisSumOfRadii = 0;
	if (sumOfRadii)
		thisSumOfRadii = *sumOfRadii;
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
			auto const& position = --Rpd::Position(positions[0]);
			auto& tooth = getTooth(teeth, position);
			if (positions[0].ordinal) {
				if (tooth.expectMajorConnectorAnchor(Rpd::MESIAL) && !shouldAnchor(teeth, position, Rpd::MESIAL))
					curve[0] = tooth.getAnglePoint(180);
			}
			else if (tooth.expectMajorConnectorAnchor(Rpd::DISTAL) && !shouldAnchor(teeth, position, Rpd::DISTAL))
				curve[0] = tooth.getAnglePoint(0);
		}
		if (considerAnchorDisplacements[1] && !isLastTooth(positions[1])) {
			auto const& position = ++Rpd::Position(positions[1]);
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
			auto& tmpPoint = getTooth(teeth, positions[0]).getAnglePoint(180);
			curve.insert(curve.begin(), tmpPoint + roundToPoint(rotate(computeNormalDirection(tmpPoint), -CV_PI / 2) * thisAvgRadius * 0.6F));
			if (distalPoints)
				(*distalPoints)[0] = curve[0];
		}
		if (isLastTooth(positions[1])) {
			auto& tmpPoint = getTooth(teeth, positions[1]).getAnglePoint(180);
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

void computeStringCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, float const& distanceScale, deque<bool> const& keepStartEndPoints, deque<bool> const& considerAnchorDisplacements, bool const& considerDistalPoints, vector<Point>& curve, float* const& sumOfRadii, int* const& nTeeth, vector<Point>* const& distalPoints) {
	vector<vector<Point>> tmpCurves;
	computeStringCurves(teeth, positions, {distanceScale}, keepStartEndPoints, considerAnchorDisplacements, considerDistalPoints, tmpCurves, sumOfRadii, nTeeth, distalPoints);
	curve = tmpCurves[0];
}

void computeInscribedCurve(vector<Point> const& cornerPoints, vector<Point>& curve, float const& smoothness, bool const& shouldAppend) {
	Point2f const &v1 = cornerPoints[0] - cornerPoints[1], &v2 = cornerPoints[2] - cornerPoints[1];
	auto const &l1 = norm(v1), &l2 = norm(v2);
	auto const &d1 = v1 / l1, &d2 = v2 / l2;
	auto const& sinTheta = d1.cross(d2);
	auto theta = asin(abs(sinTheta));
	if (d1.dot(d2) < 0)
		theta = CV_PI - theta;
	auto const& radius = static_cast<float>(min({l1, l2}) * tan(theta / 2) * smoothness);
	vector<Point> thisCurve;
	auto const& d = sinTheta < 0 ? d1 : d2;
	auto const& isValidCurve = EllipticCurve(cornerPoints[1] + roundToPoint(normalize(d1 + d2) * radius / sin(theta / 2)), Size(radius, radius), radianToDegree(atan2(d.x, -d.y)), 180 - radianToDegree(theta), sinTheta > 0).getCurve(thisCurve);
	if (!shouldAppend)
		curve.clear();
	if (isValidCurve)
		curve.insert(curve.end(), thisCurve.begin(), thisCurve.end());
	else
		curve.push_back(cornerPoints[1]);
}

void computeSmoothCurve(vector<Point> const& curve, vector<Point>& smoothCurve, bool const& isClosed, float const& smoothness) {
	vector<Point> tmpCurve;
	for (auto point = curve.begin(); point < curve.end(); ++point) {
		auto const &isFirst = point == curve.begin(), &isLast = point == curve.end() - 1;
		if (isClosed || !(isFirst || isLast))
			computeInscribedCurve({isFirst ? curve.back() : *(point - 1), *point, isLast ? curve[0] : *(point + 1)}, tmpCurve, smoothness);
		else
			tmpCurve.insert(tmpCurve.end(), *point);
	}
	smoothCurve = tmpCurve;
}

void computePiecewiseSmoothCurve(vector<Point> const& curve, vector<Point>& piecewiseSmoothCurve, bool const& smoothStart, bool const& smoothEnd) {
	vector<vector<Point>> smoothCurves(3);
	smoothCurves[1] = vector<Point>{curve.begin() + 2, curve.end() - 2};
	if (smoothStart) {
		computeInscribedCurve(vector<Point>{curve.begin(), curve.begin() + 3}, smoothCurves[0], 1);
		smoothCurves[0].insert(smoothCurves[0].begin(), curve[0]);
		smoothCurves[1].insert(smoothCurves[1].begin(), smoothCurves[0].back());
	}
	else
		smoothCurves[1].insert(smoothCurves[1].begin(), curve.begin(), curve.begin() + 2);
	if (smoothEnd) {
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

void findAnchorPoints(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Rpd::Position>& startEndPositions, const vector<Point>* const& inAnchorPoints, vector<Point>* const& outAnchorPoints) {
	startEndPositions = positions;
	vector<Point> anchorPoints;
	if (inAnchorPoints)
		anchorPoints = *inAnchorPoints;
	else {
		anchorPoints = vector<Point>(2);
		if (startEndPositions[0].zone == startEndPositions[1].zone) {
			if (!shouldAnchor(teeth, startEndPositions[0], Rpd::MESIAL))
				anchorPoints[0] = getTooth(teeth, startEndPositions[0]++).getAnglePoint(180);
		}
		else if (!shouldAnchor(teeth, startEndPositions[0], Rpd::DISTAL))
			anchorPoints[0] = getTooth(teeth, startEndPositions[0]--).getAnglePoint(0);
		if (!shouldAnchor(teeth, startEndPositions[1], Rpd::DISTAL)) {
			auto const& shouldSwap = startEndPositions[1].ordinal == 0;
			anchorPoints[1] = getTooth(teeth, startEndPositions[1]--).getAnglePoint(0);
			if (shouldSwap) {
				swap(startEndPositions[0], startEndPositions[1]);
				swap(anchorPoints[0], anchorPoints[1]);
			}
		}
	}
	if (outAnchorPoints)
		*outAnchorPoints = anchorPoints;
}

void computeLingualCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point>& curve, vector<vector<Point>>& curves, vector<Point>* const& distalPoints, const vector<Point>* const& anchorPoints) {
	curve.clear();
	if (distalPoints)
		*distalPoints = vector<Point>(2);
	vector<Rpd::Position> startEndPositions;
	vector<Point> thisAnchorPoints;
	findAnchorPoints(teeth, positions, startEndPositions, anchorPoints, &thisAnchorPoints);
	if (startEndPositions[0].zone == startEndPositions[1].zone) {
		auto dbStartPosition = startEndPositions[0];
		while (dbStartPosition <= startEndPositions[1] && !getTooth(teeth, dbStartPosition).hasDentureBase(DentureBase::DOUBLE))
			++dbStartPosition;
		auto considerLast = false;
		for (auto position = startEndPositions[0]; position <= dbStartPosition; ++position) {
			auto const& isValidPosition = position < dbStartPosition;
			auto hasMesialLingualCoverage = false, hasDistalLingualCoverage = false;
			if (isValidPosition) {
				auto& tooth = getTooth(teeth, position);
				hasMesialLingualCoverage = tooth.hasLingualCoverage(Rpd::MESIAL);
				hasDistalLingualCoverage = tooth.hasLingualCoverage(Rpd::DISTAL);
			}
			vector<Point> thisCurve;
			if (considerLast || hasDistalLingualCoverage) {
				auto const& lastPosition = --Rpd::Position(position);
				computeStringCurve(teeth, {considerLast ? lastPosition : position, hasDistalLingualCoverage ? position : lastPosition}, -distanceScales[BYPASS], {true, true}, {false, false}, false, thisCurve);
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
			computeStringCurve(teeth, {dbStartPosition, startEndPositions[1]}, -distanceScales[DENTURE_BASE_CURVE], {true, true}, {true, true}, true, dbCurve, nullptr, nullptr, distalPoints);
			computePiecewiseSmoothCurve(dbCurve, dbCurve);
			curve.insert(curve.end(), dbCurve.begin(), dbCurve.end());
		}
	}
	else {
		vector<int> const& zones{startEndPositions[0].zone, startEndPositions[1].zone};
		vector<Rpd::Position> const& startPositions{Rpd::Position(zones[0], 0), Rpd::Position(zones[1], 0)};
		vector<Tooth> const& startTeeth{getTooth(teeth, startPositions[0]), getTooth(teeth, startPositions[1])};
		vector<vector<Point>> tmpCurves(2);
		vector<Point> tmpDistalPoints(2);
		if (startTeeth[0].hasDentureBase(DentureBase::DOUBLE) && startTeeth[1].hasDentureBase(DentureBase::DOUBLE)) {
			auto dbPositions = startPositions;
			for (auto i = 0; i < 2; ++i) {
				while (getTooth(teeth, ++Rpd::Position(dbPositions[i])).hasDentureBase(DentureBase::DOUBLE))
					++dbPositions[i];
				computeLingualCurve(teeth, {++Rpd::Position(dbPositions[i]), startEndPositions[i]}, tmpCurves[i], curves, &tmpDistalPoints, new vector<Point>{Point(), thisAnchorPoints[i]});
				if (distalPoints && tmpDistalPoints[1] != Point())
					(*distalPoints)[i] = tmpDistalPoints[1];
			}
			computeStringCurve(teeth, {dbPositions[0], dbPositions[1]}, -distanceScales[DENTURE_BASE_CURVE], {true, true}, {true, true}, true, curve, nullptr, nullptr, &tmpDistalPoints);
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
				computeLingualCurve(teeth, {Rpd::Position(zones[i], 1), startEndPositions[i]}, tmpCurves[i], curves, &tmpDistalPoints, new vector<Point>{Point(), thisAnchorPoints[i]});
				if (distalPoints && tmpDistalPoints[1] != Point())
					(*distalPoints)[i] = tmpDistalPoints[1];
			}
			computeStringCurve(teeth, {startPositions[0], startPositions[1]}, -distanceScales[BYPASS], {true, true}, {false, false}, false, curve);
			computePiecewiseSmoothCurve(curve, curve);
			curves.push_back(curve);
			curve.insert(curve.begin(), tmpCurves[0].rbegin(), tmpCurves[0].rend());
			curve.insert(curve.end(), tmpCurves[1].begin(), tmpCurves[1].end());
		}
		else {
			for (auto i = 0; i < 2; ++i) {
				computeLingualCurve(teeth, {startPositions[i], startEndPositions[i]}, tmpCurves[i], curves, &tmpDistalPoints, new vector<Point>{Point(), thisAnchorPoints[i]});
				if (distalPoints && tmpDistalPoints[1] != Point())
					(*distalPoints)[i] = tmpDistalPoints[1];
			}
			curve.insert(curve.end(), tmpCurves[0].rbegin(), tmpCurves[0].rend());
			curve.insert(curve.end(), tmpCurves[1].begin(), tmpCurves[1].end());
		}
	}
}

void computeLingualCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point>& curve, vector<vector<Point>>& curves, Point& distalPoint, const vector<Point>* const& anchorPoints) {
	vector<Point> distalPoints;
	computeLingualCurve(teeth, positions, curve, curves, &distalPoints, anchorPoints);
	if (distalPoints.size())
		distalPoint = distalPoints[1];
}

void computeMesialCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point>& curve, vector<Point>* const& innerCurve) {
	auto startPositions = positions;
	for (auto i = 0; i < 2; ++i)
		if (!shouldAnchor(teeth, startPositions[i], Rpd::MESIAL))
			++startPositions[i];
	auto const& ordinal = max(startPositions[0].ordinal, startPositions[1].ordinal);
	vector<vector<Point>> curves(2);
	float sumOfRadii = 0;
	auto nTeeth = 0;
	for (auto i = 0; i < 2; ++i)
		computeStringCurve(teeth, {startPositions[i], Rpd::Position(positions[i].zone, ordinal)}, 0, {true, false}, {true, false}, false, curves[i], &sumOfRadii, &nTeeth);
	auto const& avgRadius = sumOfRadii / nTeeth;
	for (auto i = 0; i < 2; ++i)
		for (auto point = curves[i].begin() + 1; point < curves[i].end(); ++point)
			*point -= roundToPoint(computeNormalDirection(*point) * avgRadius * distanceScales[MESIAL_OR_DISTAL]);
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

void computeDistalCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point> const& distalPoints, vector<Point>& curve, vector<Point>* const& innerCurve) {
	auto endPositions = positions;
	for (auto i = 0; i < 2; ++i)
		if (!shouldAnchor(teeth, endPositions[i], Rpd::DISTAL))
			--endPositions[i];
	auto const& ordinal = min(endPositions[0].ordinal, endPositions[1].ordinal);
	vector<vector<Point>> curves(2);
	float sumOfRadii = 0;
	auto nTeeth = 0;
	for (auto i = 0; i < 2; ++i) {
		computeStringCurve(teeth, {Rpd::Position(positions[i].zone, ordinal), endPositions[i]}, 0, {false, false}, {false, true}, false, curves[i], &sumOfRadii, &nTeeth);
		if (distalPoints[i] != Point())
			curves[i].back() = distalPoints[i];
		curves[i].push_back(curves[i].back());
	}
	auto const& avgRadius = sumOfRadii / nTeeth;
	for (auto i = 0; i < 2; ++i)
		for (auto point = curves[i].begin(); point < curves[i].end() - 1; ++point)
			*point -= roundToPoint(computeNormalDirection(*point) * avgRadius * distanceScales[MESIAL_OR_DISTAL]);
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

void computeInnerCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, float const& avgRadius, vector<Point>& curve, vector<vector<Point>>& curves, const vector<Point>* const& anchorPoints) {
	vector<Rpd::Position> startEndPositions;
	vector<Point> thisAnchorPoints, tmpCurve;
	findAnchorPoints(teeth, positions, startEndPositions, anchorPoints, &thisAnchorPoints);
	if (startEndPositions[0].zone == startEndPositions[1].zone) {
		auto const& zone = startEndPositions[0].zone;
		auto const& startOrdinal = startEndPositions[0].ordinal;
		auto const& endOrdinal = startEndPositions[1].ordinal;
		auto curOrdinal = startOrdinal - 1;
		auto lastPosition = startEndPositions[0];
		auto lastAnchorPoint = thisAnchorPoints[0];
		while (curOrdinal <= endOrdinal) {
			++curOrdinal;
			auto const& isValidPosition = curOrdinal <= endOrdinal;
			auto const& isStart = lastPosition == startEndPositions[0] && !anchorPoints;
			auto isEnd = curOrdinal == endOrdinal, hasMesialClaspRootOrRest = false, hasDistalClaspRootOrRest = false, hasLingualConfrontation = false, hasSingleDb = false;
			auto thisPosition = Rpd::Position(zone, curOrdinal);
			Point thisAchorPoint;
			if (isValidPosition) {
				auto& tooth = getTooth(teeth, thisPosition);
				hasMesialClaspRootOrRest = tooth.hasClaspRootOrRest(Rpd::MESIAL);
				hasDistalClaspRootOrRest = tooth.hasClaspRootOrRest(Rpd::DISTAL) && !hasMesialClaspRootOrRest;
				hasLingualConfrontation = tooth.hasLingualConfrontation();
				hasSingleDb = tooth.hasDentureBase(DentureBase::SINGLE);
				if (!hasDistalClaspRootOrRest)
					thisAchorPoint = tooth.getAnglePoint(0);
			}
			else
				thisAchorPoint = thisAnchorPoints[1];
			if (hasMesialClaspRootOrRest || hasDistalClaspRootOrRest || hasLingualConfrontation || hasSingleDb && (curOrdinal != startOrdinal || anchorPoints) || !isValidPosition) {
				if (!isValidPosition || !hasDistalClaspRootOrRest) {
					--thisPosition;
					isEnd = !isValidPosition;
				}
				auto hasCurve = true;
				if (lastPosition.zone == thisPosition.zone && lastPosition.ordinal <= thisPosition.ordinal) {
					computeStringCurve(teeth, {lastPosition, thisPosition}, 0, {false, false}, {false, false}, false, tmpCurve);
					if (lastAnchorPoint != Point())
						tmpCurve[0] = lastAnchorPoint;
					tmpCurve.insert(tmpCurve.begin(), tmpCurve[0]);
					if (thisAchorPoint != Point())
						tmpCurve.back() = thisAchorPoint;
					tmpCurve.push_back(tmpCurve.back());
					for (auto i = 1; i < tmpCurve.size() - 1; ++i)
						tmpCurve[i] -= roundToPoint(computeNormalDirection(tmpCurve[i]) * avgRadius * distanceScales[INNER]);
					computePiecewiseSmoothCurve(tmpCurve, tmpCurve, isStart, isEnd);
				}
				else if (lastAnchorPoint != Point() && thisAchorPoint != Point())
					tmpCurve = {lastAnchorPoint, thisAchorPoint};
				else
					hasCurve = false;
				if (hasCurve) {
					curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
					curves.push_back(tmpCurve);
				}
			}
			if (isValidPosition) {
				if (hasLingualConfrontation || hasSingleDb || !hasMesialClaspRootOrRest && !hasDistalClaspRootOrRest) {
					auto thisStartOrdinal = curOrdinal;
					while (curOrdinal < endOrdinal) {
						auto& thisTooth = getTooth(teeth, Rpd::Position(zone, curOrdinal + 1));
						auto& thisHasLingualConfrontation = thisTooth.hasLingualConfrontation();
						auto& thisHasSingleDb = thisTooth.hasDentureBase(DentureBase::SINGLE);
						auto const& thisHasClaspRootOrRest = thisTooth.hasClaspRootOrRest(Rpd::MESIAL) || thisTooth.hasClaspRootOrRest(Rpd::DISTAL);
						if (!hasLingualConfrontation && !hasSingleDb && !thisHasLingualConfrontation && !thisHasSingleDb && !thisHasClaspRootOrRest || hasSingleDb && thisHasSingleDb || hasLingualConfrontation && thisHasLingualConfrontation)
							++curOrdinal;
						else
							break;
					}
					isEnd = curOrdinal == endOrdinal;
					if (hasLingualConfrontation) {
						computeLingualConfrontationCurve(teeth, {Rpd::Position(zone, thisStartOrdinal), Rpd::Position(zone, curOrdinal)}, tmpCurve);
						curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
						curves.push_back(tmpCurve);
					}
					else if (hasSingleDb) {
						computeLingualConfrontationCurve(teeth, {Rpd::Position(zone, thisStartOrdinal), Rpd::Position(zone, curOrdinal)}, tmpCurve);
						if (isStart && thisAnchorPoints[0] != Point())
							tmpCurve.insert(tmpCurve.begin(), thisAnchorPoints[0]);
						if (isEnd && thisAnchorPoints[1] != Point())
							tmpCurve.push_back(thisAnchorPoints[1]);
						curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
					}
				}
				if (hasMesialClaspRootOrRest || hasDistalClaspRootOrRest || hasLingualConfrontation || hasSingleDb) {
					lastPosition = Rpd::Position(zone, curOrdinal + (hasMesialClaspRootOrRest ? 0 : 1));
					lastAnchorPoint = getTooth(teeth, Rpd::Position(zone, curOrdinal)).getAnglePoint(hasMesialClaspRootOrRest ? 0 : 180);
				}
			}
		}
	}
	else {
		auto hasLingualConfrontation = true, hasSingleDb = true, hasNone = true;
		for (auto i = 0; i < 2; ++i) {
			auto& tooth = getTooth(teeth, Rpd::Position(startEndPositions[i].zone, 0));
			auto& thisHasLingualConfrontation = tooth.hasLingualConfrontation();
			auto& thisHasSingleDb = tooth.hasDentureBase(DentureBase::SINGLE);
			hasLingualConfrontation &= thisHasLingualConfrontation;
			hasSingleDb &= thisHasSingleDb;
			hasNone &= !thisHasLingualConfrontation && !thisHasSingleDb && !tooth.hasClaspRootOrRest(Rpd::MESIAL);
		}
		auto startPositions = startEndPositions;
		vector<vector<Point>> thisCurves(2);
		vector<Point> tmpPoints(2);
		for (auto i = 0; i < 2; ++i) {
			startPositions[i].ordinal = 0;
			if (hasLingualConfrontation || hasSingleDb || hasNone) {
				auto hasDistalClaspRootOrRest = getTooth(teeth, startPositions[i]).hasClaspRootOrRest(Rpd::DISTAL);
				while (startPositions[i] < startEndPositions[i]) {
					auto& thisTooth = getTooth(teeth, ++Rpd::Position(startPositions[i]));
					auto& thisHasLingualConfrontation = thisTooth.hasLingualConfrontation();
					auto& thisHasSingleDb = thisTooth.hasDentureBase(DentureBase::SINGLE);
					auto& thisHasMesialClaspRootOrRest = thisTooth.hasClaspRootOrRest(Rpd::MESIAL);
					if (hasNone && !hasDistalClaspRootOrRest && !thisHasLingualConfrontation && !thisHasSingleDb && !thisHasMesialClaspRootOrRest || hasSingleDb && thisHasSingleDb || hasLingualConfrontation && thisHasLingualConfrontation)
						hasDistalClaspRootOrRest = getTooth(teeth, ++startPositions[i]).hasClaspRootOrRest(Rpd::DISTAL);
					else
						break;
				}
				auto const& thisStartPosition = ++Rpd::Position(startPositions[i]);
				computeInnerCurve(teeth, {thisStartPosition, startEndPositions[i]}, avgRadius, thisCurves[i], curves, new vector<Point>{hasNone && !hasDistalClaspRootOrRest ? Point() : getTooth(teeth, startPositions[i]).getAnglePoint(180) , thisAnchorPoints[i]});
				if (hasNone && !hasDistalClaspRootOrRest)
					tmpPoints[i] = getTooth(teeth, thisStartPosition).getAnglePoint(0);
			}
			else {
				auto &tooth = getTooth(teeth, startPositions[i]), &nextTooth = getTooth(teeth, --Rpd::Position(startPositions[i]));
				computeInnerCurve(teeth, {startPositions[i], startEndPositions[i]}, avgRadius, thisCurves[i], curves, new vector<Point>{(nextTooth.hasLingualConfrontation() || nextTooth.hasDentureBase(DentureBase::SINGLE) || nextTooth.hasClaspRootOrRest(Rpd::MESIAL)) && (i == 0 || !(tooth.hasLingualConfrontation() || tooth.hasDentureBase(DentureBase::SINGLE) || tooth.hasClaspRootOrRest(Rpd::MESIAL))) ? nextTooth.getAnglePoint(0) : Point(), thisAnchorPoints[i]});
			}
		}
		curve.insert(curve.end(), thisCurves[0].rbegin(), thisCurves[0].rend());
		if (hasLingualConfrontation) {
			computeLingualConfrontationCurve(teeth, startPositions, tmpCurve);
			curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
			curves.push_back(tmpCurve);
		}
		else if (hasSingleDb) {
			computeLingualConfrontationCurve(teeth, startPositions, tmpCurve);
			curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
		}
		else if (hasNone) {
			computeStringCurve(teeth, startPositions, 0, {true, true}, {false, false}, false, tmpCurve);
			if (tmpPoints[0] != Point())
				tmpCurve[0] = tmpPoints[0];
			if (tmpPoints[1] != Point())
				tmpCurve.back() = tmpPoints[1];
			for (auto i = 1; i < tmpCurve.size() - 1; ++i)
				tmpCurve[i] -= roundToPoint(computeNormalDirection(tmpCurve[i]) * avgRadius * distanceScales[INNER]);
			deque<bool> isStartEnds(2);
			for (auto i = 0; i < 2; ++i)
				isStartEnds[i] = startPositions[i] == startEndPositions[i];
			computePiecewiseSmoothCurve(tmpCurve, tmpCurve, isStartEnds[0], isStartEnds[1]);
			curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
			curves.push_back(tmpCurve);
		}
		curve.insert(curve.end(), thisCurves[1].begin(), thisCurves[1].end());
	}
}

void computeOuterCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point>& curve, float* const& avgRadius) {
	vector<Rpd::Position> startEndPositions;
	findAnchorPoints(teeth, positions, startEndPositions);
	vector<Point> dbCurve1, dbCurve2;
	float sumOfRadii = 0;
	auto nTeeth = 0;
	computeStringCurve(teeth, startEndPositions, -distanceScales[OUTER], {true, true}, {true, true}, false, curve, &sumOfRadii, &nTeeth);
	auto thisAvgRadius = sumOfRadii / nTeeth;
	if (avgRadius)
		*avgRadius = thisAvgRadius;
	auto dbPosition = startEndPositions[0];
	auto const& isInSameZone = startEndPositions[0].zone == startEndPositions[1].zone;
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
	auto const& flag = position.ordinal < 0;
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
			dbCurve1[i] -= roundToPoint(computeNormalDirection(dbCurve1[i]) * thisAvgRadius * distanceScales[DENTURE_BASE_CURVE]);
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
			dbCurve2[i] -= roundToPoint(computeNormalDirection(dbCurve2[i]) * thisAvgRadius * distanceScales[DENTURE_BASE_CURVE]);
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

void computeLingualConfrontationCurve(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<Point>& curve) {
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

void computeLingualConfrontationCurves(const vector<Tooth> (&teeth)[nZones], vector<Rpd::Position> const& positions, vector<vector<Point>>& curves) {
	if (positions.size() == 4) {
		computeLingualConfrontationCurves(teeth, {positions[0], positions[1]}, curves);
		computeLingualConfrontationCurves(teeth, {positions[2], positions[3]}, curves);
	}
	else if (positions[0].zone == positions[1].zone) {
		auto const& zone = positions[0].zone;
		auto const& endOrdinal = positions[1].ordinal;
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

Point2f computeNormalDirection(Point2f const& point, float* const& angle) {
	auto const& curTeethEllipse = remedyImage ? remediedTeethEllipse : teethEllipse;
	auto const& direction = point - curTeethEllipse.center;
	auto thisAngle = atan2(direction.y, direction.x) - degreeToRadian(curTeethEllipse.angle);
	if (thisAngle < -CV_PI)
		thisAngle += CV_2PI;
	if (angle)
		*angle = thisAngle;
	auto const& normalDirection = Point2f(pow(curTeethEllipse.size.height, 2) * cos(thisAngle), pow(curTeethEllipse.size.width, 2) * sin(thisAngle));
	return normalDirection / norm(normalDirection);
}

bool shouldAnchor(const vector<Tooth> (&teeth)[nZones], Rpd::Position const& position, Rpd::Direction const& direction) {
	auto& tooth = getTooth(teeth, position);
	if (tooth.hasClaspRootOrRest(direction) || tooth.expectDentureBaseAnchor(direction) || tooth.hasLingualConfrontation())
		return true;
	if (direction == Rpd::MESIAL)
		return getTooth(teeth, --Rpd::Position(position)).expectDentureBaseAnchor(position.ordinal ? Rpd::DISTAL : Rpd::MESIAL);
	return !isLastTooth(position) && getTooth(teeth, ++Rpd::Position(position)).expectDentureBaseAnchor(Rpd::MESIAL);
}

bool isLastTooth(Rpd::Position const& position) { return position.ordinal == nTeethPerZone + Tooth::isEighthUsed[position.zone] - 2; }

bool queryRpds(JNIEnv* const& env, jobject const& ontModel, vector<Rpd*>& rpds) {
	auto const& clsStrExtendedIterator = "org/apache/jena/util/iterator/ExtendedIterator";
	auto const& clsStrIndividual = "org/apache/jena/ontology/Individual";
	auto const& clsStrIterator = "java/util/Iterator";
	auto const& clsStrModelCon = "org/apache/jena/rdf/model/ModelCon";
	auto const& clsStrObject = "java/lang/Object";
	auto const& clsStrOntClass = "org/apache/jena/ontology/OntClass";
	auto const& clsStrOntModel = "org/apache/jena/ontology/OntModel";
	auto const& clsStrProperty = "org/apache/jena/rdf/model/Property";
	auto const& clsStrResource = "org/apache/jena/rdf/model/Resource";
	auto const& clsStrStatement = "org/apache/jena/rdf/model/Statement";
	auto const& clsStrStmtIterator = "org/apache/jena/rdf/model/StmtIterator";
	auto const& clsStrString = "java/lang/String";
	auto const& clsIndividual = env->FindClass(clsStrIndividual);
	auto const& clsIterator = env->FindClass(clsStrIterator);
	auto const& clsModelCon = env->FindClass(clsStrModelCon);
	auto const& clsOntModel = env->FindClass(clsStrOntModel);
	auto const& clsResource = env->FindClass(clsStrResource);
	auto const& clsStatement = env->FindClass(clsStrStatement);
	auto const& midGetBoolean = env->GetMethodID(clsStatement, "getBoolean", "()Z");
	auto const& midGetInt = env->GetMethodID(clsStatement, "getInt", "()I");
	auto const& midGetLocalName = env->GetMethodID(clsResource, "getLocalName", ("()" + getClsSig(clsStrString)).c_str());
	auto const& midGetOntClass = env->GetMethodID(clsIndividual, "getOntClass", ("()" + getClsSig(clsStrOntClass)).c_str());
	auto const& midHasNext = env->GetMethodID(clsIterator, "hasNext", "()Z");
	auto const& midListIndividuals = env->GetMethodID(clsOntModel, "listIndividuals", ("()" + getClsSig(clsStrExtendedIterator)).c_str());
	auto const& midListProperties = env->GetMethodID(clsResource, "listProperties", ('(' + getClsSig(clsStrProperty) + ')' + getClsSig(clsStrStmtIterator)).c_str());
	auto const& midModelConGetProperty = env->GetMethodID(clsModelCon, "getProperty", ('(' + getClsSig(clsStrString) + ')' + getClsSig(clsStrProperty)).c_str());
	auto const& midNext = env->GetMethodID(clsIterator, "next", ("()" + getClsSig(clsStrObject)).c_str());
	auto const& midResourceGetProperty = env->GetMethodID(clsResource, "getProperty", ('(' + getClsSig(clsStrProperty) + ')' + getClsSig(clsStrStatement)).c_str());
	auto const& midStatementGetProperty = env->GetMethodID(clsStatement, "getProperty", ('(' + getClsSig(clsStrProperty) + ')' + getClsSig(clsStrStatement)).c_str());
	string const& ontPrefix = "http://www.semanticweb.org/msiip/ontologies/CDSSinRPD#";
	auto tmpStr = env->NewStringUTF((ontPrefix + "clasp_material").c_str());
	auto const& dpClaspMaterial = env->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
	env->DeleteLocalRef(tmpStr);
	tmpStr = env->NewStringUTF((ontPrefix + "clasp_tip_direction").c_str());
	auto const& dpClaspTipDirection = env->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
	env->DeleteLocalRef(tmpStr);
	tmpStr = env->NewStringUTF((ontPrefix + "clasp_tip_side").c_str());
	auto const& dpClaspTipSide = env->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
	env->DeleteLocalRef(tmpStr);
	tmpStr = env->NewStringUTF((ontPrefix + "component_position").c_str());
	auto opComponentPosition = env->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
	env->DeleteLocalRef(tmpStr);
	tmpStr = env->NewStringUTF((ontPrefix + "enable_buccal_arm").c_str());
	auto const& dpEnableBuccalArm = env->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
	env->DeleteLocalRef(tmpStr);
	tmpStr = env->NewStringUTF((ontPrefix + "enable_lingual_arm").c_str());
	auto const& dpEnableLingualArm = env->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
	env->DeleteLocalRef(tmpStr);
	tmpStr = env->NewStringUTF((ontPrefix + "enable_rest").c_str());
	auto const& dpEnableRest = env->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
	env->DeleteLocalRef(tmpStr);
	tmpStr = env->NewStringUTF((ontPrefix + "is_missing").c_str());
	auto const& dpIsMissing = env->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
	env->DeleteLocalRef(tmpStr);
	tmpStr = env->NewStringUTF((ontPrefix + "lingual_confrontation").c_str());
	auto const& dpLingualConfrontation = env->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
	env->DeleteLocalRef(tmpStr);
	tmpStr = env->NewStringUTF((ontPrefix + "rest_mesial_or_distal").c_str());
	auto const& dpRestMesialOrDistal = env->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
	env->DeleteLocalRef(tmpStr);
	tmpStr = env->NewStringUTF((ontPrefix + "tooth_ordinal").c_str());
	auto const& dpToothOrdinal = env->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
	env->DeleteLocalRef(tmpStr);
	tmpStr = env->NewStringUTF((ontPrefix + "tooth_zone").c_str());
	auto const& dpToothZone = env->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
	env->DeleteLocalRef(tmpStr);
	auto const& individuals = env->CallObjectMethod(ontModel, midListIndividuals);
	auto const& isValid = env->CallBooleanMethod(individuals, midHasNext);
	vector<Rpd*> thisRpds;
	bool thisIsEighthToothUsed[nZones] = {};
	while (env->CallBooleanMethod(individuals, midHasNext)) {
		auto const& individual = env->CallObjectMethod(individuals, midNext);
		auto const& ontClassStr = env->GetStringUTFChars(static_cast<jstring>(env->CallObjectMethod(env->CallObjectMethod(individual, midGetOntClass), midGetLocalName)), nullptr);
		auto const& tmpIt = rpdMapping_.find(ontClassStr);
		switch (tmpIt == rpdMapping_.end() ? -1 : tmpIt->second) {
			case AKERS_CLASP:
				thisRpds.push_back(AkersClasp::createFromIndividual(env, midGetBoolean, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspTipDirection, dpClaspMaterial, dpEnableBuccalArm, dpEnableLingualArm, dpEnableRest, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case CANINE_AKERS_CLASP:
				thisRpds.push_back(CanineAkersClasp::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspTipDirection, dpClaspMaterial, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case COMBINATION_ANTERIOR_POSTERIOR_PALATAL_STRAP:
				thisRpds.push_back(CombinationAnteriorPosteriorPalatalStrap::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case COMBINATION_CLASP:
				thisRpds.push_back(CombinationClasp::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspTipDirection, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case COMBINED_CLASP:
				thisRpds.push_back(CombinedClasp::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspMaterial, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case CONTINUOUS_CLASP:
				thisRpds.push_back(ContinuousClasp::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspMaterial, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case DENTURE_BASE:
				thisRpds.push_back(DentureBase::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case EDENTULOUS_SPACE:
				thisRpds.push_back(EdentulousSpace::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case FULL_PALATAL_PLATE:
				thisRpds.push_back(FullPalatalPlate::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case LINGUAL_BAR:
				thisRpds.push_back(LingualBar::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case LINGUAL_PLATE:
				thisRpds.push_back(LingualPlate::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case LINGUAL_REST:
				thisRpds.push_back(LingualRest::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpRestMesialOrDistal, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case OCCLUSAL_REST:
				thisRpds.push_back(OcclusalRest::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpRestMesialOrDistal, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case PALATAL_PLATE:
				thisRpds.push_back(PalatalPlate::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case RING_CLASP:
				thisRpds.push_back(RingClasp::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspMaterial, dpClaspTipSide, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case RPA:
				thisRpds.push_back(Rpa::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspMaterial, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case RPI:
				thisRpds.push_back(Rpi::createFromIndividual(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			case TOOTH: {
				auto const& tmp = env->CallObjectMethod(individual, midResourceGetProperty, dpIsMissing);
				if (!(tmp && env->CallBooleanMethod(tmp, midGetBoolean)) && env->CallIntMethod(env->CallObjectMethod(individual, midResourceGetProperty, dpToothOrdinal), midGetInt) == nTeethPerZone)
					thisIsEighthToothUsed[env->CallIntMethod(env->CallObjectMethod(individual, midResourceGetProperty, dpToothZone), midGetInt) - 1] = true;
			}
				break;
			case WW_CLASP:
				thisRpds.push_back(WwClasp::createFromIndividual(env, midGetBoolean, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspTipDirection, dpEnableBuccalArm, dpEnableLingualArm, dpEnableRest, dpToothZone, dpToothOrdinal, opComponentPosition, individual, thisIsEighthToothUsed));
				break;
			default: ;
		}
	}
	if (isValid) {
		rpds = thisRpds;
		copy(begin(thisIsEighthToothUsed), end(thisIsEighthToothUsed), Tooth::isEighthUsed);
	}
	return isValid;
}

void analyzeBaseImage(Mat const& base, vector<Tooth> (&remediedTeeth)[nZones], Mat (&remediedDesignImages)[2], vector<Tooth> (*const& teeth)[nZones], Mat (*const& designImages)[2], Mat* const& baseImage) {
	Mat thisBaseImage;
	copyMakeBorder(base, thisBaseImage, 80, 80, 80, 80, BORDER_CONSTANT, Scalar::all(255));
	if (baseImage)
		*baseImage = thisBaseImage;
	Mat tmpImage;
	cvtColor(thisBaseImage, tmpImage, COLOR_BGR2GRAY);
	threshold(tmpImage, tmpImage, 0, 255, THRESH_BINARY | THRESH_OTSU);
	vector<vector<Point>> contours;
	vector<Tooth> tmpTeeth;
	vector<Vec4i> hierarchy;
	findContours(tmpImage, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
	for (auto i = hierarchy[0][2]; i >= 0; i = hierarchy[i][0])
		for (auto j = hierarchy[i][2]; j >= 0; j = hierarchy[j][0])
			tmpTeeth.push_back(Tooth(contours[j]));
	vector<Point2f> centroids;
	for (auto tooth = tmpTeeth.begin(); tooth < tmpTeeth.end(); ++tooth)
		centroids.push_back(tooth->getCentroid());
	teethEllipse = fitEllipse(centroids);
	auto const& nTeeth = (nTeethPerZone - 1) * nZones;
	vector<float> angles(nTeeth);
	auto oldRemedyImage = remedyImage;
	remedyImage = false;
	for (auto i = 0; i < nTeeth; ++i)
		computeNormalDirection(centroids[i], &angles[i]);
	vector<int> idx;
	sortIdx(angles, idx, SORT_ASCENDING);
	vector<vector<uint8_t>> isInZone(nZones);
	vector<Tooth> thisTeeth[nZones];
	for (auto i = 0; i < nZones; ++i) {
		inRange(angles, CV_2PI / nZones * (i - 2), CV_2PI / nZones * (i - 1), isInZone[i]);
		thisTeeth[i].clear();
	}
	for (auto i = 0; i < nTeeth; ++i) {
		auto const& no = idx[i];
		for (auto j = 0; j < nZones; ++j)
			if (isInZone[j][no]) {
				if (j % 2)
					thisTeeth[j].push_back(tmpTeeth[no]);
				else
					thisTeeth[j].insert(thisTeeth[j].begin(), tmpTeeth[no]);
				break;
			}
	}
	auto const& imageSize = thisBaseImage.size();
	if (designImages)
		(*designImages)[0] = Mat(imageSize, CV_8U, 255);
	for (auto zone = 0; zone < nZones; ++zone) {
		for (auto ordinal = 0; ordinal < nTeethPerZone - 1; ++ordinal) {
			auto const& tooth = thisTeeth[zone][ordinal];
			if (ordinal == nTeethPerZone - 2)
				thisTeeth[zone].push_back(tooth);
			if (designImages)
				polylines((*designImages)[0], tooth.getContour(), true, 0, lineThicknessOfLevel[0], LINE_AA);
		}
		auto const& seventhTooth = thisTeeth[zone][nTeethPerZone - 2];
		auto& eighthTooth = thisTeeth[zone][nTeethPerZone - 1];
		auto const& translation = roundToPoint(rotate(computeNormalDirection(seventhTooth.getAnglePoint(180)), CV_PI * (zone % 2 - 0.5)) * seventhTooth.getRadius() * 2.16);
		auto contour = eighthTooth.getContour();
		for (auto point = contour.begin(); point < contour.end(); ++point)
			*point += translation;
		eighthTooth.setContour(contour);
		centroids.push_back(eighthTooth.getCentroid());
	}
	teethEllipse = fitEllipse(centroids);
	auto theta = degreeToRadian(teethEllipse.angle);
	auto const& direction = rotate(Point(0, 1), theta);
	float distance = 0;
	for (auto zone = 0; zone < nZones; ++zone)
		distance += thisTeeth[zone][nTeethPerZone - 1].getRadius();
	(distance *= 3) /= 4;
	auto const& translation = roundToPoint(direction * distance);
	centroids.clear();
	for (auto zone = 0; zone < nZones; ++zone) {
		auto& teethZone = thisTeeth[zone];
		auto& remediedTeethZone = remediedTeeth[zone];
		remediedTeethZone.clear();
		for (auto ordinal = 0; ordinal < nTeethPerZone; ++ordinal) {
			auto& tooth = teethZone[ordinal];
			tooth.setNormalDirection(computeNormalDirection(tooth.getCentroid()));
			if (ordinal == nTeethPerZone - 1) {
				auto contour = tooth.getContour();
				auto& centroid = tooth.getCentroid();
				theta = asin(teethZone[ordinal - 1].getNormalDirection().cross(tooth.getNormalDirection()));
				for (auto point = contour.begin(); point < contour.end(); ++point)
					*point = centroid + rotate(static_cast<Point2f>(*point) - centroid, theta);
				tooth.setContour(contour);
			}
			auto remediedTooth = tooth;
			if (zone >= nZones / 2) {
				auto contour = remediedTooth.getContour();
				for (auto point = contour.begin(); point < contour.end(); ++point)
					*point += translation;
				remediedTooth.setContour(contour);
			}
			centroids.push_back(remediedTooth.getCentroid());
			remediedTeethZone.push_back(remediedTooth);
			tooth.findAnglePoints(zone);
		}
	}
	if (teeth)
		copy(begin(thisTeeth), end(thisTeeth), *teeth);
	remediedTeethEllipse = fitEllipse(centroids);
	theta = degreeToRadian(-remediedTeethEllipse.angle);
	remediedTeethEllipse.angle = 0;
	remediedDesignImages[0] = Mat(imageSize + Size(0, distance * cos(theta)), CV_8U, 255);
	remedyImage = true;
	for (auto zone = 0; zone < nZones; ++zone) {
		for (auto ordinal = 0; ordinal < nTeethPerZone; ++ordinal) {
			auto& tooth = remediedTeeth[zone][ordinal];
			auto contour = tooth.getContour();
			if (ordinal < nTeethPerZone - 1)
				polylines(remediedDesignImages[0], contour, true, 0, lineThicknessOfLevel[0], LINE_AA);
			for (auto point = contour.begin(); point < contour.end(); ++point)
				*point = remediedTeethEllipse.center + rotate(static_cast<Point2f>(*point) - remediedTeethEllipse.center, theta);
			tooth.setContour(contour);
			tooth.setNormalDirection(computeNormalDirection(tooth.getCentroid()));
			tooth.findAnglePoints(zone);
		}
	}
	remedyImage = oldRemedyImage;
}

void updateDesign(vector<Tooth> (&teeth)[nZones], vector<Rpd*>& rpds, Mat (&designImages)[2], bool const& justLoadedImage, bool const& justLoadedRpds) {
	if (!justLoadedImage)
		for (auto zone = 0; zone < nZones; ++zone)
			for (auto ordinal = 0; ordinal < nTeethPerZone; ++ordinal)
				teeth[zone][ordinal].unsetAll();
	for (auto rpd = rpds.begin(); rpd < rpds.end(); ++rpd) {
		auto const& rpdAsMajorConnector = dynamic_cast<RpdAsMajorConnector*>(*rpd);
		if (rpdAsMajorConnector) {
			rpdAsMajorConnector->registerMajorConnector(teeth);
			rpdAsMajorConnector->registerExpectedAnchors(teeth);
			rpdAsMajorConnector->registerLingualConfrontations(teeth);
		}
		auto const& rpdWithClaspRootOrRest = dynamic_cast<RpdWithClaspRootOrRest*>(*rpd);
		if (rpdWithClaspRootOrRest)
			rpdWithClaspRootOrRest->registerClaspRootOrRest(teeth);
		auto const& dentureBase = dynamic_cast<DentureBase*>(*rpd);
		if (dentureBase)
			dentureBase->registerExpectedAnchors(teeth);
	}
	if (justLoadedRpds)
		for (auto rpd = rpds.begin(); rpd < rpds.end(); ++rpd) {
			auto const& rpdWithLingualArms = dynamic_cast<RpdWithLingualClaspArms*>(*rpd);
			if (rpdWithLingualArms)
				rpdWithLingualArms->setLingualClaspArms(teeth);
			auto const& dentureBase = dynamic_cast<DentureBase*>(*rpd);
			if (dentureBase)
				dentureBase->setSide(teeth);
		}
	for (auto rpd = rpds.begin(); rpd < rpds.end(); ++rpd) {
		auto const& rpdWithLingualCoverage = dynamic_cast<RpdWithLingualCoverage*>(*rpd);
		if (rpdWithLingualCoverage)
			rpdWithLingualCoverage->registerLingualCoverage(teeth);
		auto const& dentureBase = dynamic_cast<DentureBase*>(*rpd);
		if (dentureBase)
			dentureBase->registerDentureBase(teeth);
	}
	designImages[1] = Mat(designImages[0].size(), CV_8U, 255);
	for (auto zone = 0; zone < nZones; ++zone) {
		if (Tooth::isEighthUsed[zone])
			polylines(designImages[1], teeth[zone][nTeethPerZone - 1].getContour(), true, 0, lineThicknessOfLevel[0], LINE_AA);
	}
	for (auto rpd = rpds.begin(); rpd < rpds.end(); ++rpd)
		(*rpd)->draw(designImages[1], teeth);
}
