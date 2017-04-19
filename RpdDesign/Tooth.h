#pragma once

#include "Rpd.h"

class Tooth {
public:
	const bool& expectDentureBaseAnchor(const Rpd::Direction& direction) const;
	const bool& expectMajorConnectorAnchor(const Rpd::Direction& direction) const;
	const bool& hasClaspRootOrRest(const Rpd::Direction& direction) const;
	const bool& hasDentureBase(const DentureBase::Side& side) const;
	const bool& hasLingualConfrontation() const;
	const bool& hasLingualCoverage(const Rpd::Direction& direction) const;
	const bool& hasLingualRest(const Rpd::Direction& direction) const;
	const bool& hasMajorConnector() const;
	const float& getRadius() const;
	const Point& getAnglePoint(const int& angle) const;
	const Point2f& getCentroid() const;
	const Point2f& getNormalDirection() const;
	const vector<Point>& getContour() const;
	explicit Tooth(const vector<Point>& contour);
	vector<Point> getCurve(const int& startAngle, const int& endAngle, const bool& isConvex = true) const;
	void findAnglePoints(const int& zone);
	void setClaspRootOrRest(const Rpd::Direction& direction);
	void setContour(const vector<Point>& contour);
	void setDentureBase(const DentureBase::Side& side);
	void setExpectedDentureBaseAnchor(const Rpd::Direction& direction);
	void setExpectedMajorConnectorAnchor(const Rpd::Direction& direction);
	void setLingualConfrontation();
	void setLingualCoverage(const Rpd::Direction& direction);
	void setLingualRest(const Rpd::Direction& direction);
	void setMajorConnector();
	void setNormalDirection(const Point2f& normalDirection);
	void unsetAll();
	static bool isEighthUsed[nZones];
private:
	bool expectDistalDentureBaseAnchor_ = false, expectDistalMajorConnectorAnchor_ = false, expectMesialDentureBaseAnchor_ = false, expectMesialMajorConnectorAnchor_ = false, hasDistalClaspRootOrRest_ = false, hasDistalLingualCoverage_ = false, hasDistalLingualRest_ = false, hasDoubleSidedDentureBase_ = false, hasLingualConfrontation_ = false, hasMajorConnector_ = false, hasMesialClaspRootOrRest_ = false, hasMesialLingualCoverage_ = false, hasMesialLingualRest_ = false, hasSingleSidedDentureBase_ = false;
	float radius_;
	Point2f centroid_, normalDirection_;
	vector<int> anglePointIndices_ = vector<int>(360);
	vector<Point> contour_;
};
