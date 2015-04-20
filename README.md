Intallation:

`sudo apt-get install libpng-dev`

Compiling

`gcc -o bin/OUTPUT FILE.c -lm -lpng`

Mejorar la forma de guardar el campo size, para poder asi hacer efectivo el metodo para 2^32 caracteres (actualmente solo para 2^16)


Thanks for the template to:

/*
 * A simple libpng example program
 * http://zarb.org/~gc/html/libpng.html
 *
 * Modified by Yoshimasa Niwa to make it much simpler
 * and support all defined color_type.
 *
 * To build, use the next instruction on OS X.
 * $ brew install libpng
 * $ clang -lz -lpng15 libpng_test.c
 *
 * Copyright 2002-2010 Guillaume Cottenceau.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */