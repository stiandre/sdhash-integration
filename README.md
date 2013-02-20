sdhash-jni (pre-alpha version)
==========

This is a modified version of sdhash 2.3 (http://roussev.net/sdhash/sdhash.html) with support for JNI.

### Compile standalone on linux:

<pre><code>$ git clone https://github.com/pcbje/sdhash-jni.git
$ sudo apt-get install libssl-dev g++ maven2 openjdk-6-jdk make
$ make standalone JAVA_JNI_DIR=/path/to/jdk/include
$ java -jar sdhash-jni/target/sdhash-jni.har file1 file2 ... fileN</code></pre>
