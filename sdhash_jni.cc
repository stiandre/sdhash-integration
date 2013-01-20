#include "sdhash_jni.h"

#include "../sdbf/sdbf_class.h"

JNIEXPORT jstring JNICALL Java_SDHash_1JNI_getSDBF
(JNIEnv * env, jobject, jstring filename, jbyteArray content, jint len) {	
	
	const char * fn =  env->GetStringUTFChars(filename, 0);
	char * data = (char*)env->GetByteArrayElements(content, false);  

	class sdbf *sdbfm = new sdbf(fn, data, 0, len);
	
	return env->NewStringUTF(sdbfm->to_string().c_str());	
}
