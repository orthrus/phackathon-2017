NDKPATH = /Android/android-ndk-r14b
TOOLPATH= $(NDKPATH)/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin
#TOOLPATH= $(NDKPATH)/toolchains/aarch64-linux-android-4.9/prebuilt/darwin-x86_64/bin
TOOLPREFIX= $(TOOLPATH)/arm-linux-androideabi-
#TOOLPREFIX= $(TOOLPATH)/aarch64-linux-android-
SYSROOT= $(NDKPATH)/platforms/android-9/arch-arm

CC= $(TOOLPREFIX)g++

CFLAGS= -Wall --sysroot=$(SYSROOT) -static

$(info $(NDKPATH))
$(info $(TOOLPATH))
$(info $(TOOLPREFIX))
$(info $(SYSROOT))
all: pinball.cpp rs232.c serial.cpp

	$(info Building Pinball)
	$(info $(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^

clean:
	rm -rf *.o
	rm -rf all
