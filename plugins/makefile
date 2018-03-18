GPP = g++ -m32 -std=c++11 -Ilib -fno-stack-protector -nodefaultlibs
GCC = gcc -m32 -Ilib -fno-stack-protector -nodefaultlibs
LINK = $(GCC)
PP_OUTFILE = "./PawnPlus.so"

COMPILE_FLAGS = -c -O3 -fpack-struct=1 -fPIC -w -DLINUX -pthread

PawnPlus = -D PawnPlus $(COMPILE_FLAGS)

all: PawnPlus

clean:
	-rm -f *~ *.o *.so
  
static: GPP = g++ -m32 -std=c++11 -Ilib -fno-stack-protector -static-libgcc -static-libstdc++
static: GCC = gcc -m32 -Ilib -fno-stack-protector -static-libgcc -static-libstdc++
static: LINK = $(GPP)
static: all

PawnPlus: clean
	$(GPP) $(PawnPlus) ./lib/sdk/*.cpp
	$(GCC) $(PawnPlus) ./lib/subhook/*.c
	$(GPP) $(PawnPlus) ./src/natives/*.cpp
	$(GPP) $(PawnPlus) ./src/utils/*.cpp
	$(GPP) $(PawnPlus) ./src/*.cpp
	$(LINK) -fshort-wchar -pthread -shared -o $(PP_OUTFILE) *.o
