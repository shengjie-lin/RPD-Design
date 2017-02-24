#pragma once

using namespace cv;

class EllipticCurve {
public:
	EllipticCurve();
	EllipticCurve(Point2f center, Size axes, float inclination, float endAngle);
	EllipticCurve(Point2f center, Size axes, float inclination, float startAngle, float endAngle);
	void draw(Mat& designImage, const Scalar& color, int thickness, LineTypes lineType = LINE_AA) const;
private:
	Point2f center_;
	Size axes_;
	float inclination_, startAngle_, endAngle_;
};
