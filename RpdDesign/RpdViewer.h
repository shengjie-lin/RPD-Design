#pragma once

#include <jni.h>
#include <opencv2/core/mat.hpp>
#include <QLabel>

#include "GlobalVariables.h"

class Rpd;
class Tooth;

class RpdViewer: public QLabel {
	Q_OBJECT
public:
	RpdViewer(QWidget*const& parent, const bool& showBaseImage, const bool& showContoursImage);
	~RpdViewer();
private:
	enum RpdClass {
		AKER_CLASP = 1,
		COMBINATION_CLASP,
		COMBINED_CLASP,
		DENTURE_BASE,
		EDENTULOUS_SPACE,
		LINGUAL_REST,
		OCCLUSAL_REST,
		PALATAL_PLATE,
		RING_CLASP,
		RPA,
		RPI,
		TOOTH,
		WW_CLASP
	};

	static map<string, RpdClass> rpdMapping_;
	Mat baseImage_, curImage_, designImages_[2];
	QSize imageSize_;
	bool showBaseImage_, showDesignImage_, justLoadedRpd_ = false, justLoadedImage_ = false, isEighthToothUsed_[nZones] = {}, isEighthToothMissing_[nZones] = {};
	vector<Tooth> teeth_[nZones];
	vector<Rpd*> rpds_;
	JavaVM* vm_;
	JNIEnv* env_;
	void updateRpdDesign();
	void resizeEvent(QResizeEvent* event) override;
	void refreshDisplay();

private slots:
	void loadBaseImage();
	void loadRpdInfo();
	void onShowBaseChanged(bool showBaseImage);
	void onShowDesignChanged(bool showContoursImage);
};
