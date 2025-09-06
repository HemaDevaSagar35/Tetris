#include <raylib.h>
#include <iostream>
#include <chrono>
#include <random>

#include "shapes/l_shape.h"

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


int main(void)
{
    // Initialization
    //g++ testing.cpp -o testing $(pkg-config --cflags --libs raylib) -std=c++17
    //--------------------------------------------------------------------------------------
    const int screenWidth = 400;
    const int screenHeight = 2*screenWidth;
    const int scale = screenWidth / 10;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - keyboard input");

    int x = 0;
    int y = 0;

    LShape tetri_one = LShape(x, y, 0);
    Boundary limits = tetri_one.get_boundary();
    int rotate = 0;
    // vector<Pixel> shape = tetri_one.get_shape();
    // std::cout << "Size is " << tetri_one.get_shape().size();

    // for (auto block : shape){
    //     std::cout << "x is " << block.x << "\n";
    //     std::cout << "y is " << block.y << "\n";
    //     std::cout << "---------" << "\n";
    // }


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
            cout << "Yes" << "\n";
        } else{
            if (rotate == 1){
                cout << "Update" << "\n";
                tetri_one.update_shape(rotate);
                rotate = 0;
            };
        };

        // anti-clock rotation
        if (IsKeyDown(KEY_A) && (rotate != 1)){
            rotate = -1;
        } else{
            if (rotate == -1){
                tetri_one.update_shape(rotate);
                rotate = 0;
            }

        };

        
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - prev);
        if ((diff.count() >= 1000) && ((limits.y_max + 1)*scale < screenHeight)) {
            prev = now;
            tetri_one.update_position(0, 1);
        };
        limits = tetri_one.get_boundary();
        // cout << "y_max" << " " << limits.y_max << "\n";
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);
            // DrawRectangle(ballPosition.x, ballPosition.y, scale, scale, MAROON);
            render(tetri_one.get_shape(), scale);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}



