#pragma once

using namespace std;
using namespace cv;

class EllipticCurve {
public:
	EllipticCurve(Point2f center, Size axes, float inclination, float endAngle, bool shallReverse);
	EllipticCurve(Point2f center, Size axes, float inclination, float startAngle, float endAngle, bool shallReverse);
	vector<Point> getCurve() const;
private:
	Point2f center_;
	Size axes_;
	float inclination_, startAngle_, endAngle_;
	bool shallReverse_;
};
