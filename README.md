sdhash_jni
==========

### Building sdhash with jni:

- Move sdhash_jni.h and sdhash_jni.cc to the sdhash-src folder.
- Move sdbf_set.h and sdbf_set.cc to the sdbf folder.
- Build the java source using e.g. netbeans.
- (javah -o sdhash_jni.h -classpath build/classes SDHash_JNI)
- cd to sdhash root folder.
- Replace the Makefile.
- Compile using make.
- Copy libsdbf.so.0.0.0 to /usr/local/lib.
- Run java with VM option: -Djava.library.path=/usr/local/lib
