#include <opencv2/imgproc.hpp>

#include "EllipticCurve.h"

EllipticCurve::EllipticCurve(): inclination_(0), startAngle_(0), endAngle_(0), shallReverse_(false) {}

EllipticCurve::EllipticCurve(Point2f center, Size axes, float inclination, float endAngle, bool shallReverse): EllipticCurve(center, axes, inclination, 0, endAngle, shallReverse) {}

EllipticCurve::EllipticCurve(Point2f center, Size axes, float inclination, float startAngle, float endAngle, bool shallReverse): center_(center), axes_(axes), inclination_(inclination), startAngle_(startAngle), endAngle_(endAngle), shallReverse_(shallReverse) {}

vector<Point> EllipticCurve::getCurve() const {
	vector<Point> curve;
	if (endAngle_ - startAngle_ > 0.5) {
		ellipse2Poly(center_, axes_, inclination_, startAngle_, endAngle_, 1, curve);
		if (shallReverse_)
			return vector<Point>(curve.rbegin(), curve.rend());
	}
	return curve;
}
