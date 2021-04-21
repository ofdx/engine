GCC_ARGS=-Ofast -Wall --std=c++17 -s -Ibuild/ -Wno-unused-result -Isrc/
MINGW=x86_64-w64-mingw32-g++
SRC_ENGINE=src/engine
SRC_GAME=src/game

all: build build/assetblob build/game win

clean:
	@echo "Removing build output directory..."
	@rm -rf build

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
		x86_64-w64-mingw32-windres src/windows/icon.rc build/icon-res.o ;\
		$(MINGW) $(GCC_ARGS) -static -o build/game.exe $(SRC_ENGINE)/main.cc build/icon-res.o -Lx86_64/lib -Lx86_64_mixer/lib -Ix86_64/include -Ix86_64/include/SDL2 -Ix86_64_mixer/include -lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer -mwindows  -Wl,--no-undefined -Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lsetupapi -lversion -luuid -lstdc++fs -static-libgcc -static-libstdc++; \
		if [ -e build/game.exe ]; then chmod a-x build/game.exe; fi; \
	else \
		touch build/game.exe; \
		echo "  - No Windows build environment available."; \
	fi


# Build the game for 64-bit Linux
build/game: $(SRC_ENGINE)/main.cc build/assetblob $(SRC_ENGINE)/*.h $(SRC_GAME)/* $(SRC_GAME)/scenes/*
	@echo "Building for Linux..."
	@g++ $(GCC_ARGS) -no-pie -I/usr/include -o build/game $(SRC_ENGINE)/main.cc -lstdc++fs -lSDL2 -lSDL2_mixer
