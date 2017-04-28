#pragma once

#include <jni.h>
#include <opencv2/core/mat.hpp>
#include <QTranslator>

#include "ui_RpdDesign.h"
#include "GlobalVariables.h"

class Rpd;
class RpdViewer;
class Tooth;

class RpdDesign : public QWidget {
	Q_OBJECT
public:
	explicit RpdDesign(QWidget* const& parent = nullptr);
	~RpdDesign();
private:
	enum RpdClass {
		AKERS_CLASP = 1,
		CANINE_AKERS_CLASP,
		COMBINATION_ANTERIOR_POSTERIOR_PALATAL_STRAP,
		COMBINATION_CLASP,
		COMBINED_CLASP,
		CONTINUOUS_CLASP,
		DENTURE_BASE,
		EDENTULOUS_SPACE,
		FULL_PALATAL_PLATE,
		LINGUAL_BAR,
		LINGUAL_PLATE,
		LINGUAL_REST,
		OCCLUSAL_REST,
		PALATAL_PLATE,
		RING_CLASP,
		RPA,
		RPI,
		TOOTH,
		WW_CLASP
	};

	void changeEvent(QEvent* event) override;
	void updateViewer();
	void updateRpdDesign();
	bool isEnglish_ = true;
	bool justLoadedImage_ = false, justLoadedRpd_ = false, showBaseImage_, showDesignImage_;
	JavaVM* vm_;
	JNIEnv* env_;
	Mat baseImage_, designImages_[2], remediedDesignImages_[2];
	QSize imageSize_, remediedImageSize_;
	QTranslator chsTranslator_, engTranslator_;
	RpdViewer* rpdViewer_;
	static map<string, RpdClass> rpdMapping_;
	Ui::RpdDesignClass ui_;
	vector<Rpd*> rpds_;
	vector<Tooth> teeth_[nZones], remediedTeeth_[nZones];
private slots:
	void loadBaseImage();
	void loadRpdInfo();
	void onRemedyImageChanged(const bool& thisRemedyImage);
	void onShowBaseChanged(const bool& showBaseImage);
	void onShowDesignChanged(const bool& showContoursImage);
	void saveDesign();
	void switchLanguage(bool* const& isEnglish = nullptr);
};
