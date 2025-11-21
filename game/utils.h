#pragma once

#include <limits>
#include<raylib.h>
#include "../shapes/utils.h"



bool onboard(Shape tetri, Board board, int overlap = 1){
    // check where the tetrimone is ready to onboarded onto the board

    tetrimone = tetri.get_shape();

    int offset = 0;
    if (overlap > 0){
        offset = 1;
    }


    for(int i = 0;i<4;i++){
        int x = tetrimone[i].x;
        int y = tetrimone[i].y;

        if (board[y + offset][x] != RAYWHITE){
            return true;
        }
    }

    return false;
}


bool validmove(Shape tetrimone, Board board, int rotation = 0, int delta_x = 0, int delta_y = 0){

    // check whether movement can be made
    if ((rotation == 1) || (rotation == -1)){
        tetrimone.update_shape(rotation);
    }
    if ((delta_x != 0) || (delta_y != 0)){
        tetrimone.update_position(delta_x, delta_y);
    }

    vector<Pixel> shape = tetrimone.get_shape();

    bool overlapped = onboard(shape, board, 0);

    if(overlapped){
        return false;
    }

    return true;
}