#include <opencv2/imgproc.hpp>

#include "EllipticCurve.h"
#include "Utilities.h"

EllipticCurve::EllipticCurve(Point2f const& center, Size const& axes, float const& inclination, float const& startAngle, float const& endAngle, bool const& shouldReverse) : shouldReverse_(shouldReverse), endAngle_(endAngle), inclination_(inclination), startAngle_(startAngle), center_(center), axes_(axes) {}

EllipticCurve::EllipticCurve(Point2f const& center, Size const& axes, float const& inclination, float const& endAngle, bool const& shouldReverse) : EllipticCurve(center, axes, inclination, 0, endAngle, shouldReverse) {}

bool EllipticCurve::getCurve(vector<Point>& curve) const {
	auto const& radius = axes_.width;
	if (radius <= 0 || radius > sqrt((remedyImage ? remediedTeethEllipse : teethEllipse).size.area() / 2) && abs(endAngle_ - startAngle_) < 5)
		return false;
	ellipse2Poly(center_, axes_, inclination_, startAngle_, endAngle_, 1, curve);
	if (shouldReverse_)
		reverse(curve.begin(), curve.end());
	return true;
}
