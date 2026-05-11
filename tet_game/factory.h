#ifndef TET_GAME_FACTORY_H
#define TET_GAME_FACTORY_H

#include <stdint.h>
#include "utils.h"   /* tet_shapes/utils.h: Shape */

/* Color indices stored on the Board and tied 1:1 to piece kind here. The
 * renderer maps these to RGB565 via palette[] in Tetri/main.c (palette
 * itself stays renderer-side; will move to tet_render/palette.{h,c} in
 * step 6). Diverges from the C++ ref's color_factory[12] which picks
 * randomly per spawn -- piece-tied colors match Tetris Guideline visual
 * identity and let the user recognise pieces by colour. */
#define COLOR_BG     0
#define COLOR_T      1
#define COLOR_I      2
#define COLOR_O      3
#define COLOR_L      4
#define COLOR_J      5
#define COLOR_S      6
#define COLOR_Z      7
#define COLOR_COUNT  8

#define SHAPE_KINDS  7

/* One row of the factory: a piece constructor paired with its color index.
 * Every entry has the same signature so `shape_factory[idx].init(...)`
 * spawns any piece without a switch. Replaces the C++ array of lambdas at
 * testing_main.cpp:115-123 plus the parallel color_factory pick. */
typedef struct {
    void   (*init)(Shape *s, int8_t x, int8_t y, int8_t rotation);
    uint8_t color_idx;
} ShapeFactoryEntry;

extern const ShapeFactoryEntry shape_factory[SHAPE_KINDS];

#endif
