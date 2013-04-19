sdhash-jni (pre-alpha)
==========

This is a modified version of sdhash 2.3 (http://roussev.net/sdhash/sdhash.html) with support for JNI.

### Compile standalone on linux:

<pre><code>$ sudo apt-get install git libssl-dev g++ maven2 openjdk-6-jdk make libboost-all-dev
$ sudo update-alternatives --config javac
$ git clone https://github.com/pcbje/sdhash-jni.git
$ cd sdhash-jni
# Tell where the jdk is installed. The JAVA_JNI_DIR_EXTRA argument is to e.g. [...]/include/linux
# OSX
$ make JAVA_JNI_DIR=/Library//Java/JavaVirtualMachines/jdk1.7.0_21.jdk/Contents/Home/include/ JAVA_JNI_DIR_EXTRA=.
# Linux:
$ make JAVA_JNI_DIR=/usr/lib/jvm/java-1.6.0-openjdk/include JAVA_JNI_DIR_EXTRA=.

$ make standalone</code></pre>

Then run:

<pre><code># Compare individual files: 
$ java -jar sdhash-jni/target/sdhash-jni.jar file1 file2 ... fileN

# Comapre all files in folder:
$ java -jar sdhash-jni/target/sdhash-jni.jar folder/</code></pre>
