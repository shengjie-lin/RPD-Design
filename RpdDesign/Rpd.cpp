#include <opencv2/imgproc.hpp>

#include "Rpd.h"
#include "Tooth.h"
#include "Utilities.h"

Rpd::Position::Position(const int& zone, const int& ordinal) : zone(zone), ordinal(ordinal) {}

RpdWithPositions::RpdWithPositions(const vector<Position>& positions) : positions_(positions) {}

void RpdWithPositions::queryPositions(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, vector<Position>& positions, bool isEighthToothUsed[nZones], const bool& autoComplete) {
	auto teeth = env->CallObjectMethod(individual, midListProperties, opComponentPosition);
	auto count = 0;
	while (env->CallBooleanMethod(teeth, midHasNext)) {
		auto tooth = env->CallObjectMethod(teeth, midNext);
		auto zone = env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothZone), midGetInt) - 1;
		auto ordinal = env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothOrdinal), midGetInt) - 1;
		positions.push_back(Position(zone, ordinal));
		if (ordinal == nTeethPerZone)
			isEighthToothUsed[zone] = true;
		++count;
	}
	if (count == 1 && autoComplete)
		positions.push_back(positions[0]);
	else
	if (count == 2 && (positions[0].zone > positions[1].zone || positions[0].zone == positions[1].zone && positions[0].ordinal > positions[1].ordinal)) { swap(positions[0], positions[1]); }
}

RpdWithMaterial::RpdWithMaterial(const Material& material) : material_(material) {}

void RpdWithMaterial::queryMaterial(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspMaterial, const jobject& individual, Material& claspMaterial) {
	auto tmp = env->CallObjectMethod(individual, midResourceGetProperty, dpClaspMaterial);
	if (tmp)
		claspMaterial = static_cast<Material>(env->CallIntMethod(tmp, midGetInt));
	else
		claspMaterial = CAST;
}

RpdWithDirection::RpdWithDirection(const Direction& direction) : direction_(direction) {}

void RpdWithDirection::queryDirection(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspTipDirection, const jobject& individual, Direction& claspTipDirection) { claspTipDirection = static_cast<Direction>(env->CallIntMethod(env->CallObjectMethod(individual, midResourceGetProperty, dpClaspTipDirection), midGetInt)); }

void RpdWithLingualBlockage::registerLingualBlockage(vector<Tooth> teeth[nZones], const vector<Position>& positions, const LingualBlockage& lingualBlockage) {
	if (positions.size() == 1)
		getTooth(teeth, positions[0]).setLingualBlockage(lingualBlockage);
	else if (positions[0].zone == positions[1].zone)
		for (auto ordinal = positions[0].ordinal; ordinal <= positions[1].ordinal; ++ordinal) {
			auto& tooth = teeth[positions[0].zone][ordinal];
			if (lingualBlockage != MAJOR_CONNECTOR || tooth.getLingualBlockage() == NONE)
				tooth.setLingualBlockage(lingualBlockage);
		}
	else {
		auto step = -1;
		for (auto zone = positions[0].zone, ordinal = positions[0].ordinal; zone == positions[0].zone || ordinal <= positions[1].ordinal; ordinal += step) {
			auto& tooth = teeth[positions[0].zone][ordinal];
			if (lingualBlockage != MAJOR_CONNECTOR || tooth.getLingualBlockage() == NONE)
				tooth.setLingualBlockage(lingualBlockage);
			if (ordinal == 0)
				if (step) {
					step = 0;
					++zone;
				}
				else
					step = 1;
		}
	}

}

RpdAsClasp::RpdAsClasp(const deque<bool>& hasLingualArms) : hasLingualArms_(hasLingualArms) {}

void RpdAsClasp::setLingualArm(bool hasLingualConfrontations[nZones][nTeethPerZone], const vector<Position>& positions, deque<bool>& hasLingualArms) {
	for (auto i = 0; i < positions.size(); ++i)
		hasLingualArms[i] = !hasLingualConfrontations[positions[i].zone][positions[i].ordinal];
}

void RpdAsClasp::registerLingualBlockage(vector<Tooth> teeth[nZones], const vector<Position>& positions, const deque<bool>& hasLingualArms) {
	for (auto i = 0; i < positions.size(); ++i)
		if (hasLingualArms[i])
			RpdWithLingualBlockage::registerLingualBlockage(teeth, {positions[i]}, CLASP);
}

RpdAsMajorConnector::Anchor::Anchor(const Position& position, const RpdWithDirection::Direction& direction) : position(position), direction(direction) {}

RpdAsMajorConnector::RpdAsMajorConnector(const vector<Anchor>& anchors) : anchors_(anchors) {}

void RpdAsMajorConnector::queryAnchors(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpAnchorMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& opMajorConnectorAnchor, const jobject& individual, const Coverage& coverage, vector<Anchor>& anchors, bool isEighthToothUsed[nZones]) {
	auto count = 0;
	auto majorConnectorAnchors = env->CallObjectMethod(individual, midListProperties, opMajorConnectorAnchor);
	while (env->CallBooleanMethod(majorConnectorAnchors, midHasNext)) {
		auto anchor = env->CallObjectMethod(majorConnectorAnchors, midNext);
		auto tooth = env->CallObjectMethod(anchor, midStatementGetProperty, opComponentPosition);
		auto zone = env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothZone), midGetInt) - 1;
		auto ordinal = env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothOrdinal), midGetInt) - 1;
		anchors.push_back(Anchor(Position(zone, ordinal), static_cast<RpdWithDirection::Direction>(env->CallIntMethod(env->CallObjectMethod(anchor, midStatementGetProperty, dpAnchorMesialOrDistal), midGetInt))));
		if (ordinal == nTeethPerZone)
			isEighthToothUsed[zone] = true;
		++count;
	}
	for (auto i = 0; i < count - 1; ++i)
		for (auto j = i + 1; j < count; ++j)
			if (anchors[i].position.zone > anchors[j].position.zone)
				swap(anchors[i], anchors[j]);
	if (coverage == LINEAR) {
		if (anchors[0].position.zone == anchors[1].position.zone && anchors[0].position.ordinal > anchors[1].position.ordinal)
			swap(anchors[0], anchors[1]);
	}
	else {
		if (count == 2)
			anchors.insert(anchors.begin() + 1, {Anchor(Position(anchors[0].position.zone)),Anchor(Position(anchors[1].position.zone))});
		else if (count == 3) {
			if (anchors[0].position.zone == anchors[1].position.zone) {
				if (anchors[0].position.ordinal > anchors[1].position.ordinal)
					swap(anchors[0], anchors[1]);
				anchors.insert(anchors.begin() + 2, Anchor(Position(anchors[2].position.zone)));
			}
			else {
				anchors.insert(anchors.begin() + 1, Anchor(Position(anchors[0].position.zone)));
				if (anchors[2].position.ordinal < anchors[3].position.ordinal)
					swap(anchors[2], anchors[3]);
			}
		}
		else {
			if (anchors[0].position.ordinal > anchors[1].position.ordinal)
				swap(anchors[0], anchors[1]);
			if (anchors[2].position.ordinal < anchors[3].position.ordinal)
				swap(anchors[2], anchors[3]);
		}
	}
}

void RpdAsMajorConnector::handleEighthTooth(const bool isEighthToothMissing[nZones], bool isEighthToothUsed[nZones]) {
	for (auto i = 1; i < 3; ++i) {
		auto& position = anchors_[i].position;
		auto& zone = position.zone;
		auto& ordinal = position.ordinal;
		if (ordinal == -1)
			if (isEighthToothMissing[zone])
				ordinal = nTeethPerZone - 1;
			else {
				ordinal = nTeethPerZone;
				isEighthToothUsed[zone] = true;
			}
	}
}

void RpdAsMajorConnector::registerLingualBlockage(vector<Tooth> teeth[nZones]) const {
	RpdWithLingualBlockage::registerLingualBlockage(teeth, {anchors_[0].position, anchors_[1].position}, MAJOR_CONNECTOR);
	if (anchors_.size() == 4)
		RpdWithLingualBlockage::registerLingualBlockage(teeth, {anchors_[3].position, anchors_[2].position}, MAJOR_CONNECTOR);
}

void RpdWithOcclusalRest::registerOcclusalRest(vector<Tooth> teeth[nZones], const Position& position, const RpdWithDirection::Direction& direction) { getTooth(teeth, position).setOcclusalRest(direction); }

RpdWithLingualConfrontations::RpdWithLingualConfrontations(const vector<Anchor>& anchors, const vector<Position>& lingualConfrontations) : RpdAsMajorConnector(anchors), lingualConfrontations_(lingualConfrontations) {}

void RpdWithLingualConfrontations::registerLingualConfrontations(bool hasLingualConfrontations[nZones][nTeethPerZone]) const {
	for (auto position = lingualConfrontations_.begin(); position < lingualConfrontations_.end(); ++position)
		hasLingualConfrontations[position->zone][position->ordinal] = true;
}

void RpdWithLingualConfrontations::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	for (auto position = lingualConfrontations_.begin(); position < lingualConfrontations_.end(); ++position)
		Plating({*position}).draw(designImage, teeth);
}

void RpdWithLingualConfrontations::queryLingualConfrontations(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& individual, vector<Position>& lingualConfrontations) {
	auto lcTeeth = env->CallObjectMethod(individual, midListProperties, dpLingualConfrontation);
	while (env->CallBooleanMethod(lcTeeth, midHasNext)) {
		auto tooth = env->CallObjectMethod(lcTeeth, midNext);
		lingualConfrontations.push_back(Position(env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothZone), midGetInt) - 1, env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothOrdinal), midGetInt) - 1));
	}
}

AkerClasp::AkerClasp(const vector<Position>& positions, const Material& material, const Direction& direction) : RpdWithPositions(positions), RpdWithMaterial(material), RpdWithDirection(direction), RpdAsClasp(deque<bool>(1)) {}

AkerClasp* AkerClasp::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Direction claspTipDirection;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new AkerClasp(positions, claspMaterial, claspTipDirection);
}

void AkerClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	if (hasLingualArms_[0])
		Clasp(positions_, material_, direction_).draw(designImage, teeth);
	else
		HalfClasp(positions_, material_, direction_, HalfClasp::BUCCAL).draw(designImage, teeth);
	OcclusalRest(positions_, static_cast<Direction>(1 - direction_)).draw(designImage, teeth);
}

void AkerClasp::registerOcclusalRest(vector<Tooth> teeth[nZones]) const { RpdWithOcclusalRest::registerOcclusalRest(teeth, positions_[0], static_cast<Direction>(1 - direction_)); }

void AkerClasp::setLingualArm(bool hasLingualConfrontations[nZones][nTeethPerZone]) { RpdAsClasp::setLingualArm(hasLingualConfrontations, positions_, hasLingualArms_); }

void AkerClasp::registerLingualBlockage(vector<Tooth> teeth[nZones]) const { RpdAsClasp::registerLingualBlockage(teeth, positions_, hasLingualArms_); }

CombinationClasp::CombinationClasp(const vector<Position>& positions, const Direction& direction) : RpdWithPositions(positions), RpdWithDirection(direction), RpdAsClasp(deque<bool>(1)) {}

CombinationClasp* CombinationClasp::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Direction claspTipDirection;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	return new CombinationClasp(positions, claspTipDirection);
}

void CombinationClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	HalfClasp(positions_, RpdWithMaterial::WROUGHT_WIRE, direction_, HalfClasp::BUCCAL).draw(designImage, teeth);
	if (hasLingualArms_[0])
		HalfClasp(positions_, RpdWithMaterial::CAST, direction_, HalfClasp::LINGUAL).draw(designImage, teeth);
	OcclusalRest(positions_, static_cast<Direction>(1 - direction_)).draw(designImage, teeth);
}

void CombinationClasp::registerOcclusalRest(vector<Tooth> teeth[nZones]) const { RpdWithOcclusalRest::registerOcclusalRest(teeth, positions_[0], static_cast<Direction>(1 - direction_)); }

void CombinationClasp::setLingualArm(bool hasLingualConfrontations[nZones][nTeethPerZone]) { RpdAsClasp::setLingualArm(hasLingualConfrontations, positions_, hasLingualArms_); }

void CombinationClasp::registerLingualBlockage(vector<Tooth> teeth[nZones]) const { RpdAsClasp::registerLingualBlockage(teeth, positions_, hasLingualArms_); }

CombinedClasp::CombinedClasp(const vector<Position>& positions, const Material& material) : RpdWithPositions(positions), RpdWithMaterial(material), RpdAsClasp(deque<bool>(2)) {}

CombinedClasp* CombinedClasp::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new CombinedClasp(positions, claspMaterial);
}

void CombinedClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	auto tipDirection = positions_[0].zone == positions_[1].zone ? RpdWithDirection::MESIAL : RpdWithDirection::DISTAL;
	if (hasLingualArms_[0])
		Clasp({positions_[0]}, material_, tipDirection).draw(designImage, teeth);
	else
		HalfClasp({positions_[0]}, material_, tipDirection, HalfClasp::BUCCAL).draw(designImage, teeth);
	OcclusalRest({positions_[0]}, static_cast<RpdWithDirection::Direction>(1 - tipDirection)).draw(designImage, teeth);
	if (hasLingualArms_[1])
		Clasp({positions_[1]}, material_, RpdWithDirection::DISTAL).draw(designImage, teeth);
	else
		HalfClasp({positions_[1]}, material_, RpdWithDirection::DISTAL, HalfClasp::BUCCAL).draw(designImage, teeth);
	OcclusalRest({positions_[1]}, RpdWithDirection::MESIAL).draw(designImage, teeth);
}

void CombinedClasp::registerOcclusalRest(vector<Tooth> teeth[nZones]) const {
	RpdWithOcclusalRest::registerOcclusalRest(teeth, {positions_[0]}, positions_[0].zone == positions_[1].zone ? RpdWithDirection::DISTAL : RpdWithDirection::MESIAL);
	RpdWithOcclusalRest::registerOcclusalRest(teeth, {positions_[1]}, RpdWithDirection::MESIAL);
}

void CombinedClasp::setLingualArm(bool hasLingualConfrontations[nZones][nTeethPerZone]) { RpdAsClasp::setLingualArm(hasLingualConfrontations, positions_, hasLingualArms_); }

void CombinedClasp::registerLingualBlockage(vector<Tooth> teeth[nZones]) const { RpdAsClasp::registerLingualBlockage(teeth, positions_, hasLingualArms_); }

DentureBase::DentureBase(const vector<Position>& positions) : RpdWithPositions(positions) {}

DentureBase* DentureBase::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed, true);
	return new DentureBase(positions);
}

void DentureBase::determineTailsCoverage(const bool isEighthToothUsed[nZones]) {
	for (auto i = 0; i < 2; ++i)
		if (positions_[i].ordinal == nTeethPerZone - 1 + isEighthToothUsed[positions_[i].zone])
			coversTails_[i] = true;
}

void DentureBase::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	vector<Point> curve;
	float avgRadius;
	bool haslingualBlockage;
	computeStringCurve(teeth, positions_, curve, avgRadius, &haslingualBlockage);
	if (coversTails_[0]) {
		Point2f distalPoint = getTooth(teeth, positions_[0]).getAnglePoint(180);
		curve[0] = roundToInt(distalPoint + rotate(computeNormalDirection(distalPoint), -CV_PI / 2) * avgRadius);
	}
	if (coversTails_[1]) {
		Point2f distalPoint = getTooth(teeth, positions_[1]).getAnglePoint(180);
		curve.back() = roundToInt(distalPoint + rotate(computeNormalDirection(distalPoint), CV_PI * (positions_[1].zone % 2 - 0.5)) * avgRadius);
	}
	if (coversTails_[0] || coversTails_[1] || !haslingualBlockage) {
		vector<Point> tmpCurve(curve.size());
		for (auto i = 0; i < curve.size(); ++i) {
			auto delta = roundToInt(computeNormalDirection(curve[i]) * avgRadius * 1.75);
			tmpCurve[i] = curve[i] + delta;
			curve[i] -= delta;
		}
		curve.insert(curve.end(), tmpCurve.rbegin(), tmpCurve.rend());
		computeSmoothCurve(curve, curve, true);
		polylines(designImage, curve, true, 0, lineThicknessOfLevel[2], LINE_AA);
	}
	else {
		curve.insert(curve.begin(), curve[0]);
		curve.push_back(curve.back());
		for (auto i = 1; i < curve.size() - 1; ++i)
			curve[i] += roundToInt(computeNormalDirection(curve[i]) * avgRadius * 1.75);
		computeSmoothCurve(curve, curve);
		polylines(designImage, curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	}
}

void DentureBase::registerLingualBlockage(vector<Tooth> teeth[nZones]) const {
	if (coversTails_[0] || coversTails_[1])
		RpdWithLingualBlockage::registerLingualBlockage(teeth, positions_, DENTURE_BASE);
}

EdentulousSpace::EdentulousSpace(const vector<Position>& positions) : RpdWithPositions(positions) {}

EdentulousSpace* EdentulousSpace::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed, true);
	return new EdentulousSpace(positions);
}

void EdentulousSpace::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	vector<Point> curve;
	float avgRadius;
	computeStringCurve(teeth, positions_, curve, avgRadius);
	vector<Point> curve1(curve.size()), curve2(curve.size());
	for (auto i = 0; i < curve.size(); ++i) {
		auto delta = roundToInt(computeNormalDirection(curve[i]) * avgRadius / 4);
		curve1[i] = curve[i] + delta;
		curve2[i] = curve[i] - delta;
	}
	computeSmoothCurve(curve1, curve1);
	computeSmoothCurve(curve2, curve2);
	polylines(designImage, curve1, false, 0, lineThicknessOfLevel[2], LINE_AA);
	polylines(designImage, curve2, false, 0, lineThicknessOfLevel[2], LINE_AA);
}

LingualRest::LingualRest(const vector<Position>& positions, const Direction& direction) : RpdWithPositions(positions), RpdWithDirection(direction) {}

LingualRest* LingualRest::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Direction restMesialOrDistal;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpRestMesialOrDistal, individual, restMesialOrDistal);
	return new LingualRest(positions, restMesialOrDistal);
}

void LingualRest::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	auto curve = getTooth(teeth, positions_[0]).getCurve(240, 300);
	polylines(designImage, curve, true, 0, lineThicknessOfLevel[1], LINE_AA);
	fillPoly(designImage, vector<vector<Point>>{curve}, 0, LINE_AA);
}

PalatalPlate::PalatalPlate(const vector<Anchor>& anchors, const vector<Position>& lingualConfrontations) : RpdWithLingualConfrontations(anchors, lingualConfrontations) {}

PalatalPlate* PalatalPlate::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpAnchorMesialOrDistal, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& opMajorConnectorAnchor, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Anchor> anchors;
	vector<Position> lingualConfrontations;
	queryAnchors(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpAnchorMesialOrDistal, dpToothZone, dpToothOrdinal, opComponentPosition, opMajorConnectorAnchor, individual, PLANAR, anchors, isEighthToothUsed);
	queryLingualConfrontations(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, individual, lingualConfrontations);
	return new PalatalPlate(anchors, lingualConfrontations);
}

void PalatalPlate::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	/*TODO :  consider lingual blockage; ugly!*/
	RpdWithLingualConfrontations::draw(designImage, teeth);
	vector<Point> curve, mesialCurve = {getPoint(teeth, anchors_[3])}, distalCurve = {getPoint(teeth, anchors_[1])};
	auto delta = anchors_[3].position.ordinal - anchors_[0].position.ordinal + (anchors_[3].direction - anchors_[0].direction) * 0.9;
	if (delta < -0.5 || delta > 0)
		mesialCurve.push_back(getPoint(teeth, anchors_[3]) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[3], 1)) * getTooth(teeth, anchors_[3]).getRadius() * (1 + max(delta, .0))));
	mesialCurve.insert(mesialCurve.end(), delta > 0 ? initializer_list<Point>{roundToInt((getTooth(teeth, anchors_[3], 1).getCentroid() + getTooth(teeth, anchors_[3], 1, true).getCentroid()) / 2), getPoint(teeth, anchors_[3], 0, true) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[3], 1, true)) * getTooth(teeth, anchors_[3], true).getRadius() * (1 + delta))} : delta < 0 ? initializer_list<Point>{getPoint(teeth, anchors_[0], 0, true) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[0], 1, true)) * getTooth(teeth, anchors_[0], true).getRadius() * (1 - delta)), roundToInt((getTooth(teeth, anchors_[0], 1).getCentroid() + getTooth(teeth, anchors_[0], 1, true).getCentroid()) / 2)} : initializer_list<Point>{roundToInt((getTooth(teeth, anchors_[3], 1).getCentroid() + getTooth(teeth, anchors_[0], 1).getCentroid()) / 2)});
	if (delta < 0 || delta > 0.5)
		mesialCurve.push_back(getPoint(teeth, anchors_[0]) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[0], 1)) * getTooth(teeth, anchors_[0]).getRadius() * (1 - min(delta, .0))));
	mesialCurve.push_back(getPoint(teeth, anchors_[0]));
	computeSmoothCurve(mesialCurve, mesialCurve);
	delta = anchors_[2].position.ordinal - anchors_[1].position.ordinal + (anchors_[2].direction - anchors_[1].direction) * 0.9;
	if (delta < -0.5 || delta > 0)
		distalCurve.push_back(getPoint(teeth, anchors_[1]) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[1], -1)) * getTooth(teeth, anchors_[1]).getRadius() * (1 + max(delta, .0))));
	distalCurve.insert(distalCurve.end(), delta > 0 ? initializer_list<Point>{roundToInt((getTooth(teeth, anchors_[1], -1).getCentroid() + getTooth(teeth, anchors_[1], -1, true).getCentroid()) / 2), getPoint(teeth, anchors_[1], 0, true) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[1], -1, true)) * getTooth(teeth, anchors_[1], true).getRadius() * (1 + delta))} : delta < 0 ? initializer_list<Point>{getPoint(teeth, anchors_[2], 0, true) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[2], -1, true)) * getTooth(teeth, anchors_[2], true).getRadius() * (1 - delta)), roundToInt((getTooth(teeth, anchors_[2], -1).getCentroid() + getTooth(teeth, anchors_[2], -1, true).getCentroid()) / 2)} : initializer_list<Point>{roundToInt((getTooth(teeth, anchors_[1], -1).getCentroid() + getTooth(teeth, anchors_[2], -1).getCentroid()) / 2)});
	if (delta < 0 || delta > 0.5)
		distalCurve.push_back(getPoint(teeth, anchors_[2]) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[2], -1)) * getTooth(teeth, anchors_[2]).getRadius() * (1 - min(delta, .0))));
	distalCurve.push_back(getPoint(teeth, anchors_[2]));
	computeSmoothCurve(distalCurve, distalCurve);
	for (auto ordinal = anchors_[0].position.ordinal; ordinal <= anchors_[1].position.ordinal; ++ordinal) {
		if (ordinal == anchors_[0].position.ordinal && anchors_[0].direction == RpdWithDirection::DISTAL) {
			curve.push_back(getPoint(teeth, anchors_[0]));
			continue;
		}
		if (ordinal == anchors_[1].position.ordinal && anchors_[1].direction == RpdWithDirection::MESIAL) {
			curve.push_back(getPoint(teeth, anchors_[1]));
			continue;
		}
		auto thisCurve = teeth[anchors_[0].position.zone][ordinal].getCurve(180, 0);
		curve.insert(curve.end(), thisCurve.rbegin(), thisCurve.rend());
	}
	curve.insert(curve.end(), distalCurve.begin(), distalCurve.end());
	for (auto ordinal = anchors_[2].position.ordinal; ordinal >= anchors_[3].position.ordinal; --ordinal) {
		if (ordinal == anchors_[2].position.ordinal && anchors_[2].direction == RpdWithDirection::MESIAL) {
			curve.push_back(getPoint(teeth, anchors_[2]));
			continue;
		}
		if (ordinal == anchors_[3].position.ordinal && anchors_[3].direction == RpdWithDirection::DISTAL) {
			curve.push_back(getPoint(teeth, anchors_[3]));
			continue;
		}
		auto thisCurve = teeth[anchors_[2].position.zone][ordinal].getCurve(180, 0);
		curve.insert(curve.end(), thisCurve.begin(), thisCurve.end());
	}
	curve.insert(curve.end(), mesialCurve.begin(), mesialCurve.end());
	auto thisDesign = Mat(designImage.size(), CV_8U, 255);
	fillPoly(thisDesign, vector<vector<Point>>{curve}, 128, LINE_AA);
	bitwise_and(thisDesign, designImage, designImage);
	polylines(designImage, mesialCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	polylines(designImage, distalCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
}

RingClasp::RingClasp(const vector<Position>& positions, const Material& material) : RpdWithPositions(positions), RpdWithMaterial(material), RpdAsClasp(deque<bool>(1)) {}

RingClasp* RingClasp::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new RingClasp(positions, claspMaterial);
}

void RingClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	auto isUpper = positions_[0].zone < nZones / 2;
	auto curve = getTooth(teeth, positions_[0]).getCurve(isUpper ? 60 : 0, hasLingualArms_[0] ? (isUpper ? 0 : 300) : 180);
	OcclusalRest(positions_, RpdWithDirection::MESIAL).draw(designImage, teeth);
	if (material_ == CAST) {
		polylines(designImage, curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
		OcclusalRest(positions_, RpdWithDirection::DISTAL).draw(designImage, teeth);
	}
	else
		polylines(designImage, curve, false, 0, lineThicknessOfLevel[1], LINE_AA);
}

void RingClasp::registerOcclusalRest(vector<Tooth> teeth[nZones]) const {
	RpdWithOcclusalRest::registerOcclusalRest(teeth, positions_[0], RpdWithDirection::MESIAL);
	if (material_ == CAST)
		RpdWithOcclusalRest::registerOcclusalRest(teeth, positions_[0], RpdWithDirection::DISTAL);
}

void RingClasp::registerLingualBlockage(vector<Tooth> teeth[nZones]) const { RpdAsClasp::registerLingualBlockage(teeth, positions_, hasLingualArms_); }

void RingClasp::setLingualArm(bool hasLingualConfrontations[nZones][nTeethPerZone]) { RpdAsClasp::setLingualArm(hasLingualConfrontations, positions_, hasLingualArms_); }

Rpa::Rpa(const vector<Position>& positions, const Material& material) : RpdWithPositions(positions), RpdWithMaterial(material) {}

Rpa* Rpa::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new Rpa(positions, claspMaterial);
}

void Rpa::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	OcclusalRest(positions_, RpdWithDirection::MESIAL).draw(designImage, teeth);
	GuidingPlate(positions_).draw(designImage, teeth);
	HalfClasp(positions_, material_, RpdWithDirection::MESIAL, HalfClasp::BUCCAL).draw(designImage, teeth);
}

void Rpa::registerOcclusalRest(vector<Tooth> teeth[nZones]) const { RpdWithOcclusalRest::registerOcclusalRest(teeth, positions_[0], RpdWithDirection::MESIAL); }

Rpi::Rpi(const vector<Position>& positions) : RpdWithPositions(positions) {}

Rpi* Rpi::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	return new Rpi(positions);
}

void Rpi::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	OcclusalRest(positions_, RpdWithDirection::MESIAL).draw(designImage, teeth);
	GuidingPlate(positions_).draw(designImage, teeth);
	IBar(positions_).draw(designImage, teeth);
}

void Rpi::registerOcclusalRest(vector<Tooth> teeth[nZones]) const { RpdWithOcclusalRest::registerOcclusalRest(teeth, positions_[0], RpdWithDirection::MESIAL); }

WwClasp::WwClasp(const vector<Position>& positions, const Direction& direction) : RpdWithPositions(positions), RpdWithDirection(direction), RpdAsClasp(deque<bool>(1)) {}

WwClasp* WwClasp::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Direction claspTipDirection;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	return new WwClasp(positions, claspTipDirection);
}

void WwClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	if (hasLingualArms_[0])
		Clasp(positions_, RpdWithMaterial::WROUGHT_WIRE, direction_).draw(designImage, teeth);
	else
		HalfClasp(positions_, RpdWithMaterial::WROUGHT_WIRE, direction_, HalfClasp::BUCCAL).draw(designImage, teeth);
	OcclusalRest(positions_, static_cast<Direction>(1 - direction_)).draw(designImage, teeth);
}

void WwClasp::registerOcclusalRest(vector<Tooth> teeth[nZones]) const { RpdWithOcclusalRest::registerOcclusalRest(teeth, positions_[0], static_cast<Direction>(1 - direction_)); }

void WwClasp::setLingualArm(bool hasLingualConfrontations[nZones][nTeethPerZone]) { RpdAsClasp::setLingualArm(hasLingualConfrontations, positions_, hasLingualArms_); }

void WwClasp::registerLingualBlockage(vector<Tooth> teeth[nZones]) const { RpdAsClasp::registerLingualBlockage(teeth, positions_, hasLingualArms_); }

Clasp::Clasp(const vector<Position>& positions, const Material& material, const Direction& direction) : RpdWithPositions(positions), RpdWithMaterial(material), RpdWithDirection(direction) {}

void Clasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	auto& tooth = getTooth(teeth, positions_[0]);
	vector<Point> curve;
	if (direction_ == MESIAL)
		curve = tooth.getCurve(60, 300);
	else
		curve = tooth.getCurve(240, 120);
	if (material_ == CAST)
		polylines(designImage, curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	else
		polylines(designImage, curve, false, 0, lineThicknessOfLevel[1], LINE_AA);
}

GuidingPlate::GuidingPlate(const vector<Position>& positions) : RpdWithPositions(positions) {}

void GuidingPlate::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	auto& tooth = getTooth(teeth, positions_[0]);
	auto& centroid = tooth.getCentroid();
	auto point = roundToInt(centroid + (static_cast<Point2f>(tooth.getAnglePoint(180)) - centroid) * 1.1);
	auto direction = roundToInt(computeNormalDirection(point) * tooth.getRadius() * 2 / 3);
	line(designImage, point, point + direction, 0, lineThicknessOfLevel[2], LINE_AA);
	line(designImage, point, point - direction, 0, lineThicknessOfLevel[2], LINE_AA);
}

HalfClasp::HalfClasp(const vector<Position>& positions, const Material& material, const Direction& direction, const Side& side) : RpdWithPositions(positions), RpdWithMaterial(material), RpdWithDirection(direction), side_(side) {}

void HalfClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	auto& tooth = getTooth(teeth, positions_[0]);
	vector<Point> curve;
	switch (direction_ * 2 + side_) {
		case 0b00:
			curve = tooth.getCurve(60, 180);
			break;
		case 0b01:
			curve = tooth.getCurve(180, 300);
			break;
		case 0b10:
			curve = tooth.getCurve(0, 120);
			break;
		case 0b11:
			curve = tooth.getCurve(240, 0);
			break;
		default: ;
	}
	if (material_ == CAST)
		polylines(designImage, curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	else
		polylines(designImage, curve, false, 0, lineThicknessOfLevel[1], LINE_AA);
}

IBar::IBar(const vector<Position>& positions) : RpdWithPositions(positions) {}

void IBar::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	auto& tooth = getTooth(teeth, positions_[0]);
	auto a = tooth.getRadius() * 1.5;
	Point2f point1 = tooth.getAnglePoint(75), point2 = tooth.getAnglePoint(165);
	auto center = (point1 + point2) / 2;
	auto direction = computeNormalDirection(center);
	auto r = point1 - center;
	auto rou = norm(r);
	auto sinTheta = direction.cross(r) / rou;
	auto b = rou * abs(sinTheta) / sqrt(1 - pow(rou / a, 2) * (1 - pow(sinTheta, 2)));
	auto inclination = atan2(direction.y, direction.x);
	auto t = radianToDegree(asin(rotate(r, -inclination).y / b));
	inclination = radianToDegree(inclination);
	if (t > 0)
		t -= 180;
	ellipse(designImage, center, roundToInt(Size(a, b)), inclination, t, t + 180, 0, lineThicknessOfLevel[2], LINE_AA);
}

OcclusalRest::OcclusalRest(const vector<Position>& positions, const Direction& direction) : RpdWithPositions(positions), RpdWithDirection(direction) {}

OcclusalRest* OcclusalRest::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Direction restMesialOrDistal;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpRestMesialOrDistal, individual, restMesialOrDistal);
	return new OcclusalRest(positions, restMesialOrDistal);
}

void OcclusalRest::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	auto& tooth = getTooth(teeth, positions_[0]);
	Point2f point;
	vector<Point> curve;
	if (direction_ == MESIAL) {
		point = tooth.getAnglePoint(0);
		curve = tooth.getCurve(340, 20);
	}
	else {
		point = tooth.getAnglePoint(180);
		curve = tooth.getCurve(160, 200);
	}
	auto& centroid = tooth.getCentroid();
	curve.push_back(centroid + (point - centroid) / 2);
	polylines(designImage, curve, true, 0, lineThicknessOfLevel[1], LINE_AA);
	fillPoly(designImage, vector<vector<Point>>{curve}, 0, LINE_AA);
}

void OcclusalRest::registerOcclusalRest(vector<Tooth> teeth[nZones]) const { RpdWithOcclusalRest::registerOcclusalRest(teeth, positions_[0], direction_); }

Plating::Plating(const vector<Position>& positions) : RpdWithPositions(positions) {}

void Plating::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	auto curve = getTooth(teeth, positions_[0]).getCurve(180, 0);
	polylines(designImage, curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
}
