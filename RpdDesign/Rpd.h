#pragma once

#include <deque>
#include <jni.h>

#include "GlobalVariables.h"

using namespace rel_ops;

class Tooth;

class Rpd {
public :
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
protected :
	explicit Rpd(const vector<Position>& positions);
	static void queryPositions(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, vector<Position>& positions, bool isEighthToothUsed[nZones], const bool& autoComplete = false);
	vector<Position> positions_;
};

class RpdWithMaterial {
public :
	enum Material {
		CAST,
		WROUGHT_WIRE
	};

protected :
	explicit RpdWithMaterial(const Material& material);
	static void queryMaterial(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspMaterial, const jobject& individual, Material& claspMaterial);
	Material material_;
};

class RpdWithDirection {
protected :
	explicit RpdWithDirection(const Rpd::Direction& direction);
	static void queryDirection(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspTipDirection, const jobject& individual, Rpd::Direction& claspTipDirection);
	Rpd::Direction direction_;
};

class RpdAsLingualBlockage : public Rpd {
public :
	enum LingualBlockage {
		NONE,
		CLASP_MESIAL_REST,
		CLASP_DISTAL_REST,
		DENTURE_BASE,
		MAJOR_CONNECTOR
	};

	virtual void registerLingualBlockages(vector<Tooth> teeth[nZones]) const;
protected :
	RpdAsLingualBlockage(const vector<Position>& positions, const vector<LingualBlockage>& lingualBlockages);
	RpdAsLingualBlockage(const vector<Position>& positions, const LingualBlockage& lingualBlockage);
	void registerLingualBlockages(vector<Tooth> teeth[nZones], const deque<bool>& flags) const;
private :
	void registerLingualBlockages(vector<Tooth> teeth[nZones], const vector<Position>& positions) const;
	vector<LingualBlockage> lingualBlockages_;
};

class RpdWithLingualClaspArms : public RpdWithMaterial, public RpdAsLingualBlockage {
public :
	void setLingualArms(bool hasLingualConfrontations[nZones][nTeethPerZone]);
protected :
	RpdWithLingualClaspArms(const vector<Position>& positions, const Material& material, const vector<Direction>& tipDirections);
	RpdWithLingualClaspArms(const vector<Position>& positions, const Material& material, const Direction& tipDirection);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
private :
	void registerLingualBlockages(vector<Tooth> teeth[nZones]) const override;
	vector<Direction> tipDirections_;
	deque<bool> hasLingualClaspArms_;
};

class RpdWithLingualConfrontations : public RpdAsLingualBlockage {
public :
	void registerLingualConfrontations(bool hasLingualConfrontations[nZones][nTeethPerZone]) const;
protected :
	RpdWithLingualConfrontations(const vector<Position>& positions, const vector<Position>& lingualConfrontations);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	static void queryLingualConfrontations(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& individual, vector<Position>& lingualConfrontations);
private :
	vector<Position> lingualConfrontations_;
};

class AkerClasp : public RpdWithDirection, public RpdWithLingualClaspArms {
public :
	static AkerClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	AkerClasp(const vector<Position>& positions, const Material& material, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class CombinationClasp : public RpdWithDirection, public RpdWithLingualClaspArms {
public :
	static CombinationClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	CombinationClasp(const vector<Position>& positions, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class CombinedClasp : public RpdWithLingualClaspArms {
public :
	static CombinedClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	CombinedClasp(const vector<Position>& positions, const Material& material);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class DentureBase : public RpdAsLingualBlockage {
public :
	static DentureBase* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
	void determineTailsCoverage(const bool isEighthToothUsed[nZones]);
private :
	explicit DentureBase(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void registerLingualBlockages(vector<Tooth> teeth[nZones]) const override;
	deque<bool> coversTails_ = {false, false};
};

class EdentulousSpace : public Rpd {
public :
	static EdentulousSpace* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	explicit EdentulousSpace(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class LingualRest : public Rpd, public RpdWithDirection {
public :
	static LingualRest* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	LingualRest(const vector<Position>& positions, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class OcclusalRest : public Rpd, public RpdWithDirection {
	friend class AkerClasp;
	friend class CombinationClasp;
	friend class CombinedClasp;
	friend class RingClasp;
	friend class Rpa;
	friend class Rpi;
	friend class WwClasp;
public :
	static OcclusalRest* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	OcclusalRest(const vector<Position>& positions, const Direction& direction);
	OcclusalRest(const Position& position, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class PalatalPlate : public RpdWithLingualConfrontations {
public :
	static PalatalPlate* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpAnchorMesialOrDistal, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& opMajorConnectorAnchor, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	PalatalPlate(const vector<Position>& positions, const vector<Position>& lingualConfrontations);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class RingClasp : public Rpd, public RpdWithMaterial {
public :
	static RingClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	RingClasp(const vector<Position>& positions, const Material& material);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class Rpa : public Rpd, public RpdWithMaterial {
public :
	static Rpa* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	Rpa(const vector<Position>& positions, const Material& material);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class Rpi : public Rpd {
public :
	static Rpi* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	explicit Rpi(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class WwClasp : public RpdWithDirection, public RpdWithLingualClaspArms {
public :
	static WwClasp* createFromIndividual(JNIEnv* const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	WwClasp(const vector<Position>& positions, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
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

	friend class RpdWithLingualClaspArms;
	friend class AkerClasp;
	friend class CombinationClasp;
	friend class CombinedClasp;
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

class Plating : public Rpd {
	friend class RpdWithLingualConfrontations;
	explicit Plating(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};
