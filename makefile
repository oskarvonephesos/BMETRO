CC = CLANG
_SRC = main.c
_LIBWAV = wav.c
SOX_INSTALL = brew install sox

SRC = src/$(_SRC)
LIBWAV = libwav/$(_LIBWAV)

BMETRO: $(SRC) $(LIBWAV)
	$(CC) $(SRC) $(LIBWAV) -o bmetro -lcurses
.PHONY: sox
sox: ; $(SOX_INSTALL)
