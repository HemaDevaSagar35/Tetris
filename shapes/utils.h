#pragma once

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