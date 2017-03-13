#pragma once

#include "Rpd.h"

class Tooth {
public:
	explicit Tooth(const vector<Point>& contour);
	const vector<Point>& getContour() const;
	void setContour(const vector<Point>& contour);
	const Point& getAnglePoint(const int& angle) const;
	vector<Point> getCurve(const int& startAngle, const int& endAngle, const bool& isConvex = true) const;
	const Point2f& getCentroid() const;
	const Point2f& getNormalDirection() const;
	void setNormalDirection(const Point2f& normalDirection);
	void findAnglePoints(const int& zoneNo);
	const float& getRadius() const;
	const bool& hasMesialOcclusalRest() const;
	const bool& hasDistalOcclusalRest() const;
	void setOcclusalRest(const RpdWithDirection::Direction& direction);
	void unsetOcclusalRest();
	const RpdWithLingualBlockage::LingualBlockage& getLingualBlockage() const;
	void setLingualBlockage(const RpdWithLingualBlockage::LingualBlockage& lingualBlockage);

private:
	vector<Point> contour_;
	Point2f centroid_;
	Point2f normalDirection_;
	vector<int> anglePointIndices_ = vector<int>(360);
	float radius_;
	bool hasMesialOcclusalRest_ = false;
	bool hasDistalOcclusalRest_ = false;
	RpdWithLingualBlockage::LingualBlockage lingualBlockage_ = RpdWithLingualBlockage::NONE;
};
