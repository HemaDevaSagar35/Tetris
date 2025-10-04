#include <vector>
#include <limits>
#include "utils.h"

using namespace std;

class SShape : public Shape{

    public:
        SShape(int x, int y, int rotation) : Shape(x, y, rotation) {
            initialize_shape(x, y, rotation);

        };

        void update_shape(int rotate) override {

            if (this->rotation == 0){
                // 0 -> 90
                if (rotate == 1){

                    shape[0].y = shape[0].y + 2;

                    shape[1].x = shape[1].x + 1;
                    shape[1].y = shape[1].y + 1;

                    shape[3].x = shape[3].x + 1;
                    shape[3].y = shape[3].y - 1;

                    this->rotation = 90;

                }else{
                    // 0 - 270
                    shape[0].x = shape[0].x - 2;

                    shape[1].x = shape[1].x - 1;
                    shape[1].y = shape[1].y + 1;

                    shape[3].x = shape[3].x + 1;
                    shape[3].y = shape[3].y + 1;
                    
                    

                    this->rotation = 270;
                }
            }else if (this->rotation == 90){
                // 90 -> 180
                if (rotate == 1){
                    shape[0].x = shape[0].x - 2;
                    shape[0].y = shape[0].y - 1;

                    shape[1].x = shape[1].x - 1;

                    shape[2].y = shape[2].y - 1;

                    shape[3].x = shape[3].x + 1;
                    

                    this->rotation = 180;
                }else {
                    // 90 -> 0
                    shape[0].y = shape[0].y - 2;

                    shape[1].x = shape[1].x - 1;
                    shape[1].y = shape[1].y - 1;

                    shape[3].y = shape[3].y + 1;
                    shape[3].x = shape[3].x - 1;
                    

                    this->rotation = 0;
                }
            }else if (this->rotation == 180){
                // 180 -> 270
                if (rotate == 1){
                    
                    shape[0].y = shape[0].y - 1;

                    shape[1].x = shape[1].x - 1;

                    shape[2].y = shape[2].y + 1;

                    shape[3].x = shape[3].x - 1;
                    shape[3].y = shape[3].y + 2;

                    this->rotation = 270;
                }else{
                    // 180  -> 90
                    shape[0].x = shape[0].x + 2;
                    shape[0].y = shape[0].y + 1;

                    shape[1].x = shape[1].x + 1;

                    shape[2].y = shape[2].y + 1;

                    shape[3].x = shape[3].x - 1;
                    

                    this->rotation = 90;
                    
                }
            }else if (this->rotation == 270){
                if (rotate == 1){
                    // 270 -> 0
                    shape[0].x = shape[0].x + 2;

                    shape[1].x = shape[1].x + 1;
                    shape[1].y = shape[1].y - 1;

                    shape[3].x = shape[3].x - 1;
                    shape[3].y = shape[3].y - 1;
                    

                    this->rotation = 0;
                }else{
                    // 270 -> 180
                    
                    shape[0].y = shape[0].y + 1;

                    shape[1].x = shape[1].x + 1;

                    shape[2].y = shape[2].y - 1;

                    shape[3].x = shape[3].x + 1;
                    shape[3].y = shape[3].y - 2;

                    this->rotation = 180;

                }
            }
            update_boundary();
        };
    
    protected:
        void create_shape(int x, int y) override{
            Pixel block1(x, y);
            shape.push_back(block1);

            Pixel block2(x-1, y);
            shape.push_back(block2);

            Pixel block3(x-1, y+1);
            shape.push_back(block3);

            Pixel block4(x-2, y+1);
            shape.push_back(block4);

        };

};