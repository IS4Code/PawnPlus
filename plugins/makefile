GPP = g++ -D _GLIBCXX_USE_CXX11_ABI=0 -m32 -std=c++14 -Ilib -Isrc -fno-stack-protector
GCC = gcc -D _GLIBCXX_USE_CXX11_ABI=0 -m32 -Ilib -Isrc -fno-stack-protector
LINK = $(GPP) -Wl,-z,defs -lstdc++
PP_OUTFILE = "./PawnPlus.so"

COMPILE_FLAGS = -c -O3 -fPIC -w -DLINUX -pthread -fno-operator-names -ftemplate-depth=2048

PawnPlus = -D PawnPlus $(COMPILE_FLAGS)

all: PawnPlus

clean:
	-rm -f *~ *.o *.so
  
static: GPP = g++ -D _GLIBCXX_USE_CXX11_ABI=0 -m32 -std=c++14 -Ilib -Isrc -fno-stack-protector -static-libgcc -static-libstdc++
static: GCC = gcc -D _GLIBCXX_USE_CXX11_ABI=0 -m32 -Ilib -Isrc -fno-stack-protector -static-libgcc -static-libstdc++
static: LINK = $(GPP) -Wl,-z,defs
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
	$(GPP) $(PawnPlus) ./src/fixes/*.cpp
	$(GPP) $(PawnPlus) ./src/*.cpp
	$(LINK) -pthread -ldl -shared -o $(PP_OUTFILE) *.o
