#pragma once

#include <opencv2/core/mat.hpp>
#include <QLabel>

using namespace cv;

class RpdViewer : public QLabel {
	Q_OBJECT
public:
	explicit RpdViewer(QWidget* const& parent = nullptr);
	Mat const& getCurImage() const;
	void setCurImage(Mat const& mat);
private:
	void resizeEvent(QResizeEvent* event) override;
	Mat curImage_;
	QSize imageSize_;
};
