GPP = g++ -D _GLIBCXX_USE_CXX11_ABI=0 -m32 -std=c++11 -Ilib -Isrc -fno-stack-protector
GCC = gcc -D _GLIBCXX_USE_CXX11_ABI=0 -m32 -Ilib -Isrc -fno-stack-protector
LINK = $(GPP) -lstdc++
PP_OUTFILE = "./PawnPlus.so"

COMPILE_FLAGS = -c -O3 -fPIC -w -DLINUX -pthread -fno-operator-names -ftemplate-depth=2048

PawnPlus = -D PawnPlus $(COMPILE_FLAGS)

all: PawnPlus

clean:
	-rm -f *~ *.o *.so
  
static: GPP = g++ -D _GLIBCXX_USE_CXX11_ABI=0 -m32 -std=c++11 -Ilib -Isrc -fno-stack-protector -static-libgcc -static-libstdc++
static: GCC = gcc -D _GLIBCXX_USE_CXX11_ABI=0 -m32 -Ilib -Isrc -fno-stack-protector -static-libgcc -static-libstdc++
static: LINK = $(GPP)
static: all

PawnPlus: clean
	$(GCC) $(PawnPlus) ./lib/sdk/amx/*.c
	$(GPP) $(PawnPlus) ./lib/sdk/*.cpp
	$(GCC) $(PawnPlus) ./lib/subhook/*.c
	$(GPP) $(PawnPlus) ./src/api/*.cpp
	$(GPP) $(PawnPlus) ./src/utils/*.cpp
	$(GPP) $(PawnPlus) ./src/objects/*.cpp
	$(GPP) $(PawnPlus) ./src/modules/*.cpp
	$(GPP) $(PawnPlus) ./src/natives/*.cpp
	$(GPP) $(PawnPlus) ./src/*.cpp
	$(LINK) -fshort-wchar -pthread -shared -o $(PP_OUTFILE) *.o
