#pragma once

#include "ui_RpdDesign.h"

class RpdViewer;

class RpdDesign : public QWidget {
	Q_OBJECT
public:
	explicit RpdDesign(QWidget* const& parent = nullptr);
	~RpdDesign();
private:
	RpdViewer* rpdViewer_;
	Ui::RpdDesignClass ui_;
};
