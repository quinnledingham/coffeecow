#ifndef SNAKE_H
#define SNAKE_H

struct game_state
{
    int Menu;
    int ToneHz;
    int GreenOffset;
    int BlueOffset;
};

struct Circle
{
    int X;
    int Y;
    int Radius;
    float StartDegree;
    uint32 Color;
};

#define GRIDWIDTH 17
#define GRIDHEIGHT 17
#define GRIDSIZE 50

#define RIGHT 0
#define UP 1
#define LEFT 2
#define DOWN 3
#define NODIRECTION 4

struct SnakeNode
{
    real32 x;
    real32 y;
    int GridX;
    int GridY;
    int Direction;
    int NextDirection;
    
    SnakeNode *Next;
    SnakeNode *Previous;
};

struct InputNode
{
    int Direction;
    InputNode *Next;
};

struct Snake
{
    SnakeNode *Head;
    real32 TransitionAmt = 0;
    real32 Speed;
    int Direction;
    
    bool Moving = true;
    InputNode *InputHead;
    
    bool32 Initialized = false;
};

struct Apple
{
    int X;
    int Y;
    int Score;
};

#endif //SNAKE_H
