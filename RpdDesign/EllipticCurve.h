#pragma once

using namespace std;
using namespace cv;

class EllipticCurve {
public:
	EllipticCurve(Point2f const& center, Size const& axes, float const& inclination, float const& endAngle, bool const& shouldReverse);
	EllipticCurve(Point2f const& center, Size const& axes, float const& inclination, float const& startAngle, float const& endAngle, bool const& shouldReverse);
	bool getCurve(vector<Point>& curve) const;
private:
	bool shouldReverse_;
	float endAngle_, inclination_, startAngle_;
	Point2f center_;
	Size axes_;
};
