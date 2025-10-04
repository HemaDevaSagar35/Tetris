#pragma once

#include <limits>

using namespace std;

class Pixel{
    public:
        int x;
        int y;
        Pixel(int x, int y){
            this->x = x;
            this->y = y;
        };
};

class Boundary{
    public:
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