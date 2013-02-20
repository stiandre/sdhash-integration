sdhash-jni (pre-alpha)
==========

This is a modified version of sdhash 2.3 (http://roussev.net/sdhash/sdhash.html) with support for JNI.

### Compile standalone on linux:

<pre><code>$ sudo apt-get install git libssl-dev g++ maven2 openjdk-6-jdk make
$ sudo update-alternatives --config javac
$ git clone https://github.com/pcbje/sdhash-jni.git
$ cd sdhash-jni
$ make standalone JAVA_JNI_DIR=/usr/lib/jvm/java-1.6.0-openjdk/include
$ java -jar sdhash-jni/target/sdhash-jni.har file1 file2 ... fileN</code></pre>
