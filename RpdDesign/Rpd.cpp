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

Rpd::Position& Rpd::Position::operator++() {
	++ordinal;
	return *this;
}

Rpd::Position& Rpd::Position::operator--() {
	--ordinal;
	return *this;
}

Rpd::Rpd(const vector<Position>& positions) : positions_(positions) {}

void Rpd::queryPositions(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, vector<Position>& positions, bool isEighthToothUsed[nZones], const bool& autoComplete) {
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
	for (auto i = 0; i < count - 1; ++i)
		for (auto j = i + 1; j < count; ++j)
			if (positions[i] > positions[j])
				swap(positions[i], positions[j]);
	if (autoComplete && count == 1)
		positions.push_back(positions[0]);
}

RpdWithMaterial::RpdWithMaterial(const Material& material) : material_(material) {}

void RpdWithMaterial::queryMaterial(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspMaterial, const jobject& individual, Material& claspMaterial) { claspMaterial = static_cast<Material>(env->CallIntMethod(env->CallObjectMethod(individual, midResourceGetProperty, dpClaspMaterial), midGetInt)); }

RpdWithDirection::RpdWithDirection(const Rpd::Direction& direction) : direction_(direction) {}

void RpdWithDirection::queryDirection(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspTipDirection, const jobject& individual, Rpd::Direction& claspTipDirection) { claspTipDirection = static_cast<Rpd::Direction>(env->CallIntMethod(env->CallObjectMethod(individual, midResourceGetProperty, dpClaspTipDirection), midGetInt)); }

void RpdAsLingualBlockage::registerLingualBlockages(vector<Tooth> teeth[nZones]) const {
	if (positions_.size() == 2)
		if (positions_[0].zone == positions_[1].zone)
			registerLingualBlockages(teeth, positions_);
		else {
			registerLingualBlockages(teeth, {Position(positions_[0].zone, 0), positions_[0]});
			registerLingualBlockages(teeth, {Position(positions_[1].zone, 0), positions_[1]});
		}
	else {
		registerLingualBlockages(teeth, {positions_[0], positions_[1]});
		registerLingualBlockages(teeth, {positions_[2], positions_[3]});
	}
}

RpdAsLingualBlockage::RpdAsLingualBlockage(const vector<Position>& positions, const vector<LingualBlockage>& lingualBlockages) : Rpd(positions), lingualBlockages_(lingualBlockages) {}

RpdAsLingualBlockage::RpdAsLingualBlockage(const vector<Position>& positions, const LingualBlockage& lingualBlockage) : RpdAsLingualBlockage(positions, vector<LingualBlockage>{lingualBlockage}) {}

void RpdAsLingualBlockage::registerLingualBlockages(vector<Tooth> teeth[nZones], const vector<Position>& positions) const {
	for (auto position = positions[0]; position <= positions[1]; ++position) {
		auto& tooth = getTooth(teeth, position);
		if (lingualBlockages_[0] != MAJOR_CONNECTOR || !tooth.getLingualBlockage())
			tooth.setLingualBlockage(lingualBlockages_[0]);
	}
}

void RpdAsLingualBlockage::registerLingualBlockages(vector<Tooth> teeth[nZones], const deque<bool>& flags) const {
	for (auto i = 0; i < flags.size(); ++i)
		if (flags[i])
			getTooth(teeth, positions_[i]).setLingualBlockage(lingualBlockages_[i]);
}

void RpdWithLingualClaspArms::setLingualClaspArms(bool hasLingualConfrontations[nZones][nTeethPerZone]) {
	for (auto i = 0; i < positions_.size(); ++i) {
		auto& position = positions_[i];
		hasLingualClaspArms_[i] = !hasLingualConfrontations[position.zone][position.ordinal];
	}
}

RpdWithLingualClaspArms::RpdWithLingualClaspArms(const vector<Position>& positions, const Material& material, const vector<Direction>& tipDirections) : RpdWithMaterial(material), RpdAsLingualBlockage(positions, tipDirectionsToLingualBlockages(tipDirections)), tipDirections_(tipDirections), hasLingualClaspArms_(deque<bool>(positions.size())) {}

RpdWithLingualClaspArms::RpdWithLingualClaspArms(const vector<Position>& positions, const Material& material, const Direction& tipDirection) : RpdWithLingualClaspArms(positions, material, vector<Direction>{tipDirection}) {}

void RpdWithLingualClaspArms::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	for (auto i = 0; i < positions_.size(); ++i)
		if (hasLingualClaspArms_[i])
			HalfClasp(positions_[i], material_, tipDirections_[i], HalfClasp::LINGUAL).draw(designImage, teeth);
}

void RpdWithLingualClaspArms::registerLingualBlockages(vector<Tooth> teeth[nZones]) const { RpdAsLingualBlockage::registerLingualBlockages(teeth, hasLingualClaspArms_); }

RpdWithLingualConfrontations::RpdWithLingualConfrontations(const vector<Position>& positions, const vector<Position>& lingualConfrontations) : RpdAsLingualBlockage(positions, MAJOR_CONNECTOR), lingualConfrontations_(lingualConfrontations) {}

void RpdWithLingualConfrontations::registerLingualConfrontations(bool hasLingualConfrontations[nZones][nTeethPerZone]) const {
	for (auto position = lingualConfrontations_.begin(); position < lingualConfrontations_.end(); ++position)
		hasLingualConfrontations[position->zone][position->ordinal] = true;
}

void RpdWithLingualConfrontations::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	for (auto position = lingualConfrontations_.begin(); position < lingualConfrontations_.end(); ++position)
		Plating({*position}).draw(designImage, teeth); /*TODO*/
}

void RpdWithLingualConfrontations::queryLingualConfrontations(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& individual, vector<Position>& lingualConfrontations) {
	auto lcTeeth = env->CallObjectMethod(individual, midListProperties, dpLingualConfrontation);
	while (env->CallBooleanMethod(lcTeeth, midHasNext)) {
		auto tooth = env->CallObjectMethod(lcTeeth, midNext);
		lingualConfrontations.push_back(Position(env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothZone), midGetInt) - 1, env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothOrdinal), midGetInt) - 1));
	}
}

AkerClasp::AkerClasp(const vector<Position>& positions, const Material& material, const Direction& direction) : RpdWithDirection(direction), RpdWithLingualClaspArms(positions, material, direction) {}

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
	RpdWithLingualClaspArms::draw(designImage, teeth);
	OcclusalRest(positions_, direction_ == MESIAL ? DISTAL : MESIAL).draw(designImage, teeth);
	HalfClasp(positions_, material_, direction_, HalfClasp::BUCCAL).draw(designImage, teeth);
}

CombinationAnteriorPosteriorPalatalStrap* CombinationAnteriorPosteriorPalatalStrap::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions, lingualConfrontations;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryLingualConfrontations(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, individual, lingualConfrontations);
	return new CombinationAnteriorPosteriorPalatalStrap(positions, lingualConfrontations);
}

CombinationAnteriorPosteriorPalatalStrap::CombinationAnteriorPosteriorPalatalStrap(const vector<Position>& positions, const vector<Position>& lingualConfrontations): RpdWithLingualConfrontations(positions, lingualConfrontations) {}

void CombinationAnteriorPosteriorPalatalStrap::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	RpdWithLingualConfrontations::draw(designImage, teeth);
	vector<Point> curve, innerCurve, mesialCurve, distalCurve, tmpCurve, distalPoints(2);
	vector<vector<Point>> curves;
	computeLingualCurve(teeth, {positions_[2] , positions_[3]}, tmpCurve, curves, &distalPoints[1]);
	curve.insert(curve.end(), tmpCurve.rbegin(), tmpCurve.rend());
	computeMesialCurve(teeth, {positions_[2], positions_[0]}, mesialCurve, &innerCurve);
	curve.insert(curve.end(), mesialCurve.begin(), mesialCurve.end());
	computeLingualCurve(teeth, {positions_[0] , positions_[1]}, tmpCurve, curves, &distalPoints[0]);
	curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
	computeDistalCurve(teeth, {positions_[1], positions_[3]}, distalPoints, distalCurve, &innerCurve);
	curve.insert(curve.end(), distalCurve.begin(), distalCurve.end());
	auto thisDesign = Mat(designImage.size(), CV_8U, 255);
	fillPoly(thisDesign, vector<vector<Point>>{curve}, 128, LINE_AA);
	fillPoly(thisDesign, vector<vector<Point>>{innerCurve}, 255, LINE_AA);
	polylines(thisDesign, innerCurve, true, 0, lineThicknessOfLevel[2], LINE_AA);
	bitwise_and(thisDesign, designImage, designImage);
	for (auto thisCurve = curves.begin(); thisCurve < curves.end(); ++thisCurve)
		polylines(designImage, *thisCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	polylines(designImage, mesialCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	polylines(designImage, distalCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
}

CombinationClasp::CombinationClasp(const vector<Position>& positions, const Direction& direction) : RpdWithDirection(direction), RpdWithLingualClaspArms(positions, CAST, direction) {}

CombinationClasp* CombinationClasp::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Direction claspTipDirection;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	return new CombinationClasp(positions, claspTipDirection);
}

void CombinationClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	RpdWithLingualClaspArms::draw(designImage, teeth);
	OcclusalRest(positions_, direction_ == MESIAL ? DISTAL : MESIAL).draw(designImage, teeth);
	HalfClasp(positions_, WROUGHT_WIRE, direction_, HalfClasp::BUCCAL).draw(designImage, teeth);
}

CombinedClasp::CombinedClasp(const vector<Position>& positions, const Material& material) : RpdWithLingualClaspArms(positions, material, {positions[0].zone == positions[1].zone ? MESIAL : DISTAL, DISTAL}) {}

CombinedClasp* CombinedClasp::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new CombinedClasp(positions, claspMaterial);
}

void CombinedClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	RpdWithLingualClaspArms::draw(designImage, teeth);
	auto isInSameZone = positions_[0].zone == positions_[1].zone;
	OcclusalRest(positions_[0], isInSameZone ? DISTAL : MESIAL).draw(designImage, teeth);
	OcclusalRest(positions_[1], MESIAL).draw(designImage, teeth);
	HalfClasp(positions_[0], material_, isInSameZone ? MESIAL : DISTAL, HalfClasp::BUCCAL).draw(designImage, teeth);
	HalfClasp(positions_[1], material_, DISTAL, HalfClasp::BUCCAL).draw(designImage, teeth);
}

ContinuousClasp* ContinuousClasp::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new ContinuousClasp(positions, claspMaterial);
}

void ContinuousClasp::setLingualClaspArms(bool hasLingualConfrontations[nZones][nTeethPerZone]) {
	RpdWithLingualClaspArms::setLingualClaspArms(hasLingualConfrontations);
	hasLingualClaspArms_[0] = hasLingualClaspArms_[1] = hasLingualClaspArms_[0] && hasLingualClaspArms_[1];
}

ContinuousClasp::ContinuousClasp(const vector<Position>& positions, const Material& material) : RpdWithLingualClaspArms(positions, material, {positions[0].zone == positions[1].zone ? DISTAL : MESIAL, MESIAL}) {}

void ContinuousClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	auto isInSameZone = positions_[0].zone == positions_[1].zone;
	OcclusalRest(positions_[0], isInSameZone ? MESIAL : DISTAL).draw(designImage, teeth);
	OcclusalRest(positions_[1], DISTAL).draw(designImage, teeth);
	auto& hasLingualClaspArm = hasLingualClaspArms_[0];
	auto curve1 = getTooth(teeth, positions_[0]).getCurve(isInSameZone ? hasLingualClaspArm ? 180 : 0 : 60, isInSameZone ? 120 : hasLingualClaspArm ? 0 : 180), curve2 = getTooth(teeth, positions_[1]).getCurve(60, hasLingualClaspArm ? 0 : 180);
	if (hasLingualClaspArm)
		if (isInSameZone)
			curve2.insert(curve2.end(), curve1.begin(), curve1.end());
		else
			curve2.insert(curve2.end(), curve1.rbegin(), curve1.rend());
	else
		polylines(designImage, curve1, false, 0, lineThicknessOfLevel[1 + (material_ == CAST ? 1 : 0)], LINE_AA);
	polylines(designImage, curve2, false, 0, lineThicknessOfLevel[1 + (material_ == CAST ? 1 : 0)], LINE_AA);
}

DentureBase::DentureBase(const vector<Position>& positions) : RpdAsLingualBlockage(positions, DENTURE_BASE) {}

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

void DentureBase::registerDentureBase(vector<Tooth> teeth[nZones]) {
	if (isBlocked_ < 0)
		isBlocked_ = findLingualBlockage(teeth, positions_);
	if (!isBlocked_)
		registerDentureBase(teeth, positions_);
}

void DentureBase::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	vector<Point> curve;
	float avgRadius;
	computeStringCurve(teeth, positions_, curve, avgRadius);
	if (coversTails_[0]) {
		auto distalPoint = getTooth(teeth, positions_[0]).getAnglePoint(180);
		curve.insert(curve.begin(), distalPoint + roundToInt(rotate(computeNormalDirection(distalPoint), -CV_PI / 2) * avgRadius * 0.8F));
	}
	if (coversTails_[1]) {
		auto distalPoint = getTooth(teeth, positions_[1]).getAnglePoint(180);
		curve.push_back(distalPoint + roundToInt(rotate(computeNormalDirection(distalPoint), CV_PI * (positions_[1].zone % 2 - 0.5)) * avgRadius * 0.8F));
	}
	if (coversTails_[0] || coversTails_[1] || !isBlocked_) {
		curve.erase(curve.begin() + 1);
		curve.erase(curve.end() - 2);
		vector<Point> tmpCurve(curve.size());
		for (auto i = 0; i < curve.size(); ++i) {
			auto delta = roundToInt(computeNormalDirection(curve[i]) * avgRadius * 1.6F);
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
			curve[i] += roundToInt(computeNormalDirection(curve[i]) * avgRadius * 1.6F);
		computeSmoothCurve(curve, curve);
		polylines(designImage, curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	}
}

void DentureBase::registerLingualBlockages(vector<Tooth> teeth[nZones]) const {
	if (coversTails_[0] || coversTails_[1])
		RpdAsLingualBlockage::registerLingualBlockages(teeth);
}

void DentureBase::registerDentureBase(vector<Tooth> teeth[nZones], vector<Position> positions) {
	if (positions[0].zone == positions[1].zone)
		for (auto position = positions[0]; position <= positions[1]; ++position)
			getTooth(teeth, position).setLingualBlockage(DENTURE_BASE);
	else {
		registerDentureBase(teeth, {Position(positions[0].zone, 0), positions[0]});
		registerDentureBase(teeth, {Position(positions[1].zone, 0), positions[1]});
	}
}

EdentulousSpace::EdentulousSpace(const vector<Position>& positions) : Rpd(positions) {}

EdentulousSpace* EdentulousSpace::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed, true);
	return new EdentulousSpace(positions);
}

void EdentulousSpace::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	vector<vector<Point>> curves;
	computeStringCurves(teeth, positions_, {0.25F, -0.25F}, false, curves);
	for (auto curve = curves.begin(); curve < curves.end(); ++curve) {
		computeSmoothCurve(*curve, *curve);
		polylines(designImage, *curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	}
}

FullPalatalPlate* FullPalatalPlate::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions, lingualConfrontations;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryLingualConfrontations(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, individual, lingualConfrontations);
	return new FullPalatalPlate(positions, lingualConfrontations);
}

FullPalatalPlate::FullPalatalPlate(const vector<Position>& positions, const vector<Position>& lingualConfrontations): RpdWithLingualConfrontations(positions, lingualConfrontations) {}

void FullPalatalPlate::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	RpdWithLingualConfrontations::draw(designImage, teeth);
	vector<Point> curve, distalCurve, distalPoints;
	vector<vector<Point>> curves;
	computeLingualCurve(teeth, positions_, curve, curves, &distalPoints);
	reverse(curve.begin(), curve.end());
	computeDistalCurve(teeth, positions_, distalPoints, distalCurve);
	curve.insert(curve.end(), distalCurve.begin(), distalCurve.end());
	auto thisDesign = Mat(designImage.size(), CV_8U, 255);
	fillPoly(thisDesign, vector<vector<Point>>{curve}, 128, LINE_AA);
	bitwise_and(thisDesign, designImage, designImage);
	for (auto thisCurve = curves.begin(); thisCurve < curves.end(); ++thisCurve)
		polylines(designImage, *thisCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	polylines(designImage, distalCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
}

LingualBar* LingualBar::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions, lingualConfrontations;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryLingualConfrontations(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, individual, lingualConfrontations);
	return new LingualBar(positions, lingualConfrontations);
}

LingualBar::LingualBar(const vector<Position>& positions, const vector<Position>& lingualConfrontations): RpdWithLingualConfrontations(positions, lingualConfrontations) {}

void LingualBar::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	vector<Point> curve, tmpCurve;
	float avgRadius;
	deque<bool> hasDbCurves;
	computeOuterCurve(teeth, positions_, curve, &avgRadius, &hasDbCurves);
	bool hasLingualConfrontations[nZones][nTeethPerZone];
	registerLingualConfrontations(hasLingualConfrontations);
	computeInnerCurve(teeth, positions_, avgRadius, hasLingualConfrontations, hasDbCurves, tmpCurve);
	curve.insert(curve.end(), tmpCurve.rbegin(), tmpCurve.rend());
	auto thisDesign = Mat(designImage.size(), CV_8U, 255);
	fillPoly(thisDesign, vector<vector<Point>>{curve}, 128, LINE_AA);
	bitwise_and(thisDesign, designImage, designImage);
	polylines(designImage, curve, true, 0, lineThicknessOfLevel[2], LINE_AA);
}

LingualPlate* LingualPlate::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions, lingualConfrontations;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryLingualConfrontations(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, individual, lingualConfrontations);
	return new LingualPlate(positions, lingualConfrontations);
}

LingualPlate::LingualPlate(const vector<Position>& positions, const vector<Position>& lingualConfrontations): RpdWithLingualConfrontations(positions, lingualConfrontations) {}

void LingualPlate::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	RpdWithLingualConfrontations::draw(designImage, teeth);
	vector<Point> curve, tmpCurve;
	computeOuterCurve(teeth, positions_, curve);
	vector<vector<Point>> curves;
	computeLingualCurve(teeth, positions_, tmpCurve, curves);
	polylines(designImage, curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	for (auto thisCurve = curves.begin(); thisCurve < curves.end(); ++thisCurve)
		polylines(designImage, *thisCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	curve.insert(curve.end(), tmpCurve.rbegin(), tmpCurve.rend());
	auto thisDesign = Mat(designImage.size(), CV_8U, 255);
	fillPoly(thisDesign, vector<vector<Point>>{curve}, 128, LINE_AA);
	bitwise_and(thisDesign, designImage, designImage);
}

LingualRest::LingualRest(const vector<Position>& positions, const Direction& direction) : Rpd(positions), RpdWithDirection(direction) {}

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

OcclusalRest::OcclusalRest(const vector<Position>& positions, const Direction& direction) : Rpd(positions), RpdWithDirection(direction) {}

OcclusalRest::OcclusalRest(const Position& position, const Direction& direction) : OcclusalRest(vector<Position>{position}, direction) {}

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

PalatalPlate* PalatalPlate::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions, lingualConfrontations;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryLingualConfrontations(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, individual, lingualConfrontations);
	return new PalatalPlate(positions, lingualConfrontations);
}

PalatalPlate::PalatalPlate(const vector<Position>& positions, const vector<Position>& lingualConfrontations) : RpdWithLingualConfrontations(positions, lingualConfrontations) {}

void PalatalPlate::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	RpdWithLingualConfrontations::draw(designImage, teeth);
	vector<Point> curve, mesialCurve, distalCurve, tmpCurve, distalPoints(2);
	vector<vector<Point>> curves;
	computeLingualCurve(teeth, {positions_[2] , positions_[3]}, tmpCurve, curves, &distalPoints[1]);
	curve.insert(curve.end(), tmpCurve.rbegin(), tmpCurve.rend());
	computeMesialCurve(teeth, {positions_[2], positions_[0]}, mesialCurve);
	curve.insert(curve.end(), mesialCurve.begin(), mesialCurve.end());
	computeLingualCurve(teeth, {positions_[0] , positions_[1]}, tmpCurve, curves, &distalPoints[0]);
	curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
	computeDistalCurve(teeth, {positions_[1], positions_[3]}, distalPoints, distalCurve);
	curve.insert(curve.end(), distalCurve.begin(), distalCurve.end());
	auto thisDesign = Mat(designImage.size(), CV_8U, 255);
	fillPoly(thisDesign, vector<vector<Point>>{curve}, 128, LINE_AA);
	bitwise_and(thisDesign, designImage, designImage);
	for (auto thisCurve = curves.begin(); thisCurve < curves.end(); ++thisCurve)
		polylines(designImage, *thisCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	polylines(designImage, mesialCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	polylines(designImage, distalCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
}

RingClasp::RingClasp(const vector<Position>& positions, const Material& material) : Rpd(positions), RpdWithMaterial(material) {}

RingClasp* RingClasp::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new RingClasp(positions, claspMaterial);
}

void RingClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	OcclusalRest(positions_, MESIAL).draw(designImage, teeth);
	if (material_ == CAST)
		OcclusalRest(positions_, DISTAL).draw(designImage, teeth);
	auto isUpper = positions_[0].zone < nZones / 2;
	polylines(designImage, getTooth(teeth, positions_[0]).getCurve(isUpper ? 60 : 0, isUpper ? 0 : 300), false, 0, lineThicknessOfLevel[1 + (material_ == CAST ? 1 : 0)], LINE_AA);
}

Rpa::Rpa(const vector<Position>& positions, const Material& material) : Rpd(positions), RpdWithMaterial(material) {}

Rpa* Rpa::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new Rpa(positions, claspMaterial);
}

void Rpa::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	OcclusalRest(positions_, MESIAL).draw(designImage, teeth);
	GuidingPlate(positions_).draw(designImage, teeth);
	HalfClasp(positions_, material_, MESIAL, HalfClasp::BUCCAL).draw(designImage, teeth);
}

Rpi::Rpi(const vector<Position>& positions) : Rpd(positions) {}

Rpi* Rpi::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	return new Rpi(positions);
}

void Rpi::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	OcclusalRest(positions_, MESIAL).draw(designImage, teeth);
	GuidingPlate(positions_).draw(designImage, teeth);
	IBar(positions_).draw(designImage, teeth);
}

WwClasp::WwClasp(const vector<Position>& positions, const Direction& direction) : RpdWithDirection(direction), RpdWithLingualClaspArms(positions, WROUGHT_WIRE, direction) {}

WwClasp* WwClasp::createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]) {
	vector<Position> positions;
	Direction claspTipDirection;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	return new WwClasp(positions, claspTipDirection);
}

void WwClasp::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	OcclusalRest(positions_, direction_ == MESIAL ? DISTAL : MESIAL).draw(designImage, teeth);
	RpdWithLingualClaspArms::draw(designImage, teeth);
	HalfClasp(positions_, WROUGHT_WIRE, direction_, HalfClasp::BUCCAL).draw(designImage, teeth);
}

GuidingPlate::GuidingPlate(const vector<Position>& positions) : Rpd(positions) {}

void GuidingPlate::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	auto& tooth = getTooth(teeth, positions_[0]);
	auto& centroid = tooth.getCentroid();
	auto point = centroid + (static_cast<Point2f>(tooth.getAnglePoint(180)) - centroid) * 1.1F;
	auto direction = computeNormalDirection(point) * tooth.getRadius() * 2 / 3;
	line(designImage, point, point + direction, 0, lineThicknessOfLevel[2], LINE_AA);
	line(designImage, point, point - direction, 0, lineThicknessOfLevel[2], LINE_AA);
}

HalfClasp::HalfClasp(const vector<Position>& positions, const Material& material, const Direction& direction, const Side& side) : Rpd(positions), RpdWithMaterial(material), RpdWithDirection(direction), side_(side) {}

HalfClasp::HalfClasp(const Position& position, const Material& material, const Direction& direction, const Side& side) : HalfClasp(vector<Position>{position}, material, direction, side) {}

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
	polylines(designImage, curve, false, 0, lineThicknessOfLevel[1 + (material_ == CAST ? 1 : 0)], LINE_AA);
}

IBar::IBar(const vector<Position>& positions) : Rpd(positions) {}

void IBar::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const {
	auto& tooth = getTooth(teeth, positions_[0]);
	auto a = tooth.getRadius() * 1.5F;
	auto &p1 = tooth.getAnglePoint(75), &p2 = tooth.getAnglePoint(165);
	auto c = (p1 + p2) / 2;
	auto d = computeNormalDirection(c);
	auto r = p1 - c;
	auto rou = norm(r);
	auto sinTheta = d.cross(r) / rou;
	auto b = rou * abs(sinTheta) / sqrt(1 - pow(rou / a, 2) * (1 - pow(sinTheta, 2)));
	auto inclination = atan2(d.y, d.x);
	auto t = radianToDegree(asin(rotate(r, -inclination).y / b));
	inclination = radianToDegree(inclination);
	if (t > 0)
		t -= 180;
	ellipse(designImage, c, roundToInt(Size(a, b)), inclination, t, t + 180, 0, lineThicknessOfLevel[2], LINE_AA);
}

Plating::Plating(const vector<Position>& positions) : Rpd(positions) {}

void Plating::draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const { polylines(designImage, getTooth(teeth, positions_[0]).getCurve(180, 0), false, 0, lineThicknessOfLevel[2], LINE_AA); }
