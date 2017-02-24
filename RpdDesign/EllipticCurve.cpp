#include <opencv2/imgproc.hpp>

#include "EllipticCurve.h"

EllipticCurve::EllipticCurve(): inclination_(0), startAngle_(0), endAngle_(0) {}

EllipticCurve::EllipticCurve(Point2f center, Size axes, float inclination, float endAngle): EllipticCurve(center, axes, inclination, 0, endAngle) {}

EllipticCurve::EllipticCurve(Point2f center, Size axes, float inclination, float startAngle, float endAngle): center_(center), axes_(axes), inclination_(inclination), startAngle_(startAngle), endAngle_(endAngle) {}

void EllipticCurve::draw(Mat& designImage, const Scalar& color, int thickness, LineTypes lineType) const {
	if (endAngle_ - startAngle_ > 0.5)
		ellipse(designImage, center_, axes_, inclination_, startAngle_, endAngle_, color, thickness, lineType);
}
