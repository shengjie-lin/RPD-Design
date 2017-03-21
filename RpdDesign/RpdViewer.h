#pragma once

#include <jni.h>
#include <opencv2/core/mat.hpp>
#include <QLabel>

#include "GlobalVariables.h"

class Rpd;
class Tooth;

class RpdViewer : public QLabel {
	Q_OBJECT
public:
	RpdViewer(QWidget* const& parent, const bool& showBaseImage, const bool& showContoursImage);
	~RpdViewer();
private:
	enum RpdClass {
		AKER_CLASP = 1,
		COMBINATION_CLASP,
		COMBINED_CLASP,
		CONTINUOUS_CLASP,
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

	void refreshDisplay();
	void resizeEvent(QResizeEvent* event) override;
	void updateRpdDesign();
	bool isEighthToothUsed_[nZones] = {}, justLoadedImage_ = false, justLoadedRpd_ = false, showBaseImage_, showDesignImage_;
	JavaVM* vm_;
	JNIEnv* env_;
	Mat baseImage_, curImage_, designImages_[2];
	QSize imageSize_;
	static map<string, RpdClass> rpdMapping_;
	vector<Rpd*> rpds_;
	vector<Tooth> teeth_[nZones];
private slots :
	void loadBaseImage();
	void loadRpdInfo();
	void onShowBaseChanged(bool showBaseImage);
	void onShowDesignChanged(bool showContoursImage);
};
