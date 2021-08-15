#ifndef INPUT_H
#define INPUT_H

typedef unsigned short uint16; 

const uint16 KEY_NULL  = 0b000;
const uint16 KEY_PRESS = 0b001;
const uint16 KEY_DOWN  = 0b010;
const uint16 KEY_UP    = 0b100;

const bool WASD_MODE   = false;
const bool ARROWS_MODE = true;

struct Input
{
    uint16 z, x;
    uint16 down, left, right;
    uint16 space;
    uint16 shift;
    uint16 escape;
    uint16 f4;
    union 
    {
        struct
        {
           uint16 zero; 
           uint16 one; 
           uint16 two; 
           uint16 three; 
           uint16 four; 
           uint16 five; 
           uint16 six; 
           uint16 seven; 
           uint16 eight; 
           uint16 nine; 
        };
        uint16 numbers[10];
    };
};

bool KeyPress(uint16 key)
{
    return (key & KEY_PRESS) == KEY_PRESS;
}

bool KeyDown(uint16 key)
{
    return (key & KEY_DOWN) == KEY_DOWN;
}

bool KeyUp(uint16 key)
{
    return (key & KEY_UP) == KEY_UP;
}


#endif
