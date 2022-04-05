#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// Dimensions for the screen and playfield
const int n_screenWidth  = 240;
const int n_screenHeight = 67;
const int n_fontW        = 8;
const int n_fontH        = 16;
const int n_pfWidth      = n_screenWidth / 2;
const int n_pfHeight     = n_screenHeight;

// Playfield array and helping array to save the new state
bool playField        [n_pfWidth * n_pfHeight]{ 0 };
bool playFieldNewState[n_pfWidth * n_pfHeight]{ 0 };

// just makes life easier
int nGeneration = 0;
int nPopulation = 0;

// Helpful vector of 8 directions and function to determine if a position is inside the screen
std::vector<std::pair<int, int> > directions = { {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1}, {0,1} };
bool inside(int y, int x) { return 0 <= y && y < n_pfHeight && 0 <= x && x < n_pfWidth; }


void NextGeneration()
{
    // Double nested for() to loop over every cell in the play field
    for (int x = 0; x < n_pfWidth; x++)
    {
        for (int y = 0; y < n_pfHeight; y++)
        {
            /* The for() below counts how many live neughbours the current cell has */
            int n_liveNeighbours = 0;
            for (auto dir : directions)
            {
                int newX = x + dir.first;
                int newY = y + dir.second;

                if (inside(newY, newX) && playField[newY * n_pfWidth + newX])
                    n_liveNeighbours++;
            }

            /* These 4 if() are the cream and crop of the logic */
            if      ( playField[y * n_pfWidth + x] && n_liveNeighbours <  2)  playFieldNewState[y * n_pfWidth + x] = 0;
            else if ( playField[y * n_pfWidth + x] && n_liveNeighbours >  3)  playFieldNewState[y * n_pfWidth + x] = 0;
            else if (!playField[y * n_pfWidth + x] && n_liveNeighbours == 3)  playFieldNewState[y * n_pfWidth + x] = 1;
            else if ( playField[y * n_pfWidth + x]                         )  playFieldNewState[y * n_pfWidth + x] = 1;

            // PS we set the new state of each cell in a seperate array the size of our play field and then we just copy it over
        }
    }
    // This is where the new states are copied over
    memcpy(playField, playFieldNewState, sizeof(playField));
}
void PutString(CHAR_INFO* buf, int x, int y, std::wstring info)
{
    for (int i = 0; i < info.length(); i++)
        if (playField[y * n_pfWidth + (i + x) / 2])
            buf[y * n_screenWidth + i + x].Char.UnicodeChar = info[i],
            buf[y * n_screenWidth + i + x].Attributes = 0x00F0;
        else
            buf[y * n_screenWidth + i + x].Char.UnicodeChar = info[i],
            buf[y * n_screenWidth + i + x].Attributes = 0x000F;
}
void LoadFromFile(bool pf[], std::wstring sFile)
{
    std::ifstream input;
    input.open(sFile.c_str(), std::ios::in);

    if (!input.is_open()) return;


    memset(playField,         0, sizeof(bool) * n_pfWidth * n_pfHeight);
    memset(playFieldNewState, 0, sizeof(bool) * n_pfWidth * n_pfHeight);

    int x = 0;
    int y = 0;
    std::string line;
    while (getline(input, line))
    {
        x = 0;
        for (char c : line)
        {
            pf[y * n_pfWidth + x] = (c == '#');
            x++;
        }
        y++;
    }

    nGeneration = 0;
    input.close();
}

int main()
{
    // Setting up console
    CHAR_INFO*   screen     = new CHAR_INFO[n_screenHeight * n_screenWidth]{ 0 };
    HANDLE       hConsole   = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE       hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
    SMALL_RECT   rectWindow;
    INPUT_RECORD InputRecord;
    DWORD        Events;

    rectWindow = { 0,0,1,1 };
    SetConsoleWindowInfo(hConsole, TRUE, &rectWindow);
    COORD coord = { (short)n_screenWidth, (short)n_screenHeight };
    SetConsoleScreenBufferSize(hConsole, coord);
    SetConsoleActiveScreenBuffer(hConsole);
    rectWindow = { 0,0,(short)n_screenWidth - 1, (short)n_screenHeight - 1 };
    SetConsoleWindowInfo(hConsole, TRUE, &rectWindow);

    //CONSOLE_SCREEN_BUFFER_INFO csbi;
    //GetConsoleScreenBufferInfo(hConsole, &csbi);

    // Setting up font size and dimensions
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize = { n_fontW, n_fontH };
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(hConsole, false, &cfi);

    //Setting console mode for reading input
    SetConsoleMode(hConsoleIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
    SetConsoleDisplayMode(hConsole, CONSOLE_FULLSCREEN_MODE, 0);

    // Gotta run this once to avoid reading from an empty Input Record
    ReadConsoleInput(hConsoleIn, &InputRecord, 1, &Events);


    // VARIABLES
    bool bSimulationToggle = false;
    bool bStep             = false;
    int  nCounter          = 0;

    bool currStateRIGHT = false;
    bool prevStateRIGHT = false;

    // MAIN LOOP
    while(true)
    {
        GetNumberOfConsoleInputEvents(hConsoleIn, &Events);
        if (Events > 0) // We only want to read input if there is any, if not then the ReadConsoleInput() function is gonna block
            ReadConsoleInput(hConsoleIn, &InputRecord, 1, &Events);
     
        // MOUSE INPUT
        int mouseX = InputRecord.Event.MouseEvent.dwMousePosition.X;
        int mouseY = InputRecord.Event.MouseEvent.dwMousePosition.Y;
        if (InputRecord.Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) playField        [mouseY * n_pfWidth + mouseX / 2] = 1, nGeneration = 0;
        if (InputRecord.Event.MouseEvent.dwButtonState == RIGHTMOST_BUTTON_PRESSED)     playField        [mouseY * n_pfWidth + mouseX / 2] = 0, nGeneration = 0,
                                                                                        playFieldNewState[mouseY * n_pfWidth + mouseX / 2] = 0;

        // KEYBOARD INPUT
        if (GetAsyncKeyState(VK_CONTROL)) bSimulationToggle = false;
        if (GetAsyncKeyState(VK_SPACE))   bSimulationToggle = true;
        if (GetAsyncKeyState(0x43)) // C
            memset(playField,         0, sizeof(bool) * n_pfWidth * n_pfHeight),
            memset(playFieldNewState, 0, sizeof(bool) * n_pfWidth * n_pfHeight),
            nGeneration = 0;


        bStep = false;
        prevStateRIGHT = currStateRIGHT;
        currStateRIGHT = GetAsyncKeyState(VK_RIGHT);
        if (!prevStateRIGHT && currStateRIGHT) bStep = true;

        // Load presets
        if (GetAsyncKeyState(0x31)) // 1
            LoadFromFile(playField, L"Oscilators.txt");
        if (GetAsyncKeyState(0x32)) // 2
            LoadFromFile(playField, L"Gosper glider gun.txt");
        if (GetAsyncKeyState(0x33)) // 3
            LoadFromFile(playField, L"Simkin glider gun.txt");
        if (GetAsyncKeyState(0x34)) // 4
            LoadFromFile(playField, L"Advanced oscilator 1.txt");
        if (GetAsyncKeyState(0x35)) // 5
            LoadFromFile(playField, L"Advanced oscilator 2.txt");
        if (GetAsyncKeyState(0x36)) // 6
            LoadFromFile(playField, L"Filter stream.txt");
        if (GetAsyncKeyState(0x37)) // 7
            LoadFromFile(playField, L"Spaceships.txt");
        if (GetAsyncKeyState(0x38)) // 8
            LoadFromFile(playField, L"Reflection.txt");

        // LOGIC
        if (bSimulationToggle)
        {
            if (nCounter >= 25) // Only itterate a step of the simulation twentieth of a second (roughly; doesn't account for CPU usage fluctuation)
            {
                NextGeneration();
                nCounter = 0;
                nGeneration++;
            }
            nCounter++;
        }
        else if (bStep) // Advance a single generation
        {
            NextGeneration();
            nGeneration++;
        }


        nPopulation = 0;
        // SETTING CHARACTERS ... and counting population because i don't want to have 2 damn loops
        memset(screen, 0, sizeof(CHAR_INFO) * n_screenWidth * n_screenHeight);
        for (int x = 0; x < n_screenWidth; x++)
            for (int y = 0; y < n_screenHeight; y++)
                if (playField[y * n_pfWidth + x / 2])
                    screen[y * n_screenWidth + x].Attributes = 0x00F0,
                    nPopulation++;


        // ONSCREEN TEXT
        wchar_t s[240];
        swprintf_s(s, 240, L"Generation: %d Population: %d", nGeneration, nPopulation / 2);
        PutString(screen, 0, 0, s);

        PutString(screen, 0, 2,  L"Examples");
        PutString(screen, 0, 3,  L"1 Oscilators");
        PutString(screen, 0, 4,  L"2 Gosper glider gun");
        PutString(screen, 0, 5,  L"3 Simkin glider gun");
        PutString(screen, 0, 6,  L"4 Advanced oscilator 1");
        PutString(screen, 0, 7,  L"5 Advanced oscilator 2");
        PutString(screen, 0, 8,  L"6 Filter stream");
        PutString(screen, 0, 9,  L"7 Spaceships");
        PutString(screen, 0, 10, L"8 Reflection");

        PutString(screen, 0, 59, L"C - Clear screen");
        PutString(screen, 0, 60, L"LMB - Add live cell");
        PutString(screen, 0, 61, L"RMB - Remove live cell");
        PutString(screen, 0, 62, L"1-8 - Choose preset");
        PutString(screen, 0, 63, L"SPACE - Start");
        PutString(screen, 0, 64, L"CTRL - Stop");
        PutString(screen, 0, 65, L"ESC - Close program");
        PutString(screen, 0, 66, L"RIGHT ARROW - Step");


        // DRAWING
        WriteConsoleOutput(hConsole, screen, { (short)n_screenWidth, (short)n_screenHeight }, { 0,0 }, &rectWindow);


        // TERMINATE
        if (GetAsyncKeyState(VK_ESCAPE))
        {
            delete[] screen;
            return 0;
        }
    }
}