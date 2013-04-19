DESTDIR=
PREFIX=$(DESTDIR)/usr/local
INSTDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man/man1

SDBF_SRC = sdbf/sdbf_class.cc sdbf/sdbf_core.cc sdbf/map_file.cc sdbf/entr64.cc sdbf/base64.cc sdbf/bf_utils.cc sdbf/error.cc sdbf/sdbf_conf.cc sdbf/sdbf_set.cc base64/modp_b64.cc

SDHASH_SRC = sdhash-src/sdhash.cc sdhash-src/sdhash_threads.cc sdhash-src/sdhash_jni.cc

CC = g++
LD = $(CC)

SDHASH_OBJ = $(SDHASH_SRC:.cc=.o)
SDBF_OBJ = $(SDBF_SRC:.cc=.o)

ifeq ($(shell uname),Linux)
	CFLAGS = -fPIC -O3 -fno-strict-aliasing -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE_API -D_BSD_SOURCE -I./external -I $(JAVA_JNI_DIR) -I $(JAVA_JNI_DIR)/linux
	SHARED_LIB=libsdhash_jni-linux-x64.so
	LDFLAGS = -L . -L./external/stage/lib -lboost_regex -lboost_system -lboost_filesystem -lboost_program_options -lc -lm -lcrypto -lboost_thread -lpthread -shared -Wl,-soname -o $(SHARED_LIB)
endif

ifeq ($(shell uname),Darwin)
	SHARED_LIB=libsdhash_jni.so
	LDFLAGS = -L . -L./external/stage/lib -lboost_regex -lboost_system -lboost_filesystem -lboost_program_options -lc -lm -lcrypto -lboost_thread -lpthread -shared -Wl,-install_name,$(SHARED_LIB) -o $(SHARED_LIB)
endif

LIBSDBF=libsdbf.a

all: clean stream

sdbf.i:

$(LIBSDBF): $(SDBF_OBJ) 
	ar r $(LIBSDBF) $(SDBF_OBJ)

stream: $(SDHASH_OBJ) $(LIBSDBF)	
	$(LD) $(SDHASH_OBJ) $(SDHASH_CLIENT_OBJ) $(LIBSDBF) $(LDFLAGS)

clean:
	-@rm *.o sdhash 2> /dev/null || true
	-@rm sdhash-src/*.o sdbf/*.o 2> /dev/null || true
	-@rm base64/*.o 2> /dev/null || true
	-@rm libsdbf.a sdhash-jni.jar 2> /dev/null || true
	-@rm sdhash-jni/src/main/resources/$(SHARED_LIB) 2> /dev/null || true
	-@rm -rf build 2> /dev/null || true
	-@rm -rf sdhash-jni/target 2> /dev/null || true

veryclean: clean
	cd external; ./b2 --clean ; cd -

build_jar:
	mvn -f sdhash-jni/pom.xml clean install

build_header:
	mvn -f sdhash-jni/pom.xml clean install
	javah -o sdhash-src/sdhash_jni.h -classpath sdhash-jni/target/classes com.pcbje.sdhashjni.SDHash_JNI

standalone:
	cp -p $(SHARED_LIB) ./sdhash-jni/src/main/resources/
	mvn -f sdhash-jni/pom.xml clean install	

.cc.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $*.cc -o $*.o
