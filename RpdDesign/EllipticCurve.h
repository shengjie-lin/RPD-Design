#pragma once

using namespace std;
using namespace cv;

class EllipticCurve {
public:
	EllipticCurve(Point2f center, Size axes, float inclination, float endAngle, bool shouldReverse);
	EllipticCurve(Point2f center, Size axes, float inclination, float startAngle, float endAngle, bool shouldReverse);
	bool getCurve(vector<Point>& curve) const;
private:
	Point2f center_;
	Size axes_;
	float inclination_, startAngle_, endAngle_;
	bool shouldReverse_;
};
