sdhash-jni (pre-alpha)
==========

This is a modified version of sdhash 2.3 (http://roussev.net/sdhash/sdhash.html) with support for JNI.

Target platforms: OSX, Linux x64, Linux x86

### Maven:
Repository:
<pre><code>&lt;repositories>
  &lt;repository>
    &lt;id>pcbje.com&lt;/id>
    &lt;url>http://maven2.pcbje.com&lt;/url>
  &lt;/repository>
&lt;/repositories>
</code></pre>

Dependency:
<pre><code>&lt;dependency>
  &lt;groupId>com.pcbje&lt;/groupId>
  &lt;artifactId>sdhash-jni&lt;/artifactId>
  &lt;version>1.0-SNAPSHOT&lt;/version>
&lt;/dependency>
</code></pre>

### Compile standalone on linux:

<pre><code>$ sudo apt-get install git libssl-dev g++ maven2 openjdk-6-jdk make libboost-all-dev
$ sudo update-alternatives --config javac
$ make JAVA_JNI_DIR=/usr/lib/jvm/java-1.6.0-openjdk/include
$ make standalone</code></pre>

### Compile standalone on osx (very unstable on 10.8):
<pre><code>$ git clone https://github.com/pcbje/sdhash-jni.git
$ cd sdhash-jni
# osx >= 10.8: copy src/main/resources/boost/osx/* to /usr/lib and hope for the best.
$ make boost
$ make
$ make standalone</code></pre>


Then run:

<pre><code># Compare individual files: 
$ java -jar sdhash-jni/target/sdhash-jni.jar file1 file2 ... fileN

# Comapre all files in folder:
$ java -jar sdhash-jni/target/sdhash-jni.jar folder/</code></pre>
