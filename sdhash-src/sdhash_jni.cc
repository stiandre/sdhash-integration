#include "sdhash_jni.h"

#include "../sdbf/sdbf_class.h"
#include "../sdbf/sdbf_set.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <fstream>

JNIEXPORT jstring JNICALL Java_com_pcbje_sdhashjni_SDHash_1JNI_getSDBFFromFile
(JNIEnv * env, jobject, jstring path, jstring filename, jint size) {
	const char * p =  env->GetStringUTFChars(path, 0);
	const char * fn =  env->GetStringUTFChars(filename, 0);
	
	char * data = new char[size];
	
	fstream file;
	
	file.open(p, ios::in | ios::binary);
	
	file.read(data, size);
	
	file.close();
	
	class sdbf *sdbfm = new sdbf(fn, data, 0, size);
	
	return env->NewStringUTF(sdbfm->to_string().c_str());
}

JNIEXPORT jstring JNICALL Java_com_pcbje_sdhashjni_SDHash_1JNI_getSDBF
(JNIEnv * env, jobject, jstring filename, jbyteArray content, jint len) {		
	const char * fn =  env->GetStringUTFChars(filename, 0);
	
	char * data = (char*)env->GetByteArrayElements(content, false);  

	class sdbf *sdbfm = new sdbf(fn, data, 0, len);
	
	return env->NewStringUTF(sdbfm->to_string().c_str());	
}

JNIEXPORT jstring JNICALL Java_com_pcbje_sdhashjni_SDHash_1JNI_compare___3BI
(JNIEnv * env, jobject, jbyteArray content, jint threshold) {	
	try {
		char * data = (char*)env->GetByteArrayElements(content, false);  

		cout << data;
		cout << "\n";
	
		FILE * in = tmpfile ();

		fputs (data,in);

		rewind(in);

		sdbf_set *set1 = new sdbf_set(in);	

		fclose(in);
	
		std::string resultlist;
	
		resultlist=set1->compare_all(threshold);
	
		return env->NewStringUTF(resultlist.c_str());
	}
	catch (int e) {		
		std::stringstream out;

		out << e;

		return env->NewStringUTF(out.str().c_str());		
	}

	return NULL;
}


JNIEXPORT jstring JNICALL Java_com_pcbje_sdhashjni_SDHash_1JNI_compare__Ljava_lang_String_2I
(JNIEnv * env, jobject, jstring filename, jint threshold) {	
	sdbf_set *set1 = new sdbf_set(env->GetStringUTFChars(filename, 0));	
	
	std::string resultlist;
	
	resultlist=set1->compare_all(threshold);
	
	return env->NewStringUTF(resultlist.c_str());
}


