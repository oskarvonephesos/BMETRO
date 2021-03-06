CC = CLANG
CFLAGS ?=-Wno-deprecated
SRC = src/main.c
LIBWAV = libwav/wav.c
SOX_INSTALL = brew install sox

BMETRO: $(SRC) $(LIBWAV)
	$(CC) $(CFLAGS) $(SRC) $(LIBWAV) -o bmetro -lcurses
.PHONY: sox
sox: ; $(SOX_INSTALL)
