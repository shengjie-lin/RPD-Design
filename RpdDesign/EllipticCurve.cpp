#include <opencv2/imgproc.hpp>

#include "EllipticCurve.h"
#include "Utilities.h"

EllipticCurve::EllipticCurve(const Point2f& center, const Size& axes, const float& inclination, const float& endAngle, const bool& shouldReverse) : EllipticCurve(center, axes, inclination, 0, endAngle, shouldReverse) {}

EllipticCurve::EllipticCurve(const Point2f& center, const Size& axes, const float& inclination, const float& startAngle, const float& endAngle, const bool& shouldReverse) : shouldReverse_(shouldReverse), endAngle_(endAngle), inclination_(inclination), startAngle_(startAngle), center_(center), axes_(axes) {}

bool EllipticCurve::getCurve(vector<Point>& curve) const {
	auto& radius = axes_.width;
	if (radius < 0)
		return false;
	ellipse2Poly(center_, axes_, inclination_, startAngle_, endAngle_, 1, curve);
	if (norm(curve[0] - roundToInt(center_)) < radius / 2)
		return false;
	if (shouldReverse_)
		reverse(curve.begin(), curve.end());
	return true;
}
