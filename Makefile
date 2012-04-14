all: keyb2midi.c
	gcc -Wall -lasound -o keyb2midi keyb2midi.c

