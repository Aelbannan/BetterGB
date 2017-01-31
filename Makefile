CC = g++
COMPILER_FLAGS = -Wall -static-libstdc++ -std=c++11 -Wl,--enable-stdcall-fixup

SRC = $(wildcard src/*.cpp)
DEPS = $(wildcard src/*.h)
OBJ = $(SRC:.cpp=.o)

EXEC = GameBoy


#INCLUDE_PATHS specifies the additional include paths we'll need
INCLUDE_PATHS = -IC:\mingw_dev_lib\include\SDL2

#LIBRARY_PATHS specifies the additional library paths we'll need
LIBRARY_PATHS = 	-LC:\mingw_dev_lib\lib \
					C:\Windows\System32\winmm.dll \
					C:\Windows\System32\Comdlg32.dll



#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = -lm -lmingw32 -lSDL2main -lSDL2


# this is the target that compiles our executable
all: $(OBJS) $(DEPS)
	$(CC) $(SRC) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(EXEC)