#include "RpdDesign.h"
#include "RpdViewer.h"

RpdDesign::RpdDesign(QWidget* const& parent) : QWidget(parent) {
	ui_.setupUi(this);
	remedyImage = ui_.remedyCheckBox->isChecked();
	rpdViewer_ = new RpdViewer(this, ui_.baseCheckBox->isChecked(), ui_.designCheckBox->isChecked());
	ui_.verticalLayout->insertWidget(0, rpdViewer_);
	chsTranslator_.load("rpddesign_zh.qm");
	engTranslator_.load("rpddesign_en.qm");
	switchLanguage(&isEnglish_);
	connect(ui_.switchLanguagePushButton, SIGNAL(clicked()), this, SLOT(switchLanguage()));
	connect(ui_.loadBasePushButton, SIGNAL(clicked()), rpdViewer_, SLOT(loadBaseImage()));
	connect(ui_.loadRpdPushButton, SIGNAL(clicked()), rpdViewer_, SLOT(loadRpdInfo()));
	connect(ui_.saveDesignPushButton, SIGNAL(clicked()), rpdViewer_, SLOT(saveDesign()));
	connect(ui_.remedyCheckBox, SIGNAL(toggled(bool)), this, SLOT(onRemedyImageChanged(const bool&)));
	connect(ui_.baseCheckBox, SIGNAL(toggled(bool)), rpdViewer_, SLOT(onShowBaseChanged(const bool&)));
	connect(ui_.designCheckBox, SIGNAL(toggled(bool)), rpdViewer_, SLOT(onShowDesignChanged(const bool&)));
}

RpdDesign::~RpdDesign() { delete rpdViewer_; }

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

void RpdDesign::onRemedyImageChanged(const bool& thisRemedyImage) const {
	remedyImage = thisRemedyImage;
	ui_.baseCheckBox->setEnabled(!thisRemedyImage);
	rpdViewer_->onRemedyImageChanged();
}

void RpdDesign::switchLanguage(bool* const& isEnglish) {
	isEnglish_ = isEnglish ? *isEnglish : !isEnglish_;
	QApplication::installTranslator(&(isEnglish_ ? engTranslator_ : chsTranslator_));
}
