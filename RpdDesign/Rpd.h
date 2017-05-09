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
		Position(int const& zone, int const& ordinal);
		bool operator==(Position const& rhs) const;
		bool operator<(Position const& rhs) const;
		Position& operator++();
		Position operator++(int);
		Position& operator--();
		Position operator--(int);
		int zone, ordinal;
	};

	virtual ~Rpd() = default;
	virtual void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const = 0;
protected:
	explicit Rpd(vector<Position> const& positions);
	static void queryPositions(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, vector<Position>& positions, bool (&isEighthToothUsed)[nZones], bool const& autoComplete = false);
	vector<Position> positions_;
};

class RpdWithMaterial {
public:
	enum Material {
		CAST,
		WROUGHT_WIRE
	};

protected:
	explicit RpdWithMaterial(Material const& material);
	static void queryMaterial(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midResourceGetProperty, jobject const& dpClaspMaterial, jobject const& individual, Material& claspMaterial);
	Material material_;
};

class RpdWithDirection {
protected:
	explicit RpdWithDirection(Rpd::Direction const& direction);
	static void queryDirection(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midResourceGetProperty, jobject const& dpClaspTipDirection, jobject const& individual, Rpd::Direction& claspTipDirection);
	Rpd::Direction direction_;
};

class RpdAsMajorConnector : public Rpd {
public:
	void registerMajorConnector(vector<Tooth> (&teeth)[nZones]) const;
	void registerExpectedAnchors(vector<Tooth> (&teeth)[nZones]) const;
	void registerLingualConfrontations(vector<Tooth> (&teeth)[nZones]) const;
protected:
	RpdAsMajorConnector(vector<Position> const& positions, const bool (&hasLingualConfrontations)[nZones][nTeethPerZone]);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
	static void queryLingualConfrontations(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpLingualConfrontation, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& individual, bool (&hasLingualConfrontations)[nZones][nTeethPerZone]);
	bool hasLingualConfrontations_[nZones][nTeethPerZone];
private:
	static void registerMajorConnector(vector<Tooth> (&teeth)[nZones], vector<Position> const& positions);
	static void registerExpectedAnchors(vector<Tooth> (&teeth)[nZones], vector<Position> const& positions);
};

class RpdWithLingualCoverage : public virtual Rpd, public RpdWithMaterial {
public:
	virtual void registerLingualCoverage(vector<Tooth> (&teeth)[nZones]) const;
protected:
	RpdWithLingualCoverage(vector<Position> const& positions, Material const& material, vector<Direction> const& rootDirections);
	RpdWithLingualCoverage(vector<Position> const& positions, Material const& material, Direction const& rootDirection);
	void registerLingualCoverage(vector<Tooth> (&teeth)[nZones], deque<bool> const& flags) const;
	vector<Direction> rootDirections_;
};

class RpdWithClaspRootOrRest : public virtual Rpd {
public:
	void registerClaspRootOrRest(vector<Tooth> (&teeth)[nZones]);
protected:
	RpdWithClaspRootOrRest(vector<Position> const& positions, vector<Direction> const& rootDirections);
	RpdWithClaspRootOrRest(vector<Position> const& positions, Direction const& rootDirection);
private:
	vector<Direction> rootDirections_;
};

class RpdWithLingualClaspArms : public RpdWithLingualCoverage {
	friend class AkersClasp;
	friend class ContinuousClasp;
public:
	virtual void setLingualClaspArms(vector<Tooth> (&teeth)[nZones]);
protected:
	RpdWithLingualClaspArms(vector<Position> const& positions, Material const& material, vector<Direction> const& rootDirections);
	RpdWithLingualClaspArms(vector<Position> const& positions, Material const& material, Direction const& rootDirection);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
private:
	void registerLingualCoverage(vector<Tooth> (&teeth)[nZones]) const override;
	deque<bool> hasLingualArms_;
};

class RpdWithLingualRest : public RpdWithClaspRootOrRest, public RpdWithLingualCoverage {
protected:
	RpdWithLingualRest(vector<Position> const& positions, Material const& material, Direction const& direction);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
};

class AkersClasp : public RpdWithDirection, public RpdWithClaspRootOrRest, public RpdWithLingualClaspArms {
public:
	static AkersClasp* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetBoolean, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspTipDirection, jobject const& dpClaspMaterial, jobject const& dpEnableBuccalArm, jobject const& dpEnableLingualArm, jobject const& dpEnableRest, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
protected:
	AkersClasp(vector<Position> const& positions, Material const& material, Direction const& direction, bool const& enableBuccalArm, bool const& enableLingualArm, bool const& enableRest);
	static void queryPartEnablements(JNIEnv*const& env, jmethodID const& midGetBoolean, jmethodID const& midResourceGetProperty, jobject const& dpEnableBuccalArm, jobject const& dpEnableLingualArm, jobject const& dpEnableRest, jobject const& individual, bool& enableBuccalArm, bool& enableLingualArm, bool& enableRest);
private:
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
	void setLingualClaspArms(vector<Tooth> (&teeth)[nZones]) override;
	bool enableBuccalArm_, enableRest_;
};

class CanineAkersClasp : public RpdWithDirection, public RpdWithLingualRest {
public:
	static CanineAkersClasp* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspTipDirection, jobject const& dpClaspMaterial, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	CanineAkersClasp(vector<Position> const& positions, Material const& claspMaterial, Direction const& direction);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
	Material claspMaterial_;
};

class CombinationAnteriorPosteriorPalatalStrap : public RpdAsMajorConnector {
public:
	static CombinationAnteriorPosteriorPalatalStrap* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpLingualConfrontation, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	CombinationAnteriorPosteriorPalatalStrap(vector<Position> const& positions, const bool (&hasLingualConfrontations)[nZones][nTeethPerZone]);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
};

class CombinationClasp : public RpdWithDirection, public RpdWithClaspRootOrRest, public RpdWithLingualClaspArms {
public:
	static CombinationClasp* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspTipDirection, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	CombinationClasp(vector<Position> const& positions, Direction const& direction);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
};

class CombinedClasp : public RpdWithClaspRootOrRest, public RpdWithLingualClaspArms {
public:
	static CombinedClasp* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspMaterial, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	CombinedClasp(vector<Position> const& positions, Material const& material);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
};

class ContinuousClasp : public RpdWithClaspRootOrRest, public RpdWithLingualClaspArms {
public:
	static ContinuousClasp* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspMaterial, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	ContinuousClasp(vector<Position> const& positions, Material const& material);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
	void setLingualClaspArms(vector<Tooth> (&teeth)[nZones]) override;
};

class DentureBase : public Rpd {
public:
	enum Side {
		SINGLE,
		DOUBLE
	};

	static DentureBase* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
	void setSide(const vector<Tooth> (&teeth)[nZones]);
	void registerDentureBase(vector<Tooth> (&teeth)[nZones]) const;
	void registerExpectedAnchors(vector<Tooth> (&teeth)[nZones]) const;
private:
	explicit DentureBase(vector<Position> const& positions);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
	void registerDentureBase(vector<Tooth> (&teeth)[nZones], vector<Position> positions) const;
	static void registerExpectedAnchors(vector<Tooth> (&teeth)[nZones], vector<Position> const& positions);
	Side side_ = Side();
};

class EdentulousSpace : public Rpd {
public:
	static EdentulousSpace* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	explicit EdentulousSpace(vector<Position> const& positions);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
};

class FullPalatalPlate : public RpdAsMajorConnector {
public:
	static FullPalatalPlate* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpLingualConfrontation, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	FullPalatalPlate(vector<Position> const& positions, const bool (&hasLingualConfrontations)[nZones][nTeethPerZone]);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
};

class LingualBar : public RpdAsMajorConnector {
public:
	static LingualBar* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpLingualConfrontation, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	LingualBar(vector<Position> const& positions, const bool (&hasLingualConfrontations)[nZones][nTeethPerZone]);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
};

class LingualPlate : public RpdAsMajorConnector {
public:
	static LingualPlate* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpLingualConfrontation, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	LingualPlate(vector<Position> const& positions, const bool (&hasLingualConfrontations)[nZones][nTeethPerZone]);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
};

class LingualRest : public RpdWithDirection, public RpdWithLingualRest {
	friend class RpdWithLingualRest;
public:
	static LingualRest* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpRestMesialOrDistal, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	LingualRest(vector<Position> const& positions, Material const& material, Direction const& direction);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
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
	static OcclusalRest* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpRestMesialOrDistal, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	OcclusalRest(vector<Position> const& positions, Direction const& direction);
	OcclusalRest(Position const& position, Direction const& direction);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
};

class PalatalPlate : public RpdAsMajorConnector {
public:
	static PalatalPlate* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpLingualConfrontation, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	PalatalPlate(vector<Position> const& positions, const bool (&hasLingualConfrontations)[nZones][nTeethPerZone]);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
};

class RingClasp : public RpdWithClaspRootOrRest, public RpdWithLingualClaspArms {
public:
	static RingClasp* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspMaterial, jobject const& dpClaspTipSide, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	RingClasp(vector<Position> const& positions, Material const& material, Side const& tipSide);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
	static void queryTipSide(JNIEnv*const& env, jmethodID const& midGetInt, jmethodID const& midResourceGetProperty, jobject const& dpClaspTipSide, jobject const& individual, Side& tipSide);
	Side tipSide_;
};

class Rpa : public RpdWithMaterial, public RpdWithClaspRootOrRest {
public:
	static Rpa* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspMaterial, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	Rpa(vector<Position> const& positions, Material const& material);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
};

class Rpi : public RpdWithClaspRootOrRest {
public:
	static Rpi* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midStatementGetProperty, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	explicit Rpi(vector<Position> const& positions);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
};

class WwClasp : public AkersClasp {
public:
	static WwClasp* createFromIndividual(JNIEnv* const& env, jmethodID const& midGetBoolean, jmethodID const& midGetInt, jmethodID const& midHasNext, jmethodID const& midListProperties, jmethodID const& midNext, jmethodID const& midResourceGetProperty, jmethodID const& midStatementGetProperty, jobject const& dpClaspTipDirection, jobject const& dpEnableBuccalArm, jobject const& dpEnableLingualArm, jobject const& dpEnableRest, jobject const& dpToothZone, jobject const& dpToothOrdinal, jobject const& opComponentPosition, jobject const& individual, bool (&isEighthToothUsed)[nZones]);
private:
	WwClasp(vector<Position> const& positions, Direction const& direction, bool const& enableBuccalArm, bool const& enableLingualArm, bool const& enableRest);
};

class GuidingPlate : public Rpd {
	friend class Rpa;
	friend class Rpi;
	explicit GuidingPlate(vector<Position> const& positions);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
};

class HalfClasp : public Rpd, public RpdWithMaterial, public RpdWithDirection {
	friend class RpdWithLingualClaspArms;
	friend class AkersClasp;
	friend class CanineAkersClasp;
	friend class CombinationClasp;
	friend class CombinedClasp;
	friend class ContinuousClasp;
	friend class Rpa;
	HalfClasp(vector<Position> const& positions, Material const& material, Direction const& direction, Side const& side);
	HalfClasp(Position const& position, Material const& material, Direction const& direction, Side const& side);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
	Side side_;
};

class IBar : public Rpd {
	friend class Rpi;
	explicit IBar(vector<Position> const& positions);
	void draw(Mat const& designImage, const vector<Tooth> (&teeth)[nZones]) const override;
};
