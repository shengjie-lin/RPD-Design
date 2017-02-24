#pragma once

#include "ui_RpdDesign.h"
#include "RpdViewer.h"

class RpdDesign: public QWidget {
	Q_OBJECT

public:
	explicit RpdDesign(QWidget* parent = nullptr);

private:
	Ui::RpdDesignClass ui;
	RpdViewer* rpdViewer_;
};
