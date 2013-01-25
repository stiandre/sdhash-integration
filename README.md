sdhash-jni
==========

This is a modified version of the sdhash tool (http://roussev.net/sdhash/sdhash.html) with support for JNI.

### Building :

- Install Maven.
- Configure the parameter JAVA_JNI_DIR in Makefile to point to the folder containing 'jni.h'.
- Run 'make'.
- Add the generated 'sdhash-jni.jar' to the build path in you java project.
