

#include <raylib.h>
#include <iostream>
#include <chrono>
#include <random>

#include "shapes/l_shape.h"
#include "shapes/j_shape.h"
#include "shapes/i_shape.h"
#include "shapes/s_shape.h"
#include "shapes/z_shape.h"
#include "shapes/t_shape.h"
#include "shapes/o_shape.h"
#include "shapes/utils.h"
#include "game/utils.h"



void render(vector<Pixel> shape, const int scale){
    for (auto block : shape){
        // std::cout << "x is " << block.x;
        // std::cout << "y is " << block.y;
        // std::cout << "---------";
        DrawRectangle(block.x * scale, block.y * scale, scale, scale, MAROON);
    };
};


class Element{
    public:
        int x;
        int rotation;

        Element(int x, int rotation){
            this->x = x;
            this->rotation = rotation;
        };
};

// Element generate(int width, int height, int scale, uniform_int_distribution<> rand_position, uniform_int_distribution<> rand_rotation){
//     int x = rand_position();
//     int rotation = rand_rotation();

//     int width_limit = width/scale - 1;

//     if (x == width_limit){
//         x = x - 1;
//     };

//     return Element(x, rotation);
// };

void shape_testing(vector<Pixel> shape){
    for (auto block : shape){
        std::cout << "x is " << block.x << "\n";
        std::cout << "y is " << block.y << "\n";
        std::cout << "---------" << "\n";
    };
}

int xaxis_correction(Boundary limits, int x_max_scaled){
    if (limits.x_min < 0){
        return -1*limits.x_min;
    };

    if (limits.x_max > x_max_scaled){
        return x_max_scaled - limits.x_max;
    };

    return 0;
}


int main(void)
{
    // Initialization
    //g++ testing.cpp -o testing $(pkg-config --cflags --libs raylib) -std=c++17
    //--------------------------------------------------------------------------------------
    const int screenWidth = 400;
    const int screenHeight = 2*screenWidth;
    const int scale = screenWidth / 10;

    const int x_max_scaled = (int) screenWidth / scale - 1;

    const int board_w = (int) screenWidth / scale;
    const int board_h = (int) screenHeight / scale;

    //randomness of the position
    random_device pos_rd;
    mt19937 pos_gen(pos_rd());
    uniform_int_distribution<> pos_distrib(0, x_max_scaled);

    //randomness of the rotation;
    random_device rot_rd;
    mt19937 rot_gen(rot_rd());
    uniform_int_distribution<> rot_distrib(0, 3);

    //chose the values for the random
    int x = pos_distrib(pos_gen); // these can be random initialized with
    int y = 0; // these can be random initialized with
    int init_rotation = rot_distrib(rot_rd) * 90;

    // create the shape and do any xaxis correction
    // LShape tetri_one = LShape(x, y, init_rotation);
    // cout << x << " " << y << "\n";
    Shape* tetri_one = nullptr;
    tetri_one = new TShape(x, y, init_rotation);
    Boundary limits = tetri_one->get_boundary();
    int init_corr = xaxis_correction(limits, x_max_scaled);
    tetri_one->update_position(init_corr, 0);
    limits = tetri_one->get_boundary();
    


    int rotate = 0;
    bool reset = false;
    int delta_x = 0;

    Board board(board_w, board_h);
    InitWindow(screenWidth, screenHeight, "raylib [core] example - keyboard input");

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------
    float speed = 1;
    auto prev = std::chrono::high_resolution_clock::now();


    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        auto now = std::chrono::high_resolution_clock::now();
        
        // clock-wise rotation
        if (IsKeyDown(KEY_C) && (rotate != -1)){
            rotate = 1;
            // tetri_one.update_shape(1);
            // cout << "Yes" << "\n";
        } else{
            if (rotate == 1){
                // cout << "Update" << "\n";
                tetri_one->update_shape(rotate);
                limits = tetri_one->get_boundary();
                rotate = 0;
                int correction = xaxis_correction(limits, x_max_scaled);
                tetri_one->update_position(correction, 0);
            };
        };

        // anti-clock rotation
        if (IsKeyDown(KEY_A) && (rotate != 1)){
            rotate = -1;
        } else{
            if (rotate == -1){
                tetri_one->update_shape(rotate);
                limits = tetri_one->get_boundary();
                rotate = 0;
                reset = true;
                int correction = xaxis_correction(limits, x_max_scaled);
                tetri_one->update_position(correction, 0);
            }

        };

        // move left
        if (IsKeyDown(KEY_LEFT)){
            if (limits.x_min == 0){
                delta_x = 0;
            }
            else{
                delta_x = -1;
            };
        }else{
            if (delta_x == -1){
                tetri_one->update_position(delta_x, 0);
                limits = tetri_one->get_boundary();
                delta_x = 0;
            };
        };

        // move right
        if (IsKeyDown(KEY_RIGHT)){
            if (limits.x_max == x_max_scaled){
                delta_x = 0;
            }
            else{
                delta_x = 1;
            };
        }else{
            if (delta_x == 1){
                // cout << "Update" << "\n";
                // shape_testing(tetri_one.get_shape());
                tetri_one->update_position(delta_x, 0);
                limits = tetri_one->get_boundary();
                delta_x = 0;
            };
        };

        
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - prev);
        if ((diff.count() >= 1000) && ((limits.y_max + 1)*scale < screenHeight)) {
            prev = now;
            bool detach = onboard(tetri_one, board, 1);
            if(!detach){
                tetri_one->update_position(0, 1);
            }
            else{
                delete tetri_one;
                tetri_one = nullptr;
            }
        };
        if (!tetri_one){
            limits = tetri_one->get_boundary();
        }

        // CHESHVIKA
        // CHAISHAVI
        // SAGAR
        // VEENILA
        // SRINU - NANNA
        // SUGUNA
        // SURYA RAO - SURYA RAO - SURYA RAO
        // HANUMANTH RAO
        // SUDHA AMMAMA
        // SONU
        // chikiri
        // oh my baby

        // cout << "y_max" << " " << limits.y_max << "\n";
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);
            // DrawRectangle(ballPosition.x, ballPosition.y, scale, scale, MAROON);
            render_board(board);
            if (!tetri_one){
                render(tetri_one->get_shape(), scale);
            }

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}



