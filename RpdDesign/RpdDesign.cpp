#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QFileDialog>
#include <QMessageBox>

#include "RpdDesign.h"
#include "RpdViewer.h"
#include "Tooth.h"
#include "Utilities.h"

RpdDesign::RpdDesign(QWidget* const& parent) : QWidget(parent) {
	ui_.setupUi(this);
	rpdViewer_ = new RpdViewer(this);
	ui_.verticalLayout->insertWidget(0, rpdViewer_);
	setMinimumSize(512, 512);
	remedyImage = ui_.remedyCheckBox->isChecked();
	showBaseImage_ = ui_.baseCheckBox->isChecked();
	showDesignImage_ = ui_.designCheckBox->isChecked();
	chsTranslator_.load("rpddesign_zh.qm");
	engTranslator_.load("rpddesign_en.qm");
	switchLanguage(&isEnglish_);
	connect(ui_.switchLanguagePushButton, SIGNAL(clicked()), this, SLOT(switchLanguage()));
	connect(ui_.loadBasePushButton, SIGNAL(clicked()), this, SLOT(loadBaseImage()));
	connect(ui_.loadRpdPushButton, SIGNAL(clicked()), this, SLOT(loadRpdInfo()));
	connect(ui_.saveDesignPushButton, SIGNAL(clicked()), this, SLOT(saveDesign()));
	connect(ui_.remedyCheckBox, SIGNAL(toggled(bool)), this, SLOT(onRemedyImageChanged(const bool&)));
	connect(ui_.baseCheckBox, SIGNAL(toggled(bool)), this, SLOT(onShowBaseChanged(const bool&)));
	connect(ui_.designCheckBox, SIGNAL(toggled(bool)), this, SLOT(onShowDesignChanged(const bool&)));
	JavaVMInitArgs vmInitArgs;
	vmInitArgs.version = JNI_VERSION_1_8;
	vmInitArgs.nOptions = 1;
	vmInitArgs.options = new JavaVMOption[1];
	string optionString = "-Djava.class.path=";
	catPath(optionString, jenaLibPath, "*.jar");
	vmInitArgs.options[0].optionString = const_cast<char*>(optionString.c_str());
	vmInitArgs.ignoreUnrecognized = false;
	JNI_CreateJavaVM(&vm_, reinterpret_cast<void**>(&env_), &vmInitArgs);
	delete[] vmInitArgs.options;
}

RpdDesign::~RpdDesign() {
	for (auto rpd = rpds_.begin(); rpd < rpds_.end(); ++rpd)
		delete *rpd;
	delete rpdViewer_;
	vm_->DestroyJavaVM();
}

void RpdDesign::changeEvent(QEvent* event) {
	if (event->type() == QEvent::LanguageChange) {
		setWindowTitle(tr("RPD Design"));
		ui_.remedyCheckBox->setText(tr("Remedy"));
		ui_.baseCheckBox->setText(tr("Base"));
		ui_.designCheckBox->setText(tr("Design"));
		ui_.switchLanguagePushButton->setText(tr("Switch Language"));
		ui_.loadBasePushButton->setText(tr("Load Base"));
		ui_.loadRpdPushButton->setText(tr("Load RPD"));
		ui_.saveDesignPushButton->setText(tr("Save Design"));
	}
	else
		QWidget::changeEvent(event);
}

void RpdDesign::updateViewer() {
	auto imageSize = (remedyImage ? remediedDesignImages_[0].size : designImages_[0].size)();
	auto curImage = !remedyImage && showBaseImage_ ? baseImage_.clone() : Mat(imageSize, CV_8UC3, Scalar::all(255));
	if (showDesignImage_) {
		auto designImages = remedyImage ? remediedDesignImages_ : designImages_;
		Mat designImage;
		bitwise_and(designImages[0], designImages[1], designImage);
		cvtColor(designImage, designImage, COLOR_GRAY2BGR);
		bitwise_and(designImage, curImage, curImage);
	}
	rpdViewer_->setCurImage(curImage);
}

void RpdDesign::updateDesign() {
	for (auto i = 0; i < 2; ++i) {
		auto teeth = i ? remediedTeeth_ : teeth_;
		auto designImages = i ? remediedDesignImages_ : designImages_;
		auto imageSize = (i ? remediedDesignImages_[0].size : designImages_[0].size)();
		if (!justLoadedImage_)
			for (auto zone = 0; zone < nZones; ++zone)
				for (auto ordinal = 0; ordinal < nTeethPerZone; ++ordinal)
					teeth[zone][ordinal].unsetAll();
		for (auto rpd = rpds_.begin(); rpd < rpds_.end(); ++rpd) {
			auto rpdAsMajorConnector = dynamic_cast<RpdAsMajorConnector*>(*rpd);
			if (rpdAsMajorConnector) {
				rpdAsMajorConnector->registerMajorConnector(teeth);
				rpdAsMajorConnector->registerExpectedAnchors(teeth);
				rpdAsMajorConnector->registerLingualConfrontations(teeth);
			}
			auto rpdWithClaspRootOrRest = dynamic_cast<RpdWithClaspRootOrRest*>(*rpd);
			if (rpdWithClaspRootOrRest)
				rpdWithClaspRootOrRest->registerClaspRootOrRest(teeth);
			auto dentureBase = dynamic_cast<DentureBase*>(*rpd);
			if (dentureBase)
				dentureBase->registerExpectedAnchors(teeth);
		}
		if (justLoadedRpd_ && i == 0)
			for (auto rpd = rpds_.begin(); rpd < rpds_.end(); ++rpd) {
				auto rpdWithLingualArms = dynamic_cast<RpdWithLingualClaspArms*>(*rpd);
				if (rpdWithLingualArms)
					rpdWithLingualArms->setLingualClaspArms(teeth);
				auto dentureBase = dynamic_cast<DentureBase*>(*rpd);
				if (dentureBase)
					dentureBase->setSide(teeth);
			}
		for (auto rpd = rpds_.begin(); rpd < rpds_.end(); ++rpd) {
			auto rpdWithLingualCoverage = dynamic_cast<RpdWithLingualCoverage*>(*rpd);
			if (rpdWithLingualCoverage)
				rpdWithLingualCoverage->registerLingualCoverage(teeth);
			auto dentureBase = dynamic_cast<DentureBase*>(*rpd);
			if (dentureBase)
				dentureBase->registerDentureBase(teeth);
		}
		designImages[1] = Mat(imageSize, CV_8U, 255);
		for (auto zone = 0; zone < nZones; ++zone) {
			if (Tooth::isEighthUsed[zone])
				polylines(designImages[1], teeth[zone][nTeethPerZone - 1].getContour(), true, 0, lineThicknessOfLevel[0], LINE_AA);
		}
		for (auto rpd = rpds_.begin(); rpd < rpds_.end(); ++rpd)
			(*rpd)->draw(designImages[1], teeth);
	}
	justLoadedRpd_ = justLoadedImage_ = false;
}

void RpdDesign::loadBaseImage() {
	auto fileName = QFileDialog::getOpenFileName(this, tr("Select Base Image"), "", tr("All supported formats (*.bmp *.dib *.jpeg *.jpg *.jpe *.jp2 *.png *.pbm *.pgm *.ppm *.sr *.ras *.tiff *.tif);;Windows bitmaps (*.bmp *.dib);;JPEG files (*.jpeg *.jpg *.jpe);;JPEG 2000 files (*.jp2);;Portable Network Graphics (*.png);;Portable image format (*.pbm *.pgm *.ppm);;Sun rasters (*.sr *.ras);;TIFF files (*.tiff *.tif)"));
	if (!fileName.isEmpty()) {
		auto image = imread(fileName.toLocal8Bit().data());
		if (analyzeBaseImage(image, remediedTeeth_, remediedDesignImages_, teeth_, &baseImage_)) {
			justLoadedImage_ = true;
			updateDesign();
			updateViewer();
		}
		else
			QMessageBox::critical(this, tr("Error"), tr("Not a Valid Image!"));
	}
}

void RpdDesign::loadRpdInfo() {
	auto fileName = QFileDialog::getOpenFileName(this, tr("Select RPD Information"), "", tr("Ontology files (*.owl)"));
	if (!fileName.isEmpty()) {
		auto clsStrModel = "org/apache/jena/rdf/model/Model";
		auto clsStrModelFactory = "org/apache/jena/rdf/model/ModelFactory";
		auto clsStrOntModel = "org/apache/jena/ontology/OntModel";
		auto clsStrOntModelSpec = "org/apache/jena/ontology/OntModelSpec";
		auto clsStrString = "java/lang/String";

		auto clsModelFactory = env_->FindClass(clsStrModelFactory);
		auto clsModel = env_->FindClass(clsStrModel);
		auto clsOntModelSpec = env_->FindClass(clsStrOntModelSpec);

		auto midCreateOntologyModel = env_->GetStaticMethodID(clsModelFactory, "createOntologyModel", ('(' + getClsSig(clsStrOntModelSpec) + ')' + getClsSig(clsStrOntModel)).c_str());
		auto midRead = env_->GetMethodID(clsModel, "read", ('(' + getClsSig(clsStrString) + ')' + getClsSig(clsStrModel)).c_str());

		auto ontModel = env_->CallStaticObjectMethod(clsModelFactory, midCreateOntologyModel, env_->GetStaticObjectField(clsOntModelSpec, env_->GetStaticFieldID(clsOntModelSpec, "OWL_DL_MEM", getClsSig(clsStrOntModelSpec).c_str())));
		auto tmpStr = env_->NewStringUTF(fileName.toUtf8().data());
		env_->CallVoidMethod(ontModel, midRead, tmpStr);
		env_->ReleaseStringUTFChars(tmpStr, env_->GetStringUTFChars(tmpStr, nullptr));
		if (queryRpds(env_, ontModel, rpds_)) {
			justLoadedRpd_ = true;
			if (baseImage_.data) {
				updateDesign();
				updateViewer();
			}
		}
		else
			QMessageBox::critical(this, tr("Error"), tr("Not a Valid Ontology!"));
	}
}

void RpdDesign::onRemedyImageChanged(const bool& thisRemedyImage) {
	remedyImage = thisRemedyImage;
	ui_.baseCheckBox->setEnabled(!remedyImage);
	if (baseImage_.data)
		updateViewer();
}

void RpdDesign::onShowBaseChanged(const bool& showBaseImage) {
	showBaseImage_ = showBaseImage;
	if (baseImage_.data)
		updateViewer();
}

void RpdDesign::onShowDesignChanged(const bool& showDesignImage) {
	showDesignImage_ = showDesignImage;
	if (baseImage_.data)
		updateViewer();
}

void RpdDesign::saveDesign() {
	auto& curImage = rpdViewer_->getCurImage();
	if (curImage.data) {
		auto selectedFilter = tr("Portable Network Graphics (*.png)");
		auto fileName = QFileDialog::getSaveFileName(this, tr("Select Save Path"), "", tr("Windows bitmaps (*.bmp *.dib);;JPEG files (*.jpeg *.jpg *.jpe);;JPEG 2000 files (*.jp2);;Portable Network Graphics (*.png);;Portable image format (*.pbm *.pgm *.ppm);;Sun rasters (*.sr *.ras);;TIFF files (*.tiff *.tif)"), &selectedFilter);
		if (!fileName.isEmpty())
			imwrite(fileName.toLocal8Bit().data(), curImage);
	}
	else
		QMessageBox::critical(this, tr("Error"), tr("No Available Design!"));
}

void RpdDesign::switchLanguage(bool* const& isEnglish) {
	isEnglish_ = isEnglish ? *isEnglish : !isEnglish_;
	QApplication::installTranslator(&(isEnglish_ ? engTranslator_ : chsTranslator_));
}
