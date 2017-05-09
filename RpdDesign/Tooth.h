#pragma once

#include "Rpd.h"

class Tooth {
public:
	explicit Tooth(vector<Point> const& contour);
	bool const& expectDentureBaseAnchor(Rpd::Direction const& direction) const;
	bool const& expectMajorConnectorAnchor(Rpd::Direction const& direction) const;
	bool const& hasClaspRootOrRest(Rpd::Direction const& direction) const;
	bool const& hasDentureBase(DentureBase::Side const& side) const;
	bool const& hasLingualConfrontation() const;
	bool const& hasLingualCoverage(Rpd::Direction const& direction) const;
	bool const& hasLingualRest(Rpd::Direction const& direction) const;
	bool const& hasMajorConnector() const;
	float const& getRadius() const;
	Point const& getAnglePoint(int const& angle) const;
	Point2f const& getCentroid() const;
	Point2f const& getNormalDirection() const;
	vector<Point> const& getContour() const;
	vector<Point> getCurve(int const& startAngle, int const& endAngle, bool const& isConvex = true) const;
	void findAnglePoints(int const& zone);
	void setClaspRootOrRest(Rpd::Direction const& direction);
	void setContour(vector<Point> const& contour);
	void setDentureBase(DentureBase::Side const& side);
	void setExpectedDentureBaseAnchor(Rpd::Direction const& direction);
	void setExpectedMajorConnectorAnchor(Rpd::Direction const& direction);
	void setLingualConfrontation();
	void setLingualCoverage(Rpd::Direction const& direction);
	void setLingualRest(Rpd::Direction const& direction);
	void setMajorConnector();
	void setNormalDirection(Point2f const& normalDirection);
	void unsetAll();
	static bool isEighthUsed[nZones];
private:
	bool expectDistalDentureBaseAnchor_ = false, expectDistalMajorConnectorAnchor_ = false, expectMesialDentureBaseAnchor_ = false, expectMesialMajorConnectorAnchor_ = false, hasDistalClaspRootOrRest_ = false, hasDistalLingualCoverage_ = false, hasDistalLingualRest_ = false, hasDoubleSidedDentureBase_ = false, hasLingualConfrontation_ = false, hasMajorConnector_ = false, hasMesialClaspRootOrRest_ = false, hasMesialLingualCoverage_ = false, hasMesialLingualRest_ = false, hasSingleSidedDentureBase_ = false;
	float radius_;
	Point2f centroid_, normalDirection_;
	vector<int> anglePointIndices_ = vector<int>(360);
	vector<Point> contour_;
};
