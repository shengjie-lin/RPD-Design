#include <opencv2/imgproc.hpp>

#include "EllipticCurve.h"
#include "Utilities.h"

EllipticCurve::EllipticCurve(Point2f center, Size axes, float inclination, float endAngle, bool shouldReverse): EllipticCurve(center, axes, inclination, 0, endAngle, shouldReverse) {}

EllipticCurve::EllipticCurve(Point2f center, Size axes, float inclination, float startAngle, float endAngle, bool shouldReverse): center_(center), axes_(axes), inclination_(inclination), startAngle_(startAngle), endAngle_(endAngle), shouldReverse_(shouldReverse) {}

bool EllipticCurve::getCurve(vector<Point>& curve) const {
	if (axes_.width < 0)
		return false;
	ellipse2Poly(center_, axes_, inclination_, startAngle_, endAngle_, 1, curve);
	if (norm(curve[0] - roundToInt(center_)) < sqrt(axes_.area()) / 2)
		return false;
	if (shouldReverse_)
		reverse(curve.begin(), curve.end());
	return true;
}
