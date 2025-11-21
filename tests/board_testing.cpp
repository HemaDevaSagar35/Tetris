#include <gtest/gtest.h>
#include <raylib.h>
#include "../shapes/utils.h"
#include "../shapes/t_shape.h"
#include "../shapes/l_shape.h"
#include "../game/utils.h"

// g++ -std=c++17 tests/board_testing.cpp \
//   -isystem "$(brew --prefix googletest)/include" \
//   -L"$(brew --prefix googletest)/lib" -lgtest -lgtest_main -pthread \
//   $(pkg-config --cflags --libs raylib) \
//   -o btesting


// this is the test fixture
class BoardTest : public testing::Test{
    protected:
        Board b1;
        Board b2;
        TShape t1;
        LShape t2;
        BoardTest() : b1(10, 20), t1(3, 16, 180), b2(10, 20), t2(1, 16, 180) {}

        void SetUp() override {
            // Board 1
            int w = b1.get_board_width();
            int h = b1.get_board_height();

            int w2 = b2.get_board_width();
            int h2 = b2.get_board_height();

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

            // Board 2
            for(int i = 0;i<w;i++){
                if(i != 3){
                    b2[h - 1][i] = GREEN;
                    b2[h - 2][i] = GREEN;
                    b2[h - 3][i] = GREEN;
                }
            }

            b2[h - 4][1] = VIOLET;
            b2[h - 4][7] = VIOLET;
            b2[h - 4][8] = VIOLET;
            b2[h - 4][9] = VIOLET;

            b2[h - 5][1] = ORANGE;
            b2[h - 5][6] = ORANGE;
            b2[h - 5][7] = ORANGE;
            b2[h - 5][8] = ORANGE;
            b2[h - 5][9] = ORANGE;

            b2[h - 6][0] = BLUE;
            b2[h - 6][1] = BLUE;
            b2[h - 6][6] = BLUE;
            b2[h - 6][7] = BLUE;
            b2[h - 6][8] = BLUE;
            b2[h - 6][9] = BLUE;

            b2[h - 7][6] = YELLOW;
            b2[h - 7][8] = YELLOW;
            b2[h - 7][9] = YELLOW;

            b2[h - 8][6] = BEIGE;
            b2[h - 9][6] = BEIGE;
        }

};

TEST_F(BoardTest, LatchWorks){


    vector<Pixel> tetri = t1.get_shape();
    vector<Pixel> tetri2 = t2.get_shape();
    

    b1.latch_on(tetri, BLACK);
    b2.latch_on(tetri2, BLACK);

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



    // LATCHING FOR BOARD 2
    int w2 = b2.get_board_width();
    int h2 = b2.get_board_height();

    step_1 = true;
    for (int i = 0;i<w2;i++){
        if ((i != 3) && (b2[h2 - 1][i] != GREEN)){
            step_1 = false;
            break;
        }
    }
    EXPECT_EQ(step_1, true) << "board 2: line at index " << 0 << " got disturbed";

    step_1 = true;
    for (int i = 0;i<w2;i++){
        if ((i != 3) && (b2[h2 - 2][i] != GREEN) && (b2[h2 - 3][i] != GREEN)){
            step_1 = false;
            break;
        }
    }


    if(b2[h2 - 2][3] != BLACK){
        step_1 = false;
    }

    if(b2[h2 - 3][3] != BLACK){
        step_1 = false;
    }

    EXPECT_EQ(step_1, true) << "board 2: line at index " << "1 & 2" << " got disturbed";

    step_1 = true;
    if(b2[h - 4][1] != VIOLET){
        step_1 = false;
    }

    if(b2[h - 4][7] != VIOLET){
        step_1 = false;
    }

    if(b2[h - 4][8] != VIOLET){
        step_1 = false;
    }

    if(b2[h - 4][9] != VIOLET){
        step_1 = false;
    }

    if(b2[h - 4][2] != BLACK){
        step_1 = false;
    }

    if(b2[h - 4][3] != BLACK){
        step_1 = false;
    }

    EXPECT_EQ(step_1, true) << "board 2: line at index " << "3" << " got disturbed";

    step_1 = true;
    if(b2[h - 5][1] != ORANGE){
        step_1 = false;
    }

    if(b2[h - 5][6] != ORANGE){
        step_1 = false;
    }

    if(b2[h - 5][7] != ORANGE){
        step_1 = false;
    }

    if(b2[h - 5][8] != ORANGE){
        step_1 = false;
    }

    if(b2[h - 5][9] != ORANGE){
        step_1 = false;
    }

    EXPECT_EQ(step_1, true) << "board 2: line at index " << "4" << " got disturbed";

    step_1 = true;

    if(b2[h - 6][0] != BLUE){
        step_1 = false;
    }

    if(b2[h - 6][1] != BLUE){
        step_1 = false;
    }

    if(b2[h - 6][6] != BLUE){
        step_1 = false;
    }

    if(b2[h - 6][7] != BLUE){
        step_1 = false;
    }

    if(b2[h - 6][8] != BLUE){
        step_1 = false;
    }

    if(b2[h - 6][9] != BLUE){
        step_1 = false;
    }

    EXPECT_EQ(step_1, true) << "board 2: line at index " << "5" << " got disturbed";

    step_1 = true;
    if(b2[h - 7][6] != YELLOW){
        step_1 = false;
    }

    if(b2[h - 7][8] != YELLOW){
        step_1 = false;
    }

    if(b2[h - 7][9] != YELLOW){
        step_1 = false;
    }

    EXPECT_EQ(step_1, true) << "board 2: line at index " << "6" << " got disturbed";

    step_1 = true;
    if(b2[h - 8][6] != BEIGE){
        step_1 = false;
    }

    EXPECT_EQ(step_1, true) << "board 2: line at index " << "7" << " got disturbed";

    step_1 = true;
    if(b2[h - 9][6] != BEIGE){
        step_1 = false;
    }

    EXPECT_EQ(step_1, true) << "board 2: line at index " << "8" << " got disturbed";

    bool line_filled_2 = b2.is_lines_formed();
    EXPECT_EQ(line_filled_2, true) << "board 2: line formation is not recognized";


    vector<int> lines_formed_expected_2(20, 0);
    lines_formed_expected_2[18] = 1;
    lines_formed_expected_2[17] = 1;
    // lines_formed_expected[]

    vector<int> lines_formed_2 = b2.get_line_indexes();
    EXPECT_EQ(lines_formed_2, lines_formed_expected_2) << "board 2: lines counter is not matching";

};


TEST_F(BoardTest, ClearLinesWorks){
    // TEST - 1
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

    // TEST - 2

    vector<Pixel> tetri2 = t2.get_shape();
    b2.latch_on(tetri2, BLACK);

    b2.clear_lines();

    bool line_filled_2 = b2.is_lines_formed();
    EXPECT_EQ(line_filled_2, false) << "line formation is mis-recognized as true";

    vector<int> lines_formed_expected_2(20, 0);
    vector<int> lines_formed_2 = b2.get_line_indexes();
    EXPECT_EQ(lines_formed_2, lines_formed_expected_2) << "lines counter is not matching";

    int w2 = b2.get_board_width();
    int h2 = b2.get_board_height();
    bool step_2 = true;
    for(int i = 0;i<w2;i++){
        if ((i != 3) && (b2[h2-1][i] != GREEN)){
            step_2 = false;
            break;
        }
    }

    EXPECT_EQ(step_2, true) << "line at index " << 0 << " got disturbed";

    step_2 = true;
    if(b2[h2-2][1] != VIOLET){
        step_2 = false;
    }

    if(b2[h2 - 2][7] != VIOLET){
        step_2 = false;
    }

    if(b2[h2 - 2][8] != VIOLET){
        step_2 = false;
    }

    if(b2[h2 - 2][9] != VIOLET){
        step_2 = false;
    }

    if(b2[h2 - 2][2] != BLACK){
        step_2 = false;
    }

    if(b2[h2 - 2][3] != BLACK){
        step_2 = false;
    }

    EXPECT_EQ(step_2, true) << "line at index " << 1 << " got disturbed";

    step_2 = true;

    if(b2[h2 - 3][1] != ORANGE){
        step_2 = false;
    }

    if(b2[h2 - 3][6] != ORANGE){
        step_2 = false;
    }

    if(b2[h2 - 3][7] != ORANGE){
        step_2 = false;
    }

    if(b2[h2 - 3][8] != ORANGE){
        step_2 = false;
    }

    if(b2[h2 - 3][9] != ORANGE){
        step_2 = false;
    }

    EXPECT_EQ(step_2, true) << "line at index " << 2 << " got disturbed";

    step_2 = true;
    if(b2[h2 - 4][0] != BLUE){
        step_2 = false;
    }

    if(b2[h2 - 4][1] != BLUE){
        step_2 = false;
    }

    if(b2[h2 - 4][6] != BLUE){
        step_2 = false;
    }

    if(b2[h2 - 4][7] != BLUE){
        step_2 = false;
    }

    if(b2[h2 - 4][8] != BLUE){
        step_2 = false;
    }

    if(b2[h2 - 4][9] != BLUE){
        step_2 = false;
    }

    EXPECT_EQ(step_2, true) << "line at index " << 3 << " got disturbed";

    step_2 = true;
    if(b2[h2 - 5][6] != YELLOW){
        step_2 = false;
    }

    if(b2[h2 - 5][8] != YELLOW){
        step_2 = false;
    }

    if(b2[h2 - 5][9] != YELLOW){
        step_2 = false;
    }

    EXPECT_EQ(step_2, true) << "line at index " << 4 << " got disturbed";

    step_2 = true;
    if(b2[h2 - 6][6] != BEIGE){
        step_2 = false;
    }

    EXPECT_EQ(step_2, true) << "line at index " << 5 << " got disturbed";

    step_2 = true;
    if(b2[h2 - 7][6] != BEIGE){
        step_2 = false;
    }

    EXPECT_EQ(step_2, true) << "line at index " << 6 << " got disturbed";


};



// This is to test 
class GameTest : public testing::Test{
    
    protected:
        Board b1;
        LShape t1;

        GameTest() : b1(10, 20), t1(7, 16, 180) {}
        void SetUp() override {
            // first line
            int h = b1.get_board_height();
            int w = b1.get_board_width();

            for(int i = 0;i<w;i++){
                if ((i != 3) && (i != 4)){
                    b1[h - 1][i] = VIOLET;
                }
            }

            // second line
            for(int i = 0;i<w;i++){
                if (i != w - 1){
                    b1[h - 2][i] = ORANGE;
                }
            }

            // third line
            b1[h - 3][3] = YELLOW;

            // fourth line
            b1[h - 4][3] = YELLOW;

            // five line
            b1[h - 4][3] = YELLOW;


        }

};

TEST_F(GameTest, OnboardWorks){

    bool output = onboard(t1, b1, 1);
    EXPECT_EQ(output, true) << "It should be onboarded";
};

TEST_F(GameTest, ValidMoveWorks){

    bool output = validmove(t1, b1, 1, 0, 0);
    EXPECT_EQ(output, true) << "It is a valid move, why is it saying otherwise?";

}

