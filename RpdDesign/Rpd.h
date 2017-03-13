#pragma once

#include <deque>
#include <jni.h>

#include "GlobalVariables.h"

class Tooth;

class Rpd {
public:
	struct Position {
		explicit Position(const int& zone = 0, const int& ordinal = -1);
		int zone, ordinal;
	};

	virtual ~Rpd() = default;
	virtual void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const = 0;
};

class RpdWithSingleSlot: public virtual Rpd {
public:
	const Position& getPosition() const;
protected:
	explicit RpdWithSingleSlot(const Position& position);
	static void queryPosition(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, Position& position, bool isEighthToothUsed[nZones]);
	Position position_;
};

class RpdWithMultiSlots: public virtual Rpd {
public:
	const Position& getStartPosition() const;
	const Position& getEndPosition() const;

protected:
	RpdWithMultiSlots(const Position& startPosition, const Position& endPosition);
	static void queryPositions(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, Position& startPosition, Position& endPosition, bool isEighthToothUsed[nZones]);
	Position startPosition_, endPosition_;
};

class RpdWithMaterial: public virtual Rpd {
public:
	enum Material {
		CAST,
		WROUGHT_WIRE,
		UNSPECIFIED
	};

protected:
	explicit RpdWithMaterial(const Material& material);
	RpdWithMaterial(const Material& buccalMaterial, const Material& lingualMaterial);
	static void queryMaterial(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspMaterial, const jobject& individual, Material& claspMaterial);
	static void queryMaterial(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspMaterial, const jobject& dpBuccalClaspMaterial, const jobject& dpLingualClaspMaterial, const jobject& individual, Material& claspMaterial, Material& buccalClaspMaterial, Material& lingualClaspMaterial);
	Material material_, buccalMaterial_, lingualMaterial_;
};

class RpdWithDirection: public virtual Rpd {
public:
	enum Direction {
		MESIAL,
		DISTAL
	};

protected:
	explicit RpdWithDirection(const Direction& direction);
	static void queryDirection(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspTipDirection, const jobject& individual, Direction& claspTipDirection);
	Direction direction_;
};

class RpdWithLingualBlockage: public virtual Rpd {
public:
	enum LingualBlockage {
		NONE,
		CLASP,
		DENTURE_BASE,
		MAJOR_CONNECTOR
	};

	virtual void registerLingualBlockage(vector<Tooth> teeth[nZones]) const = 0;
protected:
	static void registerLingualBlockage(vector<Tooth> teeth[nZones], const Position& position, const LingualBlockage& lingualBlockage);
	static void registerLingualBlockage(vector<Tooth> teeth[nZones], const Position& startPosition, const Position& endPosition, const LingualBlockage& lingualBlockage);
};

class RpdAsClasp: public RpdWithLingualBlockage {
public:
	explicit RpdAsClasp(const deque<bool>& hasLingualArms);
	virtual void setLingualArm(bool hasLingualConfrontations[nZones][nTeethPerZone]) = 0;
protected:
	static void setLingualArm(bool hasLingualConfrontations[nZones][nTeethPerZone], const Position& position, bool& hasLingualArm);
	static void registerLingualBlockage(vector<Tooth> teeth[nZones], const Position& position, const bool& hasLingualArm);
	deque<bool> hasLingualArms_;
};

class RpdAsMajorConnector: public RpdWithLingualBlockage {
public:
	struct Anchor {
		explicit Anchor(const Position& position = Position(), const RpdWithDirection::Direction& direction = RpdWithDirection::DISTAL);
		Position position;
		RpdWithDirection::Direction direction;
	};

	explicit RpdAsMajorConnector(const vector<Anchor>& anchors);
	void updateEighthToothInfo(const bool isEighthToothMissing[nZones], bool isEighthToothUsed[nZones]);
	void registerLingualBlockage(vector<Tooth> teeth[nZones]) const override;
protected:
	enum Coverage {
		LINEAR,
		PLANAR
	};

	static void queryAnchors(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpAnchorMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& opMajorConnectorAnchor, const jobject& individual, const Coverage& coverage, vector<Anchor>& anchors, bool isEighthToothUsed[nZones]);
	vector<Anchor> anchors_;
};

class RpdWithOcclusalRest: public virtual Rpd {
public:
	virtual ~RpdWithOcclusalRest() = default;
	virtual void registerOcclusalRest(vector<Tooth> teeth[nZones]) const = 0;
protected:
	static void registerOcclusalRest(vector<Tooth> teeth[nZones], const Position& position, const RpdWithDirection::Direction& direction);
};

class RpdWithLingualConfrontations: public RpdAsMajorConnector {

public:
	explicit RpdWithLingualConfrontations(const vector<Anchor>& anchors, const vector<Position>& lingualConfrontations);
	void registerLingualConfrontations(bool hasLingualConfrontations[nZones][nTeethPerZone]) const;
protected:
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	static void queryLingualConfrontations(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& individual, vector<Position>& lingualConfrontations);
private:
	vector<Position> lingualConfrontations_;
};

class AkersClasp: public RpdWithSingleSlot, public RpdWithMaterial, public RpdWithDirection, public RpdWithOcclusalRest, public RpdAsClasp {
public:
	AkersClasp(const Position& position, const Material& material, const Direction& direction);
	static AkersClasp* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void registerOcclusalRest(vector<Tooth> teeth[nZones]) const override;
	void setLingualArm(bool hasLingualConfrontations[nZones][nTeethPerZone]) override;
	void registerLingualBlockage(vector<Tooth> teeth[nZones]) const override;
};

class Clasp: public RpdWithSingleSlot, public RpdWithMaterial, public RpdWithDirection {
public:
	Clasp(const Position& position, const Material& material, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class CombinationClasp: public RpdWithSingleSlot, public RpdWithDirection, public RpdWithOcclusalRest, public RpdAsClasp {
public:
	CombinationClasp(const Position& position, const Direction& direction);
	static CombinationClasp* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void registerOcclusalRest(vector<Tooth> teeth[nZones]) const override;
	void setLingualArm(bool hasLingualConfrontations[nZones][nTeethPerZone]) override;
	void registerLingualBlockage(vector<Tooth> teeth[nZones]) const override;
};

class CombinedClasp: public RpdWithMultiSlots, public RpdWithMaterial, public RpdWithOcclusalRest, public RpdAsClasp {
public:
	CombinedClasp(const Position& startPosition, const Position& endPosition, const Material& material);
	static CombinedClasp* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void registerOcclusalRest(vector<Tooth> teeth[nZones]) const override;
	void setLingualArm(bool hasLingualConfrontations[nZones][nTeethPerZone]) override;
	void registerLingualBlockage(vector<Tooth> teeth[nZones]) const override;
};

class DentureBase: public RpdWithMultiSlots, public RpdWithLingualBlockage {
public:
	DentureBase(const Position& startPosition, const Position& endPosition);
	static DentureBase* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void setCoversStartTail();
	void setCoversEndTail();
	void registerLingualBlockage(vector<Tooth> teeth[nZones]) const override;
private:
	bool coversStartTail_ = false;
	bool coversEndTail_ = false;
};

class EdentulousSpace: public RpdWithMultiSlots {
public:
	EdentulousSpace(const Position& startPosition, const Position& endPosition);
	static EdentulousSpace* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class GuidingPlate: public RpdWithSingleSlot {
public:
	explicit GuidingPlate(const Position& position);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class HalfClasp: public RpdWithSingleSlot, public RpdWithMaterial, public RpdWithDirection {
public:
	enum Side {
		BUCCAL,
		LINGUAL
	};

	HalfClasp(const Position& position, const Material& material, const Direction& direction, const Side& side);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
private:
	Side side_;
};

class IBar: public RpdWithSingleSlot {
public:
	explicit IBar(const Position& position);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class LingualRest: public RpdWithSingleSlot, public RpdWithDirection {
public:
	LingualRest(const Position& position, const Direction& direction);
	static LingualRest* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class OcclusalRest: public RpdWithSingleSlot, public RpdWithDirection, public RpdWithOcclusalRest {
public:
	OcclusalRest(const Position& position, const Direction& direction);
	static OcclusalRest* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void registerOcclusalRest(vector<Tooth> teeth[nZones]) const override;
};

class PalatalPlate: public RpdWithLingualConfrontations {
public:
	PalatalPlate(const vector<Anchor>& anchors, const vector<Position>& lingualConfrontations);
	static PalatalPlate* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpAnchorMesialOrDistal, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& opMajorConnectorAnchor, const jobject& individual, bool isEighthToothUsed[nZones]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class Plating: public RpdWithSingleSlot {
public:
	explicit Plating(const Position& position);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class RingClasp: public RpdWithSingleSlot, public RpdWithMaterial, public RpdAsClasp, public RpdWithOcclusalRest {
public:
	RingClasp(const Position& position, const Material& material);
	static RingClasp* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void registerOcclusalRest(vector<Tooth> teeth[nZones]) const override;
	void registerLingualBlockage(vector<Tooth> teeth[nZones]) const override;
	void setLingualArm(bool hasLingualConfrontations[nZones][nTeethPerZone]) override;
};

class Rpa: public RpdWithSingleSlot, public RpdWithMaterial, public RpdWithOcclusalRest {
public:
	Rpa(const Position& position, const Material& material);
	static Rpa* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void registerOcclusalRest(vector<Tooth> teeth[nZones]) const override;
};

class Rpi: public RpdWithSingleSlot, public RpdWithOcclusalRest {
public:
	explicit Rpi(const Position& position);
	static Rpi* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void registerOcclusalRest(vector<Tooth> teeth[nZones]) const override;
};

class WwClasp: public RpdWithSingleSlot, public RpdWithDirection, public RpdWithOcclusalRest, public RpdAsClasp {
public:
	WwClasp(const Position& position, const Direction& direction);
	static WwClasp* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void registerOcclusalRest(vector<Tooth> teeth[nZones]) const override;
	void setLingualArm(bool hasLingualConfrontations[nZones][nTeethPerZone]) override;
	void registerLingualBlockage(vector<Tooth> teeth[nZones]) const override;
};
