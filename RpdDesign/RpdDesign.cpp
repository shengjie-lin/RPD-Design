#include "RpdDesign.h"

RpdDesign::RpdDesign(QWidget* parent): QWidget(parent) {
	ui.setupUi(this);
	this->showMaximized();
	rpdViewer_ = new RpdViewer(this, ui.baseCheckBox->isChecked(), ui.designCheckBox->isChecked());
	ui.verticalLayout->insertWidget(0, rpdViewer_);
	connect(ui.loadBasePushButton, SIGNAL(clicked()), rpdViewer_, SLOT(loadBaseImage()));
	connect(ui.loadRpdPushButton, SIGNAL(clicked()), rpdViewer_, SLOT(loadRpdInfo()));
	connect(ui.baseCheckBox, SIGNAL(toggled(bool)), rpdViewer_, SLOT(onShowBaseChanged(bool)));
	connect(ui.designCheckBox, SIGNAL(toggled(bool)), rpdViewer_, SLOT(onShowDesignChanged(bool)));
}
