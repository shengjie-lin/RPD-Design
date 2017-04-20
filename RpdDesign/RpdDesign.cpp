#include "RpdDesign.h"
#include "RpdViewer.h"

RpdDesign::RpdDesign(QWidget* const& parent) : QWidget(parent) {
	ui_.setupUi(this);
	rpdViewer_ = new RpdViewer(this, ui_.baseCheckBox->isChecked(), ui_.designCheckBox->isChecked());
	ui_.verticalLayout->insertWidget(0, rpdViewer_);
	remedyImage = ui_.remedyCheckBox->isChecked();
	connect(ui_.loadBasePushButton, SIGNAL(clicked()), rpdViewer_, SLOT(loadBaseImage()));
	connect(ui_.loadRpdPushButton, SIGNAL(clicked()), rpdViewer_, SLOT(loadRpdInfo()));
	connect(ui_.savePushButton, SIGNAL(clicked()), rpdViewer_, SLOT(saveDesign()));
	connect(ui_.remedyCheckBox, SIGNAL(toggled(bool)), this, SLOT(onRemedyImageChanged(const bool&)));
	connect(ui_.baseCheckBox, SIGNAL(toggled(bool)), rpdViewer_, SLOT(onShowBaseChanged(const bool&)));
	connect(ui_.designCheckBox, SIGNAL(toggled(bool)), rpdViewer_, SLOT(onShowDesignChanged(const bool&)));
}

RpdDesign::~RpdDesign() { delete rpdViewer_; }

void RpdDesign::onRemedyImageChanged(const bool& thisRemedyImage) const {
	remedyImage = thisRemedyImage;
	ui_.baseCheckBox->setEnabled(!thisRemedyImage);
	rpdViewer_->onRemedyImageChanged();
}
