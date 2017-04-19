#include <opencv2/imgproc.hpp>

#include "Tooth.h"
#include "Utilities.h"

bool Tooth::isEighthUsed[nZones];

Tooth::Tooth(const vector<Point>& contour) { setContour(contour); }

const vector<Point>& Tooth::getContour() const { return contour_; }

void Tooth::setContour(const vector<Point>& contour) {
	contour_ = contour;
	auto moment = moments(contour);
	radius_ = sqrt(moment.m00 / CV_PI);
	centroid_ = Point2f(moment.m10 / moment.m00, moment.m01 / moment.m00);
}

const Point& Tooth::getAnglePoint(const int& angle) const { return contour_[anglePointIndices_[angle]]; }

vector<Point> Tooth::getCurve(const int& startAngle, const int& endAngle, const bool& isConvex) const {
	auto midAngle = (startAngle + endAngle) / 2;
	if (startAngle > endAngle)
		midAngle = (midAngle + 180) % 360;
	auto& startIdx = anglePointIndices_[startAngle];
	auto& midIdx = anglePointIndices_[midAngle];
	auto& endIdx = anglePointIndices_[endAngle];
	vector<Point> curve;
	if ((midIdx - startIdx) * (endIdx - midIdx) >= 0)
		if (startIdx < endIdx)
			curve = vector<Point>(contour_.begin() + startIdx, contour_.begin() + endIdx + 1);
		else
			curve = vector<Point>(contour_.rend() - startIdx - 1, contour_.rend() - endIdx);
	else if (startIdx < endIdx) {
		curve = vector<Point>(contour_.rend() - startIdx - 1, contour_.rend());
		curve.insert(curve.end(), contour_.rbegin(), contour_.rend() - endIdx);
	}
	else {
		curve = vector<Point>(contour_.begin() + startIdx, contour_.end());
		curve.insert(curve.end(), contour_.begin(), contour_.begin() + endIdx + 1);
	}
	if (isConvex) {
		vector<int> convexIdx;
		convexHull(curve, convexIdx);
		auto minMaxIts = minmax_element(convexIdx.begin(), convexIdx.end());
		vector<Point> convexCurve;
		if (minMaxIts.first < minMaxIts.second)
			if ((minMaxIts.second - minMaxIts.first) * 2 >= convexIdx.size())
				for (auto it = minMaxIts.first; it <= minMaxIts.second; ++it)
					convexCurve.push_back(curve[*it]);
			else {
				for (auto it = vector<int>::reverse_iterator(minMaxIts.first) - 1; it < convexIdx.rend(); ++it)
					convexCurve.push_back(curve[*it]);
				for (auto it = convexIdx.rbegin(); it < vector<int>::reverse_iterator(minMaxIts.second); ++it)
					convexCurve.push_back(curve[*it]);
			}
		else if ((minMaxIts.first - minMaxIts.second) * 2 >= convexIdx.size())
			for (auto it = vector<int>::reverse_iterator(minMaxIts.first) - 1; it < vector<int>::reverse_iterator(minMaxIts.second); ++it)
				convexCurve.push_back(curve[*it]);
		else {
			for (auto it = minMaxIts.first; it < convexIdx.end(); ++it)
				convexCurve.push_back(curve[*it]);
			for (auto it = convexIdx.begin(); it <= minMaxIts.second; ++it)
				convexCurve.push_back(curve[*it]);
		}
		return convexCurve;
	}
	return curve;
}

const Point2f& Tooth::getCentroid() const { return centroid_; }

const Point2f& Tooth::getNormalDirection() const { return normalDirection_; }

void Tooth::setNormalDirection(const Point2f& normalDirection) { normalDirection_ = normalDirection; }

void Tooth::findAnglePoints(const int& zone) {
	auto signVal = 1 - zone % 2 * 2;
	auto deltaAngle = degreeToRadian(1);
	int angle;
	float targetAngle;
	if (signVal == 1) {
		angle = 0;
		targetAngle = CV_PI / 2;
	}
	else {
		angle = 359;
		targetAngle = CV_PI / 2 * 3 - deltaAngle;
	}
	auto nPoints = contour_.size();
	auto j = 0;
	while (angle >= 0 && angle < 360) {
		auto isFound = false;
		auto d = rotate(normalDirection_, targetAngle);
		while (!isFound) {
			auto& p1 = contour_[j];
			auto& p2 = contour_[(j + 1) % nPoints];
			auto t = d.cross(centroid_ - static_cast<Point2f>(p1)) / d.cross(p2 - p1);
			if (t >= 0 && t < 1) {
				auto point = p1 + t * (p2 - p1);
				if (d.dot(static_cast<Point2f>(point) - centroid_) > 0) {
					if (point == p1)
						anglePointIndices_[angle] = j;
					else if (point == p2)
						anglePointIndices_[angle] = (j + 1) % nPoints;
					else {
						contour_.insert(contour_.begin() + ++j, point);
						++nPoints;
						for (auto k = 0; k < 360; ++k)
							if (anglePointIndices_[k] >= j)
								++anglePointIndices_[k];
						anglePointIndices_[angle] = j--;
					}
					isFound = true;
				}
			}
			if (!isFound)
				j = (j + 1) % nPoints;
		}
		angle += signVal;
		targetAngle -= deltaAngle;
	}
}

const bool& Tooth::expectDentureBaseAnchor(const Rpd::Direction& direction) const { return direction == Rpd::MESIAL ? expectMesialDentureBaseAnchor_ : expectDistalDentureBaseAnchor_; }

const bool& Tooth::expectMajorConnectorAnchor(const Rpd::Direction& direction) const { return direction == Rpd::MESIAL ? expectMesialMajorConnectorAnchor_ : expectDistalMajorConnectorAnchor_; }

const bool& Tooth::hasClaspRootOrRest(const Rpd::Direction& direction) const { return direction == Rpd::MESIAL ? hasMesialClaspRootOrRest_ : hasDistalClaspRootOrRest_; }

const bool& Tooth::hasDentureBase(const DentureBase::Side& side) const { return side == DentureBase::SINGLE ? hasSingleSidedDentureBase_ : hasDoubleSidedDentureBase_; }

const bool& Tooth::hasLingualConfrontation() const { return hasLingualConfrontation_; }

const bool& Tooth::hasLingualCoverage(const Rpd::Direction& direction) const { return direction == Rpd::MESIAL ? hasMesialLingualCoverage_ : hasDistalLingualCoverage_; }

const bool& Tooth::hasLingualRest(const Rpd::Direction& direction) const { return direction == Rpd::MESIAL ? hasMesialLingualRest_ : hasDistalLingualRest_; }

const bool& Tooth::hasMajorConnector() const { return hasMajorConnector_; }

const float& Tooth::getRadius() const { return radius_; }

void Tooth::setClaspRootOrRest(const Rpd::Direction& direction) { (direction == Rpd::MESIAL ? hasMesialClaspRootOrRest_ : hasDistalClaspRootOrRest_) = true; }

void Tooth::setDentureBase(const DentureBase::Side& side) { (side == DentureBase::SINGLE ? hasSingleSidedDentureBase_ : hasDoubleSidedDentureBase_) = true; }

void Tooth::setExpectedDentureBaseAnchor(const Rpd::Direction& direction) { (direction == Rpd::MESIAL ? expectMesialDentureBaseAnchor_ : expectDistalDentureBaseAnchor_) = true; }

void Tooth::setExpectedMajorConnectorAnchor(const Rpd::Direction& direction) { (direction == Rpd::MESIAL ? expectMesialMajorConnectorAnchor_ : expectDistalMajorConnectorAnchor_) = true; }

void Tooth::setLingualConfrontation() { hasLingualConfrontation_ = true; }

void Tooth::setLingualCoverage(const Rpd::Direction& direction) { (direction == Rpd::MESIAL ? hasMesialLingualCoverage_ : hasDistalLingualCoverage_) = true; }

void Tooth::setLingualRest(const Rpd::Direction& direction) { (direction == Rpd::MESIAL ? hasMesialLingualRest_ : hasDistalLingualRest_) = true; }

void Tooth::setMajorConnector() { hasMajorConnector_ = true; }

void Tooth::unsetAll() { expectDistalDentureBaseAnchor_ = expectDistalMajorConnectorAnchor_ = expectMesialDentureBaseAnchor_ = expectMesialMajorConnectorAnchor_ = hasDistalClaspRootOrRest_ = hasDistalLingualCoverage_ = hasDistalLingualRest_ = hasDoubleSidedDentureBase_ = hasLingualConfrontation_ = hasMajorConnector_ = hasMesialClaspRootOrRest_ = hasMesialLingualCoverage_ = hasMesialLingualRest_ = hasSingleSidedDentureBase_ = false; }
