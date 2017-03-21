#pragma once

using namespace std;
using namespace cv;

class EllipticCurve {
public:
	EllipticCurve(const Point2f& center, const Size& axes, const float& inclination, const float& endAngle, const bool& shouldReverse);
	EllipticCurve(const Point2f& center, const Size& axes, const float& inclination, const float& startAngle, const float& endAngle, const bool& shouldReverse);
	bool getCurve(vector<Point>& curve) const;
private:
	bool shouldReverse_;
	float endAngle_, inclination_, startAngle_;
	Point2f center_;
	Size axes_;
};
