# Makefile for sdhash-jni.  Change options at top if necessary

JAVA_JNI_DIR=/Developer/SDKs/MacOSX10.5.sdk/System/Library/Frameworks/JavaVM.framework/Versions/A/Headers/

DESTDIR=
PREFIX=$(DESTDIR)/usr/local
INSTDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man/man1

SHARED_LIB=libsdhash.so

SDBF_SRC = sdbf/sdbf_class.cc sdbf/sdbf_core.cc sdbf/map_file.cc sdbf/entr64.cc sdbf/base64.cc sdbf/bf_utils.cc sdbf/error.cc sdbf/sdbf_conf.cc sdbf/sdbf_set.cc base64/modp_b64.cc

SDHASH_SRC = sdhash-src/sdhash.cc sdhash-src/sdhash_threads.cc sdhash-src/sdhash_jni.cc

CC = g++
LD = $(CC)

CFLAGS = -fPIC -O3 -fno-strict-aliasing -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE_API -D_BSD_SOURCE -I./external -I $(JAVA_JNI_DIR)

SDHASH_OBJ = $(SDHASH_SRC:.cc=.o)
SDBF_OBJ = $(SDBF_SRC:.cc=.o)

LDFLAGS = -L . -L./external/stage/lib -lboost_regex -lboost_system -lboost_filesystem -lboost_program_options -lc -lm -lcrypto -lboost_thread -lpthread -shared -W,soname -o $(SHARED_LIB)

LIBSDBF=libsdbf.a

all: boost stream jni

sdbf.i:

$(LIBSDBF): $(SDBF_OBJ) 
	ar r $(LIBSDBF) $(SDBF_OBJ)

stream: $(SDHASH_OBJ) $(LIBSDBF)
	$(LD) $(SDHASH_OBJ) $(SDHASH_CLIENT_OBJ) $(LIBSDBF) $(LDFLAGS) 

boost: 
	cd external ; ./bootstrap.sh ; ./b2 link=static ; cd -

clean:
	-@rm *.o sdhash 2> /dev/null || true
	-@rm sdhash-src/*.o sdbf/*.o 2> /dev/null || true
	-@rm base64/*.o 2> /dev/null || true
	-@rm libsdbf.a sdhash-jni.jar 2> /dev/null || true
	-@rm sdhash-jni/src/main/resources/$(SHARED_LIB) 2> /dev/null || true
	-@rm -rf sdhash-jni/target 2> /dev/null || true

veryclean: clean
	cd external; ./b2 --clean ; cd -

jni: boost
	cp -p $(SHARED_LIB) ./sdhash-jni/src/main/resources/
	mvn -f sdhash-jni/pom.xml clean install	
	#javah -o sdhash-src/sdhash_jni.h -classpath sdhash-jni/target/classes com.pcbje.sdhashjni.SDHash_JNI
	$(LD) $(SDHASH_OBJ) $(SDHASH_CLIENT_OBJ) $(LIBSDBF) $(LDFLAGS)
	mv sdhash-jni/target/sdhash-jni.jar .

.cc.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $*.cc -o $*.o
