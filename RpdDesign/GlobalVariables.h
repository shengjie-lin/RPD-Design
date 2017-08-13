#pragma once

#include <map>

using namespace std;
using namespace cv;

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

enum DistanceScaledCurve {
	// 大连接体舌侧绕开卡环等覆盖物的曲线
	BYPASS,
	// 基托曲线
	DENTURE_BASE_CURVE,
	// 舌杆接近舌侧的曲线
	INNER,
	// 大连接体的近中或远中虚线
	MESIAL_OR_DISTAL,
	// 舌杆及舌板远离舌侧的曲线
	OUTER
};

// 配合DistanceScaledCurve使用，指定各类曲线的偏离倍率
const float distanceScales[]{1.5F, 1.75F, 1.8F, 2.4F, 2.5F};

// 3级线粗，分别对应牙齿轮廓、弯制卡环等、其他所有曲线
const int lineThicknessOfLevel[]{2, 5, 8};

int const nTeethPerZone = 8, nZones = 4;

map<string, RpdClass> const rpdMapping_ = {
	// Aker's卡环
	{"aker_clasp", AKERS_CLASP},
	// 尖牙Aker's卡环
	{"canine_aker_clasp", CANINE_AKERS_CLASP},
	// 前后腭带
	{"combination_anterior_posterior_palatal_strap", COMBINATION_ANTERIOR_POSTERIOR_PALATAL_STRAP},
	// 结合卡环
	{"combination_clasp", COMBINATION_CLASP},
	// 联合卡环
	{"combined_clasp", COMBINED_CLASP},
	// 连续卡环
	{"continuous_clasp", CONTINUOUS_CLASP},
	// 基托
	{"denture_base", DENTURE_BASE},
	// 缺牙区
	{"edentulous_space", EDENTULOUS_SPACE},
	// 全腭板
	{"full_palatal_plate", FULL_PALATAL_PLATE},
	// 舌杆
	{"lingual_bar", LINGUAL_BAR},
	// 舌板
	{"lingual_plate", LINGUAL_PLATE},
	// 舌支托
	{"lingual_rest", LINGUAL_REST},
	// 变异腭板，处理方式同腭板
	{"modified_palatal_plate", PALATAL_PLATE},
	// 合支托
	{"occlusal_rest", OCCLUSAL_REST},
	// 腭板
	{"palatal_plate", PALATAL_PLATE},
	// 圈形卡环
	{"ring_clasp", RING_CLASP},
	// RPA卡环组
	{"RPA_clasps", RPA},
	// RPI卡环组
	{"RPI_clasps", RPI},
	// 单腭带，处理方式同腭板
	{"single_palatal_strap", PALATAL_PLATE},
	// 牙齿
	{"tooth", TOOTH},
	// 弯制卡环
	{"wrought_wire_clasp", WW_CLASP}
};

// 是否校正牙齿底图
extern bool remedyImage;

// 校正前的牙列椭圆
extern RotatedRect teethEllipse;

// 校正后的牙列椭圆
extern RotatedRect remediedTeethEllipse;
