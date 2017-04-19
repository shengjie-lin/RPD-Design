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

	enum Side {
		BUCCAL,
		LINGUAL
	};

	struct Position {
		Position(const int& zone, const int& ordinal);
		bool operator==(const Position& rhs) const;
		bool operator<(const Position& rhs) const;
		Position& operator++();
		Position operator++(int);
		Position& operator--();
		Position operator--(int);
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

class RpdAsMajorConnector : public Rpd {
public:
	void registerMajorConnector(vector<Tooth> teeth[nZones]) const;
	void registerExpectedAnchors(vector<Tooth> teeth[nZones]) const;
	void registerLingualConfrontations(vector<Tooth> teeth[nZones]) const;
protected:
	RpdAsMajorConnector(const vector<Position>& positions, const bool hasLingualConfrontations[nZones][nTeethPerZone]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	static void queryLingualConfrontations(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& individual, bool hasLingualConfrontations[nZones][nTeethPerZone]);
	bool hasLingualConfrontations_[nZones][nTeethPerZone];
private:
	static void registerMajorConnector(vector<Tooth> teeth[nZones], const vector<Position>& positions);
	static void registerExpectedAnchors(vector<Tooth> teeth[nZones], const vector<Position>& positions);
};

class RpdWithLingualCoverage : public virtual Rpd, public RpdWithMaterial {
public:
	virtual void registerLingualCoverage(vector<Tooth> teeth[nZones]) const;
protected:
	RpdWithLingualCoverage(const vector<Position>& positions, const Material& material, const vector<Direction>& rootDirections);
	RpdWithLingualCoverage(const vector<Position>& positions, const Material& material, const Direction& rootDirection);
	void registerLingualCoverage(vector<Tooth> teeth[nZones], const deque<bool>& flags) const;
	vector<Direction> rootDirections_;
};

class RpdWithClaspRootOrRest : public virtual Rpd {
public:
	void registerClaspRootOrRest(vector<Tooth> teeth[nZones]);
protected:
	RpdWithClaspRootOrRest(const vector<Position> positions, const vector<Direction>& rootDirections);
	RpdWithClaspRootOrRest(const vector<Position> positions, const Direction& rootDirection);
private:
	vector<Direction> rootDirections_;
};

class RpdWithLingualClaspArms : public RpdWithLingualCoverage {
	friend class AkersClasp;
	friend class ContinuousClasp;
public:
	virtual void setLingualClaspArms(vector<Tooth> teeth[nZones]);
protected:
	RpdWithLingualClaspArms(const vector<Position>& positions, const Material& material, const vector<Direction>& rootDirections);
	RpdWithLingualClaspArms(const vector<Position>& positions, const Material& material, const Direction& rootDirection);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
private:
	void registerLingualCoverage(vector<Tooth> teeth[nZones]) const override;
	deque<bool> hasLingualArms_;
};

class RpdWithLingualRest : public RpdWithClaspRootOrRest, public RpdWithLingualCoverage {
protected:
	RpdWithLingualRest(const vector<Position>& positions, const Material& material, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class AkersClasp : public RpdWithDirection, public RpdWithClaspRootOrRest, public RpdWithLingualClaspArms {
public:
	static AkersClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetBoolean, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpClaspMaterial, const jobject& dpEnableBuccalArm, const jobject& dpEnableLingualArm, const jobject& dpEnableRest, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
protected:
	AkersClasp(const vector<Position>& positions, const Material& material, const Direction& direction, const bool& enableBuccalArm, const bool& enableLingualArm, const bool& enableRest);
	static void queryPartEnablements(JNIEnv*const& env, const jmethodID& midGetBoolean, const jmethodID& midResourceGetProperty, const jobject& dpEnableBuccalArm, const jobject& dpEnableLingualArm, const jobject& dpEnableRest, const jobject& individual, bool& enableBuccalArm, bool& enableLingualArm, bool& enableRest);
private:
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void setLingualClaspArms(vector<Tooth> teeth[nZones]) override;
	bool enableBuccalArm_, enableRest_;
};

class CanineAkersClasp : public RpdWithDirection, public RpdWithLingualRest {
public:
	static CanineAkersClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	CanineAkersClasp(const vector<Position>& positions, const Material& claspMaterial, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	Material claspMaterial_;
};

class CombinationAnteriorPosteriorPalatalStrap : public RpdAsMajorConnector {
public:
	static CombinationAnteriorPosteriorPalatalStrap* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	CombinationAnteriorPosteriorPalatalStrap(const vector<Position>& positions, const bool hasLingualConfrontations[nZones][nTeethPerZone]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class CombinationClasp : public RpdWithDirection, public RpdWithClaspRootOrRest, public RpdWithLingualClaspArms {
public:
	static CombinationClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	CombinationClasp(const vector<Position>& positions, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class CombinedClasp : public RpdWithClaspRootOrRest, public RpdWithLingualClaspArms {
public:
	static CombinedClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	CombinedClasp(const vector<Position>& positions, const Material& material);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class ContinuousClasp : public RpdWithClaspRootOrRest, public RpdWithLingualClaspArms {
public:
	static ContinuousClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	ContinuousClasp(const vector<Position>& positions, const Material& material);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void setLingualClaspArms(vector<Tooth> teeth[nZones]) override;
};

class DentureBase : public Rpd {
public:
	enum Side {
		SINGLE,
		DOUBLE
	};

	static DentureBase* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
	void setSide(const vector<Tooth> teeth[nZones]);
	void registerDentureBase(vector<Tooth> teeth[nZones]) const;
	void registerExpectedAnchors(vector<Tooth> teeth[nZones]) const;
private:
	explicit DentureBase(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void registerDentureBase(vector<Tooth> teeth[nZones], vector<Position> positions) const;
	static void registerExpectedAnchors(vector<Tooth> teeth[nZones], const vector<Position>& positions);
	Side side_ = Side();
};

class EdentulousSpace : public Rpd {
public:
	static EdentulousSpace* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	explicit EdentulousSpace(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class FullPalatalPlate : public RpdAsMajorConnector {
public:
	static FullPalatalPlate* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	FullPalatalPlate(const vector<Position>& positions, const bool hasLingualConfrontations[nZones][nTeethPerZone]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class LingualBar : public RpdAsMajorConnector {
public:
	static LingualBar* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	LingualBar(const vector<Position>& positions, const bool hasLingualConfrontations[nZones][nTeethPerZone]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class LingualPlate : public RpdAsMajorConnector {
public:
	static LingualPlate* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	LingualPlate(const vector<Position>& positions, const bool hasLingualConfrontations[nZones][nTeethPerZone]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class LingualRest : public RpdWithDirection, public RpdWithLingualRest {
	friend class RpdWithLingualRest;
public:
	static LingualRest* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	LingualRest(const vector<Position>& positions, const Material& material, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class OcclusalRest : public RpdWithDirection, public RpdWithClaspRootOrRest {
	friend class AkersClasp;
	friend class CombinationClasp;
	friend class CombinedClasp;
	friend class ContinuousClasp;
	friend class RingClasp;
	friend class Rpa;
	friend class Rpi;
public:
	static OcclusalRest* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	OcclusalRest(const vector<Position>& positions, const Direction& direction);
	OcclusalRest(const Position& position, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class PalatalPlate : public RpdAsMajorConnector {
public:
	static PalatalPlate* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	PalatalPlate(const vector<Position>& positions, const bool hasLingualConfrontations[nZones][nTeethPerZone]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class RingClasp : public RpdWithClaspRootOrRest, public RpdWithLingualClaspArms {
public:
	static RingClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpClaspTipSide, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	RingClasp(const vector<Position>& positions, const Material& material, const Side& tipSide);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	static void queryTipSide(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspTipSide, const jobject& individual, Side& tipSide);
	Side tipSide_;
};

class Rpa : public RpdWithMaterial, public RpdWithClaspRootOrRest {
public:
	static Rpa* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	Rpa(const vector<Position>& positions, const Material& material);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class Rpi : public RpdWithClaspRootOrRest {
public:
	static Rpi* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	explicit Rpi(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class WwClasp : public AkersClasp {
public:
	static WwClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetBoolean, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpEnableBuccalArm, const jobject& dpEnableLingualArm, const jobject& dpEnableRest, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private:
	WwClasp(const vector<Position>& positions, const Direction& direction, const bool& enableBuccalArm, const bool& enableLingualArm, const bool& enableRest);
};

class GuidingPlate : public Rpd {
	friend class Rpa;
	friend class Rpi;
	explicit GuidingPlate(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class HalfClasp : public Rpd, public RpdWithMaterial, public RpdWithDirection {
	friend class RpdWithLingualClaspArms;
	friend class AkersClasp;
	friend class CanineAkersClasp;
	friend class CombinationClasp;
	friend class CombinedClasp;
	friend class ContinuousClasp;
	friend class Rpa;
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
