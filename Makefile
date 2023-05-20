# SPDX-License-Identifier: GPL-3.0-or-later
# Makefile for Puny Prince 1.1 by Norbert de Jonge
#
# cppcheck --language=c --std=c99 --verbose PunyPrince.c
# Also try the line below with clang instead of gcc.
#
all:
	gcc -O2 -Wno-unused-result -std=c99 -g -pedantic -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes PunyPrince.c -o PunyPrince `sdl2-config --cflags --libs` -lSDL2_ttf -lSDL2_image -Wno-format-truncation -Wno-stringop-overflow
