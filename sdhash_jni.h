/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class SDHash_JNI */

#ifndef _Included_SDHash_JNI
#define _Included_SDHash_JNI
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     SDHash_JNI
 * Method:    getSDBF
 * Signature: (Ljava/lang/String;[BI)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_SDHash_1JNI_getSDBF
  (JNIEnv *, jobject, jstring, jbyteArray, jint);

/*
 * Class:     SDHash_JNI
 * Method:    compare
 * Signature: ([BI)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_SDHash_1JNI_compare
  (JNIEnv *, jobject, jbyteArray, jint);

#ifdef __cplusplus
}
#endif
#endif
