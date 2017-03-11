#pragma once

#include <jni.h>

using namespace std;
using namespace cv;

class Tooth;

class Rpd {
public:
	struct Position {
		explicit Position(const int& zone = 0, const int& ordinal = -1);
		int zone, ordinal;
	};

	virtual ~Rpd() = default;
	virtual void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const = 0;
};

class RpdWithSingleSlot: public virtual Rpd {
public:
	const Position& getPosition() const;
protected:
	explicit RpdWithSingleSlot(const Position& position);
	static void queryPosition(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, Position& position, bool isEtUsed[4]);
	Position position_;
};

class RpdWithMultiSlots: public virtual Rpd {
public:
	const Position& getStartPosition() const;
	const Position& getEndPosition() const;

protected:
	RpdWithMultiSlots(const Position& startPosition, const Position& endPosition);
	static void queryPositions(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, Position& startPosition, Position& endPosition, bool isEtUsed[4]);
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

class RpdWithLingualBlockage:public virtual Rpd {
public:
	enum LingualBlockage {
		NONE,
		CLASP,
		MAJOR_CONNECTOR
	};

	enum Scope {
		POINT,
		LINE,
		PLANE
	};

	RpdWithLingualBlockage();
	RpdWithLingualBlockage(const LingualBlockage& lingualBlockage, const Scope& scope);
	const LingualBlockage& getLingualBlockage() const;
	const Scope& getScope() const;
private:
	LingualBlockage lingualBlockage_;
	Scope scope_;
};

class RpdAsMajorConnector: public virtual Rpd {
public:
	enum Coverage {
		LINEAR,
		PLANAR
	};

	struct Anchor {
		explicit Anchor(const Position& position = Position(), const RpdWithDirection::Direction& direction = RpdWithDirection::DISTAL);
		Position position;
		RpdWithDirection::Direction direction;
	};

	explicit RpdAsMajorConnector(const vector<Anchor>& anchors);
	static void queryAnchors(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpAnchorMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& opMajorConnectorAnchor, const jobject& individual, const Coverage& coverage, vector<Anchor>& anchors, bool isEtUsed[4]);
	void updateEt(const bool isEtMissing[4], bool isEtUsed[4]);
protected:
	vector<Anchor> anchors_;
};

class AkersClasp: public RpdWithSingleSlot, public RpdWithMaterial, public RpdWithDirection, public RpdWithLingualBlockage {
public:
	AkersClasp(const Position& position, const Material& material, const Direction& direction);
	static AkersClasp* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEtUsed[4]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
};

class Clasp: public RpdWithSingleSlot, public RpdWithMaterial, public RpdWithDirection {
public:
	Clasp(const Position& position, const Material& material, const Direction& direction);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
};

class CombinationClasp:public RpdWithSingleSlot, public RpdWithDirection, public RpdWithLingualBlockage {
public:
	CombinationClasp(const Position& position, const Direction& direction);
	static CombinationClasp* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEtUsed[4]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
};

class CombinedClasp: public RpdWithMultiSlots, public RpdWithMaterial, public RpdWithLingualBlockage {
public:
	CombinedClasp(const Position& startPosition, const Position& endPosition, const Material& material);
	static CombinedClasp* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEtUsed[4]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
};

class DentureBase: public RpdWithMultiSlots {
public:
	DentureBase(const Position& startPosition, const Position& endPosition);
	static DentureBase* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEtUsed[4]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
};

class EdentulousSpace: public RpdWithMultiSlots {
public:
	EdentulousSpace(const Position& startPosition, const Position& endPosition);
	static EdentulousSpace* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEtUsed[4]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
};

class GuidingPlate: public RpdWithSingleSlot {
public:
	explicit GuidingPlate(const Position& position);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
};

class HalfClasp: public RpdWithSingleSlot, public RpdWithMaterial, public RpdWithDirection {
public:
	enum Side {
		BUCCAL,
		LINGUAL
	};

	HalfClasp(const Position& position, const Material& material, const Direction& direction, const Side& side);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
private:
	Side side_;
};

class IBar: public RpdWithSingleSlot {
public:
	explicit IBar(const Position& position);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
};

class LingualRest: public RpdWithSingleSlot, public RpdWithDirection {
public:
	LingualRest(const Position& position, const Direction& direction);
	static LingualRest* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEtUsed[4]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
};

class OcclusalRest: public RpdWithSingleSlot, public RpdWithDirection {
public:
	OcclusalRest(const Position& position, const Direction& direction);
	static OcclusalRest* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpRestMesialOrDistal, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEtUsed[4]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
};

class PalatalPlate: public RpdAsMajorConnector {
public:
	PalatalPlate(const vector<Anchor>& anchors, const vector<Position>& lingualConfrontations);
	static PalatalPlate* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midHasNext, const jmethodID& midListProperties, const jmethodID& midNext, const jmethodID& midStatementGetProperty, const jobject& dpAnchorMesialOrDistal, const jobject& dpLingualConfrontation, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& opMajorConnectorAnchor, const jobject& individual, bool isEtUsed[4]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
private:
	vector<Position> lingualConfrontations_;
};

class Plating: public RpdWithSingleSlot {
public:
	explicit Plating(const Position& position);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
};

class RingClasp: public RpdWithSingleSlot, public RpdWithMaterial, public RpdWithLingualBlockage {
public:
	RingClasp(const Position& position, const Material& material);
	static RingClasp* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEtUsed[4]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
};

class Rpa: public RpdWithSingleSlot, public RpdWithMaterial {
public:
	Rpa(const Position& position, const Material& material);
	static Rpa* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspMaterial, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEtUsed[4]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
};

class Rpi: public RpdWithSingleSlot {
public:
	explicit Rpi(const Position& position);
	static Rpi* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEtUsed[4]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
};

class WwClasp: public RpdWithSingleSlot, public RpdWithDirection, public RpdWithLingualBlockage {
public:
	WwClasp(const Position& position, const Direction& direction);
	static WwClasp* createFromIndividual(JNIEnv*const& env, const jmethodID& midGetInt, const jmethodID& midResourceGetProperty, const jmethodID& midStatementGetProperty, const jobject& dpClaspTipDirection, const jobject& dpToothZone, const jobject& dpToothOrdinal, const jobject& opComponentPosition, const jobject& individual, bool isEtUsed[4]);
	void draw(const Mat& designImage, const vector<Tooth> teeth[4]) const override;
};
