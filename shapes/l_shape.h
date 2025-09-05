#include <vector>

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

class LShape {
    vector <Pixel> shape;
    int rotation;

    public:
        LShape (int x, int y) {
            this->rotation = 0;
            create_shape(x, y);

        };

        vector <Pixel> get_shape() {
            return shape;
        };

        void update_position(int x, int y){
            // the updates here are affine like right or left or lowering
            int delta_x = x - shape[0].x;
            int delta_y = y - shape[0].y;

            shape[0].x = shape[0].x + delta_x;
            shape[0].y = shape[0].y + delta_y;

            shape[1].x = shape[1].x + delta_x;
            shape[1].y = shape[1].y + delta_y;

            shape[2].x = shape[2].x + delta_x;
            shape[2].y = shape[2].y + delta_y;

            shape[3].x = shape[3].x + delta_x;
            shape[3].y = shape[3].y + delta_y;
        };

        void update_shape(int rotate){
            // 0 -> 90 & 0 -> -90
            // 90 -> 180 & 90 -> 0
            // 180 -> 270 & 270 -> 180
            //270 -> 360 & 360 -> 270

            if (this->rotation == 0){
                if (rotate == 1){
                    shape[0].x = shape[0].x + 2;

                    shape[1].x = shape[1].x + 1;
                    shape[1].y = shape[1].y - 1;

                    shape[2].y = shape[2].y - 2;

                    shape[3].x = shape[3].x - 1;
                    shape[3].y = shape[3].y - 1;

                    this->rotation = 90;
                }else {
                    shape[0].y = shape[0].y + 1;

                    shape[1].x = shape[1].x + 1;

                    shape[2].x = shape[2].x + 2;
                    shape[2].y = shape[2].y - 1;

                    shape[3].x = shape[3].x + 1;
                    shape[3].y = shape[3].y - 2;

                    this->rotation = 270;

                }
            else if (this->rotation == 90){
                if (rotate == 1){
                    shape[0].y = shape[0].y + 2;

                    shape[1].x = shape[1].x + 1;
                    shape[1].y = shape[1].y + 1;

                    shape[2].x = shape[2].x + 2;

                    shape[3].x = shape[3].x + 1;
                    shape[3].y = shape[3].y - 1;

                    this->rotation = 180;
                }else {
                    shape[0].x = shape[0].x - 2;

                    shape[1].x = shape[1].x - 1;
                    shape[1].y = shape[1].y + 1;

                    shape[2].y = shape[2].y + 2;

                    shape[3].x = shape[3].x + 1;
                    shape[3].y = shape[3].y + 1;

                    this->rotation = 0;

                }
            }else if (this->rotation == 180){
                if (rotate == 1){
                    // 270
                    shape[0].x = shape[0].x - 2;
                    shape[0].y = shape[0].y - 1;

                    shape[1].x = shape[1].x - 1;

                    shape[2].y = shape[2].y + 1;

                    shape[3].x = shape[3].x + 1;

                    this->rotation = 270;
                }else{
                    // 90
                    shape[0].y = shape[0].y - 2;

                    shape[1].x = shape[1].x - 1;
                    shape[1].y = shape[1].y - 1;

                    shape[2].x = shape[2].x - 2;

                    shape[3].x = shape[3].x - 1;
                    shape[3].y = shape[3].y + 1;

                    this->rotation = 90;
                }
            } else if (this->rotation == 270){
                if (rotate == 1){
                    // 360
                    shape[0].y = shape[0].y - 1;

                    shape[1].x = shape[1].x - 1;

                    shape[2].x = shape[2].x - 2;
                    shape[2].y = shape[2].y + 1;

                    shape[3].x = shape[3].x - 1;
                    shape[3].y = shape[3].y + 2;
                    
                    this->rotation = 0;
                }else{
                    // 180
                    shape[0].x = shape[0].x + 2;
                    shape[0].y = shape[0].y + 1;

                    shape[1].x = shape[1].x + 1;

                    shape[2].y = shape[2].y - 1;

                    shape[3].x = shape[3].x - 1;

                    this->rotation = 180;

                }
            }

            
        }

    private:
        void create_shape(int x, int y){
            Pixel block1(x, y);
            shape.push_back(block1);

            Pixel block2(x, y + 1);
            shape.push_back(block2);

            Pixel block3(x, y + 2);
            shape.push_back(block3);

            Pixel block4(x + 1, y + 2);
            shape.push_back(block4);

        };



};