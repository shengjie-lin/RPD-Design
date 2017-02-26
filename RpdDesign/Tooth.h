#pragma once

using namespace std;
using namespace cv;

class Tooth {
public:
	explicit Tooth(const vector<Point>& contour);
	vector<Point> getContour() const;
	Point getAnglePoint(int angle) const;
	vector<Point> getCurve(int startAngle, int endAngle, bool shallReverse = false, bool isConvex = true) const;
	Point2f getCentroid() const;
	void setNormalDirection(const Point2f& normalDirection);
	void findAnglePoints(int zoneNo);
	float getRadius() const;

private:
	vector<Point> contour_;
	Point2f centroid_;
	Point2f normalDirection_;
	vector<int> anglePointIndices_ = vector<int>(360);
	float radius_;
};
