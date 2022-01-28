#ifndef GUI_H
#define GUI_H

struct Text
{
    int X;
    int Y;
    int Width;
    int Height;
    char* Text;
    int ID;
    Font* FontType;
    uint32 TextColor;
};

struct Button
{
    int X;
    int Y;
    int Width;
    int Height;
    char* Text;
    Font* FontType;
    int ID;
    uint32 Color;
    uint32 RegularColor;
    uint32 HoverColor;
    uint32 TextColor;
};


struct TextBox
{
    int X;
    int Y;
    int Width;
    int Height;
    char* Text;
    Font* FontType;
    int ID;
    int ShowCursor;
    
    uint32 Color;
    uint32 TextColor;
};

struct GUIComponent
{
    GUIComponent* next;
    void* component;
};


enum ComponentID
{
    GameStart,
    Quit,
    Join,
    TextIP,
    TextBoxIP,
    TextPort,
    TextBoxPort,
    Singleplayer
};
struct GUI
{
    int initialized;
    GUIComponent* buttons;
    GUIComponent* Texts;
    GUIComponent* TextBoxes;
};

internal int
StringLength(char* String);

internal char*
StringConcat(char* Source, char* Add);

enum NewComponentIDs
{
    Btn1,
    Btn2,
    Btn3,
    Btn4,
    PORT,
    IP
};

struct NewButton
{
    char* Text;
    Font* FontType;
    int ID;
    uint32 Color;
    uint32 RegularColor;
    uint32 HoverColor;
    uint32 TextColor;
};

struct NewText
{
    char* Text;
    int ID;
    Font* FontType;
    uint32 TextColor;
};

struct NewTextBox
{
    char* Text;
    Font* FontType;
    int ID;
    int ShowCursor;
    
    uint32 Color;
    uint32 TextColor;
};

struct NewGUIComponent
{
    int X;
    int Y;
    
    int GridX;
    int GridY;
    int Width;
    int Height;
    
    int WidthP;
    int HeightP;
    
    NewGUIComponent* Next;
    NewGUIComponent* All;
    void* Data;
};

struct Column
{
    int Width;
};

struct Row
{
    int Width = 0;
    int Height = 0;
    Column Columns[10];
};

struct NewGUI
{
    int Initialized;
    Row Rows[10];
    
    int Width = 0;
    int Height = 0;
    int Padding = 0;
    NewGUIComponent* All;
    NewGUIComponent* Buttons;
    NewGUIComponent* TextBoxes;
    NewGUIComponent* Texts;
};


#endif //GUI_H
