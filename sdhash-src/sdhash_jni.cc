#include "sdhash_jni.h"

#include "../sdbf/sdbf_class.h"
#include "../sdbf/sdbf_set.h"

JNIEXPORT jstring JNICALL Java_com_pcbje_sdhashjni_SDHash_1JNI_getSDBF
(JNIEnv * env, jobject, jstring filename, jobject content, jint len) {		
	const char * fn =  env->GetStringUTFChars(filename, 0);
	
	char * data = (char*)env->GetDirectBufferAddress(content);  
	
	class sdbf * sdbfm = new sdbf(fn, data, 0, len);
	
	return env->NewStringUTF(sdbfm->to_string().c_str());	
}

JNIEXPORT jstring JNICALL Java_com_pcbje_sdhashjni_SDHash_1JNI_compare
(JNIEnv * env, jobject, jobject content, jint threshold) {	
	try {
		char * data = (char*)env->GetDirectBufferAddress(content);  

		int length = env->GetDirectBufferCapacity(content) + 1;

		data[length - 1] = '\0';

		FILE * in = tmpfile ();

		fputs (data,in);

		rewind(in);
		
		sdbf_set * set1 = new sdbf_set(in);	
		
		fclose(in);
		
		std::string resultlist = set1->compare_all(threshold);
		
		return env->NewStringUTF(resultlist.c_str());
	}
	catch (int e) {
		cout << "An exception occurred. Exception Nr. " << e << "\n";

		return  env->NewStringUTF("");
	}
}


