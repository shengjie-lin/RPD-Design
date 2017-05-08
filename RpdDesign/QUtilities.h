#pragma once

#include <opencv2/core/mat.hpp>
#include <QImage>

using namespace cv;

QImage matToQImage(Mat const& inputMat);

QPixmap matToQPixmap(Mat const& inputMat);

Size qSizeToSize(QSize const& size);

QSize sizeToQSize(Size const& size);
