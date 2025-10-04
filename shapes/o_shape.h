#include <vector>
#include <limits>
#include "utils.h"

using namespace std;

class OShape : public Shape{

    public:
        OShape(int x, int y, int rotation) : Shape(x, y, rotation) {
            initialize_shape(x, y, rotation);

        };

        void update_shape(int rotate) override {
            update_boundary();
        };
    
    protected:
        void create_shape(int x, int y) override{
            Pixel block1(x, y);
            shape.push_back(block1);

            Pixel block2(x+1, y);
            shape.push_back(block2);

            Pixel block3(x, y+1);
            shape.push_back(block3);

            Pixel block4(x+1, y+1);
            shape.push_back(block4);

        };
        void initialize_shape(int x, int y, int rotation) override{
            create_shape(x, y);
            this->rotation = rotation;
            update_boundary();
        }

};