#include <opencv2/imgproc.hpp>

#include "Rpd.h"
#include "Tooth.h"
#include "Utilities.h"

Rpd::Position::Position(const int& zone, const int& ordinal): zone(zone), ordinal(ordinal) {}

const Rpd::Position& RpdWithSingleSlot::getPosition() const { return position_; }

RpdWithSingleSlot::RpdWithSingleSlot(const Position& position): position_(position) {}

void RpdWithSingleSlot::queryPosition(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, Position& position) {
	auto tooth = env->CallObjectMethod(individual, midResourceGetProperty, opComponentPosition);
	position.zone = env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothZone), midGetInt) - 1;
	position.ordinal = env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothOrdinal), midGetInt) - 1;
}

const Rpd::Position& RpdWithMultiSlots::getStartPosition() const { return startPosition_; }

const Rpd::Position& RpdWithMultiSlots::getEndPosition() const { return endPosition_; }

RpdWithMultiSlots::RpdWithMultiSlots(const Position& startPosition, const Position& endPosition): startPosition_(startPosition), endPosition_(endPosition) {}

void RpdWithMultiSlots::queryPositions(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, Position& startPosition, Position& endPosition) {
	auto teeth = env->CallObjectMethod(individual, midListProperties, opComponentPosition);
	vector<Position> positions;
	auto count = 0;
	while (env->CallBooleanMethod(teeth, midHasNext)) {
		auto tooth = env->CallObjectMethod(teeth, midNext);
		positions.push_back(Position(env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothZone), midGetInt) - 1, env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothOrdinal), midGetInt) - 1));
		++count;
	}
	if (count == 1)
		endPosition = positions[0];
	else if (count == 2) {
		if (positions[0].zone > positions[1].zone || positions[0].zone == positions[1].zone && positions[0].ordinal > positions[1].ordinal)
			swap(positions[0], positions[1]);
		endPosition = positions[1];
	}
	startPosition = positions[0];
}

RpdWithMaterial::RpdWithMaterial(const Material& material): material_(material), buccalMaterial_(UNSPECIFIED), lingualMaterial_(UNSPECIFIED) {}

RpdWithMaterial::RpdWithMaterial(const Material& buccalMaterial, const Material& lingualMaterial): material_(UNSPECIFIED), buccalMaterial_(buccalMaterial), lingualMaterial_(lingualMaterial) {}

void RpdWithMaterial::queryMaterial(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspMaterial, const jobject& individual, Material& claspMaterial) {
	auto tmp = env->CallObjectMethod(individual, midResourceGetProperty, dpClaspMaterial);
	if (tmp)
		claspMaterial = static_cast<Material>(env->CallIntMethod(tmp, midGetInt));
	else
		claspMaterial = CAST;
}

void RpdWithMaterial::queryMaterial(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspMaterial, const jobject& dpBuccalClaspMaterial, const jobject& dpLingualClaspMaterial, const jobject& individual, Material& claspMaterial, Material& buccalClaspMaterial, Material& lingualClaspMaterial) {
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

void RpdWithDirection::queryDirection(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspTipDirection, const jobject& individual, Direction& claspTipDirection) { claspTipDirection = static_cast<Direction>(env->CallIntMethod(env->CallObjectMethod(individual, midResourceGetProperty, dpClaspTipDirection), midGetInt)); }

RpdWithLingualBlockage::RpdWithLingualBlockage(): lingualBlockage_(CLASP), scope_(POINT) {}

RpdWithLingualBlockage::RpdWithLingualBlockage(const LingualBlockage& lingualBlockage, const Scope& scope): lingualBlockage_(lingualBlockage), scope_(scope) {}

const RpdWithLingualBlockage::LingualBlockage& RpdWithLingualBlockage::getLingualBlockage() const { return lingualBlockage_; }

const RpdWithLingualBlockage::Scope& RpdWithLingualBlockage::getScope() const { return scope_; }

RpdAsMajorConnector::Anchor::Anchor(const Position& position, const RpdWithDirection::Direction& direction): position(position), direction(direction) {}

RpdAsMajorConnector::RpdAsMajorConnector(const vector<Anchor>& anchors): anchors_(anchors) {}

void RpdAsMajorConnector::queryAnchors(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpAnchorMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& opMajorConnectorAnchor, const jobject& individual, const Coverage& coverage, vector<Anchor>& anchors) {
	auto count = 0;
	auto majorConnectorAnchors = env->CallObjectMethod(individual, midListProperties, opMajorConnectorAnchor);
	while (env->CallBooleanMethod(majorConnectorAnchors, midHasNext)) {
		auto anchor = env->CallObjectMethod(majorConnectorAnchors, midNext);
		auto tooth = env->CallObjectMethod(anchor, midStatementGetProperty, opComponentPosition);
		anchors.push_back(Anchor(Position(env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothZone), midGetInt) - 1, env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothOrdinal), midGetInt) - 1), static_cast<RpdWithDirection::Direction>(env->CallIntMethod(env->CallObjectMethod(anchor, midStatementGetProperty, dpAnchorMesialOrDistal), midGetInt))));
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

AkersClasp::AkersClasp(const Position& position, const Material& material, const Direction& direction): RpdWithSingleSlot(position), RpdWithMaterial(material), RpdWithDirection(direction), RpdWithLingualBlockage() {}

AkersClasp* AkersClasp::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual) {
	Position position;
	Direction claspTipDirection;
	Material claspMaterial;
	queryPosition(env, midGetInt, midResourceGetProperty, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, position);
	queryDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new AkersClasp(position, claspMaterial, claspTipDirection);
}

void AkersClasp::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	Clasp(position_, material_, direction_).draw(designImage, teeth);
	OcclusalRest(position_, static_cast<Direction>(1 - direction_)).draw(designImage, teeth);
}

Clasp::Clasp(const Position& position, const Material& material, const Direction& direction): RpdWithSingleSlot(position), RpdWithMaterial(material), RpdWithDirection(direction) {}

void Clasp::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	auto& tooth = getTooth(teeth, position_);
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

CombinationClasp::CombinationClasp(const Position& position, const Direction& direction): RpdWithSingleSlot(position), RpdWithDirection(direction) {}

CombinationClasp* CombinationClasp::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual) {
	Position position;
	Direction claspTipDirection;
	queryPosition(env, midGetInt, midResourceGetProperty, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, position);
	queryDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	return new CombinationClasp(position, claspTipDirection);
}

void CombinationClasp::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	HalfClasp(position_, RpdWithMaterial::WROUGHT_WIRE, direction_, HalfClasp::BUCCAL).draw(designImage, teeth);
	HalfClasp(position_, RpdWithMaterial::CAST, direction_, HalfClasp::LINGUAL).draw(designImage, teeth);
	OcclusalRest(position_, static_cast<Direction>(1 - direction_)).draw(designImage, teeth);
}

CombinedClasp::CombinedClasp(const Position& startPosition, const Position& endPosition, const Material& material): RpdWithMultiSlots(startPosition, endPosition), RpdWithMaterial(material), RpdWithLingualBlockage(CLASP, LINE) {}

CombinedClasp* CombinedClasp::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual) {
	Position startPosition, endPosition;
	Material claspMaterial;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, startPosition, endPosition);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new CombinedClasp(startPosition, endPosition, claspMaterial);
}

void CombinedClasp::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
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

DentureBase::DentureBase(const Position& startPosition, const Position& endPosition): RpdWithMultiSlots(startPosition, endPosition) {}

DentureBase* DentureBase::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual) {
	Position startPosition, endPosition;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, startPosition, endPosition);
	return new DentureBase(startPosition, endPosition);
}

void DentureBase::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	vector<Point> curve;
	float avgRadius;
	bool haslingualBlockage;
	computeStringingCurve(teeth, startPosition_, endPosition_, curve, avgRadius, &haslingualBlockage);
	auto isCoveringTail = false;
	if (startPosition_.ordinal == teeth[startPosition_.zone].size() - 1) {
		isCoveringTail = true;
		Point2f distalPoint = getTooth(teeth, startPosition_).getAnglePoint(180);
		curve[0] = roundToInt(distalPoint + rotate(computeNormalDirection(distalPoint), -CV_PI / 2) * avgRadius);
	}
	auto& zone = endPosition_.zone;
	if (endPosition_.ordinal == teeth[zone].size() - 1) {
		isCoveringTail = true;
		Point2f distalPoint = getTooth(teeth, endPosition_).getAnglePoint(180);
		curve.back() = roundToInt(distalPoint + rotate(computeNormalDirection(distalPoint), CV_PI * (zone % 2 - 0.5)) * avgRadius);
	}
	if (isCoveringTail || !haslingualBlockage) {
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

EdentulousSpace::EdentulousSpace(const Position& startPosition, const Position& endPosition): RpdWithMultiSlots(startPosition, endPosition) {}

EdentulousSpace* EdentulousSpace::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual) {
	Position startPosition, endPosition;
	queryPositions(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, startPosition, endPosition);
	return new EdentulousSpace(startPosition, endPosition);
}

void EdentulousSpace::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	vector<Point> curve;
	float avgRadius;
	computeStringingCurve(teeth, startPosition_, endPosition_, curve, avgRadius);
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

GuidingPlate::GuidingPlate(const Position& position): RpdWithSingleSlot(position) {}

void GuidingPlate::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	auto& tooth = getTooth(teeth, position_);
	auto& centroid = tooth.getCentroid();
	auto point = roundToInt(centroid + (static_cast<Point2f>(tooth.getAnglePoint(180)) - centroid) * 1.1);
	auto direction = roundToInt(computeNormalDirection(point) * tooth.getRadius() * 2 / 3);
	line(designImage, point, point + direction, 0, lineThicknessOfLevel[2], LINE_AA);
	line(designImage, point, point - direction, 0, lineThicknessOfLevel[2], LINE_AA);
}

HalfClasp::HalfClasp(const Position& position, const Material& material, const Direction& direction, const Side& side): RpdWithSingleSlot(position), RpdWithMaterial(material), RpdWithDirection(direction), side_(side) {}

void HalfClasp::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	auto& tooth = getTooth(teeth, position_);
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

void IBar::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	auto& tooth = getTooth(teeth, position_);
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

LingualRest::LingualRest(const Position& position, const Direction& direction): RpdWithSingleSlot(position), RpdWithDirection(direction) {}

LingualRest* LingualRest::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual) {
	Position position;
	Direction restMesialOrDistal;
	queryPosition(env, midGetInt, midResourceGetProperty, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, position);
	queryDirection(env, midGetInt, midResourceGetProperty, dpRestMesialOrDistal, individual, restMesialOrDistal);
	return new LingualRest(position, restMesialOrDistal);
}

void LingualRest::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	auto curve = getTooth(teeth, position_).getCurve(240, 300);
	polylines(designImage, curve, true, 0, lineThicknessOfLevel[1], LINE_AA);
	fillPoly(designImage, vector<vector<Point>>{curve}, 0, LINE_AA);
}

OcclusalRest::OcclusalRest(const Position& position, const Direction& direction): RpdWithSingleSlot(position), RpdWithDirection(direction) {}

OcclusalRest* OcclusalRest::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual) {
	Position position;
	Direction restMesialOrDistal;
	queryPosition(env, midGetInt, midResourceGetProperty, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, position);
	queryDirection(env, midGetInt, midResourceGetProperty, dpRestMesialOrDistal, individual, restMesialOrDistal);
	return new OcclusalRest(position, restMesialOrDistal);
}

void OcclusalRest::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	auto& tooth = getTooth(teeth, position_);
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

PalatalPlate::PalatalPlate(const vector<Anchor>& anchors, const vector<Position>& lingualConfrontations): RpdAsMajorConnector(anchors), lingualConfrontations_(lingualConfrontations) {}

PalatalPlate* PalatalPlate::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpAnchorMesialOrDistal, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& opMajorConnectorAnchor, const jobject& individual) {
	vector<Anchor> anchors;
	vector<Position> lingualConfrontations;
	queryAnchors(env, midGetInt, midHasNext, midListProperties, midNext, midStatementGetProperty, dpAnchorMesialOrDistal, dpToothZone, dpToothOrdinal, opComponentPosition, opMajorConnectorAnchor, individual, PLANAR, anchors);
	auto lcTeeth = env->CallObjectMethod(individual, midListProperties, dpLingualConfrontation);
	while (env->CallBooleanMethod(lcTeeth, midHasNext)) {
		auto tooth = env->CallObjectMethod(lcTeeth, midNext);
		lingualConfrontations.push_back(Position(env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothZone), midGetInt) - 1, env->CallIntMethod(env->CallObjectMethod(tooth, midStatementGetProperty, dpToothOrdinal), midGetInt) - 1));
	}
	return new PalatalPlate(anchors, lingualConfrontations);
}

void PalatalPlate::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	/*TODO: consider lingual blockage*/
	for (auto position = lingualConfrontations_.begin(); position < lingualConfrontations_.end(); ++position)
		Plating(*position).draw(designImage, teeth);
	vector<Point> curve, mesialCurve, distalCurve;
	auto delta = anchors_[3].position.ordinal - anchors_[0].position.ordinal + anchors_[3].direction - anchors_[0].direction;
	mesialCurve = {getPoint(teeth, anchors_[3]), getPoint(teeth, anchors_[3]) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[3], 1)) * getTooth(teeth, anchors_[3]).getRadius() * (1 + max(delta, 0))), getPoint(teeth, anchors_[0]) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[0], 1)) * getTooth(teeth, anchors_[0]).getRadius() * (1 - min(delta, 0))), getPoint(teeth, anchors_[0])};
	mesialCurve.insert(mesialCurve.begin() + 2, delta > 0 ? initializer_list<Point>{roundToInt((getTooth(teeth, anchors_[3], 1).getCentroid() + getTooth(teeth, anchors_[3], 1, true).getCentroid()) / 2), getPoint(teeth, anchors_[3], 0, true) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[3], 1, true)) * getTooth(teeth, anchors_[3], true).getRadius() * (1 + delta))} : delta < 0 ? initializer_list<Point>{getPoint(teeth, anchors_[0], 0, true) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[0], 1, true)) * getTooth(teeth, anchors_[0], true).getRadius() * (1 - delta)), roundToInt((getTooth(teeth, anchors_[0], 1).getCentroid() + getTooth(teeth, anchors_[0], 1, true).getCentroid()) / 2)} : initializer_list<Point>{roundToInt((getTooth(teeth, anchors_[3], 1).getCentroid() + getTooth(teeth, anchors_[0], 1).getCentroid()) / 2)});
	computeSmoothCurve(mesialCurve, mesialCurve);
	delta = anchors_[2].position.ordinal - anchors_[1].position.ordinal + anchors_[2].direction - anchors_[1].direction;
	distalCurve = {getPoint(teeth, anchors_[1]), getPoint(teeth, anchors_[1]) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[1], -1)) * getTooth(teeth, anchors_[1]).getRadius() * (1 + max(delta, 0))), getPoint(teeth, anchors_[2]) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[2], -1)) * getTooth(teeth, anchors_[2]).getRadius() * (1 - min(delta, 0))), getPoint(teeth, anchors_[2])};
	distalCurve.insert(distalCurve.begin() + 2, delta > 0 ? initializer_list<Point>{roundToInt((getTooth(teeth, anchors_[1], -1).getCentroid() + getTooth(teeth, anchors_[1], -1, true).getCentroid()) / 2), getPoint(teeth, anchors_[1], 0, true) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[1], -1, true)) * getTooth(teeth, anchors_[1], true).getRadius() * (1 + delta))} : delta < 0 ? initializer_list<Point>{getPoint(teeth, anchors_[2], 0, true) - roundToInt(computeNormalDirection(getPoint(teeth, anchors_[2], -1, true)) * getTooth(teeth, anchors_[2], true).getRadius() * (1 - delta)), roundToInt((getTooth(teeth, anchors_[2], -1).getCentroid() + getTooth(teeth, anchors_[2], -1, true).getCentroid()) / 2)} : initializer_list<Point>{roundToInt((getTooth(teeth, anchors_[1], -1).getCentroid() + getTooth(teeth, anchors_[2], -1).getCentroid()) / 2)});
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

Plating::Plating(const Position& position): RpdWithSingleSlot(position) {}

void Plating::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	auto curve = getTooth(teeth, position_).getCurve(180, 0);
	polylines(designImage, curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
}

RingClasp::RingClasp(const Position& position, const Material& material): RpdWithSingleSlot(position), RpdWithMaterial(material), RpdWithLingualBlockage() {}

RingClasp* RingClasp::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual) {
	Position position;
	Material claspMaterial;
	queryPosition(env, midGetInt, midResourceGetProperty, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, position);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new RingClasp(position, claspMaterial);
}

void RingClasp::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	auto& tooth = getTooth(teeth, position_);
	vector<Point> curve;
	if (position_.zone < 2)
		curve = tooth.getCurve(60, 0);
	else
		curve = tooth.getCurve(0, 300);
	if (material_ == CAST) {
		polylines(designImage, curve, false, 0, lineThicknessOfLevel[2], LINE_AA);
		OcclusalRest(position_, RpdWithDirection::DISTAL).draw(designImage, teeth);
	}
	else
		polylines(designImage, curve, false, 0, lineThicknessOfLevel[1], LINE_AA);
	OcclusalRest(position_, RpdWithDirection::MESIAL).draw(designImage, teeth);
}

Rpa::Rpa(const Position& position, const Material& material): RpdWithSingleSlot(position), RpdWithMaterial(material) {}

Rpa* Rpa::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual) {
	Position position;
	Material claspMaterial;
	queryPosition(env, midGetInt, midResourceGetProperty, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, position);
	queryMaterial(env, midGetInt, midResourceGetProperty, dpClaspMaterial, individual, claspMaterial);
	return new Rpa(position, claspMaterial);
}

void Rpa::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	OcclusalRest(position_, RpdWithDirection::MESIAL).draw(designImage, teeth);
	GuidingPlate(position_).draw(designImage, teeth);
	HalfClasp(position_, material_, RpdWithDirection::MESIAL, HalfClasp::BUCCAL).draw(designImage, teeth);
}

Rpi::Rpi(const Position& position): RpdWithSingleSlot(position) {}

Rpi* Rpi::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual) {
	Position position;
	queryPosition(env, midGetInt, midResourceGetProperty, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, position);
	return new Rpi(position);
}

void Rpi::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	OcclusalRest(position_, RpdWithDirection::MESIAL).draw(designImage, teeth);
	GuidingPlate(position_).draw(designImage, teeth);
	IBar(position_).draw(designImage, teeth);
}

WwClasp::WwClasp(const Position& position, const Direction& direction): RpdWithSingleSlot(position), RpdWithDirection(direction), RpdWithLingualBlockage() {}

WwClasp* WwClasp::createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual) {
	Position position;
	Direction claspTipDirection;
	queryPosition(env, midGetInt, midResourceGetProperty, midStatementGetProperty, dpToothZone, dpToothOrdinal, opComponentPosition, individual, position);
	queryDirection(env, midGetInt, midResourceGetProperty, dpClaspTipDirection, individual, claspTipDirection);
	return new WwClasp(position, claspTipDirection);
}

void WwClasp::draw(const Mat& designImage, const vector<Tooth> teeth[4]) const {
	Clasp(position_, RpdWithMaterial::WROUGHT_WIRE, direction_).draw(designImage, teeth);
	OcclusalRest(position_, static_cast<Direction>(1 - direction_)).draw(designImage, teeth);
}
