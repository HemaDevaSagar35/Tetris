#pragma once

#include <limits>
#include<raylib.h>
#include <algorithm>

using namespace std;



inline bool operator==(const Color& a, const Color& b){
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

inline bool operator!=(const Color& a, const Color& b) { return !(a==b);}

struct Pixel{
    int x;
    int y;
    Pixel(int x, int y){
        this->x = x;
        this->y = y;
    };
};

struct PixelWithColor : public Pixel{

    Color color;
    PixelWithColor(int x, int y, Color c) : Pixel(x, y), color(c) {}
};

struct Boundary{
    int x_min;
    int x_max;
    int y_max;
    Boundary(int x_min, int x_max, int y_max){
        this->x_min = x_min;
        this->x_max = x_max;
        this->y_max = y_max;
    };
};


class Shape {

    protected:
        vector <Pixel> shape;
        int rotation;
        Boundary limits{numeric_limits<int>::max(), 0, 0};

    public:
        Shape (int x, int y, int rotation) {};

        vector <Pixel> get_shape() {
            return shape;
        };

        Boundary get_boundary(){
            return limits;
        }

        void update_position(int delta_x, int delta_y){
            // the updates here are affine like right or left or lowering
            // int delta_x = x - shape[0].x;
            // int delta_y = y - shape[0].y;

            shape[0].x = shape[0].x + delta_x;
            shape[0].y = shape[0].y + delta_y;

            shape[1].x = shape[1].x + delta_x;
            shape[1].y = shape[1].y + delta_y;

            shape[2].x = shape[2].x + delta_x;
            shape[2].y = shape[2].y + delta_y;

            shape[3].x = shape[3].x + delta_x;
            shape[3].y = shape[3].y + delta_y;

            update_boundary();
        };

        virtual void update_shape(int rotate){
            // 0 -> 90 & 0 -> -90
            // 90 -> 180 & 90 -> 0
            // 180 -> 270 & 270 -> 180
            //270 -> 360 & 360 -> 270

            update_boundary();
            
        }

        virtual ~Shape() {}

    protected:
        virtual void create_shape(int x, int y){
            Pixel block1(x, y);
            shape.push_back(block1);

            Pixel block2(x, y + 1);
            shape.push_back(block2);

            Pixel block3(x, y + 2);
            shape.push_back(block3);

            Pixel block4(x + 1, y + 2);
            shape.push_back(block4);

            update_boundary();

        };

        virtual void initialize_shape(int x, int y, int rotation){
            create_shape(x, y);
            this->rotation = 0;
            while (this->rotation < rotation){
                update_shape(1);
            }
            update_boundary();

        }
    
        void update_boundary(){
            limits.x_min = numeric_limits<int>::max();
            limits.x_max = 0;
            limits.y_max = 0;
            for (auto b : shape){
                limits.x_min = min(limits.x_min, b.x);
                limits.x_max = max(limits.x_max, b.x);
                limits.y_max = max(limits.y_max, b.y);
            };
        };
};



class Board{
    
    bool lines_filled = false;
    vector<int> line_formed;
    int width;
    int height;
    int height_towered;
    vector<int> deltas;
    //w and h here are width and height. correspondiing index limit would be 
    public:
        vector<vector<Color>> board;
        Board(int w, int h){
            // initialize everything with white board
            for(int i = 0; i < h;i++){
                vector<Color> hstrip;
                for(int j=0;j<w;j++){
                    
                    Color p = RAYWHITE;
                    hstrip.push_back(p);
                    
                }
                this->board.push_back(hstrip);
                this->line_formed.push_back(0);
                this->deltas.push_back(0);

            };
            this->width = w;
            this->height = h;
            this->height_towered = h;
        };

        void latch_on(vector<Pixel> terimone, Color color){
            for(auto p : terimone){
                int x = p.x;
                int y = p.y;
                board[y][x] = color;
                this->height_towered = min(this->height_towered, y); // This might required algorithms, may we can replace with with a = (a < b) ? a: b; to avoid algos
            };

            line_formation();
        };

        int get_height_peak(){
            return this->height_towered;
        }

        bool is_lines_formed(){
            return lines_filled;
        }

        vector<int> get_line_indexes(){
            return line_formed;
        }
    
        void line_formation(){
            // here the task is to check if a line got filled completely?
            int lines_filled_counter = 0;
            for(int i = 0; i < board.size();i++){
                int counter = 0;
                for (int j = 0;j < board[i].size();j++){
                    if (board[i][j] == RAYWHITE){
                        counter +=1;
                        break;
                    };
                }
                
                line_formed[i] = 1 - counter;
                lines_filled_counter = lines_filled_counter + 1 - counter;
            }

            lines_filled = lines_filled_counter > 0;
        };


        void clear_lines(){
            // clear the lines simply - without animation on the disappearance
            // calculate downward moment
            // vector<int> deltas = calculate_delta();
            calculate_delta();
            clean_lines_fully();

            for(int i = board.size() - 2; i >= 0; i--){
                if (line_formed[i] == 1){
                    continue;
                }
                int del_y = deltas[i];
                for (int j = 0;j < board[i].size();j++){
                    board[i+del_y][j] = board[i][j];
                }
            }
            fill(this->line_formed.begin(), this->line_formed.end(), 0);
            fill(this->deltas.begin(), this->deltas.end(), 0);
            this->lines_filled = false;

        }

        int get_board_width(){
            return this->width;
        }

        int get_board_height(){
            return this->height;
        }

        // index overloading
        vector<Color>& operator[](int i){
            if (i >= height) throw out_of_range("board height out of range");
            return board[i];
        }

        const vector<Color>& operator[](int i) const {
            if (i >= height) throw out_of_range("board height out of range");
            return board[i];
        }

        void clean_lines_selectively(int left, int right){
            for(int i = 0;i<line_formed.size();i++){
                if (line_formed[i] == 0){
                    continue;
                }
                board[i][left] = RAYWHITE;
                board[i][right] = RAYWHITE;
            };

        }

        void clear_lines_selectively(int line_no){
            // clear the lines simply - without animation on the disappearance
            // calculate downward moment
            // vector<int> deltas = calculate_delta();
            // calculate_delta();
            // clean_lines_fully();

            
            if (line_formed[line_no] == 1){
                return;
            }
            int del_y = deltas[line_no];
            for (int j = 0;j < board[line_no].size();j++){
                board[line_no+del_y][j] = board[line_no][j];
            }
            // this->line_formed[line_no] = 0;
        }

        void reset_lines_deltas(){
            fill(this->line_formed.begin(), this->line_formed.end(), 0);
            fill(this->deltas.begin(), this->deltas.end(), 0);
            this->lines_filled = false;
        }

    
        void calculate_delta(){
            // vector<int> deltas(line_formed.size(), 0);

            int counter = 0;
            for (int i = line_formed.size() - 1; i > 0; i--){
                if (line_formed[i] == 1){
                    counter += 1;
                }
                this->deltas[i - 1] = counter;
            }
        }

        int get_total_lines(){
            int total = 0;
            for (int x : line_formed){
                total = total + x;
            }
            return total;
        }
    
    protected:
        void clean_lines_fully(){
            for(int i = 0;i<line_formed.size();i++){
                if (line_formed[i] == 0){
                    continue;
                }

                for (int j = 0;j < board[i].size();j++){
                    board[i][j] = RAYWHITE;
                };
            };
        }

};