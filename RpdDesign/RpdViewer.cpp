#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <QFileDialog>
#include <QMessageBox>

#include "RpdViewer.h"
#include "Tooth.h"
#include "Utilities.h"

map<string, RpdViewer::RpdClass> RpdViewer::rpdMapping_ = {
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

RpdViewer::RpdViewer(QWidget* const& parent, const bool& showBaseImage, const bool& showContoursImage) : QLabel(parent), showBaseImage_(showBaseImage), showDesignImage_(showContoursImage) {
	setAlignment(Qt::AlignCenter);
	setMinimumSize(128, 128);
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

RpdViewer::~RpdViewer() {
	for (auto rpd = rpds_.begin(); rpd < rpds_.end(); ++rpd)
		delete *rpd;
	vm_->DestroyJavaVM();
}

void RpdViewer::updateRpdDesign() {
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

void RpdViewer::resizeEvent(QResizeEvent* event) {
	QLabel::resizeEvent(event);
	if (baseImage_.data)
		refreshDisplay(false);
}

void RpdViewer::refreshDisplay(const bool& updateCurImage) {
	auto& imageSize = remedyImage ? remediedImageSize_ : imageSize_;
	if (updateCurImage) {
		curImage_ = !remedyImage && showBaseImage_ ? baseImage_.clone() : Mat(qSizeToSize(imageSize), CV_8UC3, Scalar::all(255));
		auto designImages = remedyImage ? remediedDesignImages_ : designImages_;
		if (showDesignImage_) {
			Mat designImage;
			bitwise_and(designImages[0], designImages[1], designImage);
			cvtColor(designImage, designImage, COLOR_GRAY2BGR);
			bitwise_and(designImage, curImage_, curImage_);
		}
	}
	auto curImage = curImage_.clone();
	cv::resize(curImage, curImage, qSizeToSize(imageSize.scaled(size(), Qt::KeepAspectRatio)));
	setPixmap(matToQPixmap(curImage));
}

void RpdViewer::loadBaseImage() {
	auto fileName = QFileDialog::getOpenFileName(this, tr("Select Base Image"), "", "All supported formats (*.bmp *.dib *.jpeg *.jpg *.jpe *.jp2 *.png *.pbm *.pgm *.ppm *.sr *.ras *.tiff *.tif);;Windows bitmaps (*.bmp *.dib);;JPEG files (*.jpeg *.jpg *.jpe);;JPEG 2000 files (*.jp2);;Portable Network Graphics (*.png);;Portable image format (*.pbm *.pgm *.ppm);;Sun rasters (*.sr *.ras);;TIFF files (*.tiff *.tif)");
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
			auto direction = rotate(Point(0, 1), degreeToRadian(teethEllipse.angle));
			float sumOfRadii = 0;
			for (auto zone = 0; zone < nZones; ++zone)
				sumOfRadii += teeth_[zone][nTeethPerZone - 1].getRadius();
			(sumOfRadii *= 2) /= 3;
			auto translation = roundToPoint(direction * sumOfRadii);
			remediedImageSize_ = imageSize_ + QSize(0, sumOfRadii);
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
						auto theta = asin(teeth[ordinal - 1].getNormalDirection().cross(tooth.getNormalDirection()));
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
			auto theta = degreeToRadian(-remediedTeethEllipse.angle);
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
			refreshDisplay();
		}
	}
}

void RpdViewer::loadRpdInfo() {
	auto fileName = QFileDialog::getOpenFileName(this, tr("Select RPD Information"), "", "Ontology files (*.owl)");
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
				refreshDisplay();
			}
		}
		else
			QMessageBox::critical(this, tr("Error"), tr("Not a Valid Ontology!"));
	}
}

void RpdViewer::saveDesign() {
	if (curImage_.data) {
		QString selectedFilter = "Portable Network Graphics (*.png)";
		auto fileName = QFileDialog::getSaveFileName(this, tr("Select Save Path"), "", "Windows bitmaps (*.bmp *.dib);;JPEG files (*.jpeg *.jpg *.jpe);;JPEG 2000 files (*.jp2);;Portable Network Graphics (*.png);;Portable image format (*.pbm *.pgm *.ppm);;Sun rasters (*.sr *.ras);;TIFF files (*.tiff *.tif)", &selectedFilter);
		if (!fileName.isEmpty())
			imwrite(fileName.toLocal8Bit().data(), curImage_);
	}
	else
		QMessageBox::critical(this, tr("Error"), tr("No Available Design!"));
}

void RpdViewer::onRemedyImageChanged() {
	if (baseImage_.data)
		refreshDisplay();
}

void RpdViewer::onShowBaseChanged(const bool& showBaseImage) {
	showBaseImage_ = showBaseImage;
	if (baseImage_.data)
		refreshDisplay();
}

void RpdViewer::onShowDesignChanged(const bool& showDesignImage) {
	showDesignImage_ = showDesignImage;
	if (baseImage_.data)
		refreshDisplay();
}
