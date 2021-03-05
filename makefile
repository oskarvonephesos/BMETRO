CC = CLANG
_SRC = main.c
_LIBWAV = wav.c

SRC = src/$(_SRC)
LIBWAV = libwav/$(_LIBWAV)

BMETRO: $(SRC) $(LIBWAV)
	$(CC) $(SRC) $(LIBWAV) -o bmetro -lcurses
