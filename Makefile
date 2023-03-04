GCC_ARGS=-Ofast -Wall --std=c++17 -Ibuild/ -Wno-unused-result -Isrc/
MINGW=x86_64-w64-mingw32-g++
SRC_ENGINE=src/engine
SRC_GAME=src/game

WINDIR_SDLLIB=lib/SDL2-2.0.10/x86_64-w64-mingw32
WINDIR_SDLMIXERLIB=lib/SDL2_mixer-2.0.4/x86_64-w64-mingw32
WINDIR_SDLNET=lib/SDL2_net-2.0.1/x86_64-w64-mingw32

all: build build/assetblob build/game win

clean:
	@echo "Removing build output directory..."
	@rm -rf build

dist: all
	@echo "Strip debug symbols..."
	@strip build/game
	@strip build/game.exe

run: all
	@build/game

runwin: all
	@wine build/game.exe

build:
	@mkdir build

# Deploy the default game data.
defaultgame:
	if ! [ -e "$(SRC_GAME)" ]; then cd src; tar x < ../util/game_default.tar; fi

# Base64 encode asset files into a single file.
blob: build build/assetblob
	
build/assetblob: build/encoder
	@echo "Encoding and combining assets..."
	@util/encode

build/encoder: $(SRC_ENGINE)/encoder.c $(SRC_ENGINE)/base64.h
	@echo "Building base64 encode utility..."
	@gcc -o build/encoder $(SRC_ENGINE)/encoder.c


# Build the game for 64-bit Windows
win: build/game.exe

build/game.exe: build/game
	@echo "Building for Windows..."
	@if [ -n "`which "$(MINGW)"`" ]; then \
		x86_64-w64-mingw32-windres src/engine/icon.rc build/icon-res.o; \
		$(MINGW) $(GCC_ARGS) -static -o build/game.exe $(SRC_ENGINE)/main.cc build/icon-res.o -L$(WINDIR_SDLLIB)/lib -L$(WINDIR_SDLMIXERLIB)/lib -L$(WINDIR_SDLNET)/lib -I$(WINDIR_SDLLIB)/include -I$(WINDIR_SDLLIB)/include/SDL2 -I$(WINDIR_SDLMIXERLIB)/include -I$(WINDIR_SDLNET)/include -lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer -lSDL2_net -liphlpapi -lws2_32 -mwindows  -Wl,--no-undefined -Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lsetupapi -lversion -luuid -lstdc++fs -static-libgcc -static-libstdc++; \
		if [ -e build/game.exe ]; then chmod a-x build/game.exe; fi; \
	else \
		touch build/game.exe; \
		echo "  - No Windows build environment available."; \
	fi


# Build the game for 64-bit Linux
build/game: $(SRC_ENGINE)/main.cc build/assetblob $(shell find $(SRC_ENGINE) -type f) $(shell find -L $(SRC_GAME) -type f)
	@echo "Building for Linux..."
	@g++ $(GCC_ARGS) -static-libstdc++ -no-pie -I/usr/include -o build/game $(SRC_ENGINE)/main.cc -lstdc++fs -lSDL2 -lSDL2_mixer -lSDL2_net
