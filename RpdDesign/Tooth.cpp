#include <opencv2/imgproc.hpp>

#include "Tooth.h"
#include "Utilities.h"

Tooth::Tooth(const vector<Point>& contour): contour_(contour) {
	auto moment = moments(contour_);
	radius_ = sqrt(moment.m00 / CV_PI);
	centroid_ = Point2f(moment.m10 / moment.m00, moment.m01 / moment.m00);
}

vector<Point> Tooth::getContour() const { return contour_; }

Point Tooth::getAnglePoint(int angle) const { return contour_[anglePointIndices_[angle]]; }

vector<Point> Tooth::getCurve(int startAngle, int endAngle, bool shallReverse, bool isConvex) const {
	auto midAngle = (startAngle + endAngle) / 2;
	if (startAngle > endAngle)
		midAngle = (midAngle + 180) % 360;
	auto startIdx = anglePointIndices_[startAngle], midIdx = anglePointIndices_[midAngle], endIdx = anglePointIndices_[endAngle];
	if (shallReverse)
		swap(startIdx, endIdx);
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

Point2f Tooth::getCentroid() const { return centroid_; }

void Tooth::setNormalDirection(const Point2f& normalDirection) { normalDirection_ = normalDirection; }

void Tooth::findAnglePoints(int zoneNo) {
	auto signVal = 1 - zoneNo % 2 * 2;
	float deltaAngle = CV_PI / 180;
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
			auto p1 = contour_[j];
			auto p2 = contour_[(j + 1) % nPoints];
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

float Tooth::getRadius() const { return radius_; }
