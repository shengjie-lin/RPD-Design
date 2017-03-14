#include <opencv2/imgproc.hpp>

#include "Rpd.h"
#include "Tooth.h"
#include "Utilities.h"

Rpd::Position::Position(const int& zone, const int& ordinal) : zone(zone), ordinal(ordinal) {}

bool Rpd::Position::operator==(const Position& rhs) const { return zone == rhs.zone && ordinal == rhs.ordinal; }

bool Rpd::Position::operator<(const Position& rhs) const {
	if (zone < rhs.zone)
		return true;
	if (zone == rhs.zone)
		return ordinal < rhs.ordinal;
	return false;
}

Rpd::Anchor::Anchor(const Position& position, const Direction& direction) : position(position), direction(direction) {}

bool Rpd::Anchor::operator==(const Anchor& rhs) const { return position == rhs.position && direction == rhs.direction; }

bool Rpd::Anchor::operator<(const Anchor& rhs) const {
	if (position < rhs.position)
		return true;
	if (position == rhs.position)
		return direction < rhs.direction;
	return false;
}

Rpd::Anchor& Rpd::Anchor::operator++() {
	if (direction == MESIAL)
		direction = DISTAL;
	else {
		direction = MESIAL;
		++position.ordinal;
	}
	return *this;
}

Rpd::Anchor& Rpd::Anchor::operator--() {
	if (direction == MESIAL) {
		direction = DISTAL;
		--position.ordinal;
	}
	else
		direction = MESIAL;
	return *this;
}

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

void RpdWithMaterial::queryMaterial(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspMaterial, const jobject& individual, Material& claspMaterial) { claspMaterial = static_cast<Material>(env->CallIntMethod(env->CallObjectMethod(individual, midResourceGetProperty, dpClaspMaterial), midGetInt)); }

RpdWithDirection::RpdWithDirection(const Direction& direction) : direction_(direction) {}

void RpdWithDirection::queryDirection(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspTipDirection, const jobject& individual, Direction& claspTipDirection) { claspTipDirection = static_cast<Direction>(env->CallIntMethod(env->CallObjectMethod(individual, midResourceGetProperty, dpClaspTipDirection), midGetInt)); }

void RpdAsLingualBlockage::registerLingualBlockage(vector<Tooth> teeth[nZones], const vector<Position>& positions, const LingualBlockage& lingualBlockage) {
	if (positions.size() == 1)
		getTooth(teeth, positions[0]).setLingualBlockage(lingualBlockage);
	else if (positions[0].zone == positions[1].zone)
		for (auto ordinal = positions[0].ordinal; ordinal <= positions[1].ordinal; ++ordinal) {
			auto& tooth = teeth[positions[0].zone][ordinal];
			if (lingualBlockage != MAJOR_CONNECTOR || tooth.getLingualBlockage() == NONE)
				tooth.setLingualBlockage(lingualBlockage);
		}
	else {
		registerLingualBlockage(teeth, {Position(positions[0].zone, 0),positions[0]}, lingualBlockage);
		registerLingualBlockage(teeth, {Position(positions[1].zone, 0),positions[1]}, lingualBlockage);
	}
}

void RpdWithClasps::setLingualArms(bool hasLingualConfrontations[nZones][nTeethPerZone]) {
	for (auto i = 0; i < clasps_.size(); ++i) {
		auto& position = clasps_[i].position;
		hasLingualArms_[i] = !hasLingualConfrontations[position.zone][position.ordinal];
	}
}

RpdWithClasps::RpdWithClasps(const Anchor& clasp) : clasps_({clasp}), hasLingualArms_(deque<bool>(1)) {}

RpdWithClasps::RpdWithClasps(const vector<Anchor>& clasps) : clasps_(clasps), hasLingualArms_(deque<bool>(clasps.size())) {}

void RpdWithClasps::draw(const Mat& designImage, const vector<Tooth> teeth[nZones], const RpdWithMaterial::Material& material) const {
	for (auto i = 0; i < clasps_.size(); ++i)
		if (hasLingualArms_[i])
			HalfClasp(clasps_[i], material).draw(designImage, teeth);
}

const deque<bool>& RpdWithClasps::getLingualArms() const { return hasLingualArms_; }

void RpdWithClasps::registerLingualBlockage(vector<Tooth> teeth[nZones]) const {
	for (auto i = 0; i < clasps_.size(); ++i)
		if (hasLingualArms_[i])
			RpdAsLingualBlockage::registerLingualBlockage(teeth, {clasps_[i].position}, CLASP);
}

RpdAsMajorConnector::RpdAsMajorConnector(const vector<Anchor>& anchors) : anchors_(anchors) {}

void RpdAsMajorConnector::queryAnchors(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpAnchorMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& opMajorConnectorAnchor, const jobject& individual, const Coverage& coverage, vector<Anchor>& anchors, bool isEighthToothUsed[nZones]) {
	auto count = 0;
	auto majorConnectorAnchors = env->CallObjectMethod(individual, midListProperties, opMajorConnectorAnchor);
	while (env->CallBooleanMethod(majorConnectorAnchors, midHasNext)) {
		auto anchor = env->CallObjectMethod(majorConnectorAnchors, midNext);
		auto tooth = env->CallObjectMethod(anchor, midStatementGetProperty, opComponentPosition);
		auto zone = env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothZone), midGetInt) - 1;
		auto ordinal = env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothOrdinal), midGetInt) - 1;
		anchors.push_back(Anchor(Position(zone, ordinal), static_cast<Direction>(env->CallIntMethod(env->CallObjectMethod(anchor, midStatementGetProperty, dpAnchorMesialOrDistal), midGetInt))));
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
	RpdAsLingualBlockage::registerLingualBlockage(teeth, {anchors_[0].position, anchors_[1].position}, MAJOR_CONNECTOR);
	if (anchors_.size() == 4)
		RpdAsLingualBlockage::registerLingualBlockage(teeth, {anchors_[3].position, anchors_[2].position}, MAJOR_CONNECTOR);
}

void RpdWithOcclusalRests::registerOcclusalRests(vector<Tooth> teeth[nZones]) const {
	for (auto occlusalRest = occlusalRests_.begin(); occlusalRest < occlusalRests_.end(); ++occlusalRest)
		OcclusalRest(*occlusalRest).registerOcclusalRests(teeth);
}

RpdWithOcclusalRests::RpdWithOcclusalRests(const Anchor& occlusalRest) : occlusalRests_({occlusalRest}) {}

RpdWithOcclusalRests::RpdWithOcclusalRests(const vector<Anchor>& occlusalRests) : occlusalRests_(occlusalRests) {}

void RpdWithOcclusalRests::appendOcclusalRest(const Anchor& occlusalRest) { occlusalRests_.push_back(occlusalRest); }

void RpdWithOcclusalRests::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	for (auto occlusalRest = occlusalRests_.begin(); occlusalRest < occlusalRests_.end(); ++occlusalRest)
		OcclusalRest(*occlusalRest).draw(designImage, teeth);
}

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

AkerClasp::AkerClasp(const vector<Position>& positions, const Material& material, const Direction& direction) : RpdWithPositions(positions), RpdWithMaterial(material), RpdWithDirection(direction), RpdWithOcclusalRests(Anchor(positions[0], direction == MESIAL ? DISTAL : MESIAL)), RpdWithClasps(Anchor(positions[0], direction)) {}

AkerClasp* AkerClasp::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Direction claspTipDirection;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new AkerClasp(positions, claspMaterial, claspTipDirection);
}

void AkerClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	RpdWithOcclusalRests::draw(designImage, teeth);
	RpdWithClasps::draw(designImage, teeth, material_);
	HalfClasp(positions_, material_, direction_, HalfClasp::BUCCAL).draw(designImage, teeth);
}

CombinationClasp::CombinationClasp(const vector<Position>& positions, const Direction& direction) : RpdWithPositions(positions), RpdWithDirection(direction), RpdWithOcclusalRests(Anchor(positions[0], direction == MESIAL ? DISTAL : MESIAL)), RpdWithClasps(Anchor(positions[0], direction)) {}

CombinationClasp* CombinationClasp::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Direction claspTipDirection;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	return new CombinationClasp(positions, claspTipDirection);
}

void CombinationClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	RpdWithOcclusalRests::draw(designImage, teeth);
	RpdWithClasps::draw(designImage, teeth, RpdWithMaterial::CAST);
	HalfClasp(positions_, RpdWithMaterial::WROUGHT_WIRE, direction_, HalfClasp::BUCCAL).draw(designImage, teeth);
}

CombinedClasp::CombinedClasp(const vector<Position>& positions, const Material& material) : RpdWithPositions(positions), RpdWithMaterial(material), RpdWithOcclusalRests({Anchor(positions[0], positions[0].zone == positions[1].zone ? DISTAL : MESIAL), Anchor(positions[1], MESIAL)}), RpdWithClasps({Anchor(positions[0], positions[0].zone == positions[1].zone ? MESIAL : DISTAL), Anchor(positions[1], DISTAL)}) {}

CombinedClasp* CombinedClasp::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new CombinedClasp(positions, claspMaterial);
}

void CombinedClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	RpdWithOcclusalRests::draw(designImage, teeth);
	RpdWithClasps::draw(designImage, teeth, material_);
	HalfClasp({positions_[0]}, material_, positions_[0].zone == positions_[1].zone ? MESIAL : DISTAL, HalfClasp::BUCCAL).draw(designImage, teeth);
	HalfClasp({positions_[1]}, material_, DISTAL, HalfClasp::BUCCAL).draw(designImage, teeth);
}

DentureBase::DentureBase(const vector<Position>& positions) : RpdWithPositions(positions) {}

DentureBase* DentureBase::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
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
	bool isBlockedByMajorConnector;
	computeStringCurve(teeth, positions_, curve, avgRadius, &isBlockedByMajorConnector);
	if (coversTails_[0]) {
		Point2f distalPoint = getTooth(teeth, positions_[0]).getAnglePoint(180);
		curve[0] = roundToInt(distalPoint + rotate(computeNormalDirection(distalPoint), -CV_PI / 2) * avgRadius);
	}
	if (coversTails_[1]) {
		Point2f distalPoint = getTooth(teeth, positions_[1]).getAnglePoint(180);
		curve.back() = roundToInt(distalPoint + rotate(computeNormalDirection(distalPoint), CV_PI * (positions_[1].zone % 2 - 0.5)) * avgRadius);
	}
	if (coversTails_[0] || coversTails_[1] || !isBlockedByMajorConnector) {
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
		RpdAsLingualBlockage::registerLingualBlockage(teeth, positions_, DENTURE_BASE);
}

EdentulousSpace::EdentulousSpace(const vector<Position>& positions) : RpdWithPositions(positions) {}

EdentulousSpace* EdentulousSpace::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
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

LingualRest* LingualRest::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
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

PalatalPlate* PalatalPlate::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpAnchorMesialOrDistal, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& opMajorConnectorAnchor, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Anchor> anchors;
	vector<Position> lingualConfrontations;
	queryAnchors(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpAnchorMesialOrDistal, dpToothZone, dpToothOrdinal, opComponentPosition, opMajorConnectorAnchor, individual, PLANAR, anchors, isEighthToothUsed);
	queryLingualConfrontations(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, individual, lingualConfrontations);
	return new PalatalPlate(anchors, lingualConfrontations);
}

void PalatalPlate::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	/*TODO: consider lingual blockage; ugly!*/
	RpdWithLingualConfrontations::draw(designImage, teeth);
	vector<Point> mesialCurve = {getPoint(teeth, anchors_[3])}, distalCurve = {getPoint(teeth, anchors_[1])};
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
	vector<Point> curve, tmpCurve;
	vector<vector<Point>> curves;
	computeLingualCurve(teeth, {anchors_[0] , anchors_[1]}, tmpCurve, curves);
	curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
	curve.insert(curve.end(), distalCurve.begin(), distalCurve.end());
	computeLingualCurve(teeth, {anchors_[3] , anchors_[2]}, tmpCurve, curves);
	curve.insert(curve.end(), tmpCurve.rbegin(), tmpCurve.rend());
	curve.insert(curve.end(), mesialCurve.begin(), mesialCurve.end());
	auto thisDesign = Mat(designImage.size(), CV_8U, 255);
	fillPoly(thisDesign, vector<vector<Point>>{curve}, 128, LINE_AA);
	bitwise_and(thisDesign, designImage, designImage);
	for (auto i = 0; i < curves.size(); ++i)
		polylines(designImage, curves[i], false, 0, lineThicknessOfLevel[2], LINE_AA);
	polylines(designImage, mesialCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	polylines(designImage, distalCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
}

RingClasp::RingClasp(const vector<Position>& positions, const Material& material) : RpdWithPositions(positions), RpdWithMaterial(material), RpdWithClasps(Anchor(positions[0])), RpdWithOcclusalRests(Anchor(positions[0], MESIAL)) {
	if (material == CAST)
		appendOcclusalRest(Anchor(positions[0], DISTAL));
}

RingClasp* RingClasp::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new RingClasp(positions, claspMaterial);
}

void RingClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	RpdWithOcclusalRests::draw(designImage, teeth);
	auto isUpper = positions_[0].zone < nZones / 2;
	polylines(designImage, getTooth(teeth, positions_[0]).getCurve(isUpper ? 60 : 0, getLingualArms()[0] ? (isUpper ? 0 : 300) : 180), false, 0, lineThicknessOfLevel[1 + (material_ == CAST) ? 1 : 0], LINE_AA);
}

Rpa::Rpa(const vector<Position>& positions, const Material& material) : RpdWithPositions(positions), RpdWithMaterial(material), RpdWithOcclusalRests(Anchor(positions[0], MESIAL)) {}

Rpa* Rpa::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new Rpa(positions, claspMaterial);
}

void Rpa::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	RpdWithOcclusalRests::draw(designImage, teeth);
	GuidingPlate(positions_).draw(designImage, teeth);
	HalfClasp(positions_, material_, MESIAL, HalfClasp::BUCCAL).draw(designImage, teeth);
}

Rpi::Rpi(const vector<Position>& positions) : RpdWithPositions(positions), RpdWithOcclusalRests(Anchor(positions[0], MESIAL)) {}

Rpi* Rpi::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	return new Rpi(positions);
}

void Rpi::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	RpdWithOcclusalRests::draw(designImage, teeth);
	GuidingPlate(positions_).draw(designImage, teeth);
	IBar(positions_).draw(designImage, teeth);
}

WwClasp::WwClasp(const vector<Position>& positions, const Direction& direction) : RpdWithPositions(positions), RpdWithDirection(direction), RpdWithOcclusalRests(Anchor(positions[0], direction == MESIAL ? DISTAL : MESIAL)), RpdWithClasps(Anchor(positions[0], direction)) {}

WwClasp* WwClasp::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Direction claspTipDirection;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	return new WwClasp(positions, claspTipDirection);
}

void WwClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	RpdWithOcclusalRests::draw(designImage, teeth);
	RpdWithClasps::draw(designImage, teeth, RpdWithMaterial::WROUGHT_WIRE);
	HalfClasp(positions_, RpdWithMaterial::WROUGHT_WIRE, direction_, HalfClasp::BUCCAL).draw(designImage, teeth);
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

HalfClasp::HalfClasp(const Anchor& anchor, const Material& material) : HalfClasp({anchor.position}, material, anchor.direction, LINGUAL) {}

void HalfClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	auto& tooth = getTooth(teeth, positions_[0]);
	vector<Point> curve;
	switch ((direction_ == DISTAL) * 2 + (side_ == LINGUAL)) {
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
	polylines(designImage, curve, false, 0, lineThicknessOfLevel[1 + (material_ == CAST) ? 1 : 0], LINE_AA);
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

OcclusalRest::OcclusalRest(const vector<Position>& positions, const Direction& direction) : RpdWithPositions(positions), RpdWithDirection(direction), RpdWithOcclusalRests(*this) {}

OcclusalRest::OcclusalRest(const Anchor& anchor) : OcclusalRest({anchor.position}, anchor.direction) {}

OcclusalRest* OcclusalRest::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
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

void OcclusalRest::registerOcclusalRests(vector<Tooth> teeth[nZones]) const { getTooth(teeth, positions_[0]).setOcclusalRest(direction_); }

Plating::Plating(const vector<Position>& positions) : RpdWithPositions(positions) {}

void Plating::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const { polylines(designImage, getTooth(teeth, positions_[0]).getCurve(180, 0), false, 0, lineThicknessOfLevel[2], LINE_AA); }
