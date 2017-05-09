#include <opencv2/highgui/highgui.hpp>

#include "com_shengjie_Main.h"
#include "dllmain.h"
#include "../RpdDesign/resource.h"
#include "../RpdDesign/Tooth.h"
#include "../RpdDesign/Utilities.h"

jobject matToJMat(JNIEnv* const& env, Mat const& mat) {
	auto const& clsStrMat = "org/opencv/core/Mat";
	auto const& clsMat = env->FindClass(clsStrMat);
	auto const& matConstructor = env->GetMethodID(clsMat, "<init>", "()V");
	auto const& jMat = env->NewObject(clsMat, matConstructor);
	auto const& midGetNativeObjAddr = env->GetMethodID(clsMat, "getNativeObjAddr", "()J");
	auto const& matAddr = env->CallLongMethod(jMat, midGetNativeObjAddr);
	*reinterpret_cast<Mat*>(matAddr) = mat;
	return jMat;
}

Mat& jMatToMat(JNIEnv* const& env, jobject const& jMat) {
	auto const& clsStrMat = "org/opencv/core/Mat";
	auto const& clsMat = env->FindClass(clsStrMat);
	auto const& midGetNativeObjAddr = env->GetMethodID(clsMat, "getNativeObjAddr", "()J");
	auto const& matAddr = env->CallLongMethod(jMat, midGetNativeObjAddr);
	return *reinterpret_cast<Mat*>(matAddr);
}

JNIEXPORT jobject JNICALL Java_com_shengjie_Main_getRpdDesign__Lorg_apache_jena_ontology_OntModel_2Lorg_opencv_core_Mat_2(JNIEnv* env, jclass, jobject ontModel, jobject base) {
	vector<Tooth> teeth[nZones];
	Mat designImages[2];
	analyzeBaseImage(jMatToMat(env, base), teeth, designImages);
	vector<Rpd*> rpds;
	queryRpds(env, ontModel, rpds);
	updateDesign(teeth, rpds, designImages, true, true);
	for (auto rpd = rpds.begin(); rpd < rpds.end(); ++rpd)
		delete *rpd;
	bitwise_and(designImages[0], designImages[1], designImages[0]);
	return matToJMat(env, designImages[0]);
}

JNIEXPORT jobject JNICALL Java_com_shengjie_Main_getRpdDesign__Lorg_apache_jena_ontology_OntModel_2(JNIEnv* env, jclass cls, jobject ontModel) {
	auto const& hRsrc = FindResource(dllHandle, MAKEINTRESOURCE(IDB_PNG1), TEXT("PNG"));
	auto const& pBuf = static_cast<uchar*>(LockResource(LoadResource(dllHandle, hRsrc)));
	return Java_com_shengjie_Main_getRpdDesign__Lorg_apache_jena_ontology_OntModel_2Lorg_opencv_core_Mat_2(env, cls, ontModel, matToJMat(env, imdecode(vector<uchar>(pBuf, pBuf + SizeofResource(dllHandle, hRsrc)), IMREAD_COLOR)));
}
