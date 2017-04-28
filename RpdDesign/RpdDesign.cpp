#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QFileDialog>
#include <QMessageBox>

#include "RpdDesign.h"
#include "RpdViewer.h"
#include "Tooth.h"
#include "Utilities.h"

map<string, RpdDesign::RpdClass> RpdDesign::rpdMapping_ = {
	{"aker_clasp", AKERS_CLASP},
	{"canine_aker_clasp", CANINE_AKERS_CLASP},
	{"combination_anterior_posterior_palatal_strap", COMBINATION_ANTERIOR_POSTERIOR_PALATAL_STRAP},
	{"combination_clasp", COMBINATION_CLASP},
	{"combined_clasp", COMBINED_CLASP},
	{"continuous_clasp", CONTINUOUS_CLASP},
	{"denture_base", DENTURE_BASE},
	{"edentulous_space", EDENTULOUS_SPACE},
	{"full_palatal_plate", FULL_PALATAL_PLATE},
	{"lingual_bar", LINGUAL_BAR},
	{"lingual_plate", LINGUAL_PLATE},
	{"lingual_rest", LINGUAL_REST},
	{"modified_palatal_plate", PALATAL_PLATE},
	{"occlusal_rest", OCCLUSAL_REST},
	{"palatal_plate", PALATAL_PLATE},
	{"ring_clasp", RING_CLASP},
	{"RPA_clasps", RPA},
	{"RPI_clasps", RPI},
	{"single_palatal_strap", PALATAL_PLATE},
	{"tooth", TOOTH},
	{"wrought_wire_clasp", WW_CLASP}
};

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
	auto& imageSize = remedyImage ? remediedImageSize_ : imageSize_;
	auto curImage = !remedyImage && showBaseImage_ ? baseImage_.clone() : Mat(qSizeToSize(imageSize), CV_8UC3, Scalar::all(255));
	if (showDesignImage_) {
		auto designImages = remedyImage ? remediedDesignImages_ : designImages_;
		Mat designImage;
		bitwise_and(designImages[0], designImages[1], designImage);
		cvtColor(designImage, designImage, COLOR_GRAY2BGR);
		bitwise_and(designImage, curImage, curImage);
	}
	rpdViewer_->setCurImage(curImage);
}

void RpdDesign::updateRpdDesign() {
	for (auto i = 0; i < 2; ++i) {
		auto teeth = i ? remediedTeeth_ : teeth_;
		auto designImages = i ? remediedDesignImages_ : designImages_;
		auto& imageSize = i ? remediedImageSize_ : imageSize_;
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
		designImages[1] = Mat(qSizeToSize(imageSize), CV_8U, 255);
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
		if (image.empty())
			QMessageBox::critical(this, tr("Error"), tr("Not a Valid Image!"));
		else {
			justLoadedImage_ = true;
			copyMakeBorder(image, baseImage_, 80, 80, 80, 80, BORDER_CONSTANT, Scalar::all(255));
			imageSize_ = sizeToQSize(baseImage_.size());
			Mat tmpImage;
			cvtColor(baseImage_, tmpImage, COLOR_BGR2GRAY);
			threshold(tmpImage, tmpImage, 0, 255, THRESH_BINARY | THRESH_OTSU);
			vector<vector<Point>> contours;
			vector<Tooth> tmpTeeth;
			vector<Vec4i> hierarchy;
			findContours(tmpImage, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
			for (auto i = hierarchy[0][2]; i >= 0; i = hierarchy[i][0])
				for (auto j = hierarchy[i][2]; j >= 0; j = hierarchy[j][0])
					tmpTeeth.push_back(Tooth(contours[j]));
			vector<Point2f> centroids;
			for (auto tooth = tmpTeeth.begin(); tooth < tmpTeeth.end(); ++tooth)
				centroids.push_back(tooth->getCentroid());
			teethEllipse = fitEllipse(centroids);
			auto nTeeth = (nTeethPerZone - 1) * nZones;
			vector<float> angles(nTeeth);
			auto oldRemedyImage = remedyImage;
			remedyImage = false;
			for (auto i = 0; i < nTeeth; ++i)
				computeNormalDirection(centroids[i], &angles[i]);
			vector<int> idx;
			sortIdx(angles, idx, SORT_ASCENDING);
			vector<vector<uint8_t>> isInZone(nZones);
			for (auto i = 0; i < nZones; ++i) {
				inRange(angles, CV_2PI / nZones * (i - 2), CV_2PI / nZones * (i - 1), isInZone[i]);
				teeth_[i].clear();
			}
			for (auto i = 0; i < nTeeth; ++i) {
				auto no = idx[i];
				for (auto j = 0; j < nZones; ++j)
					if (isInZone[j][no]) {
						if (j % 2)
							teeth_[j].push_back(tmpTeeth[no]);
						else
							teeth_[j].insert(teeth_[j].begin(), tmpTeeth[no]);
						break;
					}
			}
			designImages_[0] = Mat(qSizeToSize(imageSize_), CV_8U, 255);
			for (auto zone = 0; zone < nZones; ++zone) {
				for (auto ordinal = 0; ordinal < nTeethPerZone - 1; ++ordinal) {
					auto& tooth = teeth_[zone][ordinal];
					if (ordinal == nTeethPerZone - 2)
						teeth_[zone].push_back(tooth);
					polylines(designImages_[0], tooth.getContour(), true, 0, lineThicknessOfLevel[0], LINE_AA);
				}
				auto &seventhTooth = teeth_[zone][nTeethPerZone - 2], &eighthTooth = teeth_[zone][nTeethPerZone - 1];
				auto translation = roundToPoint(rotate(computeNormalDirection(seventhTooth.getAnglePoint(180)), CV_PI * (zone % 2 - 0.5)) * seventhTooth.getRadius() * 2.1);
				auto contour = eighthTooth.getContour();
				for (auto point = contour.begin(); point < contour.end(); ++point)
					*point += translation;
				eighthTooth.setContour(contour);
				centroids.push_back(eighthTooth.getCentroid());
			}
			teethEllipse = fitEllipse(centroids);
			auto theta = degreeToRadian(teethEllipse.angle);
			auto direction = rotate(Point(0, 1), theta);
			float distance = 0;
			for (auto zone = 0; zone < nZones; ++zone)
				distance += teeth_[zone][nTeethPerZone - 1].getRadius();
			(distance *= 2) /= 3;
			auto translation = roundToPoint(direction * distance);
			remediedImageSize_ = imageSize_ + QSize(0, distance * cos(theta));
			centroids.clear();
			for (auto zone = 0; zone < nZones; ++zone) {
				auto& teeth = teeth_[zone];
				auto& remediedTeeth = remediedTeeth_[zone];
				remediedTeeth.clear();
				for (auto ordinal = 0; ordinal < nTeethPerZone; ++ordinal) {
					auto& tooth = teeth[ordinal];
					tooth.setNormalDirection(computeNormalDirection(tooth.getCentroid()));
					if (ordinal == nTeethPerZone - 1) {
						auto contour = tooth.getContour();
						auto centroid = tooth.getCentroid();
						theta = asin(teeth[ordinal - 1].getNormalDirection().cross(tooth.getNormalDirection()));
						for (auto point = contour.begin(); point < contour.end(); ++point)
							*point = centroid + rotate(static_cast<Point2f>(*point) - centroid, theta);
						tooth.setContour(contour);
					}
					auto remediedTooth = tooth;
					if (zone >= nZones / 2) {
						auto contour = remediedTooth.getContour();
						for (auto point = contour.begin(); point < contour.end(); ++point)
							*point += translation;
						remediedTooth.setContour(contour);
					}
					centroids.push_back(remediedTooth.getCentroid());
					remediedTeeth.push_back(remediedTooth);
					tooth.findAnglePoints(zone);
				}
			}
			remediedTeethEllipse = fitEllipse(centroids);
			theta = degreeToRadian(-remediedTeethEllipse.angle);
			remediedTeethEllipse.angle = 0;
			remediedDesignImages_[0] = Mat(qSizeToSize(remediedImageSize_), CV_8U, 255);
			remedyImage = true;
			for (auto zone = 0; zone < nZones; ++zone) {
				for (auto ordinal = 0; ordinal < nTeethPerZone; ++ordinal) {
					auto& tooth = remediedTeeth_[zone][ordinal];
					auto contour = tooth.getContour();
					if (ordinal < nTeethPerZone - 1)
						polylines(remediedDesignImages_[0], contour, true, 0, lineThicknessOfLevel[0], LINE_AA);
					for (auto point = contour.begin(); point < contour.end(); ++point)
						*point = remediedTeethEllipse.center + rotate(static_cast<Point2f>(*point) - remediedTeethEllipse.center, theta);
					tooth.setContour(contour);
					tooth.setNormalDirection(computeNormalDirection(tooth.getCentroid()));
					tooth.findAnglePoints(zone);
				}
			}
			remedyImage = oldRemedyImage;
			updateRpdDesign();
			updateViewer();
		}
	}
}

void RpdDesign::loadRpdInfo() {
	auto fileName = QFileDialog::getOpenFileName(this, tr("Select RPD Information"), "", tr("Ontology files (*.owl)"));
	if (!fileName.isEmpty()) {
		auto clsStrExtendedIterator = "org/apache/jena/util/iterator/ExtendedIterator";
		auto clsStrIndividual = "org/apache/jena/ontology/Individual";
		auto clsStrIterator = "java/util/Iterator";
		auto clsStrModel = "org/apache/jena/rdf/model/Model";
		auto clsStrModelCon = "org/apache/jena/rdf/model/ModelCon";
		auto clsStrModelFactory = "org/apache/jena/rdf/model/ModelFactory";
		auto clsStrObject = "java/lang/Object";
		auto clsStrOntClass = "org/apache/jena/ontology/OntClass";
		auto clsStrOntModel = "org/apache/jena/ontology/OntModel";
		auto clsStrOntModelSpec = "org/apache/jena/ontology/OntModelSpec";
		auto clsStrProperty = "org/apache/jena/rdf/model/Property";
		auto clsStrResource = "org/apache/jena/rdf/model/Resource";
		auto clsStrStatement = "org/apache/jena/rdf/model/Statement";
		auto clsStrStmtIterator = "org/apache/jena/rdf/model/StmtIterator";
		auto clsStrString = "java/lang/String";

		auto clsIndividual = env_->FindClass(clsStrIndividual);
		auto clsIterator = env_->FindClass(clsStrIterator);
		auto clsModelCon = env_->FindClass(clsStrModelCon);
		auto clsModelFactory = env_->FindClass(clsStrModelFactory);
		auto clsModel = env_->FindClass(clsStrModel);
		auto clsOntModel = env_->FindClass(clsStrOntModel);
		auto clsOntModelSpec = env_->FindClass(clsStrOntModelSpec);
		auto clsResource = env_->FindClass(clsStrResource);
		auto clsStatement = env_->FindClass(clsStrStatement);

		auto midCreateOntologyModel = env_->GetStaticMethodID(clsModelFactory, "createOntologyModel", ('(' + getClsSig(clsStrOntModelSpec) + ')' + getClsSig(clsStrOntModel)).c_str());
		auto midGetBoolean = env_->GetMethodID(clsStatement, "getBoolean", "()Z");
		auto midGetInt = env_->GetMethodID(clsStatement, "getInt", "()I");
		auto midGetLocalName = env_->GetMethodID(clsResource, "getLocalName", ("()" + getClsSig(clsStrString)).c_str());
		auto midGetOntClass = env_->GetMethodID(clsIndividual, "getOntClass", ("()" + getClsSig(clsStrOntClass)).c_str());
		auto midHasNext = env_->GetMethodID(clsIterator, "hasNext", "()Z");
		auto midListIndividuals = env_->GetMethodID(clsOntModel, "listIndividuals", ("()" + getClsSig(clsStrExtendedIterator)).c_str());
		auto midListProperties = env_->GetMethodID(clsResource, "listProperties", ('(' + getClsSig(clsStrProperty) + ')' + getClsSig(clsStrStmtIterator)).c_str());
		auto midModelConGetProperty = env_->GetMethodID(clsModelCon, "getProperty", ('(' + getClsSig(clsStrString) + ')' + getClsSig(clsStrProperty)).c_str());
		auto midNext = env_->GetMethodID(clsIterator, "next", ("()" + getClsSig(clsStrObject)).c_str());
		auto midRead = env_->GetMethodID(clsModel, "read", ('(' + getClsSig(clsStrString) + ')' + getClsSig(clsStrModel)).c_str());
		auto midResourceGetProperty = env_->GetMethodID(clsResource, "getProperty", ('(' + getClsSig(clsStrProperty) + ')' + getClsSig(clsStrStatement)).c_str());
		auto midStatementGetProperty = env_->GetMethodID(clsStatement, "getProperty", ('(' + getClsSig(clsStrProperty) + ')' + getClsSig(clsStrStatement)).c_str());

		auto ontModel = env_->CallStaticObjectMethod(clsModelFactory, midCreateOntologyModel, env_->GetStaticObjectField(clsOntModelSpec, env_->GetStaticFieldID(clsOntModelSpec, "OWL_DL_MEM", getClsSig(clsStrOntModelSpec).c_str())));
		auto tmpStr = env_->NewStringUTF(fileName.toUtf8().data());
		env_->CallVoidMethod(ontModel, midRead, tmpStr);
		env_->ReleaseStringUTFChars(tmpStr, env_->GetStringUTFChars(tmpStr, nullptr));

		string ontPrefix = "http://www.semanticweb.org/msiip/ontologies/CDSSinRPD#";
		tmpStr = env_->NewStringUTF((ontPrefix + "clasp_material").c_str());
		auto dpClaspMaterial = env_->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
		env_->ReleaseStringUTFChars(tmpStr, env_->GetStringUTFChars(tmpStr, nullptr));
		tmpStr = env_->NewStringUTF((ontPrefix + "clasp_tip_direction").c_str());
		auto dpClaspTipDirection = env_->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
		env_->ReleaseStringUTFChars(tmpStr, env_->GetStringUTFChars(tmpStr, nullptr));
		tmpStr = env_->NewStringUTF((ontPrefix + "clasp_tip_side").c_str());
		auto dpClaspTipSide = env_->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
		env_->ReleaseStringUTFChars(tmpStr, env_->GetStringUTFChars(tmpStr, nullptr));
		tmpStr = env_->NewStringUTF((ontPrefix + "component_position").c_str());
		auto opComponentPosition = env_->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
		env_->ReleaseStringUTFChars(tmpStr, env_->GetStringUTFChars(tmpStr, nullptr));
		tmpStr = env_->NewStringUTF((ontPrefix + "enable_buccal_arm").c_str());
		auto dpEnableBuccalArm = env_->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
		env_->ReleaseStringUTFChars(tmpStr, env_->GetStringUTFChars(tmpStr, nullptr));
		tmpStr = env_->NewStringUTF((ontPrefix + "enable_lingual_arm").c_str());
		auto dpEnableLingualArm = env_->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
		env_->ReleaseStringUTFChars(tmpStr, env_->GetStringUTFChars(tmpStr, nullptr));
		tmpStr = env_->NewStringUTF((ontPrefix + "enable_rest").c_str());
		auto dpEnableRest = env_->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
		env_->ReleaseStringUTFChars(tmpStr, env_->GetStringUTFChars(tmpStr, nullptr));
		tmpStr = env_->NewStringUTF((ontPrefix + "is_missing").c_str());
		auto dpIsMissing = env_->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
		env_->ReleaseStringUTFChars(tmpStr, env_->GetStringUTFChars(tmpStr, nullptr));
		tmpStr = env_->NewStringUTF((ontPrefix + "lingual_confrontation").c_str());
		auto dpLingualConfrontation = env_->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
		env_->ReleaseStringUTFChars(tmpStr, env_->GetStringUTFChars(tmpStr, nullptr));
		tmpStr = env_->NewStringUTF((ontPrefix + "rest_mesial_or_distal").c_str());
		auto dpRestMesialOrDistal = env_->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
		env_->ReleaseStringUTFChars(tmpStr, env_->GetStringUTFChars(tmpStr, nullptr));
		tmpStr = env_->NewStringUTF((ontPrefix + "tooth_ordinal").c_str());
		auto dpToothOrdinal = env_->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
		env_->ReleaseStringUTFChars(tmpStr, env_->GetStringUTFChars(tmpStr, nullptr));
		tmpStr = env_->NewStringUTF((ontPrefix + "tooth_zone").c_str());
		auto dpToothZone = env_->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
		env_->ReleaseStringUTFChars(tmpStr, env_->GetStringUTFChars(tmpStr, nullptr));
		vector<Rpd*> rpds;
		auto individuals = env_->CallObjectMethod(ontModel, midListIndividuals);
		auto isValid = env_->CallBooleanMethod(individuals, midHasNext);
		bool isEighthToothUsed[nZones] = {};
		while (env_->CallBooleanMethod(individuals, midHasNext)) {
			auto individual = env_->CallObjectMethod(individuals, midNext);
			auto ontClassStr = env_->GetStringUTFChars(static_cast<jstring>(env_->CallObjectMethod(env_->CallObjectMethod(individual, midGetOntClass), midGetLocalName)), nullptr);
			switch (rpdMapping_[ontClassStr]) {
				case AKERS_CLASP:
					rpds.push_back(AkersClasp::createFromIndividual(env_, midGetBoolean, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspTipDirection, dpClaspMaterial, dpEnableBuccalArm, dpEnableLingualArm, dpEnableRest, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case CANINE_AKERS_CLASP:
					rpds.push_back(CanineAkersClasp::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspTipDirection, dpClaspMaterial, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case COMBINATION_ANTERIOR_POSTERIOR_PALATAL_STRAP:
					rpds.push_back(CombinationAnteriorPosteriorPalatalStrap::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case COMBINATION_CLASP:
					rpds.push_back(CombinationClasp::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspTipDirection, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case COMBINED_CLASP:
					rpds.push_back(CombinedClasp::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspMaterial, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case CONTINUOUS_CLASP:
					rpds.push_back(ContinuousClasp::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspMaterial, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case DENTURE_BASE:
					rpds.push_back(DentureBase::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case EDENTULOUS_SPACE:
					rpds.push_back(EdentulousSpace::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case FULL_PALATAL_PLATE:
					rpds.push_back(FullPalatalPlate::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case LINGUAL_BAR:
					rpds.push_back(LingualBar::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case LINGUAL_PLATE:
					rpds.push_back(LingualPlate::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case LINGUAL_REST:
					rpds.push_back(LingualRest::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpRestMesialOrDistal, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case OCCLUSAL_REST:
					rpds.push_back(OcclusalRest::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpRestMesialOrDistal, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case PALATAL_PLATE:
					rpds.push_back(PalatalPlate::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case RING_CLASP:
					rpds.push_back(RingClasp::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspMaterial, dpClaspTipSide, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case RPA:
					rpds.push_back(Rpa::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspMaterial, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case RPI:
					rpds.push_back(Rpi::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case TOOTH:
					if (!env_->CallBooleanMethod(env_->CallObjectMethod(individual, midResourceGetProperty, dpIsMissing), midGetBoolean) && env_->CallIntMethod(env_->CallObjectMethod(individual, midResourceGetProperty, dpToothOrdinal), midGetInt) == nTeethPerZone)
						isEighthToothUsed[env_->CallIntMethod(env_->CallObjectMethod(individual, midResourceGetProperty, dpToothZone), midGetInt) - 1] = true;
					break;
				case WW_CLASP:
					rpds.push_back(WwClasp::createFromIndividual(env_, midGetBoolean, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspTipDirection, dpEnableBuccalArm, dpEnableLingualArm, dpEnableRest, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				default: ;
			}
		}
		if (isValid) {
			justLoadedRpd_ = true;
			copy(begin(isEighthToothUsed), end(isEighthToothUsed), Tooth::isEighthUsed);
			rpds_ = rpds;
			if (baseImage_.data) {
				updateRpdDesign();
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
