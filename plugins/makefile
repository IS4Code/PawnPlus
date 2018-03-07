GPP = g++ -m32 -Ilib -fno-stack-protector
GCC = gcc -m32 -Ilib -fno-stack-protector
OUTFILE = "./PawnPlus.so"

COMPILE_FLAGS = -c -O3 -fpack-struct=1 -fPIC -w -DLINUX

Async = -D PawnPlus $(COMPILE_FLAGS)

all: PawnPlus

clean:
	-rm -f *~ *.o *.so

PawnPlus: clean
	$(GPP) $(PawnPlus) ./lib/sdk/*.cpp
	$(GPP) $(PawnPlus) ./lib/subhook/*.cpp
	$(GPP) $(PawnPlus) ./src/natives/*.cpp
	$(GPP) $(PawnPlus) ./src/utils/*.cpp
	$(GPP) $(PawnPlus) ./src/*.cpp
	$(GCC) -nodefaultlibs -fshort-wchar -shared -o $(OUTFILE) *.o
