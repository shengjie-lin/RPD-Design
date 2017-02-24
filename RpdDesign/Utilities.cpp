#include <Windows.h>

#include <opencv2/imgproc.hpp>

#include <QMessageBox>

#include "Utilities.h"

Mat qImage2Mat(const QImage& inputImage) {
	switch (inputImage.format()) {
			// 8-bit, 4 channel
		case QImage::Format_ARGB32:
		case QImage::Format_ARGB32_Premultiplied:
			return Mat(inputImage.height(), inputImage.width(), CV_8UC4, const_cast<uchar*>(inputImage.bits()), static_cast<size_t>(inputImage.bytesPerLine())).clone();
			// 8-bit, 3 channel
		case QImage::Format_RGB32:
		case QImage::Format_RGB888: {
			auto tmp = inputImage.rgbSwapped();
			if (tmp.format() == QImage::Format_RGB32)
				tmp = tmp.convertToFormat(QImage::Format_RGB888);
			return Mat(tmp.height(), tmp.width(), CV_8UC3, const_cast<uchar*>(tmp.bits()), static_cast<size_t>(tmp.bytesPerLine())).clone();
		}
			// 8-bit, 1 channel
		case QImage::Format_Indexed8:
			return Mat(inputImage.height(), inputImage.width(), CV_8UC1, const_cast<uchar*>(inputImage.bits()), static_cast<size_t>(inputImage.bytesPerLine())).clone();
		default:
			QMessageBox::information(nullptr, "", "qImage2Mat() - QImage format not handled: " + QString(inputImage.format()));
			return Mat();
	}
}

QImage mat2QImage(const Mat& inputMat) {
	switch (inputMat.type()) {
			// 8-bit, 4 channel
		case CV_8UC4:
			return QImage(inputMat.data, inputMat.cols, inputMat.rows, static_cast<int>(inputMat.step), QImage::Format_ARGB32);
			// 8-bit, 3 channel
		case CV_8UC3:
			return QImage(inputMat.data, inputMat.cols, inputMat.rows, static_cast<int>(inputMat.step), QImage::Format_RGB888).rgbSwapped();
			// 8-bit, 1 channel
		case CV_8UC1: {
			static auto isColorTableReady = false;
			static QVector<QRgb> colorTable(256);
			if (!isColorTableReady) {
				for (auto i = 0; i < 256; ++i)
					colorTable[i] = qRgb(i, i, i);
				isColorTableReady = true;
			}
			QImage image(inputMat.data, inputMat.cols, inputMat.rows, static_cast<int>(inputMat.step), QImage::Format_Indexed8);
			image.setColorTable(colorTable);
			return image;
		}
		default:
			QMessageBox::information(nullptr, "", "mat2QImage() - Mat type not handled: " + QString(to_string(inputMat.type()).data()));
			return QImage();
	}
}

Mat qPixmap2Mat(const QPixmap& inputPixmap) { return qImage2Mat(inputPixmap.toImage()); }

QPixmap mat2QPixmap(const Mat& inputMat) { return QPixmap::fromImage(mat2QImage(inputMat)); }

Size qSize2Size(const QSize& size) { return Size(size.width(), size.height()); }

QSize size2QSize(const Size& size) { return QSize(size.width, size.height); }

void concatenatePath(string& path, const string& searchDirectory, const string& extension) {
	auto searchPattern = searchDirectory + extension;
	WIN32_FIND_DATA findData;
	auto hFind = FindFirstFile(searchPattern.c_str(), &findData);
	do {
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			path.append(searchDirectory + findData.cFileName + ';');
	}
	while (FindNextFile(hFind, &findData));
	FindClose(hFind);
}

string getClsSig(const char* clsStr) { return 'L' + string(clsStr) + ';'; }

Point2f computeNormalDirection(const Point2f& point, float* angle) {
	auto direction = point - teethEllipse.center;
	auto thisAngle = atan2(direction.y, direction.x) - degree2Radian(teethEllipse.angle);
	if (thisAngle < -M_PI)
		thisAngle += M_PI * 2;
	if (angle)
		*angle = thisAngle;
	auto normalDirection = Point2f(pow(teethEllipse.size.height, 2) * cos(thisAngle), pow(teethEllipse.size.width, 2) * sin(thisAngle));
	return normalDirection / norm(normalDirection);
}

void computeInscribedCircle(const vector<Point>& anglePoints, float maxRadius, EllipticCurve& ellipticCurve, vector<Point>& tangentPoints) {
	Point2f v1 = anglePoints[0] - anglePoints[1], v2 = anglePoints[2] - anglePoints[1];
	auto l1 = norm(v1), l2 = norm(v2);
	auto d1 = v1 / l1, d2 = v2 / l2;
	auto sinTheta = d1.cross(d2);
	auto theta = asin(abs(sinTheta));
	if (d1.dot(d2) < 0)
		theta = M_PI - theta;
	auto radius = min({maxRadius, static_cast<float>(min({l1, l2}) * tan(theta / 2) / 2)});
	ellipticCurve = EllipticCurve(anglePoints[1] + roundToInt(normalize(d1 + d2) * radius / sin(theta / 2)), roundToInt(Size(radius, radius)), radian2Degree(sinTheta > 0 ? atan2(d2.x, -d2.y) : atan2(d1.x, -d1.y)), 180 - radian2Degree(theta));
	auto l = radius / tan(theta / 2);
	tangentPoints.clear();
	tangentPoints.push_back(anglePoints[1] + roundToInt(d1 * l));
	tangentPoints.push_back(anglePoints[1] + roundToInt(d2 * l));
}

void drawSmoothCurve(Mat& designImage, const vector<Point> curve, float maxRadius, const Scalar& color, int thickness, LineTypes lineType) {
	vector<Point> lineSegment(2), tangentPoints;
	vector<EllipticCurve> ellipticCurves;
	vector<vector<Point>> lineSegments;
	lineSegment[0] = curve[0];
	for (auto it = curve.begin() + 1; it < curve.end() - 1; ++it) {
		ellipticCurves.push_back(EllipticCurve());
		computeInscribedCircle({lineSegment[0], *it, *(it + 1)}, maxRadius, ellipticCurves.back(), tangentPoints);
		lineSegment[1] = tangentPoints[0];
		lineSegments.push_back(lineSegment);
		lineSegment[0] = tangentPoints[1];
	}
	lineSegment[1] = curve.back();
	lineSegments.push_back(lineSegment);
	for (auto it = ellipticCurves.begin(); it < ellipticCurves.end(); ++it)
		it->draw(designImage, color, thickness, lineType);
	for (auto it = lineSegments.begin(); it < lineSegments.end(); ++it)
		polylines(designImage, *it, false, color, thickness, lineType);
}

RotatedRect teethEllipse;

const int lineThicknessOfLevel[] = {1, 4, 7};
