#pragma once

#include <QTranslator>

#include "ui_RpdDesign.h"

class RpdViewer;

class RpdDesign : public QWidget {
	Q_OBJECT
public:
	explicit RpdDesign(QWidget* const& parent = nullptr);
	~RpdDesign();
private:
	void changeEvent(QEvent* event) override;
	bool isEnglish_ = true;
	QTranslator chsTranslator_, engTranslator_;
	RpdViewer* rpdViewer_;
	Ui::RpdDesignClass ui_;
private slots:
	void onRemedyImageChanged(const bool& thisRemedyImage) const;
	void switchLanguage(bool* const& isEnglish = nullptr);
};
