#pragma once

#include <opencv2/core/mat.hpp>

#include <QLabel>

#include "Rpd.h"

class RpdViewer: public QLabel {
	Q_OBJECT

public:
	RpdViewer(QWidget* parent, bool showBaseImage, bool showContoursImage);
	~RpdViewer();

private:
	enum RpdClass {
		AKERS_CLASP = 1,
		COMBINED_CLASP,
		DENTURE_BASE,
		EDENTULOUS_SPACE,
		LINGUAL_REST,
		OCCLUSAL_REST,
		PALATAL_PLATE,
		RING_CLASP,
		RPA,
		RPI,
		WW_CLASP
	};

	static map<string, RpdClass> rpdMapping_;
	Mat baseImage_, designImage_, curImage_;
	QSize imageSize_;
	bool hasImage_ = false, hasRpd_ = false, showBaseImage_, showDesignImage_;
	vector<vector<Tooth>> teeth_ = vector<vector<Tooth>>(4);
	vector<Rpd*> rpds_;
	JavaVM* vm_;
	JNIEnv* env_;
	void analyzeBaseImage();
	void updateRpdDesign();
	void resizeEvent(QResizeEvent* event) override;
	void refreshDisplay();

private slots:
	void loadBaseImage();
	void loadRpdInfo();
	void onShowBaseChanged(bool showBaseImage);
	void onShowDesignChanged(bool showContoursImage);
};
