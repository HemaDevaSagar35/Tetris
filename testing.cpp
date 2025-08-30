#include <raylib.h>
#include <iostream>
#include <chrono>

#include "shapes/l_shape.h"

void render(vector<Pixel> shape, const int scale){
    for (auto block : shape){
        // std::cout << "x is " << block.x;
        // std::cout << "y is " << block.y;
        // std::cout << "---------";
        DrawRectangle(block.x * scale, block.y * scale, scale, scale, MAROON);
    };
};

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

    LShape tetri_one = LShape(x, y);
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
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - prev);
        if ((diff.count() >= 1000) && ((y + 4)*scale <= screenHeight)) {
            prev = now;
            y = y + 1;
            tetri_one.update_position(x, y);
        };
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