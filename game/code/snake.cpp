/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include "snake.h"
#include "stb/stb_truetype.h"

#include "stdio.h"

#include "text.h"
#include "gui.h"

#include "text.cpp"

inline int32
RoundReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)roundf(Real32);
    return(Result);
}

#include "gui.cpp"

#include "socketq.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

#include "socketq.cpp"
#include "socketq.h"

#include "memorymanager.cpp"
#include "memorymanager.h"

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

internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
    // TODO(casey): Let's see what the optimizer does
    
    uint8 *Row = (uint8 *)Buffer->Memory;    
    for(int Y = 0;
        Y < Buffer->Height;
        ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for(int X = 0;
            X < Buffer->Width;
            ++X)
        {
            uint8 Blue = (uint8)(X + BlueOffset);
            uint8 Green = (uint8)(Y + GreenOffset);
            
            *Pixel++ = ((Green << 8) | Blue);
        }
        
        Row += Buffer->Pitch;
    }
}

internal void 
RenderRect(game_offscreen_buffer *Buffer, Rect *S, int fill, uint32 color)
{
    uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch*Buffer->Height;
    uint32 Color = color;
    
    for(int X = S->x;
        X < (S->x + S->width);
        ++X)
    {
        uint8 *Pixel = ((uint8 *)Buffer->Memory +
                        X*Buffer->BytesPerPixel +
                        S->y*Buffer->Pitch);
        
        for(int Y = S->y;
            Y < (S->y + S->height);
            ++Y)
        {
            // Check if the pixel exists
            if((Pixel >= Buffer->Memory) &&
               ((Pixel + 4) <= EndOfBuffer))
            {
                if (fill == FILL)
                {
                    *(uint32 *)Pixel = Color;
                }
                else if (fill == NOFILL)
                {
                    // Only draw border
                    if ((X == S->x) ||
                        (Y == S->y) ||
                        (X == (S->x + S->width) - 1) ||
                        (Y == (S->y + S->height) - 1))
                    {
                        *(uint32 *)Pixel = Color;
                    }
                }
            }
            
            Pixel += Buffer->Pitch;
        }
    }
}

unsigned long createRGBA(int r, int g, int b, int a)
{   
    return ((a & 0xff) << 24) + ((r & 0xff) << 16) + ((g & 0xff) << 8) + ((b & 0xff));
}

unsigned long createRGB(int r, int g, int b)
{   
    return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize.h"

internal void 
RenderRectImage(game_offscreen_buffer *Buffer, Rect *S, Image *image)
{
    Image re = {};
    re.data = image->data;
    re.x = S->width;
    re.y = S->height;
    re.n = image->n;
    //RenderImage(Buffer, &re);
    
    
    uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch*Buffer->Height;
    
    for(int X = S->x; X < (S->x + S->width); ++X)
    {
        uint8 *Pixel = ((uint8 *)Buffer->Memory + X * Buffer->BytesPerPixel +S->y*Buffer->Pitch);
        uint8 *Color = ((uint8 *)re.data + (X - S->x) * re.n);
        
        for(int Y = S->y; Y < (S->y + S->height); ++Y)
        {
            // Check if the pixel exists
            if((Pixel >= Buffer->Memory) && ((Pixel + 4) <= EndOfBuffer))
            {
                uint32 c = *Color;
                
                int r = *Color++;
                int g = *Color++;
                int b = *Color;
                Color--;
                Color--;
                
                c = createRGB(r, g, b);
                *(uint32 *)Pixel =c;
                Color += (re.n * re.x);
            }
            Pixel += Buffer->Pitch;
        }
    }
    
}

internal void
RenderBackgroundGrid(game_offscreen_buffer *Buffer, int GridX, int GridY, 
                     int GridWidth, int GridHeight, int GridSize, Image *image)
{
    for (int i = 0;
         i < GridWidth;
         i++)
    {
        for (int j = 0;
             j < GridHeight;
             j++)
        {
            Rect newRect = {GridX + (i * GridSize), GridY + (j * GridSize), GridSize, GridSize};
            RenderRect(Buffer, &newRect, NOFILL, 0xFF000000);
            RenderRectImage(Buffer, &newRect, image);
        }
    }
}



// Paints the screen white
internal void
ClearScreen(game_offscreen_buffer *Buffer)
{
    memset(Buffer->Memory, 0xFF, (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel);
    /*
        uint8 *Column = (uint8 *)Buffer->Memory;    
    for(int X = 0;
        X< Buffer->Width;
        ++X)
    {
        uint8 *Pixel = ((uint8 *)Buffer->Memory +
                        X*Buffer->BytesPerPixel);
        for(int Y = 0;
            Y < Buffer->Height;
            ++Y)
        {
            *(uint32 *)Pixel = 0xFFFFFFFF;
            Pixel += Buffer->Pitch;
        }
    }
*/
}

global_variable int GameInitialized = false;
global_variable Snake Player;
global_variable GUI menu = {};
global_variable NewGUI MainMenu = {};
global_variable NewGUI LoseMenu = {};

internal void 
NewAddSnakeNode(Snake *S, float X, float Y)
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
NewInitializeSnake(Snake *S)
{
    S->Direction = RIGHT;
    S->Length = 3;
    
    NewAddSnakeNode(S, 2, 2);
    NewAddSnakeNode(S, 1, 2);
    NewAddSnakeNode(S, 0, 2);
}

internal void
NewRenderSnake(game_offscreen_buffer *Buffer, Snake* s, int  GridX, int  GridY, 
               int GridWidth, int GridHeight, int GridSize)
{
    
    SnakeNode* cursor = s->Head;
    while(cursor != 0)
    {
        int CursorX = (int)(cursor->X * GridSize);
        int CursorY = (int)(cursor->Y * GridSize);
        Rect newSquare =
        {
            GridX + CursorX, 
            GridY + CursorY, 
            GridSize, GridSize
        };
        RenderRect(Buffer, &newSquare, FILL, 0xFFFFEBC4);
        cursor = cursor->Next;
    }
}

internal void
RenderApple(game_offscreen_buffer *Buffer, Apple* A, int GridX, int GridY, 
            int GridWidth, int GridHeight, int GridSize)
{
    Rect newSquare =
    {
        GridX + (A->X * GridSize), 
        GridY + (A->Y * GridSize), 
        GridSize, GridSize
    };
    RenderRect(Buffer, &newSquare, FILL, 0xFFff4040);
}

internal void
NewTransitionSnake(Snake* snake, float SecondsElapsed)
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
NewMoveSnake(Snake* snake)
{
    SnakeNode* cursor = snake->Head;
    
    if (snake->NextDirection == -1)
    {
        cursor->Direction = snake->Direction;
    }
    else
    {
        snake->Direction = snake->NextDirection;
        cursor->Direction = snake->NextDirection;
        snake->NextDirection = -1;
    }
    snake->LastMoveDirection = cursor->Direction;
    
    /*
    char FPSBuffer[256];
    _snprintf_s(FPSBuffer, sizeof(FPSBuffer),
                "X:%.02f Y:%.02f\n", cursor->X, cursor->Y);
    OutputDebugStringA(FPSBuffer);
    */
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
            NewAddSnakeNode(S, Cursor->X - 1, Cursor->Y);
        if (Cursor->NextDirection == UP)
            NewAddSnakeNode(S, Cursor->X, Cursor->Y + 1);
        if (Cursor->NextDirection == LEFT)
            NewAddSnakeNode(S, Cursor->X + 1, Cursor->Y);
        if (Cursor->NextDirection == DOWN)
            NewAddSnakeNode(S, Cursor->X, Cursor->Y - 1);
        
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

/*
internal int
CheckCollision(Snake* S)
{
    SnakeNode* Cursor = S->head;
    Cursor = Cursor->next;
    while(Cursor->next != 0)
    {
        if(Cursor->x == S->head->x && Cursor->y == S->head->y)
        {
            return 0;
        }
        Cursor = Cursor->next;
    }
    
    return 1;
}
*/

internal int
CheckBounds(Snake* snake, int GridWidth, int GridHeight)
{
    if (snake->Head->X  == 0 && snake->Direction == LEFT)
    {
        snake->Speed = 0;
    }
    else if(snake->Head->X + 1 == GridWidth && snake->Direction == RIGHT)
    {
        snake->Speed = 0;
    }
    else if (snake->Head->Y == 0 && snake->Direction == UP)
    {
        snake->Speed = 0;
    }
    else if (snake->Head->Y == GridHeight - 1 && snake->Direction == DOWN)
    {
        snake->Speed = 0;
    }
    else
    {
        snake->Speed = snake->MaxSpeed;
        return 1;
    }
    
    return 0;
}

#define STB_IMAGE_IMPLEMENTATION
//#define STBI_ASSERT(x)
//#define STBI_MALLOC
//#define STBI_REALLOC
//x#define STBI_FREE
#include "stb/stb_image.h"

internal void
RenderImage(game_offscreen_buffer *Buffer, Image *image)
{
    int xt = 50;
    for(int X = 0; X < image->x; ++X)
    {
        uint8 *Pixel = ((uint8 *)Buffer->Memory + X * Buffer->BytesPerPixel);
        uint8 *Color = ((uint8 *)image->data + X * image->n);
        
        for(int Y = 0; Y < image->y; ++Y)
        {
            uint32 c = *Color;
            
            int r = *Color++;
            int g = *Color++;
            int b = *Color;
            Color--;
            Color--;
            
            c = createRGB(r, g, b);
            *(uint32 *)Pixel =c;
            Pixel += Buffer->Pitch;
            Color += (image->n * image->x);
        }
    }
}



internal void
SaveImageToHeaderFile(char* filename, Image* image)
{
    char* fullDir = StringConcat("imagesaves/", filename);
    
    // Header file
    FILE *newfile = fopen(fullDir, "w");
    
    char f[sizeof(filename)];
    FilenameSearchModify(filename, f);
    char fcapital[sizeof(filename)];
    FilenameCapitalize(filename, fcapital);
    
    fprintf(newfile,
            "#ifndef %s\n"
            "#define %s\n"
            "int %sx = %d;\n"
            "int %sy = %d;\n"
            "int %sn = %d;\n"
            "const unsigned char %s[%d] = \n"
            "{\n"
            ,fcapital
            ,fcapital
            ,f
            ,image->x
            ,f
            ,image->y
            ,f
            ,image->n
            ,f
            ,(image->x * image->y * image->n));
    
    
    unsigned char* imgtosave = image->data;
    for(int i = 0; i < (image->x * image->y * image->n); i++)
    {
        fprintf(newfile,
                "%d ,",
                *imgtosave);
        *imgtosave++;
    }
    fprintf(newfile, 
            "};\n"
            "\n"
            "#endif");
    fclose(newfile);
    
    // C++ file
    char* filenamecpp = (char*)PermanentStorageAssign(fullDir, StringLength(fullDir) + 2);
    
    int j = 0;
    int extension = 0;
    char* cursor = fullDir;
    while (!extension)
    {
        if (*cursor == '.')
        {
            filenamecpp[j] = *cursor;
            extension = 1;
        }
        else
        {
            filenamecpp[j] = *cursor;
        }
        
        cursor++;
        j++;
    }
    filenamecpp[j] = 'c';
    filenamecpp[j + 1] = 'p';
    filenamecpp[j + 2] = 'p';
    filenamecpp[j + 3] = 0;
    
    FILE *newcppfile = fopen(filenamecpp, "w");
    fprintf(newcppfile, 
            "internal void\n"
            "LoadImageFrom%s(Image* image)\n"
            "{\n"
            "image->data = (unsigned char *)PermanentStorageAssign((void*)&%s, sizeof(%s));\n"
            "image->x = %sx;\n"
            "image->y = %sy;\n"
            "image->n = %sn;\n"
            "}\n"
            ,f
            ,f
            ,f
            ,f
            ,f
            ,f
            );
    
    fclose(newcppfile);
    
}


global_variable Image test;

#include "../data/imagesaves/image.h"
#include "../data/imagesaves/image.cpp"

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




internal void
RenderBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, real32 RealX, real32 RealY)
{
    int32 MinX = RoundReal32ToInt32(RealX);
    int32 MinY = RoundReal32ToInt32(RealY);
    int32 MaxX = MinX + Bitmap->Width;
    int32 MaxY = MinY + Bitmap->Height;
    
    if(MinX < 0)
    {
        MinX = 0;
    }
    
    if(MinY < 0)
    {
        MinY = 0;
    }
    
    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    
    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }
    
    uint32 *SourceRow = (uint32*)Bitmap->Memory + Bitmap->Width * (Bitmap->Height - 1);
    uint8 *DestRow = ((uint8*)Buffer->Memory +
                      MinX*Buffer->BytesPerPixel +
                      MinY*Buffer->Pitch);
    
    for(int Y = MinY; Y < MaxY; ++Y)
    {
        uint32 *Dest = (uint32*)DestRow;
        uint32 *Source = SourceRow;
        
        for(int X = MinX; X < MaxX; ++X)
        {
            real32 A = (real32)((*Source >> 24) & 0xFF) / 255.0f;
            real32 SR = (real32)((*Source >> 16) & 0xFF);
            real32 SG = (real32)((*Source >> 8) & 0xFF);
            real32 SB = (real32)((*Source >> 0) & 0xFF);
            
            real32 DR = (real32)((*Dest >> 16) & 0xFF);
            real32 DG = (real32)((*Dest >> 8) & 0xFF);
            real32 DB = (real32)((*Dest >> 0) & 0xFF);
            
            real32 R = (1.0f-A)*DR + A*SR;
            real32 G = (1.0f-A)*DG + A*SG;
            real32 B = (1.0f-A)*DB + A*SB;
            
            *Dest = (((uint32)(R + 0.5f) << 16) |
                     ((uint32)(G + 0.5f) << 8) |
                     ((uint32)(B + 0.5f) << 0));
            
            ++Dest;
            ++Source;
        }
        
        DestRow += Buffer->Pitch;
        SourceRow -= Bitmap->Width;
    }
}


loaded_bitmap yo;
Client client;

real32 BackspaceTime = 0;
int Backspace = 0;

Font Faune50 = {};
Font Faune100 = {};

real32 SnakeTimeCounter = 0;

Apple App = {};

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
        
#if SAVE_IMAGES
        // Set working directory in visual studio to the image save folder
        test.data = stbi_load("grass.jpg", &test.x, &test.y, &test.n, 0);
        SaveImageToHeaderFile("image.h", &test);
#else
        
        // Load images from imagesaves folder
        LoadImageFromimage_h(&test);
        
#endif
        //loaded_bitmap grass = LoadBMP("grass.png");
        // Where the top left corner of the grid should be for it to look
        // centered in the window
        
        
        //Rect imageRect = {0, 0, 200, 200, 0};
        unsigned char* resized = (unsigned char *)PermanentStorageBlank(imageRect.width * imageRect.height * test.n);
        stbir_resize_uint8(test.data , test.x, test.y, 0,
                           resized, imageRect.width, imageRect.height, 0, test.n);
        unsigned char* toBeFreed = test.data;
        test.data = resized;
        
#if SAVE_IMAGES
        stbi_image_free(toBeFreed);
#endif
        //yo = LoadGlyphBitmap("../Faune-TextRegular.otf", "FauneRegular", 71, 256);
        
        
        
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
        
        
        // TODO(casey): This may be more appropriate to do in the platform layer
        Memory->IsInitialized = true;
    }
    
    int intFPS = (int)(1/Input->SecondsElapsed);
    char FPS[10]; 
    sprintf(FPS, "%d", intFPS);
    Rect FPSRect = {10, 10, 100, 50, 0};
    
    int btnPress = -1;
    int tbPress = -1;
    
    UpdateNewGUI(&MainMenu, Buffer->Width, Buffer->Height);
    
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
            // NOTE(casey): Use digital movement tuning
            if(Controller->MoveLeft.EndedDown)
            {
                if (LEFT  == Player.LastMoveDirection)
                {
                    if (Player.LastMoveDirection != RIGHT)
                        Player.Direction = LEFT;
                    
                    Player.NextDirection = -1;
                }
                else
                {
                    if (Player.LastMoveDirection != RIGHT)
                        Player.NextDirection = LEFT;
                }
            }
            
            if(Controller->MoveRight.EndedDown)
            {
                if (RIGHT  == Player.LastMoveDirection)
                {
                    if (Player.LastMoveDirection != LEFT)
                        Player.Direction = RIGHT;
                    
                    Player.NextDirection = -1;
                }
                else
                {
                    if (Player.LastMoveDirection != LEFT)
                        Player.NextDirection = RIGHT;
                }
            }
            
            if(Controller->MoveDown.EndedDown)
            {
                if (DOWN == Player.LastMoveDirection)
                {
                    if (Player.LastMoveDirection != UP)
                        Player.Direction = DOWN;
                    
                    Player.NextDirection = -1;
                }
                else
                {
                    if (Player.LastMoveDirection != UP)
                        Player.NextDirection = DOWN;
                }
            }
            
            if(Controller->MoveUp.EndedDown)
            {
                if (UP == Player.LastMoveDirection)
                {
                    if (Player.LastMoveDirection != DOWN)
                        Player.Direction = UP;
                    
                    Player.NextDirection = -1;
                }
                else
                {
                    if (Player.LastMoveDirection != DOWN)
                        Player.NextDirection = UP;
                }
            }
            //sprintf(FPS, "%d", Player.Direction);
            //PrintOnScreen(Buffer, &Faune50, FPS, FPSRect.x, FPSRect.y, 0xFF000000, &FPSRect);
            
            if(Input->MouseButtons[0].EndedDown)
            {
                btnPress = CheckButtonsClick(&MainMenu, Input->MouseX, Input->MouseY);
                
                tbPress = CheckTextBoxes(&MainMenu, Input->MouseX, Input->MouseY);
            }
            
            if(Controller->Zero.EndedDown)
            {
                AddCharTextBoxText(&MainMenu, "0");
            }
            if(Controller->One.EndedDown)
            {
                AddCharTextBoxText(&MainMenu, "1");
            }
            if(Controller->Two.EndedDown)
            {
                AddCharTextBoxText(&MainMenu, "2");
            }
            if(Controller->Three.EndedDown)
            {
                AddCharTextBoxText(&MainMenu, "3");
            }
            if(Controller->Four.EndedDown)
            {
                AddCharTextBoxText(&MainMenu, "4");
            }
            if(Controller->Five.EndedDown)
            {
                AddCharTextBoxText(&MainMenu, "5");
            }
            if(Controller->Six.EndedDown)
            {
                AddCharTextBoxText(&MainMenu, "6");
            }
            if(Controller->Seven.EndedDown)
            {
                AddCharTextBoxText(&MainMenu, "7");
            }
            if(Controller->Eight.EndedDown)
            {
                AddCharTextBoxText(&MainMenu, "8");
            }
            if(Controller->Nine.EndedDown)
            {
                AddCharTextBoxText(&MainMenu, "9");
            }
            if(Controller->Period.EndedDown)
            {
                AddCharTextBoxText(&MainMenu, ".");
            }
            if(Controller->Back.EndedDown)
            {
                if (Controller->Back.HalfTransitionCount == 1)
                {
                    RemoveCharTextBoxText(&MainMenu);
                }
            }
            
        }
    }
    
    // TODO(casey): Allow sample offsets here for more robust platform options
    //GameOutputSound(SoundBuffer, GameState->ToneHz);
    
    if (btnPress == GameStart)
    {
        GameState->Menu = 0;
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
    if (GameState->Menu == 2)
    {
        if (LoseMenu.Initialized == 0)
        {
            LoseMenu.Padding = 0;
            
            int Y = 0;
            
            NewText txt = 
            {
                "YOU LOST",    // Text
                Btn1,       // ID
                &Faune100,   // Font
                0xFF000000, // TextColor
            };
            AddNewText(&LoseMenu, 0, Y++,  &txt);
            
            InitializeNewGUI(&LoseMenu);
            MainMenu.Initialized = 1;
        }
        
        CheckButtonsHover(&LoseMenu, Input->MouseX, Input->MouseY);
        
        ClearScreen(Buffer);
        RenderNewGUI(Buffer, &LoseMenu);
    }
    else if (GameState->Menu == 1)
    {
        
        if (MainMenu.Initialized == 0)
        {
            MainMenu.Padding = 10;
            
            int Y = 0;
            
            NewText TXT = {};
            NewButton btn = {};
            NewTextBox tb = {};
            
            TXT.Text = "SINGLEPLAYER";
            TXT.ID = Btn1;
            TXT.FontType = &Faune100;
            TXT.TextColor = 0xFF000000;
            AddNewText(&MainMenu, 0, Y++,  &TXT);
            
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
            AddNewButton(&MainMenu, 0, Y++, 300, 100, &btn);
            
            TXT = 
            {
                "0",    // Text
                Btn1,       // ID
                &Faune100,   // Font
                0xFFFFFFFF, // TextColor
            };
            AddNewText(&MainMenu, 0, Y++,  &TXT);
            
            TXT = 
            {
                "MULTIPLAYER",    // Text
                Btn1,       // ID
                &Faune100,   // Font
                0xFF000000, // TextColor
            };
            AddNewText(&MainMenu, 0, Y++,  &TXT);
            
            TXT = 
            {
                "IP:",    // Text
                Btn1,       // ID
                &Faune50,   // Font
                0xFF000000, // TextColor
            };
            AddNewText(&MainMenu, 0, Y,  &TXT);
            
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
            AddNewTextBox(&MainMenu, 1, Y++, 500, 50, &tb);
            
            TXT = 
            {
                "PORT:",    // Text
                Btn1,       // ID
                &Faune50,   // Font
                0xFF000000, // TextColor
            };
            AddNewText(&MainMenu, 0, Y,  &TXT);
            
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
            AddNewTextBox(&MainMenu, 1, Y++, 500, 50, &tb);
            
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
            AddNewButton(&MainMenu, 0, Y++, 200, 100, &btn);
            
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
            AddNewButton(&MainMenu, 0, Y++, 200, 100, &btn);
            
            InitializeNewGUI(&MainMenu);
            MainMenu.Initialized = 1;
        }
        
        CheckButtonsHover(&MainMenu, Input->MouseX, Input->MouseY);
        
        ClearScreen(Buffer);
        RenderNewGUI(Buffer, &MainMenu);
        //Clear();
        //RenderTriangle();
    }
    else if (GameState->Menu == 0)
    {
        if (GameInitialized == false)
        {
            NewInitializeSnake(&Player);
            GameInitialized = true;
            
            App.X = rand() % 17;
            App.Y = rand() % 17;
            
            Player.MaxSpeed = 9.0f; // m/s
            Player.Speed = Player.MaxSpeed;
            //createClient(&client, "192.168.1.75", "10109", TCP);
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
        
        
        ClearScreen(Buffer);
        
        
        NewTransitionSnake(&Player, Input->SecondsElapsed);
        if (Player.DistanceTravelled >= 1 || Player.Speed == 0)
        {
            Player.DistanceTravelled = 0;
            NewMoveSnake(&Player);
            CheckGetApple(&Player, &App);
        }
        
        CheckBounds(&Player, GRIDWIDTH, GRIDHEIGHT);
        
        RenderBackgroundGrid(Buffer, centeredX, centeredY, GRIDWIDTH, GRIDHEIGHT, GRIDSIZE, &test);
        RenderApple(Buffer, &App, centeredX, centeredY, GRIDWIDTH, GRIDHEIGHT, GRIDSIZE);
        NewRenderSnake(Buffer, &Player, centeredX, centeredY, GRIDWIDTH, GRIDHEIGHT, GRIDSIZE);
        PrintOnScreen(Buffer, &Faune50, IntToString(App.Score), 5, 5, 0xFF000000);
    }
}
