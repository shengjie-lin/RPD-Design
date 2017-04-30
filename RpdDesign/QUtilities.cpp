#include "QUtilities.h"

using namespace std;

QImage matToQImage(const cv::Mat& inputMat) {
	switch (inputMat.type()) {
		// 8-bit, 4-channel
	case CV_8UC4:
		return QImage(inputMat.data, inputMat.cols, inputMat.rows, inputMat.step, QImage::Format_ARGB32);
		// 8-bit, 3-channel
	case CV_8UC3:
		return QImage(inputMat.data, inputMat.cols, inputMat.rows, inputMat.step, QImage::Format_RGB888).rgbSwapped();
		// 8-bit, 1-channel
	case CV_8UC1: {
		static auto isColorTableReady = false;
		static QVector<QRgb> colorTable(256);
		if (!isColorTableReady) {
			for (auto i = 0; i < 256; ++i)
				colorTable[i] = qRgb(i, i, i);
			isColorTableReady = true;
		}
		QImage image(inputMat.data, inputMat.cols, inputMat.rows, inputMat.step, QImage::Format_Indexed8);
		image.setColorTable(colorTable);
		return image;
	}
	default:
		QMessageBox::warning(nullptr, "", QString(("matToQImage() - Mat type not handled: " + to_string(inputMat.type())).data()));
		return QImage();
	}
}

QPixmap matToQPixmap(const Mat& inputMat) { return QPixmap::fromImage(matToQImage(inputMat)); }

Size qSizeToSize(const QSize& size) { return Size(size.width(), size.height()); }

QSize sizeToQSize(const Size& size) { return QSize(size.width, size.height); }
