#pragma once

#include <limits>
#include<raylib.h>
#include "../shapes/utils.h"


int xaxis_correction(Boundary limits, int x_max_scaled){
    if (limits.x_min < 0){
        return -1*limits.x_min;
    };

    if (limits.x_max > x_max_scaled){
        return x_max_scaled - limits.x_max;
    };

    return 0;
}

bool onboard(Shape* tetri, Board& board, int overlap = 1){
    // check where the tetrimone is ready to onboarded onto the board

    vector<Pixel> tetrimone = tetri->get_shape();
    int w = board.get_board_width();
    int h = board.get_board_height();

    int offset = 0;
    if (overlap > 0){
        offset = 1;
    }


    for(int i = 0;i<4;i++){
        int x = tetrimone[i].x;
        int y = tetrimone[i].y;

        if (((y + offset) >= h) || (board[y + offset][x] != RAYWHITE)){
            return true;
        }
    }

    return false;
};


bool validmove(Shape* tetrimone, Board& board, int rotation = 0, int delta_x = 0, int delta_y = 0){

    // check whether movement can be made
    if ((rotation == 1) || (rotation == -1)){
        tetrimone->update_shape(rotation);
    }
    if ((delta_x != 0) || (delta_y != 0)){
        tetrimone->update_position(delta_x, delta_y);
    }

    bool overlapped = onboard(tetrimone, board, 0);

    if(overlapped){
        return false;
    }

    return true;
};

Boundary do_valid_rotation(Shape* tetri_one, Board& board, int x_max_scaled, int rotate){
    tetri_one->update_shape(rotate);
    Boundary limits = tetri_one->get_boundary();
    int correction = xaxis_correction(limits, x_max_scaled);
    tetri_one->update_position(correction, 0);
    
    bool invalid_move = onboard(tetri_one, board, 0);
    if (invalid_move){
        tetri_one->update_position(-1*correction, 0);
        tetri_one->update_shape(-1*rotate);
        limits = tetri_one->get_boundary();
    }

    return limits;
}


Boundary do_valid_move(Shape* tetri_one, Board& board, int delta_x){
    tetri_one->update_position(delta_x, 0);
    Boundary limits = tetri_one->get_boundary();
    bool invalid_move = onboard(tetri_one, board, 0);
    if (invalid_move){
        tetri_one->update_position(-1*delta_x, 0);
        limits = tetri_one->get_boundary();
    }
    return limits;
}

