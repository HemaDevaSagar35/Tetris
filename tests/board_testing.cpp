#include <gtest/gtest.h>
#include <raylib.h>
#include "../shapes/utils.h"
#include "../shapes/t_shape.h"

// g++ -std=c++17 tests/board_testing.cpp \
//   -isystem "$(brew --prefix googletest)/include" \
//   -L"$(brew --prefix googletest)/lib" -lgtest -lgtest_main -pthread \
//   $(pkg-config --cflags --libs raylib) \
//   -o btesting


// this is the test fixture
class BoardTest : public testing::Test{
    protected:
        Board b1;
        TShape t1;
        BoardTest() : b1(10, 20), t1(3, 16, 180) {}

        void SetUp() override {
            int w = b1.get_board_width();
            int h = b1.get_board_height();
            for (int i = 0;i<w;i++){
                if (i != 4){
                    b1[h - 1][i] = GREEN;
                }
            }

            for (int i = 0;i<w;i++){
                if (i != 1){
                    b1[h - 2][i] = VIOLET;
                }
            }

            for(int i = 0;i<w;i++){
                if ((i <= 2) || ( i >= 6)){
                    b1[h - 3][i] = ORANGE;
                }

            }
        }

};

TEST_F(BoardTest, LatchWorks){


    vector<Pixel> tetri = t1.get_shape();
    b1.latch_on(tetri, BLACK);
    // cout << tetri[0].x << "  " << tetri[0].y << "\n";
    // cout << tetri[1].x << "  " <<  tetri[1].y << "\n";
    // cout << tetri[2].x << "  " <<  tetri[2].y << "\n";
    // cout << tetri[3].x << "  " <<  tetri[3].y << "\n";

    //make sure latching worked properly
    // Type one
    int w = b1.get_board_width();
    int h = b1.get_board_height();
    bool step_1 = true;
    for (int i = 0;i<w;i++){
        if ((i != 4) && (b1[h - 1][i] != GREEN)){
            step_1 = false;
            break;
        }
    }

    EXPECT_EQ(step_1, true) << "line at index " << 0 << " got disturbed";

    step_1 = true;
    for (int i = 0;i<w;i++){
        if ((i != 1) && (b1[h - 2][i] != VIOLET)){
            step_1 = false;
            break;
        }
    }

    EXPECT_EQ(step_1, true) << "line at index " << 1 << " got disturbed";

    step_1 = true;
    int latched = 1;
    for (int i = 0;i<w;i++){
        if (((i <= 2) || (i >= 6)) && (b1[h - 3][i] != ORANGE)){
            step_1 = false;
            latched -= 1;
            break;
        }else if ((i > 2) && (i < 6) && (b1[h - 3][i] != BLACK)){
            step_1 = false;
            break;
        }
    }

    EXPECT_EQ(step_1, true) << "line at index " << 2 << " got disturbed as latch " << latched;

    step_1 = true;
    for (int i = 0;i<w;i++){
        if (((i == 4) &&  (b1[h - 4][4] != BLACK)) || ((i != 4) && (b1[h-4][i] != RAYWHITE))){
            step_1 = false;
            break;
        }
        
    }

    EXPECT_EQ(step_1, true) << "latched wrong at index " <<3;

    bool line_filled = b1.is_lines_formed();
    EXPECT_EQ(line_filled, true) << "line formation is not recognized";

    vector<int> lines_formed_expected(20, 0);
    lines_formed_expected[17] = 1;
    // lines_formed_expected[]

    vector<int> lines_formed = b1.get_line_indexes();
    EXPECT_EQ(lines_formed, lines_formed_expected) << "lines counter is not matching";


}

TEST_F(BoardTest, ClearLinesWorks){
    vector<Pixel> tetri = t1.get_shape();
    b1.latch_on(tetri, BLACK);

    b1.clear_lines();

    bool line_filled = b1.is_lines_formed();
    EXPECT_EQ(line_filled, false) << "line formation is mis-recognized as true";

    vector<int> lines_formed_expected(20, 0);
    vector<int> lines_formed = b1.get_line_indexes();
    EXPECT_EQ(lines_formed, lines_formed_expected) << "lines counter is not matching";


    int w = b1.get_board_width();
    int h = b1.get_board_height();
    bool step_1 = true;
    for (int i = 0;i<w;i++){
        if ((i != 4) && (b1[h - 1][i] != GREEN)){
            step_1 = false;
            break;
        }
    }

    EXPECT_EQ(step_1, true) << "line at index " << 0 << " got disturbed";

    step_1 = true;
    for (int i = 0;i<w;i++){
        if ((i != 1) && (b1[h - 2][i] != VIOLET)){
            step_1 = false;
            break;
        }
    }

    EXPECT_EQ(step_1, true) << "line at index " << 1 << " got disturbed";

    step_1 = true;
    for (int i = 0;i<w;i++){
        if (((i == 4) &&  (b1[h - 3][4] != BLACK)) || ((i != 4) && (b1[h-3][i] != RAYWHITE))){
            step_1 = false;
            break;
        }
        
    }

    EXPECT_EQ(step_1, true) << "line at index " << 2 << " got disturbed";







}