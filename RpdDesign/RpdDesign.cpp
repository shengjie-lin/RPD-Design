#include "RpdDesign.h"
#include "RpdViewer.h"

RpdDesign::RpdDesign(QWidget* const& parent) : QWidget(parent) {
	ui_.setupUi(this);
	this->showMaximized();
	rpdViewer_ = new RpdViewer(this, ui_.baseCheckBox->isChecked(), ui_.designCheckBox->isChecked());
	ui_.verticalLayout->insertWidget(0, rpdViewer_);
	connect(ui_.loadBasePushButton, SIGNAL(clicked()), rpdViewer_, SLOT(loadBaseImage()));
	connect(ui_.loadRpdPushButton, SIGNAL(clicked()), rpdViewer_, SLOT(loadRpdInfo()));
	connect(ui_.baseCheckBox, SIGNAL(toggled(bool)), rpdViewer_, SLOT(onShowBaseChanged(bool)));
	connect(ui_.designCheckBox, SIGNAL(toggled(bool)), rpdViewer_, SLOT(onShowDesignChanged(bool)));
}

RpdDesign::~RpdDesign() { delete rpdViewer_; }
