#pragma once

using namespace std;
using namespace cv;

class Tooth {
public:
	enum LingualBlocking {
		NONE,
		CLASP,
		MAJOR_CONNECTOR
	};

	explicit Tooth(const vector<Point>& contour);
	vector<Point> getContour() const;
	Point getAnglePoint(int angle) const;
	vector<Point> getCurve(int startAngle, int endAngle, bool isConvex = true) const;
	Point2f getCentroid() const;
	Point2f getNormalDirection() const;
	void setNormalDirection(const Point2f& normalDirection);
	void findAnglePoints(int zoneNo);
	float getRadius() const;
	LingualBlocking getLingualBlocking() const;
	void setLingualBlocking(LingualBlocking lingualBlocking);

private:
	vector<Point> contour_;
	Point2f centroid_;
	Point2f normalDirection_;
	vector<int> anglePointIndices_ = vector<int>(360);
	float radius_;
	LingualBlocking lingualBlocking_ = NONE;
};
