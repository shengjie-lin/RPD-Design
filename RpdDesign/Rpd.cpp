#include <opencv2/imgproc.hpp>

#include "Rpd.h"
#include "Utilities.h"

Rpd::Position::Position(int zone, int ordinal): zone(zone), ordinal(ordinal) {}

RpdWithSingleSlot::RpdWithSingleSlot(const Position& position): position_(position) {}

void RpdWithSingleSlot::extractToothInfo(JNIEnv* env, jmethodID midGetInt, jmethodID midResourceGetProperty, jmethodID midStatementGetProperty, jobject dpToothZone, jobject dpToothOrdinal, jobject opComponentPosition, jobject individual, Position& position) {
	auto tooth = env->CallObjectMethod(individual, midResourceGetProperty, opComponentPosition);
	position.zone = env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothZone), midGetInt) - 1;
	position.ordinal = env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothOrdinal), midGetInt) - 1;
}

RpdWithRangedSlots::RpdWithRangedSlots(const Position& startPosition, const Position& endPosition): startPosition_(startPosition), endPosition_(endPosition) {}

void RpdWithRangedSlots::extractToothInfo(JNIEnv* env, jmethodID midGetInt, jmethodID midHasNext, jmethodID midListProperties, jmethodID midNext, jmethodID midStatementGetProperty, jobject dpToothZone, jobject dpToothOrdinal, jobject opComponentPosition, jobject individual, Position& startPosition, Position& endPosition) {
	auto teeth = env->CallObjectMethod(individual, midListProperties, opComponentPosition);
	vector<Position> positions;
	auto count = 0;
	while (env->CallBooleanMethod(teeth, midHasNext)) {
		auto tooth = env->CallObjectMethod(teeth, midNext);
		positions.push_back(Position(env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothZone), midGetInt) - 1, env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothOrdinal), midGetInt) - 1));
		++count;
	}
	if (count == 4) {
		auto zones = {positions[0].zone, positions[1].zone, positions[2].zone, positions[3].zone};
		auto ordinals = {positions[0].ordinal, positions[1].ordinal, positions[2].ordinal, positions[3].ordinal};
		auto zone = minmax(zones);
		auto ordinal = minmax(ordinals);
		startPosition = Position(zone.first, ordinal.first);
		endPosition = Position(zone.second, ordinal.second);
	}
	else {
		if (count == 1)
			endPosition = positions[0];
		else if (count == 2) {
			if (positions[0].zone > positions[1].zone || positions[0].zone == positions[1].zone && positions[0].ordinal > positions[1].ordinal)
				swap(positions[0], positions[1]);
			endPosition = positions[1];
		}
		startPosition = positions[0];
	}
}

RpdWithMaterial::RpdWithMaterial(const Material& material): material_(material), buccalMaterial_(UNSPECIFIED), lingualMaterial_(UNSPECIFIED) {}

RpdWithMaterial::RpdWithMaterial(const Material& buccalMaterial, const Material& lingualMaterial): material_(UNSPECIFIED), buccalMaterial_(buccalMaterial), lingualMaterial_(lingualMaterial) {}

void RpdWithMaterial::extractMaterial(JNIEnv* env, jmethodID midGetInt, jmethodID midResourceGetProperty, jobject dpClaspMaterial, jobject individual, Material& claspMaterial) {
	auto tmp = env->CallObjectMethod(individual, midResourceGetProperty, dpClaspMaterial);
	if (tmp)
		claspMaterial = static_cast<Material>(env->CallIntMethod(tmp, midGetInt));
	else
		claspMaterial = CAST;
}

void RpdWithMaterial::extractMaterial(JNIEnv* env, jmethodID midGetInt, jmethodID midResourceGetProperty, jobject dpClaspMaterial, jobject dpBuccalClaspMaterial, jobject dpLingualClaspMaterial, jobject individual, Material& claspMaterial, Material& buccalClaspMaterial, Material& lingualClaspMaterial) {
	auto tmp = env->CallObjectMethod(individual, midResourceGetProperty, dpClaspMaterial);
	if (tmp) {
		claspMaterial = static_cast<Material>(env->CallIntMethod(tmp, midGetInt));
		buccalClaspMaterial = lingualClaspMaterial = UNSPECIFIED;
	}
	else {
		auto tmp1 = env->CallObjectMethod(individual, midResourceGetProperty, dpBuccalClaspMaterial);
		auto tmp2 = env->CallObjectMethod(individual, midResourceGetProperty, dpLingualClaspMaterial);
		if (tmp1 || tmp2) {
			claspMaterial = UNSPECIFIED;
			buccalClaspMaterial = tmp1 ? static_cast<Material>(env->CallIntMethod(tmp1, midGetInt)) : CAST;
			lingualClaspMaterial = tmp2 ? static_cast<Material>(env->CallIntMethod(tmp2, midGetInt)) : CAST;
		}
		else {
			claspMaterial = CAST;
			buccalClaspMaterial = lingualClaspMaterial = UNSPECIFIED;
		}
	}
}

RpdWithDirection::RpdWithDirection(const Direction& direction): direction_(direction) {}

void RpdWithDirection::extractDirection(JNIEnv* env, jmethodID midGetInt, jmethodID midResourceGetProperty, jobject dpClaspTipDirection, jobject individual, Direction& claspTipDirection) { claspTipDirection = static_cast<Direction>(env->CallIntMethod(env->CallObjectMethod(individual, midResourceGetProperty, dpClaspTipDirection), midGetInt)); }

AkersClasp::AkersClasp(const Position& position, const Material& material, const Direction& direction): RpdWithSingleSlot(position), RpdWithMaterial(material), RpdWithDirection(direction) {}

AkersClasp::AkersClasp(const Position& position, const Material& buccalMaterial, const Material& lingualMaterial, const Direction& direction): RpdWithSingleSlot(position), RpdWithMaterial(buccalMaterial, lingualMaterial), RpdWithDirection(direction) {}

AkersClasp* AkersClasp::createFromIndividual(JNIEnv* env, jmethodID midGetInt, jmethodID midResourceGetProperty, jmethodID midStatementGetProperty, jobject dpClaspTipDirection, jobject dpClaspMaterial, jobject dpBuccalClaspMaterial, jobject dpLingualClaspMaterial, jobject dpToothZone, jobject dpToothOrdinal, jobject opComponentPosition, jobject individual) {
	Position position;
	Direction claspTipDirection;
	Material claspMaterial, buccalClaspMaterial, lingualClaspMaterial;
	extractToothInfo(env, midGetInt, midResourceGetProperty, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, position);
	extractDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	extractMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, dpBuccalClaspMaterial, dpLingualClaspMaterial, individual, claspMaterial, buccalClaspMaterial, lingualClaspMaterial);
	if (claspMaterial == UNSPECIFIED)
		return new AkersClasp(position, buccalClaspMaterial, lingualClaspMaterial, claspTipDirection);
	return new AkersClasp(position, claspMaterial, claspTipDirection);
}

void AkersClasp::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	if (material_ == UNSPECIFIED) {
		HalfClasp(position_, buccalMaterial_, direction_, HalfClasp::BUCCAL).draw(designImage, teeth);
		HalfClasp(position_, lingualMaterial_, direction_, HalfClasp::LINGUAL).draw(designImage, teeth);
	}
	else
		Clasp(position_, material_, direction_).draw(designImage, teeth);
	OcclusalRest(position_, static_cast<Direction>(1 - direction_)).draw(designImage, teeth);
}

Clasp::Clasp(const Position& position, const Material& material, const Direction& direction): RpdWithSingleSlot(position), RpdWithMaterial(material), RpdWithDirection(direction) {}

void Clasp::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	auto tooth = teeth[position_.zone][position_.ordinal];
	vector<Point> curve;
	if (direction_ == MESIAL)
		curve = tooth.getCurve(60, 300, false);
	else
		curve = tooth.getCurve(240, 120, false);
	if (material_ == CAST)
		polylines(designImage, curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
	else
		polylines(designImage, curve, false, 0, lineThicknessOfLevel[1], LINE_AA);
}

CombinedClasp::CombinedClasp(const Position& startPosition, const Position& endPosition, const Material& material): RpdWithRangedSlots(startPosition, endPosition), RpdWithMaterial(material) {}

CombinedClasp* CombinedClasp::createFromIndividual(JNIEnv* env, jmethodID midGetInt, jmethodID midHasNext, jmethodID midListProperties, jmethodID midNext, jmethodID midResourceGetProperty, jmethodID midStatementGetProperty, jobject dpClaspMaterial, jobject dpToothZone, jobject dpToothOrdinal, jobject opComponentPosition, jobject individual) {
	Position startPosition, endPosition;
	Material claspMaterial;
	extractToothInfo(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, startPosition, endPosition);
	extractMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new CombinedClasp(startPosition, endPosition, claspMaterial);
}

void CombinedClasp::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	Clasp(endPosition_, material_, RpdWithDirection::DISTAL).draw(designImage, teeth);
	OcclusalRest(endPosition_, RpdWithDirection::MESIAL).draw(designImage, teeth);
	if (startPosition_.zone == endPosition_.zone) {
		Clasp(startPosition_, material_, RpdWithDirection::MESIAL).draw(designImage, teeth);
		OcclusalRest(startPosition_, RpdWithDirection::DISTAL).draw(designImage, teeth);
	}
	else {
		Clasp(startPosition_, material_, RpdWithDirection::DISTAL).draw(designImage, teeth);
		OcclusalRest(startPosition_, RpdWithDirection::MESIAL).draw(designImage, teeth);
	}
}

DentureBase::DentureBase(const Position& startPosition, const Position& endPosition): RpdWithRangedSlots(startPosition, endPosition) {}

DentureBase* DentureBase::createFromIndividual(JNIEnv* env, jmethodID midGetInt, jmethodID midHasNext, jmethodID midListProperties, jmethodID midNext, jmethodID midStatementGetProperty, jobject dpToothZone, jobject dpToothOrdinal, jobject opComponentPosition, jobject individual) {
	Position startPosition, endPosition;
	extractToothInfo(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, startPosition, endPosition);
	return new DentureBase(startPosition, endPosition);
}

void DentureBase::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	vector<Point> curve;
	const Tooth *curTooth = nullptr, *lastTooth = nullptr;
	float sumOfRadii = 0;
	auto count = 0;
	if (startPosition_.zone == endPosition_.zone) {
		for (auto zone = startPosition_.zone, ordinal = startPosition_.ordinal; ordinal <= endPosition_.ordinal; ++ordinal) {
			++count;
			if (curTooth)
				lastTooth = curTooth;
			curTooth = &teeth[zone][ordinal];
			sumOfRadii += (*curTooth).getRadius();
			if (ordinal == startPosition_.ordinal)
				curve.push_back((*curTooth).getAnglePoint(0));
			else
				curve.push_back(((*curTooth).getAnglePoint(0) + (*lastTooth).getAnglePoint(180)) / 2);
			curve.push_back((*curTooth).getCentroid());
			if (ordinal == endPosition_.ordinal)
				curve.push_back((*curTooth).getAnglePoint(180));
		}
	}
	else {
		auto step = -1;
		for (auto zone = startPosition_.zone, ordinal = startPosition_.ordinal; zone == startPosition_.zone || ordinal <= endPosition_.ordinal; ordinal += step) {
			++count;
			if (curTooth)
				lastTooth = curTooth;
			curTooth = &teeth[zone][ordinal];
			sumOfRadii += (*curTooth).getRadius();
			if (zone == startPosition_.zone && ordinal == startPosition_.ordinal)
				curve.push_back((*curTooth).getAnglePoint(180));
			else
				curve.push_back(((*curTooth).getAnglePoint(step ? 90 * (1 - step) : 0) + (*lastTooth).getAnglePoint(step ? 90 * (1 + step) : 0)) / 2);
			curve.push_back((*curTooth).getCentroid());
			if (zone == endPosition_.zone && ordinal == endPosition_.ordinal)
				curve.push_back((*curTooth).getAnglePoint(180));
			if (ordinal == 0)
				if (step) {
					step = 0;
					++zone;
				}
				else
					step = 1;
		}
	}
	curve.insert(curve.begin(), curve[0]);
	curve.push_back(curve.back());
	auto avgRadius = sumOfRadii / count;
	for (auto i = 0; i < curve.size(); ++i)
		curve[i] += roundToInt(computeNormalDirection(curve[i], nullptr) * avgRadius * (i == 0 || i == curve.size() - 1 ? 0.25 : 1.6));
	drawSmoothCurve(designImage, curve, avgRadius * 5, 0, lineThicknessOfLevel[2]);
}

EdentulousSpace::EdentulousSpace(const Position& startPosition, const Position& endPosition): RpdWithRangedSlots(startPosition, endPosition) {}

EdentulousSpace* EdentulousSpace::createFromIndividual(JNIEnv* env, jmethodID midGetInt, jmethodID midHasNext, jmethodID midListProperties, jmethodID midNext, jmethodID midStatementGetProperty, jobject dpToothZone, jobject dpToothOrdinal, jobject opComponentPosition, jobject individual) {
	Position startPosition, endPosition;
	extractToothInfo(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, startPosition, endPosition);
	return new EdentulousSpace(startPosition, endPosition);
}

void EdentulousSpace::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	vector<Point> curve;
	const Tooth *curTooth = nullptr, *lastTooth = nullptr;
	float sumOfRadii = 0;
	auto count = 0;
	if (startPosition_.zone == endPosition_.zone) {
		for (auto zone = startPosition_.zone, ordinal = startPosition_.ordinal; ordinal <= endPosition_.ordinal; ++ordinal) {
			++count;
			if (curTooth)
				lastTooth = curTooth;
			curTooth = &teeth[zone][ordinal];
			sumOfRadii += (*curTooth).getRadius();
			if (ordinal == startPosition_.ordinal)
				curve.push_back((*curTooth).getAnglePoint(0));
			else
				curve.push_back(((*curTooth).getAnglePoint(0) + (*lastTooth).getAnglePoint(180)) / 2);
			curve.push_back((*curTooth).getCentroid());
			if (ordinal == endPosition_.ordinal)
				curve.push_back((*curTooth).getAnglePoint(180));
		}
	}
	else {
		auto step = -1;
		for (auto zone = startPosition_.zone, ordinal = startPosition_.ordinal; zone == startPosition_.zone || ordinal <= endPosition_.ordinal; ordinal += step) {
			++count;
			if (curTooth)
				lastTooth = curTooth;
			curTooth = &teeth[zone][ordinal];
			sumOfRadii += (*curTooth).getRadius();
			if (zone == startPosition_.zone && ordinal == startPosition_.ordinal)
				curve.push_back((*curTooth).getAnglePoint(180));
			else
				curve.push_back(((*curTooth).getAnglePoint(step ? 90 * (1 - step) : 0) + (*lastTooth).getAnglePoint(step ? 90 * (1 + step) : 0)) / 2);
			curve.push_back((*curTooth).getCentroid());
			if (zone == endPosition_.zone && ordinal == endPosition_.ordinal)
				curve.push_back((*curTooth).getAnglePoint(180));
			if (ordinal == 0)
				if (step) {
					step = 0;
					++zone;
				}
				else
					step = 1;
		}
	}
	auto avgRadius = sumOfRadii / count;
	vector<Point> curve1(curve.size()), curve2(curve.size());
	for (auto i = 0; i < curve.size(); ++i) {
		auto delta = roundToInt(computeNormalDirection(curve[i], nullptr) * avgRadius * 0.25);
		curve1[i] = curve[i] + delta;
		curve2[i] = curve[i] - delta;
	}
	drawSmoothCurve(designImage, curve1, avgRadius * 5, 0, lineThicknessOfLevel[2]);
	drawSmoothCurve(designImage, curve2, avgRadius * 5, 0, lineThicknessOfLevel[2]);
}

GuidingPlate::GuidingPlate(const Position& position): RpdWithSingleSlot(position) {}

void GuidingPlate::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	auto tooth = teeth[position_.zone][position_.ordinal];
	Point centroid = tooth.getCentroid();
	auto point = centroid + (tooth.getAnglePoint(180) - centroid) * 1.1;
	auto direction = roundToInt(computeNormalDirection(point, nullptr) * tooth.getRadius() * 2 / 3);
	line(designImage, point, point + direction, 0, lineThicknessOfLevel[2], LINE_AA);
	line(designImage, point, point - direction, 0, lineThicknessOfLevel[2], LINE_AA);
}

HalfClasp::HalfClasp(const Position& position, const Material& material, const Direction& direction, const Side& side): RpdWithSingleSlot(position), RpdWithMaterial(material), RpdWithDirection(direction), side_(side) {}

void HalfClasp::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	auto tooth = teeth[position_.zone][position_.ordinal];
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

IBar::IBar(const Position& position): RpdWithSingleSlot(position) {}

void IBar::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	auto tooth = teeth[position_.zone][position_.ordinal];
	auto a = tooth.getRadius() * 1.5;
	Point2f point1 = tooth.getAnglePoint(75), point2 = tooth.getAnglePoint(165);
	auto center = (point1 + point2) / 2;
	auto direction = computeNormalDirection(center, nullptr);
	auto r = point1 - center;
	auto rou = norm(r);
	auto sinTheta = direction.cross(r) / rou;
	auto b = rou * abs(sinTheta) / sqrt(1 - pow(rou / a, 2) * (1 - pow(sinTheta, 2)));
	auto inclination = atan2(direction.y, direction.x);
	auto t = radian2Degree(asin(rotate(r, -inclination).y / b));
	inclination = radian2Degree(inclination);
	if (t > 0)
		t -= 180;
	EllipticCurve(center, roundToInt(Size(a, b)), inclination, t, t + 180).draw(designImage, 0, lineThicknessOfLevel[2]);
}

LingualRest::LingualRest(const Position& position, const Direction& direction): RpdWithSingleSlot(position), RpdWithDirection(direction) {}

LingualRest* LingualRest::createFromIndividual(JNIEnv* env, jmethodID midGetInt, jmethodID midResourceGetProperty, jmethodID midStatementGetProperty, jobject dpRestMesialOrDistal, jobject dpToothZone, jobject dpToothOrdinal, jobject opComponentPosition, jobject individual) {
	Position position;
	Direction restMesialOrDistal;
	extractToothInfo(env, midGetInt, midResourceGetProperty, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, position);
	extractDirection(env, midGetInt, midResourceGetProperty, dpRestMesialOrDistal, individual, restMesialOrDistal);
	return new LingualRest(position, restMesialOrDistal);
}

void LingualRest::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	auto curve = teeth[position_.zone][position_.ordinal].getCurve(240, 300);
	polylines(designImage, curve, true, 0, lineThicknessOfLevel[1], LINE_AA);
	fillConvexPoly(designImage, curve, 0, LINE_AA);
}

OcclusalRest::OcclusalRest(const Position& position, const Direction& direction): RpdWithSingleSlot(position), RpdWithDirection(direction) {}

OcclusalRest* OcclusalRest::createFromIndividual(JNIEnv* env, jmethodID midGetInt, jmethodID midResourceGetProperty, jmethodID midStatementGetProperty, jobject dpRestMesialOrDistal, jobject dpToothZone, jobject dpToothOrdinal, jobject opComponentPosition, jobject individual) {
	Position position;
	Direction restMesialOrDistal;
	extractToothInfo(env, midGetInt, midResourceGetProperty, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, position);
	extractDirection(env, midGetInt, midResourceGetProperty, dpRestMesialOrDistal, individual, restMesialOrDistal);
	return new OcclusalRest(position, restMesialOrDistal);
}

void OcclusalRest::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	auto tooth = teeth[position_.zone][position_.ordinal];
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
	auto centroid = tooth.getCentroid();
	curve.push_back(centroid + (point - centroid) / 2);
	polylines(designImage, curve, true, 0, lineThicknessOfLevel[1], LINE_AA);
	fillConvexPoly(designImage, curve, 0, LINE_AA);
}

PalatalPlate::PalatalPlate(const Position& startPosition, const Position& endPosition, const vector<Position>& lingualConfrontations): RpdWithRangedSlots(startPosition, endPosition), lingualConfrontations_(lingualConfrontations) {}

PalatalPlate* PalatalPlate::createFromIndividual(JNIEnv* env, jmethodID midGetInt, jmethodID midHasNext, jmethodID midListProperties, jmethodID midNext, jmethodID midStatementGetProperty, jobject dpToothZone, jobject dpToothOrdinal, jobject opComponentPosition, jobject opMajorConnectorKeyPosition, jobject individual) {
	Position startPosition, endPosition;
	vector<Position> lingualConfrontations;
	extractToothInfo(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, startPosition, endPosition);
	auto teeth = env->CallObjectMethod(individual, midListProperties, opMajorConnectorKeyPosition);
	while (env->CallBooleanMethod(teeth, midHasNext)) {
		auto tooth = env->CallObjectMethod(teeth, midNext);
		lingualConfrontations.push_back(Position(env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothZone), midGetInt) - 1, env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothOrdinal), midGetInt) - 1));
	}
	return new PalatalPlate(startPosition, endPosition, lingualConfrontations);
}

void PalatalPlate::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	for (auto it = lingualConfrontations_.begin(); it < lingualConfrontations_.end(); ++it)
		Plating(Position(it->zone, it->ordinal)).draw(designImage, teeth);
	/*TODO*/
}

Plating::Plating(const Position& position): RpdWithSingleSlot(position) {}

void Plating::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	auto curve = teeth[position_.zone][position_.ordinal].getCurve(180, 359);
	polylines(designImage, curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
}

RingClasp::RingClasp(const Position& position, const Material& material): RpdWithSingleSlot(position), RpdWithMaterial(material) {}

RingClasp* RingClasp::createFromIndividual(JNIEnv* env, jmethodID midGetInt, jmethodID midResourceGetProperty, jmethodID midStatementGetProperty, jobject dpClaspMaterial, jobject dpToothZone, jobject dpToothOrdinal, jobject opComponentPosition, jobject individual) {
	Position position;
	Material claspMaterial;
	extractToothInfo(env, midGetInt, midResourceGetProperty, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, position);
	extractMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new RingClasp(position, claspMaterial);
}

void RingClasp::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	auto tooth = teeth[position_.zone][position_.ordinal];
	vector<Point> curve;
	if (position_.zone < 2)
		curve = tooth.getCurve(60, 0, false);
	else
		curve = tooth.getCurve(0, 300, false);
	if (material_ == CAST) {
		polylines(designImage, curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
		OcclusalRest(position_, RpdWithDirection::DISTAL).draw(designImage, teeth);
	}
	else
		polylines(designImage, curve, false, 0, lineThicknessOfLevel[1], LINE_AA);
	OcclusalRest(position_, RpdWithDirection::MESIAL).draw(designImage, teeth);
}

Rpa::Rpa(const Position& position, const Material& material): RpdWithSingleSlot(position), RpdWithMaterial(material) {}

Rpa* Rpa::createFromIndividual(JNIEnv* env, jmethodID midGetInt, jmethodID midResourceGetProperty, jmethodID midStatementGetProperty, jobject dpClaspMaterial, jobject dpToothZone, jobject dpToothOrdinal, jobject opComponentPosition, jobject individual) {
	Position position;
	Material claspMaterial;
	extractToothInfo(env, midGetInt, midResourceGetProperty, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, position);
	extractMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new Rpa(position, claspMaterial);
}

void Rpa::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	OcclusalRest(position_, RpdWithDirection::MESIAL).draw(designImage, teeth);
	GuidingPlate(position_).draw(designImage, teeth);
	HalfClasp(position_, material_, RpdWithDirection::MESIAL, HalfClasp::BUCCAL).draw(designImage, teeth);
}

Rpi::Rpi(const Position& position): RpdWithSingleSlot(position) {}

Rpi* Rpi::createFromIndividual(JNIEnv* env, jmethodID midGetInt, jmethodID midResourceGetProperty, jmethodID midStatementGetProperty, jobject dpToothZone, jobject dpToothOrdinal, jobject opComponentPosition, jobject individual) {
	Position position;
	extractToothInfo(env, midGetInt, midResourceGetProperty, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, position);
	return new Rpi(position);
}

void Rpi::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	OcclusalRest(position_, RpdWithDirection::MESIAL).draw(designImage, teeth);
	GuidingPlate(position_).draw(designImage, teeth);
	IBar(position_).draw(designImage, teeth);
}

WwClasp::WwClasp(const Position& position, const Direction& direction): RpdWithSingleSlot(position), RpdWithDirection(direction) {}

WwClasp* WwClasp::createFromIndividual(JNIEnv* env, jmethodID midGetInt, jmethodID midResourceGetProperty, jmethodID midStatementGetProperty, jobject dpClaspTipDirection, jobject dpToothZone, jobject dpToothOrdinal, jobject opComponentPosition, jobject individual) {
	Position position;
	Direction claspTipDirection;
	extractToothInfo(env, midGetInt, midResourceGetProperty, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, position);
	extractDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	return new WwClasp(position, claspTipDirection);
}

void WwClasp::draw(Mat& designImage, const vector<vector<Tooth>>& teeth) const {
	Clasp(position_, RpdWithMaterial::WROUGHT_WIRE, direction_).draw(designImage, teeth);
	OcclusalRest(position_, static_cast<Direction>(1 - direction_)).draw(designImage, teeth);
}
