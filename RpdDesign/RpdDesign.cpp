#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QFileDialog>
#include <QMessageBox>

#include "RpdDesign.h"
#include "resource.h"
#include "RpdViewer.h"
#include "Tooth.h"
#include "Utilities.h"

string RpdDesign::jenaLibPath = "D:/Utilities/apache-jena-3.2.0/lib/";

RpdDesign::RpdDesign(QWidget* const& parent) : QWidget(parent) {
	ui_.setupUi(this);
	rpdViewer_ = new RpdViewer(this);
	ui_.verticalLayout->insertWidget(0, rpdViewer_);
	setMinimumSize(600, 600);
	remedyImage = ui_.remedyCheckBox->isChecked();
	showBaseImage_ = ui_.baseCheckBox->isChecked();
	showDesignImage_ = ui_.designCheckBox->isChecked();
	chsTranslator_.load(":/qrc/rpddesign_zh.qm");
	engTranslator_.load(":/qrc/rpddesign_en.qm");
	switchLanguage(&isEnglish_);
	connect(ui_.switchLanguagePushButton, SIGNAL(clicked()), this, SLOT(switchLanguage()));
	connect(ui_.loadBasePushButton, SIGNAL(clicked()), this, SLOT(loadBaseImage()));
	connect(ui_.loadDefaultBasePushButton, SIGNAL(clicked()), this, SLOT(loadDefaultBaseImage()));
	connect(ui_.loadRpdPushButton, SIGNAL(clicked()), this, SLOT(loadRpdInfo()));
	connect(ui_.saveDesignPushButton, SIGNAL(clicked()), this, SLOT(saveDesign()));
	connect(ui_.remedyCheckBox, SIGNAL(toggled(bool)), this, SLOT(onRemedyImageChanged(bool const&)));
	connect(ui_.baseCheckBox, SIGNAL(toggled(bool)), this, SLOT(onShowBaseChanged(bool const&)));
	connect(ui_.designCheckBox, SIGNAL(toggled(bool)), this, SLOT(onShowDesignChanged(bool const&)));
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
		ui_.loadDefaultBasePushButton->setText(tr("Load Default Base"));
		ui_.loadRpdPushButton->setText(tr("Load RPD"));
		ui_.saveDesignPushButton->setText(tr("Save Design"));
	}
	else
		QWidget::changeEvent(event);
}

void RpdDesign::updateViewer() {
	auto const& curImage = !remedyImage && showBaseImage_ ? baseImage_.clone() : Mat((remedyImage ? remediedDesignImages_[0].size : designImages_[0].size)(), CV_8UC3, Scalar::all(255));
	if (showDesignImage_) {
		auto designImages = remedyImage ? remediedDesignImages_ : designImages_;
		Mat designImage;
		bitwise_and(designImages[0], designImages[1], designImage);
		cvtColor(designImage, designImage, COLOR_GRAY2BGR);
		bitwise_and(designImage, curImage, curImage);
	}
	rpdViewer_->setCurImage(curImage);
}

void RpdDesign::analyzeAndUpdate(Mat const& base) {
	analyzeBaseImage(base, remediedTeeth_, remediedDesignImages_, &teeth_, &designImages_, &baseImage_);
	updateDesign(teeth_, rpds_, designImages_, true, justLoadedRpds_);
	updateDesign(remediedTeeth_, rpds_, remediedDesignImages_, true, justLoadedRpds_);
	justLoadedRpds_ = false;
	updateViewer();
}

void RpdDesign::loadBaseImage() {
	auto const& fileName = QFileDialog::getOpenFileName(this, tr("Select Base Image"), "", tr("All supported formats (*.bmp *.dib *.jpeg *.jpg *.jpe *.jp2 *.png *.pbm *.pgm *.ppm *.sr *.ras *.tiff *.tif);;Windows bitmaps (*.bmp *.dib);;JPEG files (*.jpeg *.jpg *.jpe);;JPEG 2000 files (*.jp2);;Portable Network Graphics (*.png);;Portable image format (*.pbm *.pgm *.ppm);;Sun rasters (*.sr *.ras);;TIFF files (*.tiff *.tif)"));
	if (!fileName.isEmpty()) {
		auto const& image = imread(fileName.toLocal8Bit().data());
		if (image.empty())
			QMessageBox::critical(this, tr("Error"), tr("Not a Valid Image!"));
		else
			analyzeAndUpdate(image);
	}
}

void RpdDesign::loadDefaultBaseImage() {
	auto const& hRsrc = FindResource(nullptr, MAKEINTRESOURCE(IDB_PNG1), TEXT("PNG"));
	auto const& pBuf = static_cast<uchar*>(LockResource(LoadResource(nullptr, hRsrc)));
	analyzeAndUpdate(imdecode(vector<uchar>(pBuf, pBuf + SizeofResource(nullptr, hRsrc)), IMREAD_COLOR));
}

void RpdDesign::loadRpdInfo() {
	auto const& fileName = QFileDialog::getOpenFileName(this, tr("Select RPD Information"), "", tr("Ontology files (*.owl)"));
	if (!fileName.isEmpty()) {
		auto const& clsStrModel = "org/apache/jena/rdf/model/Model";
		auto const& clsStrModelFactory = "org/apache/jena/rdf/model/ModelFactory";
		auto const& clsStrOntModel = "org/apache/jena/ontology/OntModel";
		auto const& clsStrOntModelSpec = "org/apache/jena/ontology/OntModelSpec";
		auto const& clsStrString = "java/lang/String";
		auto const& clsModelFactory = env_->FindClass(clsStrModelFactory);
		auto const& clsModel = env_->FindClass(clsStrModel);
		auto const& clsOntModelSpec = env_->FindClass(clsStrOntModelSpec);
		auto const& midCreateOntologyModel = env_->GetStaticMethodID(clsModelFactory, "createOntologyModel", ('(' + getClsSig(clsStrOntModelSpec) + ')' + getClsSig(clsStrOntModel)).c_str());
		auto const& midRead = env_->GetMethodID(clsModel, "read", ('(' + getClsSig(clsStrString) + ')' + getClsSig(clsStrModel)).c_str());
		auto const& ontModel = env_->CallStaticObjectMethod(clsModelFactory, midCreateOntologyModel, env_->GetStaticObjectField(clsOntModelSpec, env_->GetStaticFieldID(clsOntModelSpec, "OWL_DL_MEM", getClsSig(clsStrOntModelSpec).c_str())));
		auto const& tmpStr = env_->NewStringUTF(fileName.toUtf8().data());
		env_->CallVoidMethod(ontModel, midRead, tmpStr);
		env_->DeleteLocalRef(tmpStr);
		if (queryRpds(env_, ontModel, rpds_))
			if (baseImage_.data) {
				updateDesign(teeth_, rpds_, designImages_, false, true);
				updateDesign(remediedTeeth_, rpds_, remediedDesignImages_, false, true);
				updateViewer();
			}
			else
				justLoadedRpds_ = true;
		else
			QMessageBox::critical(this, tr("Error"), tr("Not a Valid Ontology!"));
	}
}

void RpdDesign::onRemedyImageChanged(bool const& thisRemedyImage) {
	remedyImage = thisRemedyImage;
	ui_.baseCheckBox->setEnabled(!remedyImage);
	if (baseImage_.data)
		updateViewer();
}

void RpdDesign::onShowBaseChanged(bool const& showBaseImage) {
	showBaseImage_ = showBaseImage;
	if (baseImage_.data)
		updateViewer();
}

void RpdDesign::onShowDesignChanged(bool const& showDesignImage) {
	showDesignImage_ = showDesignImage;
	if (baseImage_.data)
		updateViewer();
}

void RpdDesign::saveDesign() {
	auto& curImage = rpdViewer_->getCurImage();
	if (curImage.data) {
		auto const& fileName = QFileDialog::getSaveFileName(this, tr("Select Save Path"), "", tr("Windows bitmaps (*.bmp *.dib);;JPEG files (*.jpeg *.jpg *.jpe);;JPEG 2000 files (*.jp2);;Portable Network Graphics (*.png);;Portable image format (*.pbm *.pgm *.ppm);;Sun rasters (*.sr *.ras);;TIFF files (*.tiff *.tif)"), new QString(tr("Portable Network Graphics (*.png)")));
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
