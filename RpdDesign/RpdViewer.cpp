#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <QFileDialog>
#include <QMessageBox>

#include "RpdViewer.h"
#include "Tooth.h"
#include "Utilities.h"

map<string, RpdViewer::RpdClass> RpdViewer::rpdMapping_ = {
	{"Aker_clasp", AKER_CLASP},
	{"combination_clasp", COMBINATION_CLASP},
	{"combined_clasp", COMBINED_CLASP},
	{"continuous_clasp", CONTINUOUS_CLASP},
	{"denture_base", DENTURE_BASE},
	{"edentulous_space", EDENTULOUS_SPACE},
	{"full_palatal_plate", FULL_PALATAL_PLATE},
	{"lingual_rest", LINGUAL_REST},
	{"occlusal_rest", OCCLUSAL_REST},
	{"palatal_plate", PALATAL_PLATE},
	{"ring_clasp", RING_CLASP},
	{"RPA_clasps", RPA},
	{"RPI_clasps", RPI},
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
	if (!justLoadedImage_)
		for (auto zone = 0; zone < nZones; ++zone)
			for (auto ordinal = 0; ordinal < nTeethPerZone + 1; ++ordinal)
				teeth_[zone][ordinal].setLingualBlockage(RpdAsLingualBlockage::NONE);
	if (justLoadedRpd_) {
		bool hasLingualConfrontations[nZones][nTeethPerZone] = {};
		for (auto rpd = rpds_.begin(); rpd < rpds_.end(); ++rpd) {
			auto dentureBase = dynamic_cast<DentureBase*>(*rpd);
			if (dentureBase)
				dentureBase->determineTailsCoverage(isEighthToothUsed_);
			auto rpdWithLingualConfrontations = dynamic_cast<RpdWithLingualConfrontations*>(*rpd);
			if (rpdWithLingualConfrontations)
				rpdWithLingualConfrontations->registerLingualConfrontations(hasLingualConfrontations);
		}
		for (auto rpd = rpds_.begin(); rpd < rpds_.end(); ++rpd) {
			auto rpdWithLingualClaspArms = dynamic_cast<RpdWithLingualClaspArms*>(*rpd);
			if (rpdWithLingualClaspArms)
				rpdWithLingualClaspArms->setLingualClaspArms(hasLingualConfrontations);
		}
	}
	for (auto rpd = rpds_.begin(); rpd < rpds_.end(); ++rpd) {
		auto rpdAsLingualBlockage = dynamic_cast<RpdAsLingualBlockage*>(*rpd);
		if (rpdAsLingualBlockage)
			rpdAsLingualBlockage->registerLingualBlockages(teeth_);
	}
	justLoadedRpd_ = justLoadedImage_ = false;
	designImages_[1] = Mat(qSizeToSize(imageSize_), CV_8U, 255);
	for (auto zone = 0; zone < nZones; ++zone) {
		if (isEighthToothUsed_[zone])
			polylines(designImages_[1], teeth_[zone][nTeethPerZone].getContour(), true, 0, lineThicknessOfLevel[0], LINE_AA);
	}
	for (auto rpd = rpds_.begin(); rpd < rpds_.end(); ++rpd)
		(*rpd)->draw(designImages_[1], teeth_);
}

void RpdViewer::resizeEvent(QResizeEvent* event) {
	QLabel::resizeEvent(event);
	if (baseImage_.data)
		refreshDisplay();
}

void RpdViewer::refreshDisplay() {
	auto curImage = showBaseImage_ ? baseImage_.clone() : Mat(qSizeToSize(imageSize_), CV_8UC3, Scalar::all(255));
	if (showDesignImage_) {
		Mat designImage;
		bitwise_and(designImages_[0], designImages_[1], designImage);
		cvtColor(designImage, designImage, COLOR_GRAY2BGR);
		bitwise_and(designImage, curImage, curImage);
	}
	cv::resize(curImage, curImage, qSizeToSize(imageSize_.scaled(size(), Qt::KeepAspectRatio)));
	setPixmap(matToQPixmap(curImage));
}

void RpdViewer::loadBaseImage() {
	auto fileName = QFileDialog::getOpenFileName(this, u8"选择图像", "", "All supported formats (*.bmp *.dib *.jpeg *.jpg *.jpe *.jp2 *.png *.pbm *.pgm *.ppm *.sr *.ras *.tiff *.tif);;Windows bitmaps (*.bmp *.dib);;JPEG files (*.jpeg *.jpg *.jpe);;JPEG 2000 files (*.jp2);;Portable Network Graphics (*.png);;Portable image format (*.pbm *.pgm *.ppm);;Sun rasters (*.sr *.ras);;TIFF files (*.tiff *.tif)");
	if (!fileName.isEmpty()) {
		auto image = imread(fileName.toLocal8Bit().data());
		if (image.empty())
			QMessageBox::information(this, u8"错误", u8"无效的图像文件！");
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
			auto nTeeth = tmpTeeth.size();
			vector<Point2f> centroids(nTeeth);
			for (auto i = 0; i < nTeeth; ++i)
				centroids[i] = tmpTeeth[i].getCentroid();
			teethEllipse = fitEllipse(centroids);
			vector<float> angles(nTeeth);
			for (auto i = 0; i < nTeeth; ++i)
				tmpTeeth[i].setNormalDirection(computeNormalDirection(centroids[i], &angles[i]));
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
				for (auto ordinal = 0; ordinal < nTeethPerZone; ++ordinal) {
					auto& tooth = teeth_[zone][ordinal];
					if (ordinal == nTeethPerZone - 1)
						teeth_[zone].push_back(tooth);
					tooth.findAnglePoints(zone);
					polylines(designImages_[0], tooth.getContour(), true, 0, lineThicknessOfLevel[0], LINE_AA);
				}
				auto &seventhTooth = teeth_[zone][nTeethPerZone - 1], &eighthTooth = teeth_[zone][nTeethPerZone];
				auto contour = eighthTooth.getContour();
				auto translation = roundToInt(rotate(computeNormalDirection(seventhTooth.getAnglePoint(180)), CV_PI * (zone % 2 - 0.5)) * seventhTooth.getRadius() * 2.1);
				for (auto point = contour.begin(); point < contour.end(); ++point)
					*point += translation;
				eighthTooth.setContour(contour);
				auto eighthCentroid = eighthTooth.getCentroid();
				auto theta = asin(computeNormalDirection(seventhTooth.getCentroid()).cross(computeNormalDirection(eighthCentroid)));
				for (auto point = contour.begin(); point < contour.end(); ++point)
					*point = eighthCentroid + rotate(static_cast<Point2f>(*point) - eighthCentroid, theta);
				eighthTooth.setContour(contour);
				eighthTooth.findAnglePoints(zone);
			}
			updateRpdDesign();
			refreshDisplay();
		}
	}
}

void RpdViewer::loadRpdInfo() {
	auto fileName = QFileDialog::getOpenFileName(this, u8"选择RPD信息", "", "Ontology files (*.owl)");
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
		tmpStr = env_->NewStringUTF((ontPrefix + "component_position").c_str());
		auto opComponentPosition = env_->CallObjectMethod(ontModel, midModelConGetProperty, tmpStr);
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
		bool isEighthToothUsed[nZones] = {};
		auto individuals = env_->CallObjectMethod(ontModel, midListIndividuals);
		while (env_->CallBooleanMethod(individuals, midHasNext)) {
			auto individual = env_->CallObjectMethod(individuals, midNext);
			auto ontClassStr = env_->GetStringUTFChars(static_cast<jstring>(env_->CallObjectMethod(env_->CallObjectMethod(individual, midGetOntClass), midGetLocalName)), nullptr);
			switch (rpdMapping_[ontClassStr]) {
				case AKER_CLASP:
					rpds.push_back(AkerClasp::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspTipDirection, dpClaspMaterial, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
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
					rpds.push_back(RingClasp::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspMaterial, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case RPA:
					rpds.push_back(Rpa::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspMaterial, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case RPI:
					rpds.push_back(Rpi::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				case WW_CLASP:
					rpds.push_back(WwClasp::createFromIndividual(env_, midGetInt, midHasNext, midListProperties, midNext, midResourceGetProperty, midStatementGetProperty, dpClaspTipDirection, dpToothZone, dpToothOrdinal, opComponentPosition, individual, isEighthToothUsed));
					break;
				default: ;
			}
		}
		if (rpds.size()) {
			justLoadedRpd_ = true;
			copy(begin(isEighthToothUsed), end(isEighthToothUsed), isEighthToothUsed_);
			rpds_ = rpds;
			if (baseImage_.data) {
				updateRpdDesign();
				refreshDisplay();
			}
		}
		else
			QMessageBox::information(this, u8"警告", u8"文件中不存在有效的RPD实例！");
	}
}

void RpdViewer::onShowBaseChanged(bool showBaseImage) {
	showBaseImage_ = showBaseImage;
	if (baseImage_.data)
		refreshDisplay();
}

void RpdViewer::onShowDesignChanged(bool showDesignImage) {
	showDesignImage_ = showDesignImage;
	if (baseImage_.data)
		refreshDisplay();
}
