NDKPATH = /Android/android-ndk-r14b
TOOLPATH= $(NDKPATH)/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin
TOOLPREFIX= $(TOOLPATH)/arm-linux-androideabi-
SYSROOT= $(NDKPATH)/platforms/android-9/arch-arm
CC= $(TOOLPREFIX)g++

INCLUDE_PATH:=
LIBRARY_PATH:=
	#-L$(NDKPATH)/sources/cxx-stl/stlport/libs/armeabi-v7a
LIBS:=
	#-llibstlport_static

CFLAGS= -std=c++14 -Wall --sysroot=$(SYSROOT) -static $(INCLUDE_PATH)

SOURCES=$(wildcard src/*.cpp)
OBJECTS=$(patsubst %.cpp,%.o, $(SOURCES))

all: ${OBJECTS}
	${CC} ${CFLAGS} -o pinball ${OBJECTS} ${LIBS}

clean:
	rm -f src/*.o
	rm -f pinball

.cpp.o:
	${CC} ${CFLAGS} ${INCLUDE_PATH} -c $< -o $@
