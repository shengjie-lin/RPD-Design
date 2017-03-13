#pragma once

#include "Rpd.h"

class Tooth {
public :
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
	void setOcclusalRest(const Rpd::Direction& direction);
	void unsetOcclusalRest();
	const RpdAsLingualBlockage::LingualBlockage& getLingualBlockage() const;
	void setLingualBlockage(const RpdAsLingualBlockage::LingualBlockage& lingualBlockage);
private :
	bool hasDistalOcclusalRest_ = false, hasMesialOcclusalRest_ = false;
	float radius_;
	Point2f centroid_, normalDirection_;
	RpdAsLingualBlockage::LingualBlockage lingualBlockage_ = RpdAsLingualBlockage::NONE;
	vector<int> anglePointIndices_ = vector<int>(360);
	vector<Point> contour_;
};
