#include <opencv2/imgproc.hpp>

#include "Rpd.h"
#include "Tooth.h"
#include "Utilities.h"

Rpd::Position::Position(int const& zone, int const& ordinal) : zone(zone), ordinal(ordinal) {}

bool Rpd::Position::operator==(Position const& rhs) const { return zone == rhs.zone && ordinal == rhs.ordinal; }

bool Rpd::Position::operator<(Position const& rhs) const {
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
	if (ordinal)
		--ordinal;
	else
		zone += 1 - zone % 2 * 2;
	return *this;
}

Rpd::Position Rpd::Position::operator++(int) {
	auto position(*this);
	++*this;
	return position;
}

Rpd::Position Rpd::Position::operator--(int) {
	auto position(*this);
	--*this;
	return position;
}

Rpd::Rpd(vector<Position> const& positions) : positions_(positions) {}

void Rpd::queryPositions(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, vector<Position>& positions, bool (&isEighthToothUsed)[nZones], bool const& autoComplete) {
	auto const& teeth = env->CallObjectMethod(individual, midListProperties, opComponentPosition);
	auto count = 0;
	while (env->CallBooleanMethod(teeth, midHasNext)) {
		auto const& tooth = env->CallObjectMethod(teeth, midNext);
		auto const& zone = env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothZone), midGetInt) - 1;
		auto const& ordinal = env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothOrdinal), midGetInt) - 1;
		positions.push_back(Position(zone, ordinal));
		if (ordinal == nTeethPerZone - 1)
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

RpdWithMaterial::RpdWithMaterial(Material const& material) : material_(material) {}

void RpdWithMaterial::queryMaterial(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midResourceGetProperty, jobject const& dpClaspMaterial, jobject const& individual, Material& claspMaterial) {
	auto const& tmp = env->CallObjectMethod(individual, midResourceGetProperty, dpClaspMaterial);
	claspMaterial = tmp ? static_cast<Material>(env->CallIntMethod(tmp, midGetInt)) : CAST;
}

RpdWithDirection::RpdWithDirection(Rpd::Direction const& direction) : direction_(direction) {}

void RpdWithDirection::queryDirection(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midResourceGetProperty, jobject const& dpClaspTipDirection, jobject const& individual, Rpd::Direction& claspTipDirection) { claspTipDirection = static_cast<Rpd::Direction>(env->CallIntMethod(env->CallObjectMethod(individual, midResourceGetProperty, dpClaspTipDirection), midGetInt)); }

void RpdAsMajorConnector::registerMajorConnector(vector<Tooth> (&teeth)[nZones]) const { registerMajorConnector(teeth, positions_); }

void RpdAsMajorConnector::registerMajorConnector(vector<Tooth> (&teeth)[nZones], vector<Position> const& positions) {
	if (positions.size() == 2)
		if (positions[0].zone == positions[1].zone)
			for (auto position = positions[0]; position <= positions[1]; ++position)
				getTooth(teeth, position).setMajorConnector();
		else
			for (auto i = 0; i < 2; ++i)
				registerMajorConnector(teeth, {Position(positions[i].zone, 0), positions[i]});
	else
		for (auto i = 0; i < 3; ++i)
			registerMajorConnector(teeth, {positions[i] , positions[++i]});
}

void RpdAsMajorConnector::registerExpectedAnchors(vector<Tooth> (&teeth)[nZones]) const { registerExpectedAnchors(teeth, positions_); }

void RpdAsMajorConnector::registerExpectedAnchors(vector<Tooth> (&teeth)[nZones], vector<Position> const& positions) {
	if (positions.size() == 2) {
		getTooth(teeth, positions[0]).setExpectedMajorConnectorAnchor(positions[0].zone == positions[1].zone ? MESIAL : DISTAL);
		getTooth(teeth, positions[1]).setExpectedMajorConnectorAnchor(DISTAL);
	}
	else
		for (auto i = 0; i < 3; ++i)
			registerExpectedAnchors(teeth, {positions[i] , positions[++i]});
}

void RpdAsMajorConnector::registerLingualConfrontations(vector<Tooth> (&teeth)[nZones]) const {
	for (auto zone = 0; zone < nZones; ++zone)
		for (auto ordinal = 0; ordinal < nTeethPerZone; ++ordinal)
			if (hasLingualConfrontations_[zone][ordinal])
				teeth[zone][ordinal].setLingualConfrontation();
}

RpdAsMajorConnector::RpdAsMajorConnector(vector<Position> const& positions, const bool (&hasLingualConfrontations)[nZones][nTeethPerZone]) : Rpd(positions) {
	for (auto zone = 0; zone < nZones; ++zone)
		copy(begin(hasLingualConfrontations[zone]), end(hasLingualConfrontations[zone]), hasLingualConfrontations_[zone]);
}

void RpdAsMajorConnector::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	vector<vector<Point>> curves;
	computeLingualConfrontationCurves(teeth, positions_, curves);
	for (auto curve = curves.begin(); curve < curves.end(); ++curve)
		polylines(designImage, *curve, false, 0, lineThicknessOfLevel[2], LINE_AA);

}

void RpdAsMajorConnector::queryLingualConfrontations(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpLingualConfrontation, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& individual, bool (&hasLingualConfrontations)[nZones][nTeethPerZone]) {
	auto const& lcTeeth = env->CallObjectMethod(individual, midListProperties, dpLingualConfrontation);
	while (env->CallBooleanMethod(lcTeeth, midHasNext)) {
		auto const& tooth = env->CallObjectMethod(lcTeeth, midNext);
		hasLingualConfrontations[env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothZone), midGetInt) - 1][env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothOrdinal), midGetInt) - 1] = true;
	}
}

void RpdWithLingualCoverage::registerLingualCoverage(vector<Tooth> (&teeth)[nZones]) const {
	deque<bool> tmpFlags(positions_.size());
	for (auto flag = tmpFlags.begin(); flag < tmpFlags.end(); ++flag)
		*flag = true;
	registerLingualCoverage(teeth, tmpFlags);
}

RpdWithLingualCoverage::RpdWithLingualCoverage(vector<Position> const& positions, Material const& material, vector<Direction> const& rootDirections) : Rpd(positions), RpdWithMaterial(material), rootDirections_(rootDirections) {}

RpdWithLingualCoverage::RpdWithLingualCoverage(vector<Position> const& positions, Material const& material, Direction const& rootDirection) : RpdWithLingualCoverage(positions, material, vector<Direction>{rootDirection}) {}

void RpdWithLingualCoverage::registerLingualCoverage(vector<Tooth> (&teeth)[nZones], deque<bool> const& flags) const {
	for (auto i = 0; i < positions_.size(); ++i)
		if (flags[i])
			getTooth(teeth, positions_[i]).setLingualCoverage(rootDirections_[i]);
}

void RpdWithClaspRootOrRest::registerClaspRootOrRest(vector<Tooth> (&teeth)[nZones]) {
	for (auto i = 0; i < rootDirections_.size(); ++i)
		getTooth(teeth, positions_[positions_.size() == 1 ? 0 : i]).setClaspRootOrRest(rootDirections_[i]);
}

RpdWithClaspRootOrRest::RpdWithClaspRootOrRest(vector<Position> const& positions, vector<Direction> const& rootDirections) : Rpd(positions), rootDirections_(rootDirections) {}

RpdWithClaspRootOrRest::RpdWithClaspRootOrRest(vector<Position> const& positions, Direction const& rootDirection) : RpdWithClaspRootOrRest(positions, vector<Direction>{rootDirection}) {}

void RpdWithLingualClaspArms::setLingualClaspArms(vector<Tooth> (&teeth)[nZones]) {
	for (auto i = 0; i < positions_.size(); ++i)
		hasLingualArms_[i] = !getTooth(teeth, positions_[i]).hasLingualConfrontation();
}

RpdWithLingualClaspArms::RpdWithLingualClaspArms(vector<Position> const& positions, Material const& material, vector<Direction> const& rootDirections) : Rpd(positions), RpdWithLingualCoverage(positions, material, rootDirections), hasLingualArms_(deque<bool>(positions.size())) {}

RpdWithLingualClaspArms::RpdWithLingualClaspArms(vector<Position> const& positions, Material const& material, Direction const& rootDirection) : RpdWithLingualClaspArms(positions, material, vector<Direction>{rootDirection}) {}

void RpdWithLingualClaspArms::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	for (auto i = 0; i < positions_.size(); ++i)
		if (hasLingualArms_[i])
			HalfClasp(positions_[i], material_, ~rootDirections_[i], LINGUAL).draw(designImage, teeth);
}

void RpdWithLingualClaspArms::registerLingualCoverage(vector<Tooth> (&teeth)[nZones]) const { RpdWithLingualCoverage::registerLingualCoverage(teeth, hasLingualArms_); }

RpdWithLingualRest::RpdWithLingualRest(vector<Position> const& positions, Material const& material, Direction const& direction) : Rpd(positions), RpdWithClaspRootOrRest(positions, direction), RpdWithLingualCoverage(positions, material, direction) {}

void RpdWithLingualRest::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	for (auto i = 0; i < positions_.size(); ++i)
		LingualRest(vector<Position>{positions_[i]}, material_, RpdWithLingualCoverage::rootDirections_[i]).draw(designImage, teeth);
}

AkersClasp::AkersClasp(vector<Position> const& positions, Material const& material, Direction const& direction, bool const& enableBuccalArm, bool const& enableLingualArm, bool const& enableRest) : Rpd(positions), RpdWithDirection(direction), RpdWithClaspRootOrRest(positions, ~direction), RpdWithLingualClaspArms(positions, material, ~direction), enableBuccalArm_(enableBuccalArm), enableRest_(enableRest) { hasLingualArms_[0] = enableLingualArm; }

void AkersClasp::queryPartEnablements(JNIEnv* const& env, jmethodID const& midGetBoolean, jmethodID const& midResourceGetProperty, jobject const& dpEnableBuccalArm, jobject const& dpEnableLingualArm, jobject const& dpEnableRest, jobject const& individual, bool& enableBuccalArm, bool& enableLingualArm, bool& enableRest) {
	auto tmp = env->CallObjectMethod(individual, midResourceGetProperty, dpEnableRest);
	enableRest = tmp ? env->CallBooleanMethod(tmp, midGetBoolean) : true;
	tmp = env->CallObjectMethod(individual, midResourceGetProperty, dpEnableBuccalArm);
	enableBuccalArm = tmp ? env->CallBooleanMethod(tmp, midGetBoolean) : true;
	tmp = env->CallObjectMethod(individual, midResourceGetProperty, dpEnableLingualArm);
	enableLingualArm = tmp ? env->CallBooleanMethod(tmp, midGetBoolean) : true;
}

AkersClasp* AkersClasp::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetBoolean, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspTipDirection, jobject const& dpClaspMaterial, jobject const& dpEnableBuccalArm, jobject const& dpEnableLingualArm, jobject const& dpEnableRest, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	Direction claspTipDirection;
	Material claspMaterial;
	bool enableBuccalArm, enableLingualArm, enableRest;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	queryPartEnablements(env, midGetBoolean, midResourceGetProperty, dpEnableBuccalArm, dpEnableLingualArm, dpEnableRest, individual, enableBuccalArm, enableLingualArm, enableRest);
	return new AkersClasp(positions, claspMaterial, claspTipDirection, enableBuccalArm, enableLingualArm, enableRest);
}

void AkersClasp::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	RpdWithLingualClaspArms::draw(designImage, teeth);
	if (enableRest_)
		OcclusalRest(positions_, ~direction_).draw(designImage, teeth);
	if (enableBuccalArm_)
		HalfClasp(positions_, material_, direction_, BUCCAL).draw(designImage, teeth);
}

void AkersClasp::setLingualClaspArms(vector<Tooth> (&teeth)[nZones]) {
	if (hasLingualArms_[0])
		RpdWithLingualClaspArms::setLingualClaspArms(teeth);
}

CanineAkersClasp* CanineAkersClasp::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspTipDirection, jobject const& dpClaspMaterial, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	Direction claspTipDirection;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new CanineAkersClasp(positions, claspMaterial, claspTipDirection);
}

CanineAkersClasp::CanineAkersClasp(vector<Position> const& positions, Material const& claspMaterial, Direction const& direction) : Rpd(positions), RpdWithDirection(direction), RpdWithLingualRest(positions, WROUGHT_WIRE, ~direction), claspMaterial_(claspMaterial) {}

void CanineAkersClasp::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	RpdWithLingualRest::draw(designImage, teeth);
	HalfClasp(positions_, claspMaterial_, direction_, BUCCAL).draw(designImage, teeth);
}

CombinationAnteriorPosteriorPalatalStrap* CombinationAnteriorPosteriorPalatalStrap::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpLingualConfrontation, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	bool hasLingualConfrontations[nZones][nTeethPerZone] = {};
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryLingualConfrontations(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, individual, hasLingualConfrontations);
	return new CombinationAnteriorPosteriorPalatalStrap(positions, hasLingualConfrontations);
}

CombinationAnteriorPosteriorPalatalStrap::CombinationAnteriorPosteriorPalatalStrap(vector<Position> const& positions, const bool (&hasLingualConfrontations)[nZones][nTeethPerZone]) : RpdAsMajorConnector(positions, hasLingualConfrontations) {}

void CombinationAnteriorPosteriorPalatalStrap::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	RpdAsMajorConnector::draw(designImage, teeth);
	vector<Point> curve, innerCurve, mesialCurve, distalCurve, tmpCurve, distalPoints(2);
	vector<vector<Point>> curves;
	computeLingualCurve(teeth, {positions_[2], positions_[3]}, tmpCurve, curves, distalPoints[1]);
	curve.insert(curve.end(), tmpCurve.rbegin(), tmpCurve.rend());
	computeMesialCurve(teeth, {positions_[2], positions_[0]}, mesialCurve, &innerCurve);
	curve.insert(curve.end(), mesialCurve.begin(), mesialCurve.end());
	computeLingualCurve(teeth, {positions_[0], positions_[1]}, tmpCurve, curves, distalPoints[0]);
	curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
	computeDistalCurve(teeth, {positions_[1], positions_[3]}, distalPoints, distalCurve, &innerCurve);
	curve.insert(curve.end(), distalCurve.begin(), distalCurve.end());
	auto const& thisDesign = Mat(designImage.size(), CV_8U, 255);
	fillPoly(thisDesign, vector<vector<Point>>{curve}, 128, LINE_AA);
	fillPoly(thisDesign, vector<vector<Point>>{innerCurve}, 255, LINE_AA);
	polylines(thisDesign, innerCurve, true, 0, lineThicknessOfLevel[2], LINE_AA);
	bitwise_and(thisDesign, designImage, designImage);
	for (auto thisCurve = curves.begin(); thisCurve < curves.end(); ++thisCurve)
		polylines(designImage, *thisCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	polylines(designImage, mesialCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	polylines(designImage, distalCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
}

CombinationClasp::CombinationClasp(vector<Position> const& positions, Direction const& direction) : Rpd(positions), RpdWithDirection(direction), RpdWithClaspRootOrRest(positions, ~direction), RpdWithLingualClaspArms(positions, CAST, ~direction) {}

CombinationClasp* CombinationClasp::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspTipDirection, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	Direction claspTipDirection;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	return new CombinationClasp(positions, claspTipDirection);
}

void CombinationClasp::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	RpdWithLingualClaspArms::draw(designImage, teeth);
	OcclusalRest(positions_, ~direction_).draw(designImage, teeth);
	HalfClasp(positions_, WROUGHT_WIRE, direction_, BUCCAL).draw(designImage, teeth);
}

CombinedClasp::CombinedClasp(vector<Position> const& positions, Material const& material) : Rpd(positions), RpdWithClaspRootOrRest(positions, {positions[0].zone == positions[1].zone ? DISTAL : MESIAL, MESIAL}), RpdWithLingualClaspArms(positions, material, {positions[0].zone == positions[1].zone ? DISTAL : MESIAL, MESIAL}) {}

CombinedClasp* CombinedClasp::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspMaterial, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new CombinedClasp(positions, claspMaterial);
}

void CombinedClasp::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	RpdWithLingualClaspArms::draw(designImage, teeth);
	auto isInSameZone = positions_[0].zone == positions_[1].zone;
	OcclusalRest(positions_[0], isInSameZone ? DISTAL : MESIAL).draw(designImage, teeth);
	OcclusalRest(positions_[1], MESIAL).draw(designImage, teeth);
	HalfClasp(positions_[0], material_, isInSameZone ? MESIAL : DISTAL, BUCCAL).draw(designImage, teeth);
	HalfClasp(positions_[1], material_, DISTAL, BUCCAL).draw(designImage, teeth);
}

ContinuousClasp* ContinuousClasp::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspMaterial, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new ContinuousClasp(positions, claspMaterial);
}

void ContinuousClasp::setLingualClaspArms(vector<Tooth> (&teeth)[nZones]) {
	RpdWithLingualClaspArms::setLingualClaspArms(teeth);
	hasLingualArms_[0] = hasLingualArms_[1] = hasLingualArms_[0] && hasLingualArms_[1];
}

ContinuousClasp::ContinuousClasp(vector<Position> const& positions, Material const& material) : Rpd(positions), RpdWithClaspRootOrRest(positions, {positions[0].zone == positions[1].zone ? MESIAL : DISTAL, DISTAL}), RpdWithLingualClaspArms(positions, material, {positions[0].zone == positions[1].zone ? MESIAL : DISTAL, DISTAL}) {}

void ContinuousClasp::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	auto const& isInSameZone = positions_[0].zone == positions_[1].zone;
	OcclusalRest(positions_[0], isInSameZone ? MESIAL : DISTAL).draw(designImage, teeth);
	OcclusalRest(positions_[1], DISTAL).draw(designImage, teeth);
	auto const& hasLingualClaspArm = hasLingualArms_[0];
	auto curve1 = getTooth(teeth, positions_[0]).getCurve(isInSameZone ? hasLingualClaspArm ? 180 : 0 : 60, isInSameZone ? 120 : hasLingualClaspArm ? 0 : 180), curve2 = getTooth(teeth, positions_[1]).getCurve(60, hasLingualClaspArm ? 0 : 180);
	if (hasLingualClaspArm) {
		if (!isInSameZone)
			reverse(curve1.begin(), curve1.end());
		curve2.insert(curve2.end(), curve1.begin(), curve1.end());
	}
	else
		polylines(designImage, curve1, false, 0, lineThicknessOfLevel[1 + (material_ == CAST)], LINE_AA);
	polylines(designImage, curve2, false, 0, lineThicknessOfLevel[1 + (material_ == CAST)], LINE_AA);
}

DentureBase::DentureBase(vector<Position> const& positions) : Rpd(positions) {}

DentureBase* DentureBase::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed, true);
	return new DentureBase(positions);
}

void DentureBase::setSide(const vector<Tooth> (&teeth)[nZones]) {
	auto isCoveringTail = false;
	for (auto i = 0; i < 2; ++i)
		if (isLastTooth(positions_[i])) {
			isCoveringTail = true;
			break;
		}
	side_ = !isCoveringTail && isBlockedByMajorConnector(teeth, positions_) ? SINGLE : DOUBLE;
}

void DentureBase::registerDentureBase(vector<Tooth> (&teeth)[nZones]) const { registerDentureBase(teeth, positions_); }

void DentureBase::registerExpectedAnchors(vector<Tooth> (&teeth)[nZones]) const { registerExpectedAnchors(teeth, positions_); }

void DentureBase::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	if (side_ == DOUBLE) {
		vector<vector<Point>> curves;
		computeStringCurves(teeth, positions_, {distanceScales[DENTURE_BASE_CURVE], -distanceScales[DENTURE_BASE_CURVE]}, {true, true}, {true, true}, true, curves);
		for (auto i = 0; i < 2; ++i)
			computePiecewiseSmoothCurve(curves[i], curves[i]);
		curves[0].insert(curves[0].end(), curves[1].rbegin(), curves[1].rend());
		polylines(designImage, curves[0], true, 0, lineThicknessOfLevel[2], LINE_AA);
	}
	else {
		vector<Point> curve;
		computeStringCurve(teeth, positions_, distanceScales[DENTURE_BASE_CURVE], {true, true}, {true, true}, false, curve);
		computeSmoothCurve(curve, curve);
		polylines(designImage, curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	}
}

void DentureBase::registerDentureBase(vector<Tooth> (&teeth)[nZones], vector<Position> positions) const {
	if (positions[0].zone == positions[1].zone)
		for (auto position = positions[0]; position <= positions[1]; ++position)
			getTooth(teeth, position).setDentureBase(side_);
	else {
		registerDentureBase(teeth, {Position(positions[0].zone, 0), positions[0]});
		registerDentureBase(teeth, {Position(positions[1].zone, 0), positions[1]});
	}
}

void DentureBase::registerExpectedAnchors(vector<Tooth> (&teeth)[nZones], vector<Position> const& positions) {
	if (positions.size() == 2) {
		getTooth(teeth, positions[0]).setExpectedDentureBaseAnchor(positions[0].zone == positions[1].zone ? MESIAL : DISTAL);
		getTooth(teeth, positions[1]).setExpectedDentureBaseAnchor(DISTAL);
	}
	else {
		registerExpectedAnchors(teeth, {positions[0] , positions[1]});
		registerExpectedAnchors(teeth, {positions[2] , positions[3]});
	}
}

EdentulousSpace::EdentulousSpace(vector<Position> const& positions) : Rpd(positions) {}

EdentulousSpace* EdentulousSpace::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed, true);
	return new EdentulousSpace(positions);
}

void EdentulousSpace::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	vector<vector<Point>> curves;
	computeStringCurves(teeth, positions_, {0.25F, -0.25F}, {false, false}, {false, false}, false, curves);
	for (auto curve = curves.begin(); curve < curves.end(); ++curve) {
		computeSmoothCurve(*curve, *curve);
		polylines(designImage, *curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	}
}

FullPalatalPlate* FullPalatalPlate::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpLingualConfrontation, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	bool hasLingualConfrontations[nZones][nTeethPerZone] = {};
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryLingualConfrontations(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, individual, hasLingualConfrontations);
	return new FullPalatalPlate(positions, hasLingualConfrontations);
}

FullPalatalPlate::FullPalatalPlate(vector<Position> const& positions, const bool (&hasLingualConfrontations)[nZones][nTeethPerZone]) : RpdAsMajorConnector(positions, hasLingualConfrontations) {}

void FullPalatalPlate::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	RpdAsMajorConnector::draw(designImage, teeth);
	vector<Point> curve, distalCurve, distalPoints;
	vector<vector<Point>> curves;
	computeLingualCurve(teeth, positions_, curve, curves, &distalPoints);
	reverse(curve.begin(), curve.end());
	computeDistalCurve(teeth, positions_, distalPoints, distalCurve);
	curve.insert(curve.end(), distalCurve.begin(), distalCurve.end());
	auto const& thisDesign = Mat(designImage.size(), CV_8U, 255);
	fillPoly(thisDesign, vector<vector<Point>>{curve}, 128, LINE_AA);
	bitwise_and(thisDesign, designImage, designImage);
	for (auto thisCurve = curves.begin(); thisCurve < curves.end(); ++thisCurve)
		polylines(designImage, *thisCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	polylines(designImage, distalCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
}

LingualBar* LingualBar::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpLingualConfrontation, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	bool hasLingualConfrontations[nZones][nTeethPerZone] = {};
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryLingualConfrontations(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, individual, hasLingualConfrontations);
	return new LingualBar(positions, hasLingualConfrontations);
}

LingualBar::LingualBar(vector<Position> const& positions, const bool (&hasLingualConfrontations)[nZones][nTeethPerZone]) : RpdAsMajorConnector(positions, hasLingualConfrontations) {}

void LingualBar::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	vector<Point> curve, tmpCurve;
	vector<vector<Point>> curves;
	float avgRadius;
	computeOuterCurve(teeth, positions_, curve, &avgRadius);
	polylines(designImage, curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	computeInnerCurve(teeth, positions_, avgRadius, tmpCurve, curves);
	for (auto thisCurve = curves.begin(); thisCurve < curves.end(); ++thisCurve)
		polylines(designImage, *thisCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	curve.insert(curve.end(), tmpCurve.rbegin(), tmpCurve.rend());
	auto const& thisDesign = Mat(designImage.size(), CV_8U, 255);
	fillPoly(thisDesign, vector<vector<Point>>{curve}, 128, LINE_AA);
	bitwise_and(thisDesign, designImage, designImage);
}

LingualPlate* LingualPlate::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpLingualConfrontation, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	bool hasLingualConfrontations[nZones][nTeethPerZone] = {};
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryLingualConfrontations(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, individual, hasLingualConfrontations);
	return new LingualPlate(positions, hasLingualConfrontations);
}

LingualPlate::LingualPlate(vector<Position> const& positions, const bool (&hasLingualConfrontations)[nZones][nTeethPerZone]) : RpdAsMajorConnector(positions, hasLingualConfrontations) {}

void LingualPlate::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	RpdAsMajorConnector::draw(designImage, teeth);
	vector<Point> curve, tmpCurve;
	computeOuterCurve(teeth, positions_, curve);
	vector<vector<Point>> curves;
	computeLingualCurve(teeth, positions_, tmpCurve, curves);
	polylines(designImage, curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	for (auto thisCurve = curves.begin(); thisCurve < curves.end(); ++thisCurve)
		polylines(designImage, *thisCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	curve.insert(curve.end(), tmpCurve.rbegin(), tmpCurve.rend());
	auto const& thisDesign = Mat(designImage.size(), CV_8U, 255);
	fillPoly(thisDesign, vector<vector<Point>>{curve}, 128, LINE_AA);
	bitwise_and(thisDesign, designImage, designImage);
}

LingualRest::LingualRest(vector<Position> const& positions, Material const& material, Direction const& direction) : Rpd(positions), RpdWithDirection(direction), RpdWithLingualRest(positions, material, direction) {}

LingualRest* LingualRest::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpRestMesialOrDistal, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	Direction restMesialOrDistal;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpRestMesialOrDistal, individual, restMesialOrDistal);
	return new LingualRest(positions, CAST, restMesialOrDistal);
}

void LingualRest::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	auto& tooth = getTooth(teeth, positions_[0]);
	auto curve = tooth.getCurve(240, 300);
	vector<Point> tmpCurve{curve.back(), curve[0]};
	auto& centroid = tooth.getCentroid();
	for (auto i = 0; i < 2; ++i)
		tmpCurve.insert(tmpCurve.end() - 1, centroid + (static_cast<Point2f>(tmpCurve[i * 2]) - centroid) * 0.8F);
	computePiecewiseSmoothCurve(tmpCurve, tmpCurve);
	curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
	polylines(designImage, curve, true, 0, lineThicknessOfLevel[1 + (material_ == CAST)], LINE_AA);
	fillPoly(designImage, vector<vector<Point>>{curve}, 0, LINE_AA);
	auto const& isMesial = direction_ == MESIAL;
	polylines(designImage, tooth.getCurve(isMesial ? 300 : 180, isMesial ? 0 : 240), false, 0, lineThicknessOfLevel[1 + (material_ == CAST)], LINE_AA);
}

OcclusalRest::OcclusalRest(vector<Position> const& positions, Direction const& direction) : Rpd(positions), RpdWithDirection(direction), RpdWithClaspRootOrRest(positions, direction) {}

OcclusalRest::OcclusalRest(Position const& position, Direction const& direction) : OcclusalRest(vector<Position>{position}, direction) {}

OcclusalRest* OcclusalRest::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpRestMesialOrDistal, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	Direction restMesialOrDistal;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpRestMesialOrDistal, individual, restMesialOrDistal);
	return new OcclusalRest(positions, restMesialOrDistal);
}

void OcclusalRest::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	auto& tooth = getTooth(teeth, positions_[0]);
	auto const& isMesial = direction_ == MESIAL;
	auto curve = tooth.getCurve(isMesial ? 340 : 160, isMesial ? 20 : 200);
	vector<Point> tmpCurve{curve.back(), (tooth.getCentroid() + static_cast<Point2f>(tooth.getAnglePoint(isMesial ? 0 : 180))) / 2, curve[0]};
	computeSmoothCurve(tmpCurve, tmpCurve, false, 0.3F);
	curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
	polylines(designImage, curve, true, 0, lineThicknessOfLevel[1], LINE_AA);
	fillPoly(designImage, vector<vector<Point>>{curve}, 0, LINE_AA);
}

PalatalPlate* PalatalPlate::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpLingualConfrontation, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	bool hasLingualConfrontations[nZones][nTeethPerZone] = {};
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryLingualConfrontations(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpLingualConfrontation, dpToothZone, dpToothOrdinal, individual, hasLingualConfrontations);
	return new PalatalPlate(positions, hasLingualConfrontations);
}

PalatalPlate::PalatalPlate(vector<Position> const& positions, const bool (&hasLingualConfrontations)[nZones][nTeethPerZone]) : RpdAsMajorConnector(positions, hasLingualConfrontations) {}

void PalatalPlate::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	RpdAsMajorConnector::draw(designImage, teeth);
	vector<Point> curve, mesialCurve, distalCurve, tmpCurve, distalPoints(2);
	vector<vector<Point>> curves;
	computeLingualCurve(teeth, {positions_[2], positions_[3]}, tmpCurve, curves, distalPoints[1]);
	curve.insert(curve.end(), tmpCurve.rbegin(), tmpCurve.rend());
	computeMesialCurve(teeth, {positions_[2], positions_[0]}, mesialCurve);
	curve.insert(curve.end(), mesialCurve.begin(), mesialCurve.end());
	computeLingualCurve(teeth, {positions_[0], positions_[1]}, tmpCurve, curves, distalPoints[0]);
	curve.insert(curve.end(), tmpCurve.begin(), tmpCurve.end());
	computeDistalCurve(teeth, {positions_[1], positions_[3]}, distalPoints, distalCurve);
	curve.insert(curve.end(), distalCurve.begin(), distalCurve.end());
	auto const& thisDesign = Mat(designImage.size(), CV_8U, 255);
	fillPoly(thisDesign, vector<vector<Point>>{curve}, 128, LINE_AA);
	bitwise_and(thisDesign, designImage, designImage);
	for (auto thisCurve = curves.begin(); thisCurve < curves.end(); ++thisCurve)
		polylines(designImage, *thisCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	polylines(designImage, mesialCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	polylines(designImage, distalCurve, false, 0, lineThicknessOfLevel[2], LINE_AA);
}

RingClasp::RingClasp(vector<Position> const& positions, Material const& material, Side const& tipSide) : Rpd(positions), RpdWithClaspRootOrRest(positions, material == CAST ? vector<Direction>{MESIAL, DISTAL} : vector<Direction>{MESIAL}), RpdWithLingualClaspArms(positions, material, MESIAL), tipSide_(tipSide) {}

RingClasp* RingClasp::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspMaterial, jobject const& dpClaspTipSide, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	Material claspMaterial;
	Side tipSide;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	queryTipSide(env, midGetInt, midResourceGetProperty, dpClaspTipSide, individual, tipSide);
	return new RingClasp(positions, claspMaterial, tipSide);
}

void RingClasp::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	OcclusalRest(positions_, MESIAL).draw(designImage, teeth);
	if (material_ == CAST)
		OcclusalRest(positions_, DISTAL).draw(designImage, teeth);
	auto const& isBuccal = tipSide_ == BUCCAL;
	polylines(designImage, getTooth(teeth, positions_[0]).getCurve(isBuccal ? 60 : 0, isBuccal ? 0 : 300), false, 0, lineThicknessOfLevel[1 + (material_ == CAST)], LINE_AA);
}

void RingClasp::queryTipSide(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midResourceGetProperty, jobject const& dpClaspTipSide, jobject const& individual, Side& tipSide) {
	auto const& tmp = env->CallObjectMethod(individual, midResourceGetProperty, dpClaspTipSide);
	tipSide = tmp ? static_cast<Side>(env->CallIntMethod(tmp, midGetInt)) : BUCCAL;
}

Rpa::Rpa(vector<Position> const& positions, Material const& material) : Rpd(positions), RpdWithMaterial(material), RpdWithClaspRootOrRest(positions, {MESIAL, DISTAL}) {}

Rpa* Rpa::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspMaterial, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new Rpa(positions, claspMaterial);
}

void Rpa::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	OcclusalRest(positions_, MESIAL).draw(designImage, teeth);
	GuidingPlate(positions_).draw(designImage, teeth);
	HalfClasp(positions_, material_, MESIAL, BUCCAL).draw(designImage, teeth);
}

Rpi::Rpi(vector<Position> const& positions) : Rpd(positions), RpdWithClaspRootOrRest(positions, {MESIAL, DISTAL}) {}

Rpi* Rpi::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	return new Rpi(positions);
}

void Rpi::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	OcclusalRest(positions_, MESIAL).draw(designImage, teeth);
	GuidingPlate(positions_).draw(designImage, teeth);
	IBar(positions_).draw(designImage, teeth);
}

WwClasp::WwClasp(vector<Position> const& positions, Direction const& direction, bool const& enableBuccalArm, bool const& enableLingualArm, bool const& enableRest) : Rpd(positions), AkersClasp(positions, WROUGHT_WIRE, direction, enableBuccalArm, enableLingualArm, enableRest) {}

WwClasp* WwClasp::createFromIndividual(JNIEnv* const& env, jmethodID const& midGetBoolean, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspTipDirection, jobject const& dpEnableBuccalArm, jobject const& dpEnableLingualArm, jobject const& dpEnableRest, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]) {
	vector<Position> positions;
	Direction claspTipDirection;
	bool enableBuccalArm, enableLingualArm, enableRest;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, positions, isEighthToothUsed);
	queryDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	queryPartEnablements(env, midGetBoolean, midResourceGetProperty, dpEnableBuccalArm, dpEnableLingualArm, dpEnableRest, individual, enableBuccalArm, enableLingualArm, enableRest);
	return new WwClasp(positions, claspTipDirection, enableBuccalArm, enableLingualArm, enableRest);
}

GuidingPlate::GuidingPlate(vector<Position> const& positions) : Rpd(positions) {}

void GuidingPlate::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	auto& tooth = getTooth(teeth, positions_[0]);
	auto& centroid = tooth.getCentroid();
	auto const& point = centroid + (static_cast<Point2f>(tooth.getAnglePoint(180)) - centroid) * 1.1F;
	auto const& direction = computeNormalDirection(point) * tooth.getRadius() * 2 / 3;
	line(designImage, point, point + direction, 0, lineThicknessOfLevel[2], LINE_AA);
	line(designImage, point, point - direction, 0, lineThicknessOfLevel[2], LINE_AA);
}

HalfClasp::HalfClasp(vector<Position> const& positions, Material const& material, Direction const& direction, Side const& side) : Rpd(positions), RpdWithMaterial(material), RpdWithDirection(direction), side_(side) {}

HalfClasp::HalfClasp(Position const& position, Material const& material, Direction const& direction, Side const& side) : HalfClasp(vector<Position>{position}, material, direction, side) {}

void HalfClasp::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
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
	polylines(designImage, curve, false, 0, lineThicknessOfLevel[1 + (material_ == CAST)], LINE_AA);
}

IBar::IBar(vector<Position> const& positions) : Rpd(positions) {}

void IBar::draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const {
	auto& tooth = getTooth(teeth, positions_[0]);
	auto const& a = tooth.getRadius() * 1.5F;
	auto &p1 = tooth.getAnglePoint(75), &p2 = tooth.getAnglePoint(165);
	auto const& c = (p1 + p2) / 2;
	auto const& d = computeNormalDirection(c);
	auto const& r = p1 - c;
	auto const& rou = norm(r);
	auto const& sinTheta = d.cross(r) / rou;
	auto const& b = rou * abs(sinTheta) / sqrt(1 - pow(rou / a, 2) * (1 - pow(sinTheta, 2)));
	auto inclination = atan2(d.y, d.x);
	auto t = radianToDegree(asin(rotate(r, -inclination).y / b));
	inclination = radianToDegree(inclination);
	if (t > 0)
		t -= 180;
	ellipse(designImage, c, Size(a, b), inclination, t, t + 180, 0, lineThicknessOfLevel[2], LINE_AA);
}
