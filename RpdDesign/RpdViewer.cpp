#include <opencv2/imgproc.hpp>

#include "RpdViewer.h"
#include "Utilities.h"

RpdViewer::RpdViewer(QWidget* const& parent) : QLabel(parent) { setAlignment(Qt::AlignCenter); }

const Mat& RpdViewer::getCurImage() const { return curImage_; }

void RpdViewer::setCurImage(const Mat& mat) {
	curImage_ = mat;
	imageSize_ = sizeToQSize(mat.size());
	resizeEvent(nullptr);
}

void RpdViewer::resizeEvent(QResizeEvent* event) {
	if (event)
		QLabel::resizeEvent(event);
	if (curImage_.data) {
		Mat curImage;
		cv::resize(curImage_, curImage, qSizeToSize(imageSize_.scaled(size(), Qt::KeepAspectRatio)));
		setPixmap(matToQPixmap(curImage));
	}
}
