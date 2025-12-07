

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



void render(vector<Pixel> shape, const int scale, Color color){
    for (auto block : shape){
        // std::cout << "x is " << block.x;
        // std::cout << "y is " << block.y;
        // std::cout << "---------";
        DrawRectangle(block.x * scale, block.y * scale, scale, scale, color);
    };
};

void render_board(Board b, const int scale){
    int h = b.get_board_height();
    int w = b.get_board_width();
    for(int i = 0;i < h;i++){
        for(int j = 0;j < w;j++){
            DrawRectangle(j * scale, i * scale, scale, scale, b[i][j]);
        }
    };
}


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
};



int main(void)
{
    // Initialization
    //g++ testing.cpp -o testing $(pkg-config --cflags --libs raylib) -std=c++17
    //--------------------------------------------------------------------------------------
    // ################################# 1 BELOW ARE STAGED ONLY ONCE ##################################
    //######################### 1.1 window size and FPS ###########################################
    const int screenWidth = 400;
    const int screenHeight = 2*screenWidth;
    const int scale = screenWidth / 10;

    const int x_max_scaled = (int) screenWidth / scale - 1;

    const int board_w = (int) screenWidth / scale;
    const int board_h = (int) screenHeight / scale;

    Board board(board_w, board_h);
    SetTargetFPS(60);   // Set our game to run at 60 frames-per-second
    InitWindow(screenWidth, screenHeight, "raylib [core] example - keyboard input");

    //########################## 1.2 Tetrimones/Color Factory Setup ###################################
    static const std::array<std::function<Shape*(int, int, int)>, 7> teri_factory = {{
        [](int x, int y, int r) { return new OShape(x, y, r); },
        [](int x, int y, int r) { return new IShape(x, y, r); },
        [](int x, int y, int r) { return new TShape(x, y, r); },
        [](int x, int y, int r) { return new LShape(x, y, r); },
        [](int x, int y, int r) { return new JShape(x, y, r); },
        [](int x, int y, int r) { return new SShape(x, y, r); },
        [](int x, int y, int r) { return new ZShape(x, y, r); },
    }};

    std::vector<Color> color_factory = {
        GRAY,
        YELLOW,
        GOLD,
        ORANGE,
        PINK,
        MAROON,
        GREEN,
        SKYBLUE,
        BLUE,
        PURPLE,
        VIOLET,
        BROWN,
    };

    
    //########################## 1.3 Random states for tetrimone, position, rotation and color #############
    //randomness of the position
    random_device pos_rd;
    mt19937 pos_gen(pos_rd());
    uniform_int_distribution<> pos_distrib(0, x_max_scaled);

    //randomness of the rotation;
    random_device rot_rd;
    mt19937 rot_gen(rot_rd());
    uniform_int_distribution<> rot_distrib(0, 3);

    //randomness of the tetrimone type itself;
    random_device shape_rd;
    mt19937 shape_gen(shape_rd());
    uniform_int_distribution<> shape_distrib(0, 6);

    //randomness on the color (NOTE: we have to see what will happen to it)
    random_device color_rd;
    mt19937 color_gen(color_rd());
    uniform_int_distribution<> color_distrib(0, 11);
    
    
    //######################## 1.4 State variable that will be re-used ##################################
    Shape* tetri_one = nullptr; // creating a tetrimone reference
    int x = 0;
    int y = 0;
    int r = 0;
    int init_corr = 0;
    Color color = MAROON;
    Boundary limits = Boundary(0, 0, 0);

    int rotate = 0;
    bool reset = false;
    int delta_x = 0;


    float speed = 1;
    auto prev = std::chrono::high_resolution_clock::now();

    // ###################### 2.0 Starting the Main Below ##############################

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {

        //Update
        //############### if tetri is null create one #######################
        if (!tetri_one){
            x = pos_distrib(pos_gen);
            r = rot_distrib(rot_rd) * 90;
            color = color_factory[color_distrib(color_gen)];

            tetri_one = teri_factory[shape_distrib(shape_gen)](x, y, r);
            limits = tetri_one->get_boundary();
            init_corr = xaxis_correction(limits, x_max_scaled);
            tetri_one->update_position(init_corr, 0);
            limits = tetri_one->get_boundary();

            rotate = 0;
            reset = false;
            delta_x = 0;

            prev = std::chrono::high_resolution_clock::now();

        }


        // Update
        //----------------------------------------------------------------------------------
        auto now = std::chrono::high_resolution_clock::now();
        
        // clock-wise rotation
        if ((tetri_one) && IsKeyDown(KEY_C) && (rotate != -1)){
            rotate = 1;
            // tetri_one.update_shape(1);
            // cout << "Yes" << "\n";
        } else{
            if ((tetri_one) && (rotate == 1)){
                // cout << "Update" << "\n";
                limits = do_valid_rotation(tetri_one, board, x_max_scaled, rotate);
                rotate = 0;

            };
        };

        // anti-clock rotation
        if ((tetri_one) && IsKeyDown(KEY_A) && (rotate != 1)){
            rotate = -1;
        } else{
            if ((tetri_one) && (rotate == -1)){
                limits = do_valid_rotation(tetri_one, board, x_max_scaled, rotate);
                rotate = 0;
                reset = true;
            }

        };

        // move left
        if ((tetri_one) && IsKeyDown(KEY_LEFT)){
            if (limits.x_min == 0){
                delta_x = 0;
            }
            else{
                delta_x = -1;
            };
        }else{
            if ((tetri_one) && (delta_x == -1)){
                limits = do_valid_move(tetri_one, board, delta_x);
                delta_x = 0;
            };
        };

        // move right
        if ((tetri_one) && IsKeyDown(KEY_RIGHT)){
            if (limits.x_max == x_max_scaled){
                delta_x = 0;
            }
            else{
                delta_x = 1;
            };
        }else{
            if ((tetri_one) && (delta_x == 1)){
                limits = do_valid_move(tetri_one, board, delta_x);
                delta_x = 0;
            };
        };

        
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - prev);
        if ((tetri_one) && (diff.count() >= 1000) && ((limits.y_max + 1)*scale < screenHeight)) {
            prev = now;
            bool detach = onboard(tetri_one, board, 1);
            cout << "checking detach and it is currently " << detach << "\n";
            if(!detach){
                cout << "updating the fall of the tetrimone " << "\n";
                tetri_one->update_position(0, 1);
            }
            else{
                vector<Pixel> tetri_block = tetri_one->get_shape();
                board.latch_on(tetri_block, color);
                delete tetri_one;
                tetri_one = nullptr;
            }
        };


        if (tetri_one){
            cout << "getting the limits " << "\n";
            limits = tetri_one->get_boundary();
            if ((limits.y_max + 1) == board_h){
                vector<Pixel> tetri_block = tetri_one->get_shape();
                board.latch_on(tetri_block, color);
                delete tetri_one;
                tetri_one = nullptr;
            }
        }


        BeginDrawing();

            ClearBackground(RAYWHITE);
            // DrawRectangle(ballPosition.x, ballPosition.y, scale, scale, MAROON);
            // cout << "calling board render" << "\n";
            render_board(board, scale);
            if (tetri_one){
                // cout << "calling the render shape too" << "\n";
                render(tetri_one->get_shape(), scale, color);
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



