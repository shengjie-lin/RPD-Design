#pragma once

#include <jni.h>
/* Header for class com_shengjie_Main */

#ifdef __cplusplus
extern "C" {
#endif
	/*
	 * Class:     com_shengjie_Main
	 * Method:    getRpdDesign
	 * Signature: (Lorg/apache/jena/ontology/OntModel;Lorg/opencv/core/Mat;)Lorg/opencv/core/Mat;
	 */
	JNIEXPORT jobject JNICALL Java_com_shengjie_Main_getRpdDesign__Lorg_apache_jena_ontology_OntModel_2Lorg_opencv_core_Mat_2(JNIEnv* env, jclass, jobject ontModel, jobject base);

	/*
	 * Class:     com_shengjie_Main
	 * Method:    getRpdDesign
	 * Signature: (Lorg/apache/jena/ontology/OntModel;)Lorg/opencv/core/Mat;
	 */
	JNIEXPORT jobject JNICALL Java_com_shengjie_Main_getRpdDesign__Lorg_apache_jena_ontology_OntModel_2(JNIEnv* env, jclass cls, jobject ontModel);

#ifdef __cplusplus
}
#endif
