/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include "snake.h"

#include <math.h>
inline int32
RoundReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)roundf(Real32);
    return(Result);
}

#include "stdio.h"

#if !defined(RAYLIB_H)
#include "text.h"
#endif

#include "gui.h"
#include "memorymanager.cpp" 
#include "memorymanager.h"

#if !defined(RAYLIB_H)
#include "text.cpp"
#include "renderer.cpp"
#include "image.cpp"
#endif


#include "gui.cpp"

//#include "socketq.cpp"
//#include "socketq.h"



internal void
GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz)
{
    local_persist real32 tSine;
    int16 ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
    
    int16 *SampleOut = SoundBuffer->Samples;
    for(int SampleIndex = 0;
        SampleIndex < SoundBuffer->SampleCount;
        ++SampleIndex)
    {
        // TODO(casey): Draw this out for people
        real32 SineValue = sinf(tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;
        
        tSine += 2.0f*Pi32*1.0f/(real32)WavePeriod;
    }
}

global_variable int GameInitialized = false;
global_variable Snake Player;
global_variable GUI MainMenu = {};
global_variable GUI LoseMenu = {};

internal void 
AddSnakeNode(Snake *S, float X, float Y)
{
    if (S->Head == 0)
    {
        SnakeNode part1 = {};
        part1.X = X;
        part1.Y = Y;
        
        S->Head = (SnakeNode*)PermanentStorageAssign(&part1, sizeof(SnakeNode));
        return;
    }
    
    SnakeNode* cursor = S->Head;
    while(cursor->Next != 0)
    {
        cursor = cursor->Next;
    }
    SnakeNode partnew = {};
    partnew.X = X;
    partnew.Y = Y;
    partnew.Previous = cursor;
    partnew.Direction = partnew.Previous->Direction;
    partnew.NextDirection = partnew.Previous->NextDirection;
    
    cursor->Next = (SnakeNode*)PermanentStorageAssign(&partnew, sizeof(SnakeNode));
}

internal void
AddLinkedListNode(LinkedList *L, void *Data)
{
    LinkedListNode TempN = {};
    TempN.Data = Data;
    TempN.Next = 0;
    
    LinkedListNode* N = (LinkedListNode*)PermanentStorageAssign(&TempN, sizeof(LinkedListNode));
    if (L->Head == 0)
    {
        L->Head = N;
    }
    else
    {
        LinkedListNode* Cursor = L->Head;
        while(Cursor->Next != 0)
        {
            Cursor = Cursor->Next;
        }
        Cursor->Next = N;
    }
}

internal void
RemoveFirstLinkedListNode(LinkedList* L)
{
    if (L->Head != 0)
    {
        if (L->Head->Next == 0)
        {
            L->Head = 0;
        }
        else
        {
            L->Head = L->Head->Next;
        }
    }
}

internal void
AddInputLog(Snake *S, int32 Num)
{
    void* VoidNum = (void*)PermanentStorageAssign(&Num, sizeof(int32));
    AddLinkedListNode(&S->InputLog, VoidNum);
}


internal void
InitializeSnake(Snake *S)
{
    S->Direction = RIGHT;
    S->Length = 3;
    
    AddSnakeNode(S, 2, 2);
    AddSnakeNode(S, 1, 2);
    AddSnakeNode(S, 0, 2);
}

global_variable Image test;
global_variable Image right;
global_variable Image up;
global_variable Image left;
global_variable Image down;
global_variable Image apple;

internal void
RenderSnake(Snake* s, int  GridX, int  GridY, 
            int GridWidth, int GridHeight, int GridSize)
{
    SnakeNode* cursor = s->Head;
    while(cursor->Next != 0)
    {
        cursor = cursor->Next;
    }
    
    while(cursor != s->Head)
    {
        
        int CursorX = (int)(cursor->X * GridSize);
        int CursorY = (int)(cursor->Y * GridSize);
        Rect newSquare =
        {
            GridX + CursorX, 
            GridY + CursorY, 
            GridSize, GridSize
        };
        
        RenderRect(&newSquare, FILL, 0xFFFF5E5E);
        
        cursor = cursor->Previous;
    }
    
    int CursorX = (int)(cursor->X * GridSize);
    int CursorY = (int)(cursor->Y * GridSize);
    
    Rect newSquare =
    {
        GridX + CursorX + (0 * (GRIDSIZE / 2)), 
        GridY + CursorY + (0 * (GRIDSIZE / 2)), 
        GRIDSIZE, GRIDSIZE
    };
    //RenderRect(&newSquare, FILL, 0xFFFF5E5E);
    if (cursor == s->Head)
    {
        loaded_bitmap BMP = {};
        BMP.Width = GridSize;
        BMP.Height = GridSize;
        BMP.Pitch = BITMAP_BYTES_PER_PIXEL * BMP.Width;
        
        if (cursor->Direction == RIGHT)
        {
            BMP.Memory = right.data;
        }
        else if (cursor->Direction == LEFT)
        {
            BMP.Memory = left.data;
        }
        else if (cursor->Direction == UP)
        {
            BMP.Memory = down.data;
        }
        else if (cursor->Direction == DOWN)
        {
            BMP.Memory = up.data;
        }
        RenderBitmap(&BMP, (real32)(GridX + CursorX), (real32)(GridY + CursorY));
        
    }
}

internal void
RenderApple(Apple* A, int GridX, int GridY, 
            int GridWidth, int GridHeight, int GridSize)
{
    Rect newSquare =
    {
        GridX + (A->X * GridSize), 
        GridY + (A->Y * GridSize), 
        GridSize, GridSize
    };
    //RenderRect(&newSquare, FILL, 0xFFff4040);
    //RenderRectImage(&newSquare, &apple);
    loaded_bitmap BMP = {};
    BMP.Width = GridSize;
    BMP.Height = GridSize;
    BMP.Pitch = BITMAP_BYTES_PER_PIXEL * BMP.Width;
    BMP.Memory = apple.data;
    RenderBitmap(&BMP, (real32)newSquare.x, (real32)newSquare.y);
}


internal void
TransitionSnake(Snake* snake, float SecondsElapsed)
{
    float Dm = (snake->Speed * SecondsElapsed);
    float Dp = Dm;
    
    snake->DistanceTravelled += Dp;
    
    SnakeNode* cursor = snake->Head;
    while(cursor != 0)
    {
        if (cursor->Direction == RIGHT)
        {
            cursor->X += Dp;
        }
        else if (cursor->Direction == UP)
        {
            cursor->Y -= Dp;
        }
        else if (cursor->Direction == LEFT)
        {
            cursor->X -= Dp;
        }
        else if (cursor->Direction == DOWN)
        {
            cursor->Y += Dp;
        }
        cursor = cursor->Next;
    }
}

internal void
MoveSnake(Snake* snake)
{
    
    SnakeNode* cursor = snake->Head;
    
    if (snake->InputLog.Head == 0)
    {
        AddInputLog(snake, snake->Direction);
    }
    
    int32* NextDir = (int32*)snake->InputLog.Head->Data;
    
    /*
    char FPSBuffer[256];
    _snprintf_s(FPSBuffer, sizeof(FPSBuffer),
                "NEXTDIR: %d DIRECTION:%d\n", *NextDir, snake->Direction);
    OutputDebugStringA(FPSBuffer);
    */
    
    if (*NextDir == UP && snake->Direction == DOWN) {}
    else if (*NextDir == DOWN && snake->Direction == UP) {}
    else if (*NextDir == RIGHT && snake->Direction == LEFT) {}
    else if (*NextDir == LEFT && snake->Direction == RIGHT) {}
    else
    {
        snake->Direction = *NextDir;
        cursor->Direction = snake->Direction;
    }
    RemoveFirstLinkedListNode(&snake->InputLog);
    
    cursor->X = (real32)(RoundReal32ToInt32(cursor->X));
    cursor->Y = (real32)(RoundReal32ToInt32(cursor->Y));
    cursor = cursor->Next;
    
    while(cursor != 0)
    {
        cursor->X = (real32)(RoundReal32ToInt32(cursor->X));
        cursor->Y = (real32)(RoundReal32ToInt32(cursor->Y));
        
        cursor->Direction = cursor->NextDirection;
        cursor->NextDirection = cursor->Previous->Direction;
        cursor = cursor->Next;
    }
}

internal void
CheckGetApple(Snake* S, Apple* A)
{
    if(S->Head->X == A->X && S->Head->Y == A->Y)
    {
        A->Score++;
        S->Length++;
        
        SnakeNode* Cursor = S->Head;
        while(Cursor->Next != 0)
        {
            Cursor = Cursor->Next;
        }
        
        if (Cursor->NextDirection == RIGHT)
            AddSnakeNode(S, Cursor->X - 1, Cursor->Y);
        if (Cursor->NextDirection == UP)
            AddSnakeNode(S, Cursor->X, Cursor->Y + 1);
        if (Cursor->NextDirection == LEFT)
            AddSnakeNode(S, Cursor->X + 1, Cursor->Y);
        if (Cursor->NextDirection == DOWN)
            AddSnakeNode(S, Cursor->X, Cursor->Y - 1);
        
        int Good = 0;
        while(!Good)
        {
            A->X = rand() % 17;
            A->Y = rand() % 17;
            Good = 1;
            
            Cursor = S->Head;
            while(Cursor != 0)
            {
                if (Cursor->X == A->X && Cursor->Y == A->Y)
                {
                    Good = 0;
                }
                Cursor = Cursor->Next;
            }
        }
    }
}

internal int
CheckBounds(Snake* snake, int GridWidth, int GridHeight)
{
    // Snake hits the wall
    if (snake->Head->X  == 0 && snake->Direction == LEFT)
    {
        snake->Speed = 0;
        return 0;
    }
    else if(snake->Head->X + 1 == GridWidth && snake->Direction == RIGHT)
    {
        snake->Speed = 0;
        return 0;
    }
    else if (snake->Head->Y == 0 && snake->Direction == UP)
    {
        snake->Speed = 0;
        return 0;
    }
    else if (snake->Head->Y == GridHeight - 1 && snake->Direction == DOWN)
    {
        snake->Speed = 0;
        return 0;
    }
    
    // Snake hits itself
    SnakeNode* Cursor = snake->Head->Next;
    while(Cursor != 0)
    {
        if (snake->Head->X - 1 == Cursor->X && snake->Head->Y == Cursor->Y && snake->Direction == LEFT)
        {
            snake->Speed = 0;
            return 0;
        }
        else if (snake->Head->X + 1 == Cursor->X && snake->Head->Y == Cursor->Y && snake->Direction == RIGHT)
        {
            snake->Speed = 0;
            return 0;
        }
        else if (snake->Head->Y - 1 == Cursor->Y && snake->Head->X == Cursor->X && snake->Direction == UP)
        {
            snake->Speed = 0;
            return 0;
        }
        else if (snake->Head->Y + 1 == Cursor->Y && snake->Head->X == Cursor->X && snake->Direction == DOWN)
        {
            snake->Speed = 0;
            return 0;
        }
        
        Cursor = Cursor->Next;
    }
    
    snake->Speed = snake->MaxSpeed;
    return 1;
}


entire_file
ReadEntireFile(char *FileName)
{
    entire_file Result = {};
    
    FILE *In = fopen(FileName, "rb");
    if(In)
    {
        fseek(In, 0, SEEK_END);
        Result.ContentsSize = ftell(In);
        fseek(In, 0, SEEK_SET);
        
        Result.Contents = malloc(Result.ContentsSize);
        fread(Result.Contents, Result.ContentsSize, 1, In);
        fclose(In);
    }
    else
    {
        printf("ERROR: Cannot open file %s.\n", FileName);
    }
    
    return(Result);
}


loaded_bitmap yo;
//Client client;

real32 BackspaceTime = 0;
int Backspace = 0;

Font Faune50 = {};
Font Faune100 = {};

real32 SnakeTimeCounter = 0;

Apple App = {};


#include "../data/imagesaves/grass.h"
#include "../data/imagesaves/grass.cpp"
#include "../data/imagesaves/right.h"
#include "../data/imagesaves/right.cpp"
#include "../data/imagesaves/up.h"
#include "../data/imagesaves/up.cpp"
#include "../data/imagesaves/left.h"
#include "../data/imagesaves/left.cpp"
#include "../data/imagesaves/down.h"
#include "../data/imagesaves/down.cpp"
#include "../data/imagesaves/apple.h"
#include "../data/imagesaves/apple.cpp"

#include "../data/imagesaves/faunefifty.h"
#include "../data/imagesaves/fauneonehundred.h"

internal void
GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer,
                    game_sound_output_buffer *SoundBuffer)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    
    int centeredX = (Buffer->Width - (GRIDWIDTH * GRIDSIZE)) / 2;
    int centeredY = (Buffer->Height - (GRIDHEIGHT * GRIDSIZE)) / 2;
    //Rect imageRect = {centeredX, centeredY, (GRIDWIDTH * GRIDSIZE), (GRIDHEIGHT * GRIDSIZE), 0};
    Rect imageRect = {centeredX, centeredY, (GRIDSIZE), (GRIDSIZE), 0};
    
    if(!Memory->IsInitialized)
    {
        
#if SNAKE_INTERNAL
        char *Filename = __FILE__;
        debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
        if(File.Contents)
        {
            DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
            DEBUGPlatformFreeFileMemory(File.Contents);
        }
#endif
        
        GameState->ToneHz = 256;
        GameState->Menu = 1;
        
        manager.NextStorage = (char*)Memory->PermanentStorage + sizeof(game_state);
        
#if !defined(RAYLIB_H)
#if SAVE_IMAGES
        test = LoadImageResize("grass2.png", GRIDSIZE, GRIDSIZE, "grass.h");
        right = LoadImageResize("right.png", GRIDSIZE, GRIDSIZE, "right.h");
        up = LoadImageResize("up.png", GRIDSIZE, GRIDSIZE, "up.h");
        left = LoadImageResize("left.png", GRIDSIZE, GRIDSIZE, "left.h");
        down = LoadImageResize("down.png", GRIDSIZE, GRIDSIZE, "down.h");
        apple = LoadImageResize("apple.png", GRIDSIZE, GRIDSIZE, "apple.h");
#else
        LoadImageFromgrass_h(&test);
        LoadImageFromright_h(&right);
        LoadImageFromup_h(&up);
        LoadImageFromleft_h(&left);
        LoadImageFromdown_h(&down);
        LoadImageFromapple_h(&apple);
#endif
#else
        testTexture = LoadImageResize("grass.jpg", GRIDSIZE, GRIDSIZE);
#endif
        //loaded_bitmap grass = LoadBMP("grass.png");
        // Where the top left corner of the grid should be for it to look
        // centered in the window
        
#if !defined(RAYLIB_H)
#if SAVE_IMAGES
        
        Faune50 = LoadEntireFont("Faune-TextRegular.otf", 50);
        Faune100 = LoadEntireFont("Faune-TextRegular.otf", 100);
        SaveFontToHeaderFile("FAUNE50", "faunefifty.h", "imagesaves/faunefifty.h" ,&Faune50);
        SaveFontToHeaderFile("FAUNE100", "fauneonehundred.h", "imagesaves/fauneonehundred.h" ,&Faune100);
#else
        LoadFontFromHeaderFile(&Faune50, FAUNE50stbtt, FAUNE50stbttuserdata, FAUNE50stbttdata, FAUNE50Size,
                               FAUNE50Ascent, FAUNE50Scale, FAUNE50FontChar, FAUNE50FontCharMemory);
        LoadFontFromHeaderFile(&Faune100, FAUNE100stbtt, FAUNE100stbttuserdata, FAUNE100stbttdata, FAUNE100Size,
                               FAUNE100Ascent, FAUNE100Scale, FAUNE100FontChar, FAUNE100FontCharMemory);
#endif
#else 
        Faune100 = LoadFontEx("Faune-TextRegular.otf", 100, 0, 255);
        Faune50 = LoadFontEx("Faune-TextRegular.otf", 50, 0, 255);
#endif
        
        
        // TODO(casey): This may be more appropriate to do in the platform layer
        Memory->IsInitialized = true;
    }
    
    int intFPS = (int)(1/Input->SecondsElapsed);
    char FPS[10]; 
    sprintf(FPS, "%d", intFPS);
    Rect FPSRect = {10, 10, 100, 50, 0};
    
    int btnPress = -1;
    int tbPress = -1;
    
    for(int ControllerIndex = 0;
        ControllerIndex < ArrayCount(Input->Controllers);
        ++ControllerIndex)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        if(Controller->IsAnalog)
        {
            // NOTE(casey): Use analog movement tuning
            GameState->BlueOffset += (int)(4.0f*Controller->StickAverageX);
            GameState->ToneHz = 256 + (int)(128.0f*Controller->StickAverageY);
        }
        else
        {
            if (GameState->Menu == 0)
            {
                if(Controller->MoveLeft.NewEndedDown)
                {
                    AddInputLog(&Player, LEFT);
                }
                
                if(Controller->MoveRight.NewEndedDown)
                {
                    AddInputLog(&Player, RIGHT);
                }
                
                if(Controller->MoveDown.NewEndedDown)
                {
                    AddInputLog(&Player, DOWN);
                }
                
                if(Controller->MoveUp.NewEndedDown)
                {
                    AddInputLog(&Player, UP);
                }
            }
            
            //sprintf(FPS, "%d", Player.Direction);
            //PrintOnScreen(Buffer, &Faune50, FPS, FPSRect.x, FPSRect.y, 0xFF000000, &FPSRect);
            
            if(Controller->Zero.NewEndedDown)
            {
                AddCharTextBoxText(&MainMenu, "0");
            }
            if(Controller->One.NewEndedDown)
            {
                AddCharTextBoxText(&MainMenu, "1");
            }
            if(Controller->Two.NewEndedDown)
            {
                AddCharTextBoxText(&MainMenu, "2");
            }
            if(Controller->Three.NewEndedDown)
            {
                AddCharTextBoxText(&MainMenu, "3");
            }
            if(Controller->Four.NewEndedDown)
            {
                AddCharTextBoxText(&MainMenu, "4");
            }
            if(Controller->Five.NewEndedDown)
            {
                AddCharTextBoxText(&MainMenu, "5");
            }
            if(Controller->Six.NewEndedDown)
            {
                AddCharTextBoxText(&MainMenu, "6");
            }
            if(Controller->Seven.NewEndedDown)
            {
                AddCharTextBoxText(&MainMenu, "7");
            }
            if(Controller->Eight.NewEndedDown)
            {
                AddCharTextBoxText(&MainMenu, "8");
            }
            if(Controller->Nine.NewEndedDown)
            {
                AddCharTextBoxText(&MainMenu, "9");
            }
            if(Controller->Period.NewEndedDown)
            {
                AddCharTextBoxText(&MainMenu, ".");
            }
            if(Controller->Back.NewEndedDown)
            {
                RemoveCharTextBoxText(&MainMenu);
            }
            
        }
    }
    
    // TODO(casey): Allow sample offsets here for more robust platform options
    //GameOutputSound(SoundBuffer, GameState->ToneHz);
    
    
    if (GameState->Menu == 2)
    {
        if(Input->MouseButtons[0].EndedDown)
        {
            btnPress = CheckButtonsClick(&LoseMenu, Input->MouseX, Input->MouseY);
        }
        
        if (btnPress == Restart)
        {
            GameState->Menu = 0;
        }
        
        if (LoseMenu.Initialized == 0)
        {
            LoseMenu.Padding = 10;
            
            int Y = 0;
            Text TXT = {};
            Button BTN = {};
            
            TXT.Text = "YOU LOST";
            TXT.ID = Btn1;
            TXT.FontType = &Faune100;
            TXT.TextColor = 0xFF000000;
            AddText(&LoseMenu, 0, Y++,  &TXT);
            
            BTN.Text = "RESTART";    // Text
            BTN.FontType =&Faune100;   // Font
            BTN.ID = Restart;       // ID
            BTN.RegularColor = 0xFF32a89b; // RegularColor
            BTN.HoverColor = 0xFFeba434; // HoverColor
            BTN.TextColor = 0xFFFFFFFF; // TextColor
            AddButton(&LoseMenu, 0, Y++, 350, 100, &BTN);
            
            InitializeGUI(&LoseMenu);
            LoseMenu.Initialized = 1;
        }
        
        CheckButtonsHover(&LoseMenu, Input->MouseX, Input->MouseY);
        UpdateGUI(&LoseMenu, Buffer->Width, Buffer->Height);
        //ClearScreen();
        RenderGUI(Buffer, &LoseMenu);
    }
    else if (GameState->Menu == 1)
    {
        if(Input->MouseButtons[0].EndedDown)
        {
            btnPress = CheckButtonsClick(&MainMenu, Input->MouseX, Input->MouseY);
            
            tbPress = CheckTextBoxes(&MainMenu, Input->MouseX, Input->MouseY);
        }
        
        if (btnPress == GameStart)
        {
            GameState->Menu = 0;
        }
        else if (btnPress == Btn4)
        {
            //createClient(&client, GetTextBoxText(&MainMenu, IP), GetTextBoxText(&MainMenu, PORT), TCP);
        }
        else if (btnPress == Quit)
        {
            Input->quit = 1;
        }
        
        if (tbPress == IP)
        {
            ChangeTextBoxShowCursor(&MainMenu, IP);
        }
        if (tbPress == PORT)
        {
            ChangeTextBoxShowCursor(&MainMenu, PORT);
        }
        
        if (MainMenu.Initialized == 0)
        {
            MainMenu.Padding = 10;
            
            int Y = 0;
            
            Text TXT = {};
            Button btn = {};
            TextBox tb = {};
            
            TXT.Text = "SINGLEPLAYER";
            TXT.ID = Btn1;
            TXT.FontType = &Faune100;
            TXT.TextColor = 0xFF000000;
            AddText(&MainMenu, 0, Y++,  &TXT);
            
            btn = 
            {
                0,
                0,
                "START",    // Text
                &Faune100,   // Font
                GameStart,       // ID
                0,          // Color (CurrentColor)
                0xFF32a89b, // RegularColor
                0xFFeba434, // HoverColor
                0xFFFFFFFF, // TextColor
            };
            AddButton(&MainMenu, 0, Y++, 300, 100, &btn);
            
            TXT = 
            {
                "0",    // Text
                Btn1,       // ID
                &Faune100,   // Font
                0xFFFFFFFF, // TextColor
            };
            AddText(&MainMenu, 0, Y++,  &TXT);
            
            TXT = 
            {
                "MULTIPLAYER",    // Text
                Btn1,       // ID
                &Faune100,   // Font
                0xFF000000, // TextColor
            };
            AddText(&MainMenu, 0, Y++,  &TXT);
            
            TXT = 
            {
                "IP:",    // Text
                Btn1,       // ID
                &Faune50,   // Font
                0xFF000000, // TextColor
            };
            AddText(&MainMenu, 0, Y,  &TXT);
            
            tb =
            {
                0,
                0,
                "",
                &Faune50,
                IP,
                0,
                0xFFb3b3b3,
                0xFF000000
            };
            AddTextBox(&MainMenu, 1, Y++, 500, 50, &tb);
            
            TXT = 
            {
                "PORT:",    // Text
                Btn1,       // ID
                &Faune50,   // Font
                0xFF000000, // TextColor
            };
            AddText(&MainMenu, 0, Y,  &TXT);
            
            tb =
            {
                0,
                0,
                "",
                &Faune50,
                PORT,
                0,
                0xFFb3b3b3,
                0xFF000000
            };
            AddTextBox(&MainMenu, 1, Y++, 500, 50, &tb);
            
            btn = 
            {
                0,
                0,
                "JOIN",    // Text
                &Faune100,   // Font
                Btn4,       // ID
                0,          // Color (CurrentColor)
                0xFF32a89b, // RegularColor
                0xFFeba434, // HoverColor
                0xFFFFFFFF, // TextColor
            };
            AddButton(&MainMenu, 0, Y++, 200, 100, &btn);
            
            btn = 
            {
                0,
                0,
                "QUIT",    // Text
                &Faune100,   // Font
                Quit,       // ID
                0,          // Color (CurrentColor)
                0xFF32a89b, // RegularColor
                0xFFeba434, // HoverColor
                0xFFFFFFFF, // TextColor
            };
            AddButton(&MainMenu, 0, Y++, 200, 100, &btn);
            
            InitializeGUI(&MainMenu);
            MainMenu.Initialized = 1;
        }
        
        CheckButtonsHover(&MainMenu, Input->MouseX, Input->MouseY);
        
        
#if !defined(RAYLIB_H)
        UpdateGUI(&MainMenu, Buffer->Width, Buffer->Height);
        ClearScreen();
        RenderGUI(Buffer, &MainMenu);
#else
        BeginDrawing();
        ClearBackground(WHITE);
        RenderGUI(Buffer, &MainMenu);
        EndDrawing();
#endif
        //Clear();
        //RenderTriangle();
    }
    else if (GameState->Menu == 0)
    {
        if (GameInitialized == false)
        {
            InitializeSnake(&Player);
            GameInitialized = true;
            
            App.X = rand() % 17;
            App.Y = rand() % 17;
            
            Player.MaxSpeed = 8.0f; // m/s
            Player.Speed = Player.MaxSpeed;
            
            App.Score = 0;
        }
        
        
        //Worked to send messages to the test server
        /*
        sendq(&client, "yo", 50);
        
        char buffer[500];
        memset(buffer, 0, sizeof(buffer));
        recvq(&client, buffer, 500);
        printf("%s\n", buffer);
        */
        //SnakeTimeCounter += Input->SecondsElapsed;
        
        
        TransitionSnake(&Player, Input->SecondsElapsed);
        if (Player.DistanceTravelled >= 1 || Player.Speed == 0)
        {
            Player.DistanceTravelled = 0;
            MoveSnake(&Player);
            CheckGetApple(&Player, &App);
        }
        
        
        if(!CheckBounds(&Player, GRIDWIDTH, GRIDHEIGHT))
        {
            GameState->Menu = 2;
            GameInitialized = false;
            Player = {};
        }
        else
        {
            
            
#if !defined(RAYLIB_H)
            ClearScreen();
            RenderBackgroundGrid(centeredX, centeredY, GRIDWIDTH, GRIDHEIGHT, GRIDSIZE, &test);
            RenderApple(&App, centeredX, centeredY, GRIDWIDTH, GRIDHEIGHT, GRIDSIZE);
            RenderSnake(&Player, centeredX, centeredY, GRIDWIDTH, GRIDHEIGHT, GRIDSIZE);
            PrintOnScreen(&Faune50, IntToString(App.Score), 5, 5, 0xFF000000);
#else
            BeginDrawing();
            ClearBackground(WHITE);
            RenderBackgroundGrid(centeredX, centeredY, GRIDWIDTH, GRIDHEIGHT, GRIDSIZE, &test, &testTexture);
            RenderApple(&App, centeredX, centeredY, GRIDWIDTH, GRIDHEIGHT, GRIDSIZE);
            RenderSnake(&Player, centeredX, centeredY, GRIDWIDTH, GRIDHEIGHT, GRIDSIZE);
            PrintOnScreen(&Faune50, IntToString(App.Score), 5, 5, 0xFF000000);
            DrawFPS(300, 10);
            EndDrawing();
#endif
            
        }
    }
}
