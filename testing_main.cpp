

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
    const int fr = 60;

    const int x_max_scaled = (int) screenWidth / scale - 1;

    const int board_w = (int) screenWidth / scale;
    const int board_h = (int) screenHeight / scale;

    Board board(board_w, board_h);
    SetTargetFPS(fr);   // Set our game to run at 60 frames-per-second
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

    random_device hori_ani_rd;
    mt19937 hori_ani_gen(hori_ani_rd());
    uniform_int_distribution<> hori_ani_distrib(1, 2);
    
    
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
    int downward_ghost = -1;
    int downward_ghost_speed = 62;

    float factor = 0.5;
    int width_animation = (factor * 1000) / (board_w / 2); //millisecond
    int height_animation = (factor * 1000) / (board_h); //millsecond
    int width_left = 0;
    int width_right = board_w - 1;
    int width_animation_direction = 0;
    bool width_animation_completed = false;
    // bool height_animation_completed = false;
    int height_start = board_h - 1;

    int game_over = 0;
    float speed = 1;
    int gravity_drop = 750; // after these milliseconds the tetrimone will move down
    int gravity_drop_min = 250; // this is minimum milliseconds the tetrimone can ever mode
    int gravity_rate = 50;
    int gravity_update = 120000;
    auto game_start = std::chrono::high_resolution_clock::now(); // Gravity is hooked through this
    int score = 0;

    auto prev = std::chrono::high_resolution_clock::now();
    auto width_prev = std::chrono::high_resolution_clock::now();
    auto height_prev = std::chrono::high_resolution_clock::now();

    // ###################### 2.0 Starting the Main Below ##############################

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {

        //Update
        //cout << "window happening" << "\n";
        // ################ refine gravity if required ######################
        auto game_now = std::chrono::high_resolution_clock::now();
        auto game_diff = std::chrono::duration_cast<std::chrono::milliseconds>(game_now - game_start);
        if ((game_diff.count() >= gravity_update) && (gravity_drop > gravity_drop_min) && (downward_ghost == -1)){
            gravity_drop = gravity_drop - gravity_rate;
            game_start = game_now;
        };


        //############### if tetri is null create one #######################
        if ((!tetri_one) && (!board.is_lines_formed())){
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
            width_prev = std::chrono::high_resolution_clock::now();
            height_prev = std::chrono::high_resolution_clock::now();

            width_left = 0;
            width_right = board_w - 1;
            width_animation_direction = 0;
            width_animation_completed = false;
            // bool height_animation_completed = false;
            height_start = board_h - 1;
            downward_ghost = -1;

            int height_peaked = board.get_height_peak();
            // cout << "height peaked is " << board.get_height_peak() << "\n";
            // cout << "limit peak is " << limits.y_max << "\n";
            if (height_peaked <= limits.y_max){
                // cout << "Problem" << "\n";
                game_over = 1; // Simplistic choice I see
            };

        };

        if (game_over == 1){
            // cout << "GAME OVER" << "\n";
            continue;
        };
        // Update
        //----------------------------------------------------------------------------------
        
        
        // clock-wise rotation
        if ((tetri_one) && IsKeyDown(KEY_C) && (rotate != -1) && (downward_ghost == -1)){
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
        if ((tetri_one) && IsKeyDown(KEY_A) && (rotate != 1) && (downward_ghost == -1)){
            rotate = -1;
        } else{
            if ((tetri_one) && (rotate == -1)){
                limits = do_valid_rotation(tetri_one, board, x_max_scaled, rotate);
                rotate = 0;
                reset = true;
            }

        };

        // move left
        if ((tetri_one) && IsKeyDown(KEY_LEFT) && (downward_ghost == -1)){
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
        if ((tetri_one) && IsKeyDown(KEY_RIGHT) && (downward_ghost == -1)){
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

        // move down
        if ((tetri_one) && IsKeyDown(KEY_DOWN)){
            downward_ghost = gravity_drop;
        }else{
            if ((tetri_one) && (downward_ghost > -1)){
                gravity_drop = downward_ghost_speed;
            };
        };

        auto now = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - prev);
        if ((tetri_one) && (diff.count() >= gravity_drop) && ((limits.y_max + 1)*scale < screenHeight)) {
            prev = now;
            bool detach = onboard(tetri_one, board, 1);
            if(!detach){
                tetri_one->update_position(0, 1);
            }
            else{
                vector<Pixel> tetri_block = tetri_one->get_shape();
                board.latch_on(tetri_block, color);
                delete tetri_one;
                tetri_one = nullptr;
                if (downward_ghost > -1){
                    gravity_drop = downward_ghost;
                    downward_ghost = -1;
                }
            }
        };


        if (tetri_one){
            limits = tetri_one->get_boundary();
            if ((limits.y_max + 1) == board_h){
                vector<Pixel> tetri_block = tetri_one->get_shape();
                board.latch_on(tetri_block, color);
                delete tetri_one;
                tetri_one = nullptr;
                if (downward_ghost > -1){
                    gravity_drop = downward_ghost;
                    downward_ghost = -1;
                }
            }
        }

        // ############################################### ANIMATION ##########################################
        if ((!tetri_one) && (board.is_lines_formed()) && (!width_animation_completed)){
            // 1. first we have to make the lines
            if (width_animation_direction == 0){
                int int_ani_check = hori_ani_distrib(hori_ani_gen);
                if (int_ani_check == 1){
                    width_animation_direction = 1;
                    width_left = 0;
                    width_right = board_w - 1;
                }else if (int_ani_check == 2){
                    // cout << "reversed is selected " << "\n";
                    width_animation_direction = -1;
                    width_right = board_w / 2;
                    width_left = width_right + (board_w % 2 - 1);
                };
            };
            // cout << "width animation is going" <<  "\n";
            // cout << "width animation direction is " << width_animation_direction << "\n";
            auto width_now = std::chrono::high_resolution_clock::now();
            auto width_diff = std::chrono::duration_cast<std::chrono::milliseconds>(width_now - width_prev);
            // cout << "diff count is " <<  width_diff.count() << " reference is " << width_animation;
            // cout << "width left is "<< width_left << " and width right is " << width_right;
            if ((width_left <= width_right) && (width_left >= 0) && (width_right < board_w) && (width_diff.count() >= width_animation)){
                // cout << "cleaning inside " << "\n";
                board.clean_lines_selectively(width_left, width_right);
                // cout << "cleaning inside finished and width animation_direction is "<< width_animation_direction << "\n";
                width_left = width_left + (1 * width_animation_direction);
                width_right = width_right - (1 * width_animation_direction);
                // cout << "inside width left is "<< width_left << " and width right is " << width_right; 
                width_prev = width_now;
            }else if ((width_left > width_right) || !((width_left >= 0) && (width_right < board_w))){
                width_animation_completed = true;
                board.calculate_delta();
            }
            
        }

        if ((!tetri_one) && (board.is_lines_formed()) && (width_animation_completed)){
            auto height_now = std::chrono::high_resolution_clock::now();
            auto height_diff = std::chrono::duration_cast<std::chrono::milliseconds>(height_now - height_prev);
            if ((height_start >= 0) && (height_diff.count() >= height_animation)){
                board.clear_lines_selectively(height_start);
                height_start = height_start - 1;
                height_prev = height_now;
            }else if (height_start < 0){
                int total_lines = board.get_total_lines();
                score = score + total_lines;
                board.reset_lines_deltas();

            }
        }

        BeginDrawing();

            ClearBackground(RAYWHITE);
            render_board(board, scale);
            if (tetri_one){
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



