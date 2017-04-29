#pragma once

#include <map>
#include <opencv2/core/types.hpp>

using namespace std;
using namespace cv;

const int lineThicknessOfLevel[]{1, 4, 7}, nTeethPerZone = 8, nZones = 4;

const string jenaLibPath = "D:/Utilities/apache-jena-3.2.0/lib/";

enum RpdClass {
	AKERS_CLASP,
	CANINE_AKERS_CLASP,
	COMBINATION_ANTERIOR_POSTERIOR_PALATAL_STRAP,
	COMBINATION_CLASP,
	COMBINED_CLASP,
	CONTINUOUS_CLASP,
	DENTURE_BASE,
	EDENTULOUS_SPACE,
	FULL_PALATAL_PLATE,
	LINGUAL_BAR,
	LINGUAL_PLATE,
	LINGUAL_REST,
	OCCLUSAL_REST,
	PALATAL_PLATE,
	RING_CLASP,
	RPA,
	RPI,
	TOOTH,
	WW_CLASP
};

const map<string, RpdClass> rpdMapping_ = {
	{"aker_clasp", AKERS_CLASP},
	{"canine_aker_clasp", CANINE_AKERS_CLASP},
	{"combination_anterior_posterior_palatal_strap", COMBINATION_ANTERIOR_POSTERIOR_PALATAL_STRAP},
	{"combination_clasp", COMBINATION_CLASP},
	{"combined_clasp", COMBINED_CLASP},
	{"continuous_clasp", CONTINUOUS_CLASP},
	{"denture_base", DENTURE_BASE},
	{"edentulous_space", EDENTULOUS_SPACE},
	{"full_palatal_plate", FULL_PALATAL_PLATE},
	{"lingual_bar", LINGUAL_BAR},
	{"lingual_plate", LINGUAL_PLATE},
	{"lingual_rest", LINGUAL_REST},
	{"modified_palatal_plate", PALATAL_PLATE},
	{"occlusal_rest", OCCLUSAL_REST},
	{"palatal_plate", PALATAL_PLATE},
	{"ring_clasp", RING_CLASP},
	{"RPA_clasps", RPA},
	{"RPI_clasps", RPI},
	{"single_palatal_strap", PALATAL_PLATE},
	{"tooth", TOOTH},
	{"wrought_wire_clasp", WW_CLASP}
};

extern bool remedyImage;

extern RotatedRect teethEllipse;

extern RotatedRect remediedTeethEllipse;
