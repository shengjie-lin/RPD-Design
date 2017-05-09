#include <opencv2/imgproc.hpp>

#include "Tooth.h"
#include "Utilities.h"

bool Tooth::isEighthUsed[nZones];

Tooth::Tooth(vector<Point> const& contour) { setContour(contour); }

vector<Point> const& Tooth::getContour() const { return contour_; }

void Tooth::setContour(vector<Point> const& contour) {
	contour_ = contour;
	auto const& moment = moments(contour);
	radius_ = sqrt(moment.m00 / CV_PI);
	centroid_ = Point2f(moment.m10 / moment.m00, moment.m01 / moment.m00);
}

Point const& Tooth::getAnglePoint(int const& angle) const { return contour_[anglePointIndices_[angle]]; }

vector<Point> Tooth::getCurve(int const& startAngle, int const& endAngle, bool const& isConvex) const {
	auto midAngle = (startAngle + endAngle) / 2;
	if (startAngle > endAngle)
		midAngle = (midAngle + 180) % 360;
	auto const& startIdx = anglePointIndices_[startAngle];
	auto const& midIdx = anglePointIndices_[midAngle];
	auto const& endIdx = anglePointIndices_[endAngle];
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
		auto const& minMaxIts = minmax_element(convexIdx.begin(), convexIdx.end());
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

Point2f const& Tooth::getCentroid() const { return centroid_; }

Point2f const& Tooth::getNormalDirection() const { return normalDirection_; }

void Tooth::setNormalDirection(Point2f const& normalDirection) { normalDirection_ = normalDirection; }

void Tooth::findAnglePoints(int const& zone) {
	auto const& signVal = 1 - zone % 2 * 2;
	auto const& deltaAngle = degreeToRadian(1);
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
		auto const& d = rotate(normalDirection_, targetAngle);
		while (!isFound) {
			auto const& p1 = contour_[j];
			auto const& p2 = contour_[(j + 1) % nPoints];
			auto const& t = d.cross(centroid_ - static_cast<Point2f>(p1)) / d.cross(p2 - p1);
			if (t >= 0 && t < 1) {
				auto const& point = p1 + t * (p2 - p1);
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

bool const& Tooth::expectDentureBaseAnchor(Rpd::Direction const& direction) const { return direction == Rpd::MESIAL ? expectMesialDentureBaseAnchor_ : expectDistalDentureBaseAnchor_; }

bool const& Tooth::expectMajorConnectorAnchor(Rpd::Direction const& direction) const { return direction == Rpd::MESIAL ? expectMesialMajorConnectorAnchor_ : expectDistalMajorConnectorAnchor_; }

bool const& Tooth::hasClaspRootOrRest(Rpd::Direction const& direction) const { return direction == Rpd::MESIAL ? hasMesialClaspRootOrRest_ : hasDistalClaspRootOrRest_; }

bool const& Tooth::hasDentureBase(DentureBase::Side const& side) const { return side == DentureBase::SINGLE ? hasSingleSidedDentureBase_ : hasDoubleSidedDentureBase_; }

bool const& Tooth::hasLingualConfrontation() const { return hasLingualConfrontation_; }

bool const& Tooth::hasLingualCoverage(Rpd::Direction const& direction) const { return direction == Rpd::MESIAL ? hasMesialLingualCoverage_ : hasDistalLingualCoverage_; }

bool const& Tooth::hasLingualRest(Rpd::Direction const& direction) const { return direction == Rpd::MESIAL ? hasMesialLingualRest_ : hasDistalLingualRest_; }

bool const& Tooth::hasMajorConnector() const { return hasMajorConnector_; }

float const& Tooth::getRadius() const { return radius_; }

void Tooth::setClaspRootOrRest(Rpd::Direction const& direction) { (direction == Rpd::MESIAL ? hasMesialClaspRootOrRest_ : hasDistalClaspRootOrRest_) = true; }

void Tooth::setDentureBase(DentureBase::Side const& side) { (side == DentureBase::SINGLE ? hasSingleSidedDentureBase_ : hasDoubleSidedDentureBase_) = true; }

void Tooth::setExpectedDentureBaseAnchor(Rpd::Direction const& direction) { (direction == Rpd::MESIAL ? expectMesialDentureBaseAnchor_ : expectDistalDentureBaseAnchor_) = true; }

void Tooth::setExpectedMajorConnectorAnchor(Rpd::Direction const& direction) { (direction == Rpd::MESIAL ? expectMesialMajorConnectorAnchor_ : expectDistalMajorConnectorAnchor_) = true; }

void Tooth::setLingualConfrontation() { hasLingualConfrontation_ = true; }

void Tooth::setLingualCoverage(Rpd::Direction const& direction) { (direction == Rpd::MESIAL ? hasMesialLingualCoverage_ : hasDistalLingualCoverage_) = true; }

void Tooth::setLingualRest(Rpd::Direction const& direction) { (direction == Rpd::MESIAL ? hasMesialLingualRest_ : hasDistalLingualRest_) = true; }

void Tooth::setMajorConnector() { hasMajorConnector_ = true; }

void Tooth::unsetAll() { expectDistalDentureBaseAnchor_ = expectDistalMajorConnectorAnchor_ = expectMesialDentureBaseAnchor_ = expectMesialMajorConnectorAnchor_ = hasDistalClaspRootOrRest_ = hasDistalLingualCoverage_ = hasDistalLingualRest_ = hasDoubleSidedDentureBase_ = hasLingualConfrontation_ = hasMajorConnector_ = hasMesialClaspRootOrRest_ = hasMesialLingualCoverage_ = hasMesialLingualRest_ = hasSingleSidedDentureBase_ = false; }
