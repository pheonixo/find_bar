CFLAGS = -x c -std=gnu99 -pipe -fno-builtin -fmessage-length=0  \
  -g `pkg-config --cflags gtk+-3.0`

LFLAGS := `pkg-config --libs gtk+-3.0` -lm

fbar: find_bar.c find_bar.h
	gcc $(CFLAGS) -o fbar find_bar.c $(LFLAGS)
