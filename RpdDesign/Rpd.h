#pragma once

#include <deque>
#include <jni.h>

#include "GlobalVariables.h"

using namespace rel_ops;

class Tooth;

class Rpd {
public:
	enum Direction {
		MESIAL,
		DISTAL
	};

	struct Position {
		Position(const int& zone, const int& ordinal);
		bool operator==(const Position& rhs) const;
		bool operator<(const Position& rhs) const;
		Position& operator++();
		Position& operator--();
		int zone, ordinal;
	};

	virtual ~Rpd() = default;
	virtual void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const = 0;
protected:
	explicit Rpd(const vector<Position>& positions);
	static void queryPositions(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, vector<Position>& positions, bool isEighthToothUsed[nZones], const bool& autoComplete = false);
	vector<Position> positions_;
};

class RpdWithMaterial {
public:
	enum Material {
		CAST,
		WROUGHT_WIRE
	};

protected:
	explicit RpdWithMaterial(const Material& material);
	static void queryMaterial(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspMaterial, const jobject& individual, Material& claspMaterial);
	Material material_;
};

class RpdWithDirection {
protected:
	explicit RpdWithDirection(const Rpd::Direction& direction);
	static void queryDirection(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspTipDirection, const jobject& individual, Rpd::Direction& claspTipDirection);
	Rpd::Direction direction_;
};

class RpdAsLingualBlockage : public Rpd {
	friend class RpdWithLingualArms;
public:
	enum LingualBlockage {
		NONE,
		ARMED_DISTAL_REST,
		ARMED_MESIAL_REST,
		DENTURE_BASE,
		DENTURE_BASE_TAIL,
		MAJOR_CONNECTOR
	};

	virtual void registerLingualBlockages(vector<Tooth> teeth[nZones]) const;
protected:
	RpdAsLingualBlockage(const vector<Position>& positions, const vector<LingualBlockage>& lingualBlockages);
	RpdAsLingualBlockage(const vector<Position>& positions, const LingualBlockage& lingualBlockage);
private:
	void registerLingualBlockages(vector<Tooth> teeth[nZones], const vector<Position>& positions) const;
	vector<LingualBlockage> lingualBlockages_;
};

class RpdWithLingualArms : public RpdWithMaterial, public RpdAsLingualBlockage {
	friend class AkersClasp;
	friend class ContinuousClasp;
public:
	virtual void setLingualClaspArms(bool hasLingualConfrontations[nZones][nTeethPerZone]);
protected:
	RpdWithLingualArms(const vector<Position>& positions, const Material& material, const vector<Direction>& tipDirections);
	RpdWithLingualArms(const vector<Position>& positions, const Material& material, const Direction& tipDirection);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
private:
	void registerLingualBlockages(vector<Tooth> teeth[nZones]) const override;
	vector<Direction> tipDirections_;
	deque<bool> hasLingualArms_;
};

class RpdWithLingualConfrontations : public RpdAsLingualBlockage {
	friend class LingualBar;
public:
	void registerLingualConfrontations(bool hasLingualConfrontations[nZones][nTeethPerZone]) const;
protected:
	RpdWithLingualConfrontations(const vector<Position>& positions, const bool hasLingualConfrontations[nZones][nTeethPerZone]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	static void queryLingualConfrontations(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& individual, bool hasLingualConfrontations[nZones][nTeethPerZone]);
private:
	bool hasLingualConfrontations_[nZones][nTeethPerZone];
};

class AkersClasp : public RpdWithDirection, public RpdWithLingualArms {
public:
	static AkersClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetBoolean, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpClaspMaterial, const jobject& dpEnableBuccalArm, const jobject& dpEnableLingualArm, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
protected:
	AkersClasp(const vector<Position>& positions, const Material& material, const Direction& direction, const bool& enableBuccalArm, const bool& enableLingualArm);
	static void queryArmEnablements(JNIEnv* const& env, const jmethodID& midGetBoolean, const jmethodID& midResourceGetProperty, const jobject& dpEnableBuccalArm, const jobject& dpEnableLingualArm, const jobject& individual, bool& enableBuccalArm, bool& enableLingualArm);
private:
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void setLingualClaspArms(bool hasLingualConfrontations[nZones][nTeethPerZone]) override;
	bool enableBuccalArm_;
};

class CanineAkersClasp : public RpdWithDirection, public RpdWithLingualArms {
public:
	static CanineAkersClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	CanineAkersClasp(const vector<Position>& positions, const Material& material, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class CombinationAnteriorPosteriorPalatalStrap : public RpdWithLingualConfrontations {
public:
	static CombinationAnteriorPosteriorPalatalStrap* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	CombinationAnteriorPosteriorPalatalStrap(const vector<Position>& positions, const bool hasLingualConfrontations[nZones][nTeethPerZone]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class CombinationClasp : public RpdWithDirection, public RpdWithLingualArms {
public:
	static CombinationClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	CombinationClasp(const vector<Position>& positions, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class CombinedClasp : public RpdWithLingualArms {
public:
	static CombinedClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	CombinedClasp(const vector<Position>& positions, const Material& material);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class ContinuousClasp : public RpdWithLingualArms {
public:
	static ContinuousClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	ContinuousClasp(const vector<Position>& positions, const Material& material);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void setLingualClaspArms(bool hasLingualConfrontations[nZones][nTeethPerZone]) override;
};

class DentureBase : public RpdAsLingualBlockage {
public:
	static DentureBase* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
	void determineTailsCoverage(const bool isEighthToothUsed[nZones]);
	void registerDentureBase(vector<Tooth> teeth[nZones]);
private:
	explicit DentureBase(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void registerLingualBlockages(vector<Tooth> teeth[nZones]) const override;
	static void registerDentureBase(vector<Tooth> teeth[nZones], vector<Position> positions);
	deque<bool> isCoveringTails_ = {false, false};
	int isBlocked_ = -1;
};

class EdentulousSpace : public Rpd {
public:
	static EdentulousSpace* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	explicit EdentulousSpace(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class FullPalatalPlate : public RpdWithLingualConfrontations {
public:
	static FullPalatalPlate* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	FullPalatalPlate(const vector<Position>& positions, const bool hasLingualConfrontations[nZones][nTeethPerZone]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class LingualBar : public RpdWithLingualConfrontations {
public:
	static LingualBar* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	LingualBar(const vector<Position>& positions, const bool hasLingualConfrontations[nZones][nTeethPerZone]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class LingualPlate : public RpdWithLingualConfrontations {
public:
	static LingualPlate* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	LingualPlate(const vector<Position>& positions, const bool hasLingualConfrontations[nZones][nTeethPerZone]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class LingualRest : public RpdWithDirection, public RpdWithLingualArms {
	friend class CanineAkersClasp;
public:
	static LingualRest* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	LingualRest(const vector<Position>& positions, const Material& material, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class OcclusalRest : public Rpd, public RpdWithDirection {
	friend class AkersClasp;
	friend class CombinationClasp;
	friend class CombinedClasp;
	friend class ContinuousClasp;
	friend class RingClasp;
	friend class Rpa;
	friend class Rpi;
	friend class WwClasp;
public:
	static OcclusalRest* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	OcclusalRest(const vector<Position>& positions, const Direction& direction);
	OcclusalRest(const Position& position, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class PalatalPlate : public RpdWithLingualConfrontations {
public:
	static PalatalPlate* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	PalatalPlate(const vector<Position>& positions, const bool hasLingualConfrontations[nZones][nTeethPerZone]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class RingClasp : public Rpd, public RpdWithMaterial {
public:
	static RingClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	RingClasp(const vector<Position>& positions, const Material& material);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class Rpa : public Rpd, public RpdWithMaterial {
public:
	static Rpa* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	Rpa(const vector<Position>& positions, const Material& material);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class Rpi : public Rpd {
public:
	static Rpi* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	explicit Rpi(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class WwClasp : public AkersClasp {
public:
	static WwClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetBoolean, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpEnableBuccalArm, const jobject& dpEnableLingualArm, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	WwClasp(const vector<Position>& positions, const Direction& direction, const bool& enableBuccalArm, const bool& enableLingualArm);
};

class GuidingPlate : public Rpd {
	friend class Rpa;
	friend class Rpi;
	explicit GuidingPlate(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class HalfClasp : public Rpd, public RpdWithMaterial, public RpdWithDirection {
	enum Side {
		BUCCAL,
		LINGUAL
	};

	friend class RpdWithLingualArms;
	friend class AkersClasp;
	friend class CanineAkersClasp;
	friend class CombinationClasp;
	friend class CombinedClasp;
	friend class ContinuousClasp;
	friend class Rpa;
	friend class WwClasp;
	HalfClasp(const vector<Position>& positions, const Material& material, const Direction& direction, const Side& side);
	HalfClasp(const Position& position, const Material& material, const Direction& direction, const Side& side);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	Side side_;
};

class IBar : public Rpd {
	friend class Rpi;
	explicit IBar(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};
