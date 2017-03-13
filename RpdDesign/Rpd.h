#pragma once

#include <deque>
#include <jni.h>

#include "GlobalVariables.h"

class Tooth;

class Rpd {
public :
	enum Direction {
		MESIAL,
		DISTAL
	};

	struct Position {
		explicit Position(const int& zone = 0, const int& ordinal = -1);
		int zone, ordinal;
	};

	struct Anchor {
		explicit Anchor(const Position& position, const Direction& direction = DISTAL);
		Position position;
		Direction direction;
	};

	virtual ~Rpd() = default;
	virtual void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const = 0;
};

class RpdWithPositions : public virtual Rpd {
protected :
	explicit RpdWithPositions(const vector<Position>& positions);
	static void queryPositions(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, vector<Position>& positions, bool isEighthToothUsed[nZones], const bool& autoComplete = false);
	vector<Position> positions_;
};

class RpdWithMaterial : public virtual Rpd {
public :
	enum Material {
		CAST,
		WROUGHT_WIRE
	};

protected :
	explicit RpdWithMaterial(const Material& material);
	static void queryMaterial(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspMaterial, const jobject& individual, Material& claspMaterial);
	Material material_;
};

class RpdWithDirection : public virtual Rpd {
protected :
	explicit RpdWithDirection(const Direction& direction);
	static void queryDirection(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jobject& dpClaspTipDirection, const jobject& individual, Direction& claspTipDirection);
	Direction direction_;
};

class RpdAsLingualBlockage : public virtual Rpd {
public :
	enum LingualBlockage {
		NONE,
		CLASP,
		DENTURE_BASE,
		MAJOR_CONNECTOR
	};

	virtual void registerLingualBlockage(vector<Tooth> teeth[nZones]) const = 0;
protected :
	static void registerLingualBlockage(vector<Tooth> teeth[nZones], const vector<Position>& positions, const LingualBlockage& lingualBlockage);
};

class RpdWithClasps : public RpdAsLingualBlockage {
public :
	void setLingualArms(bool hasLingualConfrontations[nZones][nTeethPerZone]);
protected :
	explicit RpdWithClasps(const Anchor& clasp);
	explicit RpdWithClasps(const vector<Anchor>& clasps);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones], const RpdWithMaterial::Material& material) const;
	const deque<bool>& getLingualArms() const;
private :
	void registerLingualBlockage(vector<Tooth> teeth[nZones]) const override;
	vector<Anchor> clasps_;
	deque<bool> hasLingualArms_;
};

class RpdAsMajorConnector : public RpdAsLingualBlockage {
public :
	void handleEighthTooth(const bool isEighthToothMissing[nZones], bool isEighthToothUsed[nZones]);
protected :
	enum Coverage {
		LINEAR,
		PLANAR
	};

	explicit RpdAsMajorConnector(const vector<Anchor>& anchors);
	static void queryAnchors(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpAnchorMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& opMajorConnectorAnchor, const jobject& individual, const Coverage& coverage, vector<Anchor>& anchors, bool isEighthToothUsed[nZones]);
	vector<Anchor> anchors_;
private :
	void registerLingualBlockage(vector<Tooth> teeth[nZones]) const override;
};

class RpdWithOcclusalRests : public virtual Rpd {
public :
	virtual void registerOcclusalRests(vector<Tooth> teeth[nZones]) const;
protected :
	explicit RpdWithOcclusalRests(const Anchor& occlusalRest);
	explicit RpdWithOcclusalRests(const vector<Anchor>& occlusalRests);
	void appendOcclusalRest(const Anchor& occlusalRest);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
private :
	vector<Anchor> occlusalRests_;
};

class RpdWithLingualConfrontations : public RpdAsMajorConnector {
public :
	void registerLingualConfrontations(bool hasLingualConfrontations[nZones][nTeethPerZone]) const;
protected :
	explicit RpdWithLingualConfrontations(const vector<Anchor>& anchors, const vector<Position>& lingualConfrontations);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	static void queryLingualConfrontations(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& individual, vector<Position>& lingualConfrontations);
private :
	vector<Position> lingualConfrontations_;
};

class AkerClasp : public RpdWithPositions, public RpdWithMaterial, public RpdWithDirection, public RpdWithOcclusalRests, public RpdWithClasps {
public :
	static AkerClasp* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	AkerClasp(const vector<Position>& positions, const Material& material, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class CombinationClasp : public RpdWithPositions, public RpdWithDirection, public RpdWithOcclusalRests, public RpdWithClasps {
public :
	static CombinationClasp* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	CombinationClasp(const vector<Position>& positions, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class CombinedClasp : public RpdWithPositions, public RpdWithMaterial, public RpdWithOcclusalRests, public RpdWithClasps {
public :
	static CombinedClasp* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	CombinedClasp(const vector<Position>& positions, const Material& material);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class DentureBase : public RpdWithPositions, public RpdAsLingualBlockage {
public :
	static DentureBase* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
	void determineTailsCoverage(const bool isEighthToothUsed[nZones]);
private :
	explicit DentureBase(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void registerLingualBlockage(vector<Tooth> teeth[nZones]) const override;
	deque<bool> coversTails_ = {false, false};
};

class EdentulousSpace : public RpdWithPositions {
public :
	static EdentulousSpace* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	explicit EdentulousSpace(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class LingualRest : public RpdWithPositions, public RpdWithDirection {
public :
	static LingualRest* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	LingualRest(const vector<Position>& positions, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class PalatalPlate : public RpdWithLingualConfrontations {
public :
	static PalatalPlate* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpAnchorMesialOrDistal, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& opMajorConnectorAnchor, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	PalatalPlate(const vector<Anchor>& anchors, const vector<Position>& lingualConfrontations);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class RingClasp : public RpdWithPositions, public RpdWithMaterial, public RpdWithClasps, public RpdWithOcclusalRests {
public :
	static RingClasp* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	RingClasp(const vector<Position>& positions, const Material& material);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class Rpa : public RpdWithPositions, public RpdWithMaterial, public RpdWithOcclusalRests {
public :
	static Rpa* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	Rpa(const vector<Position>& positions, const Material& material);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class Rpi : public RpdWithPositions, public RpdWithOcclusalRests {
public :
	static Rpi* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	explicit Rpi(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class WwClasp : public RpdWithPositions, public RpdWithDirection, public RpdWithOcclusalRests, public RpdWithClasps {
public :
	static WwClasp* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	WwClasp(const vector<Position>& positions, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class GuidingPlate : public RpdWithPositions {
	friend class Rpa;
	friend class Rpi;
	explicit GuidingPlate(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class HalfClasp : public RpdWithPositions, public RpdWithMaterial, public RpdWithDirection {
	enum Side {
		BUCCAL,
		LINGUAL
	};

	friend class RpdWithClasps;
	friend class AkerClasp;
	friend class CombinationClasp;
	friend class CombinedClasp;
	friend class Rpa;
	friend class WwClasp;
	HalfClasp(const vector<Position>& positions, const Material& material, const Direction& direction, const Side& side);
	HalfClasp(const Anchor& anchor, const Material& material);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	Side side_;
};

class IBar : public RpdWithPositions {
	friend class Rpi;
	explicit IBar(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};

class OcclusalRest : public RpdWithPositions, public RpdWithDirection, public RpdWithOcclusalRests {
	friend class RpdWithOcclusalRests;
public :
	static OcclusalRest* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEighthToothUsed[nZones]);
private :
	OcclusalRest(const vector<Position>& positions, const Direction& direction);
	explicit OcclusalRest(const Anchor& anchor);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
	void registerOcclusalRests(vector<Tooth> teeth[nZones]) const override;
};

class Plating : public RpdWithPositions {
	friend class RpdWithLingualConfrontations;
	explicit Plating(const vector<Position>& positions);
	void draw(const Mat& designImage, const vector<Tooth> teeth[nZones]) const override;
};
