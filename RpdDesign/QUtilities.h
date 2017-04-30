#pragma once

#include <opencv2/core/mat.hpp>
#include <QMessageBox>

using namespace cv;

QImage matToQImage(const Mat& inputMat);

QPixmap matToQPixmap(const Mat& inputMat);

Size qSizeToSize(const QSize& size);

QSize sizeToQSize(const Size& size);
