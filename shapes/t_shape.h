#include <vector>
#include <limits>
#include "utils.h"

using namespace std;

class TShape : public Shape{

    public:
        TShape(int x, int y, int rotation) : Shape(x, y, rotation) {
            initialize_shape(x, y, rotation)

        };

        void update_shape(int rotate) override {
            

        };
    
    protected:
        void create_shape(int x, int y) override{
            Pixel block1(x, y);
            shape.push_back(block1);

            Pixel block2(x+1, y);
            shape.push_back(block2);

            Pixel block3(x+2, y);
            shape.push_back(block3);

            Pixel block4(x+1, y+1);
            shape.push_back(block4);

        };

}