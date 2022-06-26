#define QLIB_WINDOW_APPLICATION
#include "qlib/application.h"

#include "qlib/random.h"
#include "qlib/socketq.h"
#include "qlib/text.h"
#include "qlib/menu.h"

#include "coffee_cow.h"
#include "snake.h"


inline void
DrawRock(game_assets *Assets, v2 GridCoords, v2 RealGridDim, real32 Z)
{
    v2 RockCoords = GridCoords - (RealGridDim/6);
    v2 RockDim = v2(RealGridDim.x * 4/3 - 1, RealGridDim.y * 4/3 - 1);
    Push(RenderGroup, v3(RockCoords, Z), RockDim, GetTexture(Assets, GAI_Rocks), 0, BlendMode::gl_src_alpha);
}

inline void
DrawBackground(Texture *Tex, v2 TopLeftCornerCoords, v2 PlatformDim, real32 Z)
{
    v2 BackgroundCoords = TopLeftCornerCoords - 5;
    v2 BackgroundDim = 0;
    if (PlatformDim.x >= PlatformDim.y)
        BackgroundDim = v2(PlatformDim.x + 5, PlatformDim.x + 5);
    else
        BackgroundDim = v2(PlatformDim.y + 5, PlatformDim.y + 5);
    
    BackgroundCoords.y -= ((BackgroundDim.y - (PlatformDim.y + 5)) / 2);
    
    Push(RenderGroup, v3(BackgroundCoords, 0.0f), BackgroundDim, Tex, 0, BlendMode::gl_src_alpha);
}

inline void
DrawBackground(game_assets *Assets, v2 TopLeftCornerCoords, v2 PlatformDim, real32 Z)
{
    v2 BackgroundCoords = TopLeftCornerCoords - 5;
    v2 BackgroundDim = PlatformDim + 5;
    Push(RenderGroup, v3(BackgroundCoords, Z), BackgroundDim, GetTexture(Assets, GAI_Background), 0, BlendMode::gl_src_alpha);
}

inline void
DrawGrid(game_assets *Assets, v2 GridCoords, v2 GridDim, real32 GridSize, real32 Z)
{
    for (int i = 0; i < (int)GridDim.x; i++) {
        for (int j = 0; j < (int)GridDim.y; j++) {
            v3 Coords = v3((GridCoords.x + (i * GridSize)), (GridCoords.y + (j * GridSize)), Z);
            Push(RenderGroup, Coords, v2(GridSize), GetTexture(Assets, GAI_Grid), 0, BlendMode::gl_src_alpha);
        }
    }
}

inline void
DrawScore(game_assets *Assets, int Score, v2 TopLeftCornerCoords)
{
    strinq ScoreStrinq = S() + "Score: " + Score;
    
    font_string FontString = {};
    FontStringInit(&FontString, GetFont(Assets, GAFI_Rubik), ScoreStrinq.Data, 50, 0xFFFFFFFF);
    FontStringPrint(&FontString, TopLeftCornerCoords + 10);
    
    v2 ScoreSize = FontStringGetDim(&FontString);
    Push(RenderGroup, v3(TopLeftCornerCoords + 5, 50.0f), ScoreSize + 10, 0x96000000, 0.0f);
}

internal void
DrawCoffee(game_assets *Assets, Coffee *Cof, real32 Seconds, v2 GridCoords, real32 GridSize, real32 Z)
{
    Cof->Rotation += (Seconds * 100);
    if (Cof->Rotation > 360) Cof->Rotation -= 360;
    
    if (Cof->IncreasingHeight) {
        Cof->Height += (Seconds * 2);
        if (Cof->Height > (GridSize * 0.05f)) Cof->IncreasingHeight = false;
    }
    else if (!Cof->IncreasingHeight) {
        Cof->Height -= (Seconds * 2);
        if (Cof->Height < (-GridSize * 0.05f)) Cof->IncreasingHeight = true;
    }
    
    v3 Coords = v3(GridCoords.x + (Cof->Coords.x * GridSize) - (Cof->Height/2),
                   GridCoords.y + (Cof->Coords.y * GridSize) - (Cof->Height/2),
                   Z);
    Push(RenderGroup, Coords, v2(GridSize + Cof->Height), GetTexture(Assets, GAI_CoffeeTex), Cof->Rotation, BlendMode::gl_src_alpha);
}

internal void
DrawCoffeeCowNodes(game_assets *Assets, CoffeeCow *Cow, real32 GridX, real32 GridY, int GridSize)
{
    for (int i = 0; i < Cow->Nodes.Size; i++) {
        CoffeeCowNode *Node = (CoffeeCowNode*)Cow->Nodes[i];
        v3 Coords = v3(roundf(GridX + ((Node->Coords.x) * GridSize)), roundf(GridY + ((Node->Coords.y) * GridSize)), 0.5f);
        Push(RenderGroup, Coords, v2(GridSize, GridSize), GetTexture(Assets, GAI_Straight), 0.0f, BlendMode::gl_one);
    }
}

inline v2
GetSize(v3 Left, v3 Right, real32 GridSize)
{
    v2 Size;
    Size.x = (Left.x - Right.x) * -1;
    Size.y = (Left.y - Right.y) * -1;
    if (Size.x == 0) Size.x = GridSize;
    if (Size.y == 0) Size.y = GridSize;
    
    return Size;
}

// with down being the default direction
internal real32
GetRotation(int Direction)
{
    if (Direction == LEFT) return 90.0f;
    else if (Direction == RIGHT) return 270.0f;
    else if (Direction == UP) return 180.0f;
    else if (Direction == DOWN) return 0.0f;
    else return 0.0f;
}

internal real32
GetAngleDiff(int StartDirection, int EndDirection)
{
    real32 Goal;
    real32 BRotation = GetRotation(StartDirection);
    real32 ERotation = GetRotation(EndDirection);
    if (StartDirection == DOWN && EndDirection == RIGHT)
        Goal = -90;
    else if (StartDirection == RIGHT && EndDirection == DOWN)
        Goal = 90;
    else
        Goal = ERotation - BRotation;
    
    return Goal;
}

internal void
DrawCoffeeCow(game_assets *Assets, CoffeeCow *Cow, v2 CoffeeCoords, real32 Seconds, real32 GridX, real32 GridY, real32 GridSize)
{
    // Put the head, tail, and corners into a list
    Arr Corners = {};
    Corners.Init(Cow->Nodes.Size + 2, sizeof(v3));
    
    for (int i = 0; i < Cow->Nodes.Size; i++)
    {
        CoffeeCowNode *Node = (CoffeeCowNode*)Cow->Nodes[i];
        real32 OutlineZ = 0.0f;
        
        v3 Coords = v3(Node->Coords, 0.0f);
        real32 TransitionAmt = Cow->TransitionAmt;
        if (Node->CurrentDirection == RIGHT) {
            Coords.x = roundf(GridX + ((Coords.x + TransitionAmt)* GridSize));
            Coords.y = roundf(GridY + (Coords.y * GridSize));
        }
        else if (Node->CurrentDirection == UP) {
            Coords.x = roundf(GridX + (Coords.x * GridSize));
            Coords.y = roundf(GridY + ((Coords.y - TransitionAmt) * GridSize));
        }
        else if (Node->CurrentDirection == LEFT) {
            Coords.x = roundf(GridX + ((Coords.x - TransitionAmt) * GridSize));
            Coords.y = roundf(GridY + (Coords.y * GridSize));
        }
        else if (Node->CurrentDirection == DOWN) {
            Coords.x = roundf(GridX + (Coords.x * GridSize));
            Coords.y = roundf(GridY + ((Coords.y + TransitionAmt) * GridSize));
        }
        
        if (i == 0)
        {
            real32 HeadResize = GridSize * 0.05f;
            v3 Temp = Coords;
            Temp.x -= (HeadResize/2);
            Temp.y -= (HeadResize/2);
            Temp.z = 2.0f;
            Push(RenderGroup, Temp, v2(GridSize + HeadResize), GetTexture(Assets, GAI_Head), GetRotation(Node->CurrentDirection), BlendMode::gl_one);
            Corners.Push(&Coords);
            
            CoffeeCowNode *Head = (CoffeeCowNode*)Cow->Nodes[i];
            v2 NextCoords = Node->Coords;
            if (Node->CurrentDirection == RIGHT) NextCoords.x += 1;
            else if (Node->CurrentDirection == UP) NextCoords.y -= 1;
            else if (Node->CurrentDirection == LEFT) NextCoords.x -= 1;
            else if (Node->CurrentDirection == DOWN) NextCoords.y += 1;
            
            v2 NextNextCoords = NextCoords;
            if (Node->CurrentDirection == RIGHT) NextNextCoords.x += 1;
            else if (Node->CurrentDirection == UP) NextNextCoords.y -= 1;
            else if (Node->CurrentDirection == LEFT) NextNextCoords.x -= 1;
            else if (Node->CurrentDirection == DOWN) NextNextCoords.y += 1;
            
            real32 MouthOffset = GridSize * 0.30f;
            if ((NextCoords.x == CoffeeCoords.x && NextCoords.y == CoffeeCoords.y) ||
                (NextNextCoords.x == CoffeeCoords.x && NextNextCoords.y == CoffeeCoords.y)) {
                if (Cow->MouthOpening < MouthOffset)
                    Cow->MouthOpening += (MouthOffset * Seconds * Cow->Speed);
            }
            else {
                if (Cow->MouthOpening > 0) {
                    Cow->MouthOpening -= (MouthOffset * Seconds * Cow->Speed);
                }
            }
            
            v3 MouthCoords = Temp;
            if (Node->CurrentDirection == RIGHT) MouthCoords.x += Cow->MouthOpening;
            else if (Node->CurrentDirection == UP) MouthCoords.y -= Cow->MouthOpening;
            else if (Node->CurrentDirection == LEFT) MouthCoords.x -= Cow->MouthOpening;
            else if (Node->CurrentDirection == DOWN) MouthCoords.y += Cow->MouthOpening;
            
            MouthCoords.z = 0.0f;
            Push(RenderGroup, MouthCoords, v2(GridSize + HeadResize), GetTexture(Assets, GAI_Tongue), GetRotation(Node->CurrentDirection), BlendMode::gl_one);
            
            Temp.z = OutlineZ;
            Coords.z = OutlineZ;
            Push(RenderGroup, Temp, v2(GridSize + HeadResize), GetTexture(Assets, GAI_HeadOutline), GetRotation(Node->CurrentDirection), BlendMode::gl_one);
        }
        if (i == Cow->Nodes.Size - 1) {
            Coords.z = 0.5f;
            Push(RenderGroup, Coords, v2(GridSize, GridSize), GetTexture(Assets, GAI_Corner), 0.0f, BlendMode::gl_one);
            Corners.Push(&Coords);
            
            v3 TailCoords = v3();
            v3 OldDirTailCoords = Coords;
            v3 NewDirTailCoords = Coords;
            real32 TailOffset = GridSize * 0.90f;
            
            if (Node->CurrentDirection == RIGHT) NewDirTailCoords.x -= TailOffset;
            else if (Node->CurrentDirection == UP) NewDirTailCoords.y += TailOffset;
            else if (Node->CurrentDirection == LEFT) NewDirTailCoords.x += TailOffset;
            else if (Node->CurrentDirection == DOWN) NewDirTailCoords.y -= TailOffset;
            
            if (Cow->Tail.OldDir == RIGHT) OldDirTailCoords.x -= TailOffset;
            else if (Cow->Tail.OldDir == UP) OldDirTailCoords.y += TailOffset;
            else if (Cow->Tail.OldDir == LEFT) OldDirTailCoords.x += TailOffset;
            else if (Cow->Tail.OldDir == DOWN) OldDirTailCoords.y -= TailOffset;
            
            
            real32 TransitionAmt = Cow->TransitionAmt;
            
            real32 Rotation = 0;
            if (Cow->Tail.ChangingDirections) {
                real32 Goal = GetAngleDiff(Cow->Tail.OldDir, Cow->Tail.NewDir);
                real32 Transition = Cow->TransitionAmt;
                v3 DiffCoords = NewDirTailCoords - OldDirTailCoords;
                DiffCoords = DiffCoords *  Transition;
                TailCoords = OldDirTailCoords + DiffCoords;
                Rotation = (Transition * Goal) + GetRotation(Cow->Tail.OldDir);
            }
            else {
                TailCoords = NewDirTailCoords;
                Rotation = GetRotation(Node->CurrentDirection);
            }
            TailCoords.z = 0.0f;
            Push(RenderGroup, TailCoords, v2(GridSize, GridSize), GetTexture(Assets, GAI_Tail), Rotation, BlendMode::gl_one);
            
            Coords.z = OutlineZ;
            Push(RenderGroup, Coords, v2(GridSize, GridSize), GetTexture(Assets, GAI_CornerOutline), 0.0f, BlendMode::gl_one);
        }
        else if (i != Cow->Nodes.Size - 1) {
            CoffeeCowNode *NextNode = (CoffeeCowNode*)Cow->Nodes[i + 1];
            if (NextNode->CurrentDirection != Node->CurrentDirection) {
                v3 Temp = v3(roundf(GridX + ((Node->Coords.x) * GridSize)),
                             roundf(GridY + ((Node->Coords.y) * GridSize)),
                             0.5f);
                Push(RenderGroup, Temp, v2(GridSize, GridSize), GetTexture(Assets, GAI_Corner), 0.0f, BlendMode::gl_one);
                Corners.Push(&Temp);
                
                Temp.z = OutlineZ;
                Push(RenderGroup, Temp, v2(GridSize, GridSize), GetTexture(Assets, GAI_CornerOutline), 0.0f, BlendMode::gl_one);
            }
        }
        
        if (Node->Streak) {
            Coords.z = 3.0f;
            real32 Rotation = 0.0f;
            
            CoffeeCowNode *NextNode = (CoffeeCowNode*)Cow->Nodes[i + 1];
            CoffeeCowNode *PreviousNode = (CoffeeCowNode*)Cow->Nodes[i - 1];
            
            bool32 MovingIntoCorner = (PreviousNode->CurrentDirection != Node->CurrentDirection);
            bool32 MovingOutOfCorner = (NextNode->CurrentDirection != Node->CurrentDirection);
            
            int Dir = Node->CurrentDirection;
            int PDir = PreviousNode->CurrentDirection;
            int NDir = NextNode->CurrentDirection;
            
            if (MovingIntoCorner && !MovingOutOfCorner) {
                real32 Goal = GetAngleDiff(Dir, PDir);
                real32 Transition = Cow->TransitionAmt - 0.5f;
                if (Transition < 0)
                    Transition = 0;
                else
                    Transition *= 2;
                Rotation = (Transition * (Goal/2)) + GetRotation(Dir);
            }
            else if (MovingOutOfCorner && !MovingIntoCorner) {
                real32 Goal = GetAngleDiff(NDir, Dir);
                real32 Transition = Cow->TransitionAmt;
                if (Transition > 0.5f) {
                    Transition = 0;
                    Rotation = GetRotation(Dir);
                }
                else {
                    Transition *= 2;
                    Rotation = (Transition * (Goal/2)) + GetRotation(NDir) + (Goal/2);
                }
            }
            else if (MovingIntoCorner && MovingOutOfCorner) {
                real32 Goal = 0;
                real32 Transition = Cow->TransitionAmt;
                if (Transition > 0.5f) {
                    Transition = Cow->TransitionAmt - 0.5f;
                    Transition *= 2;
                    Goal = GetAngleDiff(Dir, PDir);
                    Rotation = (Transition * (Goal/2)) + GetRotation(Dir);
                }
                else {
                    Transition *= 2;
                    Goal = GetAngleDiff(NDir, Dir);
                    Rotation = (Transition * (Goal/2)) + GetRotation(NDir) + (Goal/2);
                }
            }
            else
                Rotation = GetRotation(Node->CurrentDirection);
            
            real32 OutlineFactor = 0.96f;
            real32 Outline = (real32)(GridSize - (GridSize * OutlineFactor));
            
            Coords.x += Outline;
            Coords.y += Outline;
            Coords.z = 3.0f;
            
            Push(RenderGroup, Coords, v2(GridSize - (Outline*2), GridSize - (Outline*2)), Assets->Spots[Node->Streak-1], Rotation, BlendMode::gl_one);
        }
    }
    
    // Draw the connect rects
    for (int i = 0; i < (Corners.Size - 1); i++)
    {
        v3 *Node = (v3*)Corners[i];
        v3 *NextNode = (v3*)Corners[i + 1];
        v3 *Left;
        v3 *Right;
        if (Node->x < NextNode->x || Node->y < NextNode->y) {
            Left = Node;
            Right = NextNode;
        }
        else {
            Left = NextNode;
            Right = Node;
        }
        
        real32 OutlineFactor = 0.96f;
        real32 Outline = (real32)(GridSize - (GridSize * OutlineFactor));
        
        v3 Coords = v3(Left->x, Left->y, 0.5f);
        if (Left->y == Right->y) Coords = Coords + v3(GridSize/2, Outline, 0);
        if (Left->x == Right->x) Coords = Coords + v3(Outline, GridSize/2, 0);
        
        if (Left->y != Right->y || Left->x != Right->x) {
            Push(RenderGroup, Coords, GetSize(*Left, *Right, GridSize - (Outline*2)), 0xFFFFFFFF, 0.0f);
            
            if (Left->y == Right->y) Coords.y -= Outline;
            if (Left->x == Right->x) Coords.x -= Outline;
            Coords.z = 0.0f;
            
            Push(RenderGroup, Coords, GetSize(*Left, *Right, (real32)GridSize), GetTexture(Assets, GAI_StraightOutline), 0.0f, BlendMode::gl_one);
        }
    }
    
    Corners.Del();
}

internal void
AddInput(CoffeeCow *Cow, int NewDirection)
{
    Cow->Speed = 8; // m/s
    if (Cow->Inputs.Size >= 3)
        return;
    
    int OldDirection = 0;
    if (Cow->Inputs.Size == 0) {
        OldDirection = Cow->Direction;
    }
    else 
        OldDirection = *(int*)Cow->Inputs[Cow->Inputs.Size - 1];
    
    if (!(OldDirection == NewDirection) && // Same Direction
        !(OldDirection + 2 == NewDirection) && // Backwards Direction
        !(OldDirection - 2 == NewDirection)) {
        Cow->Inputs.Push((void*)&NewDirection);
        //Cow->TransitionAmt = 1;
    }
}

internal void
MoveInDirection(v2 *Coords, int Direction)
{
    if (Direction == RIGHT)
        Coords->x += 1;
    else if (Direction == UP)
        Coords->y -= 1;
    else if (Direction == LEFT)
        Coords->x -= 1;
    else if (Direction == DOWN)
        Coords->y += 1;
}

internal bool32
MoveCoffeeCow(CoffeeCow *Cow, real32 SecondsElapsed, v2 GridDim)
{
    real32 Dm = (Cow->Speed * SecondsElapsed);
    real32 TransitionAmt = Cow->TransitionAmt + Dm;
    CoffeeCowNode *Head = (CoffeeCowNode*)Cow->Nodes[0];
    
    // Check collision with wall
    {
        v2 Coords = Head->Coords;
        int Direction = Cow->Direction;
        if ((Direction == LEFT && Coords.x <= 0) ||
            (Direction == RIGHT && Coords.x >= GridDim.x - 1) ||
            (Direction == UP && Coords.y <= 0) ||
            (Direction == DOWN && Coords.y >= GridDim.y - 1))
            return false;
    }
    
    // Check collision with self
    {
        v2 HeadCoords = v2(Head->Coords);
        MoveInDirection(&HeadCoords, Head->CurrentDirection);
        for (int i = 1; i < Cow->Nodes.Size; i++) {
            CoffeeCowNode* Node = (CoffeeCowNode*)Cow->Nodes[i];
            if (HeadCoords.x == Node->Coords.x &&
                HeadCoords.y == Node->Coords.y)
                return false;
        }
    }
    
    if (TransitionAmt > 1) {
        for (int i = 0; i < Cow->Nodes.Size; i++)
        {
            CoffeeCowNode* Node = (CoffeeCowNode*)Cow->Nodes[i];
            MoveInDirection(&Node->Coords, Node->CurrentDirection);
            
            if (i == 0 && Cow->Inputs.Size != 0) {
                Cow->Direction = *(int*)Cow->Inputs[0];
                Node->CurrentDirection = Cow->Direction;
                Cow->Inputs.PopFront();
            }
            else if (i != 0) {
                if (i == Cow->Nodes.Size-1 && Node->CurrentDirection != Node->NextDirection) {
                    Cow->Tail.ChangingDirections = true;
                    Cow->Tail.OldDir = Node->CurrentDirection;
                    Cow->Tail.NewDir = Node->NextDirection;
                }
                else
                    Cow->Tail.ChangingDirections = false;
                
                Node->CurrentDirection = Node->NextDirection;
                CoffeeCowNode* PreviousNode = (CoffeeCowNode*)Cow->Nodes[i-1];
                Node->NextDirection = PreviousNode->CurrentDirection;
            }
        }
        
        Dm = TransitionAmt - 1;
        Cow->TransitionAmt = Dm;
    } 
    else
        Cow->TransitionAmt += Dm;
    
    return true;
}

internal void
AddCoffeeCowNode(CoffeeCow *Cow, int X, int Y, int CDirection, int NDirection)
{
    CoffeeCowNode NewNode = {};
    NewNode.Coords = v2(X, Y);
    NewNode.CurrentDirection = CDirection;
    NewNode.NextDirection = NDirection;
    NewNode.Streak = false;
    
    if (Cow->Nodes.Size > 1) {
        CoffeeCowNode *LastTail = (CoffeeCowNode*)Cow->Nodes[Cow->Nodes.Size - 1];
        int Rand = Random(1, 5);
        if (Rand < 4)
            LastTail->Streak = Rand;
        else
            LastTail->Streak = 0;
    }
    
    Cow->Nodes.Push((void*)&NewNode);
}

internal void
InitializeCow(CoffeeCow *Cow, v2 GridDim)
{
    Cow->Nodes.Clear();
    Cow->TransitionAmt = 0;
    Cow->Score = 0;
    Cow->Inputs.Clear();
    
    Cow->Nodes.Init((int)(GridDim.x * GridDim.y), sizeof(CoffeeCowNode));
    int Startx = Random(1, (int)GridDim.x - 1);
    int Starty = Random(1, (int)GridDim.y - 1);
    int incx = 0;
    int incy = 0;
    
    int Direction = 0;
    int HalfWidth = (int)(GridDim.x/2);
    int HalfHeight = (int)(GridDim.y/2);
    if (Startx < HalfWidth) Direction = RIGHT;
    else if (Startx >= HalfWidth) Direction = LEFT;
    
    if (Direction == RIGHT) incx = -1;
    else if (Direction == LEFT) incx = 1;
    
    if (Startx < 4) Startx = 4;
    else if (Startx > (int)GridDim.x - 4) Startx = (int)GridDim.y - 4;
    
    for (int i = 0; i < 4; i++) {
        AddCoffeeCowNode(Cow, Startx, Starty, Direction, Direction);
        Startx += incx;
    }
    
    Cow->Inputs.Init(3, sizeof(int));
    Cow->Speed = 8; // m/s
    Cow->Direction = Direction;
}

internal void
MoveCoffee(CoffeeCow *Cow, Coffee *Cof, v2 GridDim)
{
    bool32 ValidLocation = false;
    while (!ValidLocation) {
        Cof->Coords = v2(Random(0, (int)GridDim.x - 1), Random(0, (int)GridDim.y - 1));
        ValidLocation = true;
        Cof->IncreasingHeight = true;
        Cof->Height = 0;
        for (int i = 0; i < Cow->Nodes.Size; i++) {
            CoffeeCowNode* Node = (CoffeeCowNode*)Cow->Nodes[i];
            if (Node->Coords.x == Cof->Coords.x && Node->Coords.y == Cof->Coords.y)
                ValidLocation = false;
        }
    }
}
inline void InitializeCoffee(CoffeeCow *Cow, Coffee *Cof, v2 GridDim)
{
    *Cof = {};
    MoveCoffee(Cow, Cof, GridDim);
}

internal void
CollectCoffee(CoffeeCow *Cow, Coffee *Cof, v2 GridDim)
{
    CoffeeCowNode* Head = (CoffeeCowNode*)Cow->Nodes[0];
    if (Head->Coords.x == Cof->Coords.x && Head->Coords.y == Cof->Coords.y) {
        Cow->Score++;
        Cof->NewLocation = true;
        
        // Make Cow Longer
        CoffeeCowNode* LastNode = (CoffeeCowNode*)Cow->Nodes[Cow->Nodes.Size-1];
        int NewX = (int)LastNode->Coords.x;
        int NewY = (int)LastNode->Coords.y;
        
        if (LastNode->CurrentDirection == UP) NewY++;
        else if (LastNode->CurrentDirection == LEFT) NewX++;
        else if (LastNode->CurrentDirection == DOWN) NewY--;
        else if (LastNode->CurrentDirection == RIGHT) NewX--;
        
        AddCoffeeCowNode(Cow, NewX, NewY, LastNode->CurrentDirection, LastNode->CurrentDirection);
        MoveCoffee(Cow, Cof, GridDim);
    }
}

struct game_asset_load
{
    game_assets *Assets;
    const char* FileName;
    int ID;
    
    Texture **Tex;
    
    font **Fonts;
    real32 FontScale;
    
    inline game_asset_load() : Assets(0),  FileName(0) {}
    inline game_asset_load(const char* _FileName) : FileName(_FileName) {}
};
internal PLATFORM_WORK_QUEUE_CALLBACK(LoadTextureAsset)
{
    game_asset_load *GAL = (game_asset_load*)Data;
    GAL->Tex[GAL->ID] = LoadTexture(GAL->FileName);
}
internal PLATFORM_WORK_QUEUE_CALLBACK(LoadFontAsset)
{
    game_asset_load *GAL = (game_asset_load*)Data;
    GAL->Fonts[GAL->ID] = LoadFont(GAL->FileName);
}

inline void InitTexture(Texture *Tex)
{
    Assert(Tex != 0);
    Tex->Init(Tex->data);
}

internal void
LoadAssets(platform_work_queue *Queue, game_assets *Assets)
{
    game_asset_load GameAssetLoad[GAI_Count];
    GameAssetLoad[GAI_Background] = game_asset_load("sand.png");
    GameAssetLoad[GAI_Grass] = game_asset_load("grass.png");
    GameAssetLoad[GAI_Grid] = game_asset_load("grid.png");
    GameAssetLoad[GAI_Rocks] = game_asset_load("rocks.png");
    GameAssetLoad[GAI_CoffeeTex] = game_asset_load("coffee.png");
    GameAssetLoad[GAI_Head] = game_asset_load("cowhead.png");
    GameAssetLoad[GAI_Straight] = game_asset_load("straight.png");
    GameAssetLoad[GAI_Corner] = game_asset_load("circle.png");
    GameAssetLoad[GAI_CoffeeStreak] = game_asset_load("coffeestreak.png");
    GameAssetLoad[GAI_HeadOutline] = game_asset_load("cowheadoutline.png");
    GameAssetLoad[GAI_StraightOutline] = game_asset_load("straightoutline.png");
    GameAssetLoad[GAI_CornerOutline] = game_asset_load("circleoutline.png");
    GameAssetLoad[GAI_Tail] = game_asset_load("tail.png");
    GameAssetLoad[GAI_Tongue] = game_asset_load("tongue.png");
    GameAssetLoad[GAI_Miz] = game_asset_load("logo.png");
    GameAssetLoad[GAI_MainMenuBack] = game_asset_load("mainmenuback.png");
    GameAssetLoad[GAI_Join] = game_asset_load("join.png");
    GameAssetLoad[GAI_JoinAlt] = game_asset_load("joinalt.png");
    GameAssetLoad[GAI_JoinAltHover] = game_asset_load("joinalthover.png");
    GameAssetLoad[GAI_JoinHover] = game_asset_load("joinhover.png");
    
    for (int i = 0; i < GAI_Count; i++) {
        GameAssetLoad[i].Assets = Assets;
        GameAssetLoad[i].ID = i;
        GameAssetLoad[i].Tex = Assets->Textures;
        //Assert(GameAssetLoad[i].FileName != 0);
        Win32AddEntry(Queue, LoadTextureAsset, &GameAssetLoad[i]);
    }
    
    game_asset_load GameAssetSpotLoad[3];
    GameAssetSpotLoad[0] = game_asset_load("spot1.png");
    GameAssetSpotLoad[1] = game_asset_load("spot2.png");
    GameAssetSpotLoad[2] = game_asset_load("spot3.png");
    for (int i = 0; i < 3; i++) {
        GameAssetSpotLoad[i].Assets = Assets;
        GameAssetSpotLoad[i].ID = i;
        GameAssetSpotLoad[i].Tex = Assets->Spots;
        Win32AddEntry(Queue, LoadTextureAsset, &GameAssetSpotLoad[i]);
    }
    
    game_asset_load GameAssetFontLoad[GAFI_Count];
    GameAssetFontLoad[GAFI_Rubik] = game_asset_load("Rubik-Medium.ttf");
    for (int i = 0; i < GAFI_Count; i++) {
        GameAssetFontLoad[i].Assets = Assets;
        GameAssetFontLoad[i].ID = i;
        GameAssetFontLoad[i].Fonts = Assets->Fonts;
        Win32AddEntry(Queue, LoadFontAsset, &GameAssetFontLoad[i]);
    }
    
    Win32CompleteAllWork(Queue);
    
    for (int i = 0; i < GAI_Count; i++) {
        InitTexture(GetTexture(Assets, (game_asset_id)i));
        
        
    }
    for (int i = 0; i < 3; i++)
        InitTexture(Assets->Spots[i]);
}

internal PLATFORM_WORK_QUEUE_CALLBACK(RecvData)
{
    game_state *GameState = (game_state*)Data;
    CoffeeCow *CowPlayer2 = &GameState->Player2;
    
    char Buffer[BUF_SIZE];
    int bytes = SocketqRecv(&GameState->Client, BufferC(Buffer, BUF_SIZE), BUF_SIZE);
    if (bytes > 0) {
        game_packet *PacketRecv = (game_packet*)Buffer; 
        ServerCoffeeCow *Cow = &PacketRecv->Cow;
        CowPlayer2->TransitionAmt = Cow->TransitionAmt;
        CowPlayer2->Score = Cow->Score;
        CowPlayer2->Nodes.Clear();
        for (int i = 0; i < Cow->NumOfNodes; i++) {
            CoffeeCowNode Node = {};
            Node.Coords = v2(Cow->Nodes[i].Coords.x, Cow->Nodes[i].Coords.y);
            Node.CurrentDirection = Cow->Nodes[i].CurrentDirection;
            Node.Streak = Cow->Nodes[i].Streak;
            CowPlayer2->Nodes.Push(&Node);
        }
    }
    
    
}

internal PLATFORM_WORK_QUEUE_CALLBACK(SendData)
{
    game_state *GameState = (game_state*)Data;
    CoffeeCow *CowPlayer = &GameState->Player1;
    
    game_packet PacketSend = {};
    PacketSend.Cow.TransitionAmt = CowPlayer->TransitionAmt;
    PacketSend.Cow.Score = CowPlayer->Score;
    for (int i = 0; i < CowPlayer->Nodes.Size; i++) {
        CoffeeCowNode* N = (CoffeeCowNode*)CowPlayer->Nodes[i];
        ServerCoffeeCowNode Node = {};
        Node.Coords = iv2(N->Coords);
        Node.CurrentDirection = (int8)N->CurrentDirection;
        Node.Streak = (int8)N->Streak;
        PacketSend.Cow.Nodes[i] = Node;
        PacketSend.Cow.NumOfNodes++;
    }
    PacketSend.Disconnect = GameState->Disconnect;
    
    char Buffer[BUF_SIZE];
    SocketqSend(&GameState->Client, BufferCC(Buffer, BUF_SIZE, &PacketSend, sizeof(game_packet)), SEND_BUFFER_SIZE);
    
    
}

DWORD WINAPI SendRecvFunction(LPVOID lpParam)
{
    game_state *GameState = (game_state*)lpParam;
    
    while(1) {
        Win32AddEntry(GameState->Queue, RecvData, GameState);
        Win32AddEntry(GameState->Queue, SendData, GameState);
        Sleep(1);
        Win32CompleteAllWork(GameState->Queue);
        
        if (GameState->Disconnect == 1) {
            Win32AddEntry(GameState->Queue, SendData, GameState);
            Sleep(1);
            Win32CompleteAllWork(GameState->Queue);
            GameState->Disconnect = 0;
            break;
        }
    }
    
    return true;
}

#include "main_menu.cpp" 
#include "game_over_menu.cpp"
#include "pause_menu.cpp" 
#include "multiplayer_menu.cpp"
#include "local_multiplayer_menu.cpp"

inline void
CoffeeCowProcessInput(platform_controller_input *Controller, CoffeeCow *Cow)
{
    if(OnKeyDown(&Controller->MoveLeft)) AddInput(Cow, LEFT);
    if(OnKeyDown(&Controller->MoveRight))AddInput(Cow, RIGHT);
    if(OnKeyDown(&Controller->MoveDown)) AddInput(Cow, DOWN);
    if(OnKeyDown(&Controller->MoveUp)) AddInput(Cow, UP);
}

void UpdateRender(platform* p)
{
    game_state *GameState = (game_state*)p->Memory.TransientStorage;
    GameState->Queue = &p->Queue;
    
    v2 TopLeftCornerCoords = GetTopLeftCornerCoords(p);
    v2 PlatformDim = GetDim(p);
    
    Camera *C = &GameState->C;
    
    if (!p->Initialized)
    {
        p->Initialized = true;
        
        Manager.Next = (char*)p->Memory.PermanentStorage;
        qalloc(sizeof(game_state));
        
        *C = {};
        C->Position = v3(0, 0, -900);
        C->Target = v3(0, 0, 0);
        C->Up = v3(0, 1, 0);
        C->FOV = 90.0f;
        C->F = 0.01f;
        
        GameState->Menu = menu_mode::main_menu;
        GameState->Game = game_mode::not_in_game;
        GameState->GridDim = v2(17, 17);
        
        LoadAssets(&p->Queue, &GameState->Assets);
    }
    
    real32 NewGridSize = p->Dimension.Height / (GameState->GridDim.x + 6);
    if (NewGridSize != GameState->GridSize)
        GameState->GridSize = NewGridSize;
    
    platform_keyboard_input *Keyboard = &p->Input.Keyboard;
    if (OnKeyDown(&Keyboard->F5))
        GameState->ShowFPS = !GameState->ShowFPS;
    
    if (GameState->ShowFPS) {
        real32 fps = 0;
        if (p->Input.WorkSecondsElapsed != 0) {
            fps= 1 / p->Input.WorkSecondsElapsed;
            strinq FPS = S() + (int)fps;
            
            font_string FontString = {};
            FontStringInit(&FontString, GetFont(&GameState->Assets, GAFI_Rubik), FPS.Data, 50, 0xFFFFFFFF);
            v2 SDim = FontStringGetDim(&FontString);
            FontStringPrint(&FontString, v2((p->Dimension.Width/2)-(int)SDim.x-10, -p->Dimension.Height/2 + 10));
        }
    }
    
    C->Dimension = p->Dimension;
    
    if (GameState->Game == game_mode::singleplayer) {
        CoffeeCow *Cow = &GameState->Player1;
        Coffee *Cof = &GameState->Collect;
        
        if (GameState->ResetGame) {
            InitializeCow(Cow, GameState->GridDim);
            InitializeCoffee(Cow, Cof, GameState->GridDim);
            GameState->ResetGame = false;
        }
        
        if (OnKeyDown(&p->Input.CurrentInputInfo.Controller->Start))
            MenuToggle(GameState, menu_mode::not_in_menu, menu_mode::pause_menu);
        
        if (GameState->Menu != menu_mode::pause_menu) {
            if (GameState->Menu != menu_mode::game_over_menu)
                CoffeeCowProcessInput(p->Input.CurrentInputInfo.Controller, Cow);
            
            if (!MoveCoffeeCow(Cow, p->Input.WorkSecondsElapsed, GameState->GridDim)) 
                SetMenu(GameState, menu_mode::game_over_menu);
            
            CollectCoffee(Cow, Cof, GameState->GridDim);
            
            v2 RealGridDim = v2(GameState->GridDim.x * GameState->GridSize, GameState->GridDim.y * GameState->GridSize);
            v2 GridCoords = (RealGridDim * -1) / 2;
            DrawRock(&GameState->Assets, GridCoords, RealGridDim, -0.001f);
            DrawBackground(&GameState->Assets, TopLeftCornerCoords, PlatformDim, -1.0f);
            DrawGrid(&GameState->Assets, GridCoords, GameState->GridDim, GameState->GridSize, -0.1f);
            Push(RenderGroup, v3(GridCoords, -0.2f), RealGridDim, GetTexture(&GameState->Assets, GAI_Grass), 0, BlendMode::gl_src_alpha);
            
            DrawCoffeeCow(&GameState->Assets, Cow, Cof->Coords, p->Input.WorkSecondsElapsed, GridCoords.x, GridCoords.y, GameState->GridSize);
            DrawCoffee(&GameState->Assets, Cof, p->Input.WorkSecondsElapsed, GridCoords, GameState->GridSize, 0.1f);
            DrawScore(&GameState->Assets, Cow->Score, TopLeftCornerCoords);
        }
    }
    else if (GameState->Game == game_mode::multiplayer) {
        CoffeeCow *CowPlayer = &GameState->Player1;
        CoffeeCow *CowPlayer2 = &GameState->Player2;
        Coffee *Collect = &GameState->Collect;
        
        if (GameState->ResetGame) {
            InitializeCow(CowPlayer, GameState->GridDim);
            CowPlayer2->Nodes.Init((int)(GameState->GridDim.x * GameState->GridDim.y), sizeof(CoffeeCowNode));
            
            SocketqInit(&GameState->Client, GameState->IP, GameState->Port, TCP);
            GameState->ResetGame = false;
            GameState->Disconnect = 0;
            
            DWORD ThreadID;
            //HANDLE ThreadHandle = CreateThread(0, 0, SendRecvFunction, GameState, 0, &ThreadID);
            //CloseHandle(ThreadHandle);
            GameState->ThreadHandle = CreateThread(0, 0, SendRecvFunction, GameState, 0, &ThreadID);
        }
        
        if (OnKeyDown(&p->Input.CurrentInputInfo.Controller->Start))
            MenuToggle(GameState, menu_mode::not_in_menu, menu_mode::pause_menu);
        
        if (GameState->Menu != menu_mode::pause_menu) {
            CoffeeCowProcessInput(p->Input.CurrentInputInfo.Controller, CowPlayer);
            
            if (!MoveCoffeeCow(CowPlayer, p->Input.WorkSecondsElapsed, GameState->GridDim))
                SetMenu(GameState, menu_mode::game_over_menu);
            
            CollectCoffee(CowPlayer, Collect, GameState->GridDim);
            
            v2 RealGridDim = v2(GameState->GridDim.x * GameState->GridSize, GameState->GridDim.y * GameState->GridSize);
            v2 GridCoords = (RealGridDim * -1) / 2;
            DrawRock(&GameState->Assets, GridCoords, RealGridDim, -.001f);
            DrawBackground(&GameState->Assets, TopLeftCornerCoords, PlatformDim, -1.0f);
            DrawGrid(&GameState->Assets, GridCoords, GameState->GridDim, GameState->GridSize, -0.1f);
            Push(RenderGroup, v3(GridCoords, -0.2f), RealGridDim, GetTexture(&GameState->Assets, GAI_Grass), 0, BlendMode::gl_src_alpha);
            DrawCoffeeCow(&GameState->Assets, CowPlayer, Collect->Coords, p->Input.WorkSecondsElapsed, GridCoords.x, GridCoords.y, GameState->GridSize);
            DrawCoffeeCow(&GameState->Assets, CowPlayer2, Collect->Coords, p->Input.WorkSecondsElapsed, GridCoords.x, GridCoords.y, GameState->GridSize);
        }
    }
    else if (GameState->Game == game_mode::local_multiplayer) {
        for (int i = 0; i < 4; i++) {
            if (GameState->Players[i].Input != 0) {
                if (OnKeyDown(&GameState->Players[i].Input->Start))
                    MenuToggle(GameState, menu_mode::not_in_menu, menu_mode::pause_menu);
            }
        }
        if (GameState->Menu != menu_mode::pause_menu) {
            Coffee *Collect = &GameState->Collect;
            
            if (GameState->ResetGame) {
                for (int i = 0; i < 4; i++) {
                    if (GameState->Players[i].Input != 0) {
                        InitializeCow(&GameState->Players[i], GameState->GridDim);
                    }
                }
                GameState->ResetGame = false;
            }
            
            for (int i = 0; i < 4; i++) {
                if (GameState->Players[i].Input != 0) {
                    if (GameState->Menu != menu_mode::game_over_menu)
                        CoffeeCowProcessInput(GameState->Players[i].Input, &GameState->Players[i]);
                    
                    if (!MoveCoffeeCow(&GameState->Players[i], p->Input.WorkSecondsElapsed, GameState->GridDim)) 
                        SetMenu(GameState, menu_mode::game_over_menu);
                }
            }
            
            v2 RealGridDim = v2(GameState->GridDim.x * GameState->GridSize, GameState->GridDim.y * GameState->GridSize);
            v2 GridCoords = (RealGridDim * -1) / 2;
            for (int i = 0; i < 4; i++) {
                if (GameState->Players[i].Input != 0) {
                    DrawCoffeeCow(&GameState->Assets, &GameState->Players[i], Collect->Coords, p->Input.WorkSecondsElapsed, GridCoords.x, GridCoords.y, GameState->GridSize);
                }
            }
            DrawRock(&GameState->Assets, GridCoords, RealGridDim, -.001f);
            DrawBackground(&GameState->Assets, TopLeftCornerCoords, PlatformDim, -1.0f);
            DrawGrid(&GameState->Assets, GridCoords, GameState->GridDim, GameState->GridSize, -0.1f);
            Push(RenderGroup, v3(GridCoords, -0.2f), RealGridDim, GetTexture(&GameState->Assets, GAI_Grass), 0, BlendMode::gl_src_alpha);
        }
    }
    
    if (GameState->Menu == menu_mode::main_menu) {
        enum menu_component_id
        {
            MCI_Singleplayer,
            MCI_LocalMultiplayer,
            MCI_Multiplayer,
            MCI_Quit,
            MCI_Count
        };
        pair_int_string IDs[] = {
            pairintstring(MCI_Singleplayer),
            pairintstring(MCI_LocalMultiplayer),
            pairintstring(MCI_Multiplayer),
            pairintstring(MCI_Quit),
            pairintstring(MCI_Count)
        };
        
        menu *Menu = GetMenu(GameState, menu_mode::main_menu);
        if (DoMenu(Menu, "main.menu", p, GameState, IDs, MCI_Count)) 
        {
            if (Menu->Events.ButtonClicked == MCI_Singleplayer) {
                PlatformSetCursorMode(&p->Input.Mouse, platform_cursor_mode::Arrow);
                SetMenu(GameState, menu_mode::not_in_menu);
                GameState->ResetGame = true;
                GameState->Game = game_mode::singleplayer;
            }
            else if (Menu->Events.ButtonClicked == MCI_Multiplayer) {
                SetMenu(GameState, menu_mode::multiplayer_menu);
            }
            else if (Menu->Events.ButtonClicked == MCI_LocalMultiplayer) {
                for (int i = 0; i < ArrayCount(p->Input.Controllers); i++) {
                    platform_controller_input *Controller = &p->Input.Controllers[i];
                    Controller->IgnoreInputs = false;
                }
                SetMenu(GameState, menu_mode::local_multiplayer_menu);
            }
            else if (Menu->Events.ButtonClicked == MCI_Quit)
                p->Input.Quit = 1;
            
            if (Menu->ScreenDim != GetDim(p)) {
                ResizeMenu(Menu, GetDim(p));
                UpdateMenu(Menu);
            }
        }
        
        DrawMenu(Menu, 100.0f);
        DrawBackground(GetTexture(&GameState->Assets, GAI_MainMenuBack), GetTopLeftCornerCoords(p), GetDim(p), 0.0f);
    }
    else if (GameState->Menu == menu_mode::multiplayer_menu) {
        enum menu_component_id
        {
            MCI_IP,
            MCI_Port,
            MCI_Join,
            MCI_Back,
            MCI_PortText,
            MCI_Count
        };
        pair_int_string IDs[] = {
            pairintstring(MCI_IP),
            pairintstring(MCI_Port),
            pairintstring(MCI_Join),
            pairintstring(MCI_Back),
            pairintstring(MCI_PortText),
            pairintstring(MCI_Count)
        };
        
        menu *Menu = GetMenu(GameState, menu_mode::multiplayer_menu);
        if (DoMenu(Menu, "online_multiplayer.menu", p, GameState, IDs, MCI_Count)) 
        {
            if (Menu->Events.ButtonClicked == MCI_Join) {
                PlatformSetCursorMode(&p->Input.Mouse, platform_cursor_mode::Arrow);
                GameState->ResetGame = true;
                SetMenu(GameState, menu_mode::not_in_menu);
                GameState->Game = game_mode::multiplayer;
                
                GameState->IP = MenuGetTextBoxText(Menu, MCI_IP);
                GameState->Port = MenuGetTextBoxText(Menu, MCI_Port);
            }
            else if (Menu->Events.ButtonClicked == MCI_Back) {
                SetMenu(GameState, menu_mode::main_menu);
            }
            
            if (Menu->ScreenDim != GetDim(p)) {
                ResizeMenu(Menu, GetDim(p));
                UpdateMenu(Menu);
            }
        }
        
        DrawMenu(Menu, 100.0f);
        DrawBackground(GetTexture(&GameState->Assets, GAI_MainMenuBack), GetTopLeftCornerCoords(p), GetDim(p), 0.0f);
    }
    else if (GameState->Menu == menu_mode::pause_menu) {
        
        enum menu_component_id
        {
            MCI_Reset,
            MCI_Menu,
            MCI_Count
        };
        pair_int_string IDs[] = {
            pairintstring(MCI_Reset),
            pairintstring(MCI_Menu),
            pairintstring(MCI_Count)
        };
        
        menu *Menu = GetMenu(GameState, menu_mode::pause_menu);
        if (DoMenu(Menu, "pause.menu", p, GameState, IDs, MCI_Count)) 
        {
            if (Menu->Events.ButtonClicked == MCI_Menu) {
                GameState->Game = game_mode::not_in_game;
                SetMenu(GameState, menu_mode::main_menu);
            }
            else if (Menu->Events.ButtonClicked == MCI_Reset) {
                GameState->ResetGame = true;
                GameState->Game = game_mode::singleplayer;
                SetMenu(GameState, menu_mode::not_in_menu);
            }
            
            if (Menu->ScreenDim != GetDim(p)) {
                ResizeMenu(Menu, GetDim(p));
                UpdateMenu(Menu);
            }
        }
        
        DrawMenu(Menu, 100.0f);
        DrawBackground(GetTexture(&GameState->Assets, GAI_MainMenuBack), GetTopLeftCornerCoords(p), GetDim(p), 0.0f);
    }
    else if (GameState->Menu == menu_mode::game_over_menu) {
        
        enum menu_component_id
        {
            MCI_Reset,
            MCI_Menu,
            MCI_Count
        };
        pair_int_string IDs[] = {
            pairintstring(MCI_Reset),
            pairintstring(MCI_Menu),
            pairintstring(MCI_Count)
        };
        
        menu *Menu = GetMenu(GameState, menu_mode::game_over_menu);
        if (DoMenu(Menu, "game_over.menu", p, GameState, IDs, MCI_Count)) 
        {
            if (Menu->Events.ButtonClicked == MCI_Menu) {
                GameState->Game = game_mode::not_in_game;
                SetMenu(GameState, menu_mode::main_menu);
            }
            else if (Menu->Events.ButtonClicked == MCI_Reset) {
                GameState->ResetGame = true;
                GameState->Game = game_mode::singleplayer;
                SetMenu(GameState, menu_mode::not_in_menu);
            }
            
            if (Menu->ScreenDim != GetDim(p)) {
                ResizeMenu(Menu, GetDim(p));
                UpdateMenu(Menu);
            }
        }
        
        DrawMenu(Menu, 100.0f);
    }
    else if (GameState->Menu == menu_mode::local_multiplayer_menu) {
        enum menu_component_id
        {
            MCI_Default,
            MCI_Start,
            MCI_Back,
            PI_Player1,
            PI_Player2,
            PI_Player3,
            PI_Player4,
            
            MCI_Count
        };
        pair_int_string IDs[] = {
            pairintstring(MCI_Default),
            pairintstring(MCI_Start),
            pairintstring(MCI_Back),
            pairintstring(PI_Player1),
            pairintstring(PI_Player2),
            pairintstring(PI_Player3),
            pairintstring(PI_Player4),
        };
        
        menu *Menu = GetMenu(GameState, menu_mode::local_multiplayer_menu);
        if (DoMenu(Menu, "local_multiplayer.menu", p, GameState, IDs, MCI_Count)) 
        {
            if (Menu->Events.ButtonClicked == MCI_Start) {
                for (int i = 0; i < MCI_Count; i++) {
                    menu_component *Player = MenuGetComponent(Menu->CheckBoxes, i);
                    menu_component_checkbox *CheckBox = (menu_component_checkbox*)Player->Data;
                    GameState->Players[i].Input = CheckBox->Controller;
                }
                
                PlatformSetCursorMode(&p->Input.Mouse, platform_cursor_mode::Arrow);
                SetMenu(GameState, menu_mode::not_in_menu);
                GameState->ResetGame = true;
                GameState->Game = game_mode::local_multiplayer;
                
                
            }
            else if (Menu->Events.ButtonClicked == MCI_Back) {
                SetMenu(GameState, menu_mode::main_menu);
            }
            
            if (Menu->ScreenDim != GetDim(p)) {
                ResizeMenu(Menu, GetDim(p));
                UpdateMenu(Menu);
            }
        }
        
        DrawMenu(Menu, 100.0f);
        DrawBackground(GetTexture(&GameState->Assets, GAI_MainMenuBack), GetTopLeftCornerCoords(p), GetDim(p), 0.0f);
    }
    BeginMode2D(*C);
    RenderPieceGroup(RenderGroup);
    EndMode2D();
}