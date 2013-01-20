sdhash_jni
==========

### Building sdhash with jni:

- Move sdhash_jni.cc to the sdhash-src folder.
- Build the java source using e.g. netbeans.
- Create header file: "javah -o sdhash_jni.h -classpath build/classes SDHash_JNI".
- Move sdhash_jni.h to the sdhash-src folder.
- cd to sdhash root folder.
- Backup the original Makefile and replace it with the one here.
- Compile using make.
- Move libsdbf.so.0.0.0 to /usr/local/lib.
