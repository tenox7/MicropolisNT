/* Main entry point for MicropolisNT (Windows NT version)
 * Based on original Micropolis code from MicropolisLegacy project
 */

#include "sim.h"
#include "tools.h"
#include <commdlg.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifndef WS_EX_CLIENTEDGE
#define WS_EX_CLIENTEDGE 0
#endif

#ifndef MB_ICONWARNING
#define MB_ICONWARNING  MB_ICONASTERISK
#define MB_ICONERROR    MB_ICONSTOP
#endif

#define IDM_FILE_NEW 1001
#define IDM_FILE_OPEN 1002
#define IDM_FILE_EXIT 1003
#define IDM_TILESET_BASE 2000
#define IDM_TILESET_MAX 2100
#define IDM_SIM_PAUSE 3001
#define IDM_SIM_SLOW 3002
#define IDM_SIM_MEDIUM 3003
#define IDM_SIM_FAST 3004

/* Scenario menu IDs */
#define IDM_SCENARIO_BASE 4000
#define IDM_SCENARIO_DULLSVILLE 4001
#define IDM_SCENARIO_SANFRANCISCO 4002
#define IDM_SCENARIO_HAMBURG 4003
#define IDM_SCENARIO_BERN 4004
#define IDM_SCENARIO_TOKYO 4005
#define IDM_SCENARIO_DETROIT 4006
#define IDM_SCENARIO_BOSTON 4007
#define IDM_SCENARIO_RIO 4008

/* View menu IDs */
#define IDM_VIEW_INFOWINDOW 4100
#define IDM_VIEW_LOGWINDOW 4101
#define IDM_VIEW_POWER_OVERLAY 4102
#define IDM_VIEW_DEBUG_LOGS 4103

/* Info window definitions */
#define INFO_WINDOW_CLASS "MicropolisInfoWindow"
#define INFO_WINDOW_WIDTH 300
#define INFO_WINDOW_HEIGHT 200
#define INFO_TIMER_ID 2
#define INFO_TIMER_INTERVAL 500 /* Update info window every 500ms */

/* Log window definitions */
#define LOG_WINDOW_CLASS "MicropolisLogWindow"
#define LOG_WINDOW_WIDTH 400
#define LOG_WINDOW_HEIGHT 300
#define LOG_TEXT_ID 101      /* ID for the text edit control */
#define MAX_LOG_BUFFER 32000 /* Maximum size for the log text buffer */
#define MAX_LOG_ENTRIES 100  /* Maximum number of log entries to keep */

/* Tool menu IDs */
#define IDM_TOOL_BASE 5000
#define IDM_TOOL_BULLDOZER 5001
#define IDM_TOOL_ROAD 5002
#define IDM_TOOL_RAIL 5003
#define IDM_TOOL_WIRE 5004
#define IDM_TOOL_PARK 5005
#define IDM_TOOL_RESIDENTIAL 5006
#define IDM_TOOL_COMMERCIAL 5007
#define IDM_TOOL_INDUSTRIAL 5008
#define IDM_TOOL_FIRESTATION 5009
#define IDM_TOOL_POLICESTATION 5010
#define IDM_TOOL_STADIUM 5011
#define IDM_TOOL_SEAPORT 5012
#define IDM_TOOL_POWERPLANT 5013
#define IDM_TOOL_NUCLEAR 5014
#define IDM_TOOL_AIRPORT 5015
#define IDM_TOOL_QUERY 5016

/* Define needed for older Windows SDK compatibility */
#ifndef LR_CREATEDIBSECTION
#define LR_CREATEDIBSECTION 0x2000
#endif

#ifndef LR_DEFAULTCOLOR
#define LR_DEFAULTCOLOR 0
#endif

#ifndef LR_DEFAULTSIZE
#define LR_DEFAULTSIZE 0x0040
#endif

#ifndef LR_LOADFROMFILE
#define LR_LOADFROMFILE 0x0010
#endif

#ifndef IMAGE_BITMAP
#define IMAGE_BITMAP 0
#endif

#define TILE_SIZE 16

/* Main map and history arrays - now defined in simulation.h/c */
short Map[WORLD_Y][WORLD_X];
short ResHis[HISTLEN / 2];
short ComHis[HISTLEN / 2];
short IndHis[HISTLEN / 2];
short CrimeHis[HISTLEN / 2];
short PollutionHis[HISTLEN / 2];
short MoneyHis[HISTLEN / 2];
short MiscHis[MISCHISTLEN / 2];

HWND hwndMain = NULL; /* Main window handle - used by other modules */
HWND hwndInfo = NULL; /* Info window handle for displaying city stats */
HWND hwndLog = NULL;  /* Log window handle for displaying game events */

/* Log window variables */
HWND hwndLogText = NULL;        /* Handle to the edit control in the log window */
char logBuffer[MAX_LOG_BUFFER]; /* Buffer to hold all log text */
int logBufferPos = 0;           /* Current position in log buffer */
int showDebugLogs = 1; /* Flag to control whether debug logs are shown (enabled by default) */
static HBITMAP hbmBuffer = NULL;
static HDC hdcBuffer = NULL;
static HBITMAP hbmTiles = NULL;
static HDC hdcTiles = NULL;
static HPALETTE hPalette = NULL;

static int cxClient = 0;
static int cyClient = 0;
static int xOffset = 0;
static int yOffset = 0;
static int toolbarWidth = 108; /* 3-column toolbar width */

static BOOL isMouseDown = FALSE; /* Used for map dragging */
static int lastMouseX = 0;
static int lastMouseY = 0;

char cityFileName[MAX_PATH]; /* Current city filename - used by other modules */
static HMENU hMenu = NULL;
static HMENU hFileMenu = NULL;
static HMENU hTilesetMenu = NULL;
static HMENU hSimMenu = NULL;
static HMENU hScenarioMenu = NULL;
static HMENU hToolMenu = NULL;
static char currentTileset[MAX_PATH] = "classic";
static int powerOverlayEnabled = 0; /* Power overlay display toggle */

/* External reference to scenario variables (defined in scenarios.c) */
extern short ScenarioID;    /* Current scenario ID (0 = none) */
extern short DisasterEvent; /* Current disaster type */
extern short DisasterWait;  /* Countdown to next disaster */
extern short ScoreType;     /* Score type for scenario */
extern short ScoreWait;     /* Score wait for scenario */

/* Micropolis tile flags - These must match simulation.h */
/* Using LOMASK from simulation.h */
/* Use the constants from simulation.h for consistency */

/* Tile type constants */
#define TILE_DIRT 0
#define TILE_RIVER 2
#define TILE_REDGE 3
#define TILE_CHANNEL 4
#define TILE_FIRSTRIVEDGE 5
#define TILE_LASTRIVEDGE 20
#define TILE_WATER_LOW TILE_RIVER
#define TILE_WATER_HIGH TILE_LASTRIVEDGE

#define TILE_TREEBASE 21
#define TILE_WOODS_LOW TILE_TREEBASE
#define TILE_LASTTREE 36
#define TILE_WOODS 37
#define TILE_UNUSED_TRASH1 38
#define TILE_UNUSED_TRASH2 39
#define TILE_WOODS_HIGH TILE_UNUSED_TRASH2
#define TILE_WOODS2 40
#define TILE_WOODS3 41
#define TILE_WOODS4 42
#define TILE_WOODS5 43

#define TILE_RUBBLE 44
#define TILE_LASTRUBBLE 47

#define TILE_FLOOD 48
#define TILE_LASTFLOOD 51

#define TILE_RADTILE 52

#define TILE_FIRE 56
#define TILE_FIREBASE TILE_FIRE
#define TILE_LASTFIRE 63

#define TILE_ROADBASE 64
#define TILE_HBRIDGE 64
#define TILE_VBRIDGE 65
#define TILE_ROADS 66
#define TILE_INTERSECTION 76
#define TILE_HROADPOWER 77
#define TILE_VROADPOWER 78
#define TILE_LASTROAD 206

#define TILE_POWERBASE 208
#define TILE_HPOWER 208
#define TILE_VPOWER 209
#define TILE_LHPOWER 210
#define TILE_LVPOWER 211
#define TILE_LASTPOWER 222

#define TILE_RAILBASE 224
#define TILE_HRAIL 224
#define TILE_VRAIL 225
#define TILE_LASTRAIL 238

#define TILE_RESBASE 240
#define TILE_RESCLR 244
#define TILE_HOUSE 249
#define TILE_LASTRES 420

#define TILE_COMBASE 423
#define TILE_COMCLR 427
#define TILE_LASTCOM 611

/* Tileset constants - commented out because they're defined elsewhere
   If you need to modify these, update the definitions where they are first defined
 #define TILE_TOTAL_COUNT   1024  Maximum number of tiles in the tileset
 #define TILES_IN_ROW       32    Number of tiles per row in the tileset bitmap */
#ifndef TILE_TOTAL_COUNT
#define TILE_TOTAL_COUNT 1024 /* Maximum number of tiles in the tileset */
#endif
#ifndef TILES_IN_ROW
#define TILES_IN_ROW 32 /* Number of tiles per row in the tileset bitmap */
#endif

#define TILE_INDBASE 612
#define TILE_INDCLR 616
#define TILE_LASTIND 692

#define TILE_PORTBASE 693
#define TILE_PORT 698
#define TILE_LASTPORT 708

#define TILE_AIRPORTBASE 709
#define TILE_AIRPORT 716
#define TILE_LASTAIRPORT 744

#define TILE_COALBASE 745
#define TILE_POWERPLANT 750
#define TILE_LASTPOWERPLANT 760

#define TILE_FIRESTBASE 761
#define TILE_FIRESTATION 765
#define TILE_LASTFIRESTATION 769

#define TILE_POLICESTBASE 770
#define TILE_POLICESTATION 774
#define TILE_LASTPOLICESTATION 778

#define TILE_STADIUMBASE 779
#define TILE_STADIUM 784
#define TILE_LASTSTADIUM 799

#define TILE_NUCLEARBASE 811
#define TILE_NUCLEAR 816
#define TILE_LASTNUCLEAR 826

#ifndef TILE_TOTAL_COUNT
#define TILE_TOTAL_COUNT 960
#endif

#define TILES_IN_ROW 32
#define TILES_PER_ROW 32
LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK infoWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK logWndProc(HWND, UINT, WPARAM, LPARAM);

/* Function to add a game log entry */
void addGameLog(const char *format, ...);
/* Function to add a debug log entry */
void addDebugLog(const char *format, ...);
void initializeGraphics(HWND hwnd);
void cleanupGraphics(void);
int loadCity(char *filename);
int loadFile(char *filename);
void drawCity(HDC hdc);
void drawTile(HDC hdc, int x, int y, short tileValue);
int getBaseFromTile(short tile);
void swapShorts(short *buf, int len);
void resizeBuffer(int cx, int cy);
void scrollView(int dx, int dy);
void openCityDialog(HWND hwnd);
HBITMAP loadBitmapFile(const char *filename);
int loadTileset(const char *filename);
HPALETTE createSystemPalette(void);
HMENU createMainMenu(void);
void populateTilesetMenu(HMENU hSubMenu);
int changeTileset(HWND hwnd, const char *tilesetName);
void ForceFullCensus(void);
void createNewMap(HWND hwnd);

/* External functions - defined in simulation.c */
extern int SimRandom(int range);
extern void SetValves(int r, int c, int i);
extern const char *GetCityClassName(void);
extern void RandomlySeedRand(void);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc, wcInfo;
    MSG msg;
    RECT rect;
    int mainWindowX, mainWindowY;

    /* Register main window class */
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc = wndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MicropolisNT";

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    /* Register info window class */
    wcInfo.style = CS_HREDRAW | CS_VREDRAW;
    wcInfo.lpfnWndProc = infoWndProc;
    wcInfo.cbClsExtra = 0;
    wcInfo.cbWndExtra = 0;
    wcInfo.hInstance = hInstance;
    wcInfo.hIcon = NULL;
    wcInfo.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcInfo.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcInfo.lpszMenuName = NULL;
    wcInfo.lpszClassName = INFO_WINDOW_CLASS;

    if (!RegisterClass(&wcInfo)) {
        MessageBox(NULL, "Info Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    /* Register log window class */
    wcInfo.lpfnWndProc = logWndProc;
    wcInfo.lpszClassName = LOG_WINDOW_CLASS;

    if (!RegisterClass(&wcInfo)) {
        MessageBox(NULL, "Log Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hMenu = createMainMenu();

    /* Create main window */
    hwndMain = CreateWindowEx(WS_EX_CLIENTEDGE, "MicropolisNT", "MicropolisNT - Tileset: classic",
                              WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
                              908, 600, /* Additional 108px width for the 3-column toolbar */
                              NULL, hMenu, hInstance, NULL);

    if (hwndMain == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    /* Get main window position to position info window appropriately */
    GetWindowRect(hwndMain, &rect);
    mainWindowX = rect.left;
    mainWindowY = rect.top;

    /* Create info window */
    hwndInfo =
        CreateWindowEx(WS_EX_CLIENTEDGE, INFO_WINDOW_CLASS, "Micropolis Info",
                       WS_OVERLAPPEDWINDOW | WS_VISIBLE, mainWindowX + rect.right - rect.left + 10,
                       mainWindowY, /* Position to right of main window */
                       INFO_WINDOW_WIDTH, INFO_WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);

    if (hwndInfo == NULL) {
        MessageBox(NULL, "Info Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        /* Continue anyway, just without the info window */
    } else {
        /* Start timer to update info window */
        SetTimer(hwndInfo, INFO_TIMER_ID, INFO_TIMER_INTERVAL, NULL);
    }

    /* Initialize log system before creating the window */
    logBufferPos = 0;
    logBuffer[0] = '\0';

    /* Create log window */
    hwndLog = CreateWindowEx(
        WS_EX_CLIENTEDGE, LOG_WINDOW_CLASS, "Micropolis Log",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,                       /* Visible by default */
        mainWindowX, mainWindowY + rect.bottom - rect.top + 10, /* Position below main window */
        LOG_WINDOW_WIDTH, LOG_WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);

    if (hwndLog == NULL) {
        MessageBox(NULL, "Log Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        /* Continue anyway, just without the log window */
    }

    /* Initialize graphics first */
    initializeGraphics(hwndMain);
    
    /* Then create new map (this will set the tileset to classic) */
    createNewMap(hwndMain);

    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);

    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    cleanupGraphics();

    /* Clean up info window timer */
    if (hwndInfo) {
        KillTimer(hwndInfo, INFO_TIMER_ID);
    }

    return msg.wParam;
}

/**
 * Adds an entry to the game log
 */
void addGameLog(const char *format, ...) {
    va_list args;
    char buffer[512];
    char timeBuffer[64];
    SYSTEMTIME st;
    int len;
    int textLen;

    /* Only proceed if we have a text control */
    if (!hwndLogText) {
        return;
    }

    /* Get current time */
    GetLocalTime(&st);
    sprintf(timeBuffer, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);

    /* Format message */
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    /* Add time prefix + message + newline to the log buffer */
    len = lstrlen(timeBuffer) + lstrlen(buffer) + 2; /* +2 for newline and null terminator */

    /* Check if we need to make room in the buffer */
    if (logBufferPos + len >= MAX_LOG_BUFFER) {
        /* Buffer is full, remove some old text */
        int amountToRemove = MAX_LOG_BUFFER / 4; /* Remove 25% of the buffer */

        /* Find the position after several newlines */
        int i;
        for (i = amountToRemove; i < logBufferPos; i++) {
            if (logBuffer[i] == '\n') {
                amountToRemove = i + 1;
                break;
            }
        }

        /* Shift buffer */
        memmove(logBuffer, logBuffer + amountToRemove, logBufferPos - amountToRemove);
        logBufferPos -= amountToRemove;
    }

    /* Add new entry to buffer */
    strcpy(logBuffer + logBufferPos, timeBuffer);
    logBufferPos += lstrlen(timeBuffer);

    strcpy(logBuffer + logBufferPos, buffer);
    logBufferPos += lstrlen(buffer);

    /* Add newline */
    logBuffer[logBufferPos++] = '\r';
    logBuffer[logBufferPos++] = '\n';
    logBuffer[logBufferPos] = '\0';

    /* Update the text control */
    SetWindowText(hwndLogText, logBuffer);

    /* Scroll to the bottom */
    textLen = GetWindowTextLength(hwndLogText);
    SendMessage(hwndLogText, EM_SETSEL, textLen, textLen);
    SendMessage(hwndLogText, EM_SCROLLCARET, 0, 0);
}

/**
 * Adds a debug entry to the game log (only visible when debug logging is enabled)
 */
void addDebugLog(const char *format, ...) {
    va_list args;
    char buffer[512];
    char debugPrefix[16] = "[DEBUG] ";

    /* Only process debug logs if debug logging is enabled */
    if (!showDebugLogs || !hwndLogText) {
        return;
    }

    /* Format debug message */
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    /* Create a new message with debug prefix */
    {
        char fullMessage[512];
        strcpy(fullMessage, debugPrefix);
        strcat(fullMessage, buffer);

        /* Add to log with the debug prefix (addGameLog will add the timestamp) */
        addGameLog("%s", fullMessage);
    }
}

/**
 * Log window procedure - handles messages for the log window
 */
LRESULT CALLBACK logWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    RECT clientRect;
    HMENU hMenu;
    HMENU hViewMenu;
    HFONT hFont;

    switch (msg) {
    case WM_CREATE: {
        /* Get client area dimensions */
        GetClientRect(hwnd, &clientRect);

        /* Create the edit control for the log - fill the entire window */
        hwndLogText = CreateWindowEx(
            WS_EX_CLIENTEDGE, "EDIT", "",
            WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 0,
            0,                                   /* Position - start at the top-left corner */
            clientRect.right, clientRect.bottom, /* Size - full client area */
            hwnd, (HMENU)LOG_TEXT_ID, NULL, NULL);

        if (!hwndLogText) {
            MessageBox(hwnd, "Failed to create log text control", "Error", MB_OK | MB_ICONERROR);
            return -1;
        }

        /* Set default font */
		#if(WINVER >= 0x0400)
			hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		#else
			hFont = (HFONT)GetStockObject(SYSTEM_FIXED_FONT);
		#endif
        if (hFont) {
            SendMessage(hwndLogText, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
        }

        /* Initialize log buffer */
        logBufferPos = 0;
        logBuffer[0] = '\0';

        /* Add initial log message */
        addGameLog("Log window initialized");

        return 0;
    }

    case WM_SIZE: {
        /* Get new client area dimensions */
        GetClientRect(hwnd, &clientRect);

        /* Resize the edit control to fill the entire window */
        if (hwndLogText) {
            SetWindowPos(hwndLogText, NULL, 0, 0,             /* Position - top-left corner */
                         clientRect.right, clientRect.bottom, /* Size - full client area */
                         SWP_NOZORDER);
        }
        return 0;
    }

    case WM_CLOSE:
        /* Don't destroy, just hide the window */
        ShowWindow(hwnd, SW_HIDE);

        /* Update menu checkmark */
        if (hwndMain) {
            hMenu = GetMenu(hwndMain);
            hViewMenu = GetSubMenu(hMenu, 4); /* View is the 5th menu (0-based index) */
            if (hViewMenu) {
                CheckMenuItem(hViewMenu, IDM_VIEW_LOGWINDOW, MF_BYCOMMAND | MF_UNCHECKED);
            }
        }
        return 0;

    case WM_DESTROY:
        hwndLogText = NULL;
        hwndLog = NULL;
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/**
 * Info window procedure - handles messages for the info window
 */
LRESULT CALLBACK infoWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc;
        RECT clientRect;
        char buffer[256];
        char *baseName;
        char *lastSlash;
        char *lastFwdSlash;
        char *dot;
        char nameBuffer[MAX_PATH];
        int y = 10;

        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &clientRect);

        /* Fill background */
        FillRect(hdc, &clientRect, (HBRUSH)GetStockObject(BLACK_BRUSH));

        /* Set text attributes */
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));

        /* Draw title */
        TextOut(hdc, 10, y, "CITY INFO", 9);
        y += 25;

        /* Draw city name */
        if (cityFileName[0] != '\0') {
            /* Make a copy of the path to work with */
            char tempPath[MAX_PATH];
            lstrcpy(tempPath, cityFileName);

            /* Extract the city name from the path */
            baseName = tempPath;
            lastSlash = strrchr(tempPath, '\\');
            lastFwdSlash = strrchr(tempPath, '/');

            if (lastSlash && lastSlash > baseName) {
                baseName = lastSlash + 1;
            }
            if (lastFwdSlash && lastFwdSlash > baseName) {
                baseName = lastFwdSlash + 1;
            }

            lstrcpy(nameBuffer, baseName);
            dot = strrchr(nameBuffer, '.');
            if (dot) {
                *dot = '\0';
            }
            wsprintf(buffer, "City Name: %s", nameBuffer);
        } else {
            /* New city */
            wsprintf(buffer, "City Name: New City");
        }
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        /* Draw date and funds */
        wsprintf(buffer, "Year: %d  Month: %d", CityYear, CityMonth + 1);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        wsprintf(buffer, "Funds: $%d", (int)TotalFunds);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        /* Draw population */
        wsprintf(buffer, "Population: %d", (int)CityPop);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        /* Draw detailed population breakdown */
        wsprintf(buffer, "Residential: %d", ResPop);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        wsprintf(buffer, "Commercial: %d", ComPop);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        wsprintf(buffer, "Industrial: %d", IndPop);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        /* Draw demand values */
        wsprintf(buffer, "Demand - R:%d C:%d I:%d", RValve, CValve, IValve);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        /* Draw city assessment */
        wsprintf(buffer, "Score: %d  Assessment: %s", CityScore, GetCityClassName());
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        wsprintf(buffer, "Approval Rating: %d%%",
                 (CityYes > 0) ? (CityYes * 100 / (CityYes + CityNo)) : 0);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        /* Draw other stats */
        wsprintf(buffer, "Traffic: %d  Pollution: %d", TrafficAverage, PollutionAverage);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        wsprintf(buffer, "Crime: %d  Land Value: %d", CrimeAverage, LVAverage);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_TIMER:
        if (wParam == INFO_TIMER_ID) {
            /* Repaint the window to update the stats */
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        break;

    case WM_CLOSE:
        /* Don't destroy, just hide the window */
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, INFO_TIMER_ID);
        hwndInfo = NULL;
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* These constants should match those in simulation.c */
#define SIM_TIMER_ID 1
#define SIM_TIMER_INTERVAL 50

/* Update menu when simulation speed changes */
void UpdateSimulationMenu(HWND hwnd, int speed) {
    /* Update menu checkmarks */
    CheckMenuRadioItem(hSimMenu, IDM_SIM_PAUSE, IDM_SIM_FAST, IDM_SIM_PAUSE + speed, MF_BYCOMMAND);
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        /* Initialize toolbar */
        CreateToolbar(hwnd, 0, 0, 108, 600);

        CheckMenuRadioItem(hTilesetMenu, 0, GetMenuItemCount(hTilesetMenu) - 1, 0, MF_BYPOSITION);
        /* Initialize simulation */
        DoSimInit();
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_FILE_NEW:
            createNewMap(hwnd);
            return 0;
            
        case IDM_FILE_OPEN:
            openCityDialog(hwnd);
            return 0;

        case IDM_FILE_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;

        case IDM_SIM_PAUSE:
            SetSimulationSpeed(hwnd, SPEED_PAUSED);
            addGameLog("Simulation paused");
            return 0;

        case IDM_SIM_SLOW:
            SetSimulationSpeed(hwnd, SPEED_SLOW);
            addGameLog("Simulation speed: Slow");
            return 0;

        case IDM_SIM_MEDIUM:
            SetSimulationSpeed(hwnd, SPEED_MEDIUM);
            addGameLog("Simulation speed: Medium");
            return 0;

        case IDM_SIM_FAST:
            SetSimulationSpeed(hwnd, SPEED_FAST);
            addGameLog("Simulation speed: Fast");
            return 0;

        /* Scenario menu items */
        case IDM_SCENARIO_DULLSVILLE:
            loadScenario(1);
            return 0;

        case IDM_SCENARIO_SANFRANCISCO:
            loadScenario(2);
            return 0;

        case IDM_SCENARIO_HAMBURG:
            loadScenario(3);
            return 0;

        case IDM_SCENARIO_BERN:
            loadScenario(4);
            return 0;

        case IDM_SCENARIO_TOKYO:
            loadScenario(5);
            return 0;

        case IDM_SCENARIO_DETROIT:
            loadScenario(6);
            return 0;

        case IDM_SCENARIO_BOSTON:
            loadScenario(7);
            return 0;

        case IDM_SCENARIO_RIO:
            loadScenario(8);
            return 0;

        /* View menu items */
        case IDM_VIEW_INFOWINDOW:
            if (hwndInfo) {
                HMENU hMenu = GetMenu(hwnd);
                HMENU hViewMenu = GetSubMenu(hMenu, 4); /* View is the 5th menu (0-based index) */
                UINT state = GetMenuState(hViewMenu, IDM_VIEW_INFOWINDOW, MF_BYCOMMAND);

                if (state & MF_CHECKED) {
                    /* Hide info window */
                    CheckMenuItem(hViewMenu, IDM_VIEW_INFOWINDOW, MF_BYCOMMAND | MF_UNCHECKED);
                    ShowWindow(hwndInfo, SW_HIDE);
                } else {
                    /* Show info window */
                    CheckMenuItem(hViewMenu, IDM_VIEW_INFOWINDOW, MF_BYCOMMAND | MF_CHECKED);
                    ShowWindow(hwndInfo, SW_SHOW);
                    SetFocus(hwnd); /* Keep focus on main window */
                }
            }
            return 0;

        case IDM_VIEW_LOGWINDOW:
            if (hwndLog) {
                HMENU hMenu = GetMenu(hwnd);
                HMENU hViewMenu = GetSubMenu(hMenu, 4); /* View is the 5th menu (0-based index) */
                UINT state = GetMenuState(hViewMenu, IDM_VIEW_LOGWINDOW, MF_BYCOMMAND);

                if (state & MF_CHECKED) {
                    /* Hide log window */
                    CheckMenuItem(hViewMenu, IDM_VIEW_LOGWINDOW, MF_BYCOMMAND | MF_UNCHECKED);
                    ShowWindow(hwndLog, SW_HIDE);
                } else {
                    /* Show log window */
                    CheckMenuItem(hViewMenu, IDM_VIEW_LOGWINDOW, MF_BYCOMMAND | MF_CHECKED);
                    ShowWindow(hwndLog, SW_SHOW);
                    SetFocus(hwnd); /* Keep focus on main window */

                    /* Add a log entry when window is shown */
                    addGameLog("Log window opened");
                }
            }
            return 0;

        case IDM_VIEW_DEBUG_LOGS: {
            HMENU hMenu = GetMenu(hwnd);
            HMENU hViewMenu = GetSubMenu(hMenu, 4); /* View is the 5th menu (0-based index) */
            UINT state = GetMenuState(hViewMenu, IDM_VIEW_DEBUG_LOGS, MF_BYCOMMAND);

            /* Toggle debug logging */
            showDebugLogs = (state & MF_CHECKED) ? 0 : 1;

            if (showDebugLogs) {
                /* Enable debug logging */
                CheckMenuItem(hViewMenu, IDM_VIEW_DEBUG_LOGS, MF_BYCOMMAND | MF_CHECKED);
                addGameLog("Debug logging enabled");
                addDebugLog("Debug logging initialized");
            } else {
                /* Disable debug logging */
                CheckMenuItem(hViewMenu, IDM_VIEW_DEBUG_LOGS, MF_BYCOMMAND | MF_UNCHECKED);
                addGameLog("Debug logging disabled");
            }
        }
            return 0;

        case IDM_VIEW_POWER_OVERLAY: {
            HMENU hMenu = GetMenu(hwnd);
            HMENU hViewMenu = GetSubMenu(hMenu, 4); /* View is the 5th menu (0-based index) */
            UINT state = GetMenuState(hViewMenu, IDM_VIEW_POWER_OVERLAY, MF_BYCOMMAND);

            /* Toggle power overlay */
            powerOverlayEnabled = (state & MF_CHECKED) ? 0 : 1;

            if (powerOverlayEnabled) {
                /* Enable power overlay */
                CheckMenuItem(hViewMenu, IDM_VIEW_POWER_OVERLAY, MF_BYCOMMAND | MF_CHECKED);
            } else {
                /* Disable power overlay */
                CheckMenuItem(hViewMenu, IDM_VIEW_POWER_OVERLAY, MF_BYCOMMAND | MF_UNCHECKED);
            }

            /* Force a redraw to show/hide the overlay */
            InvalidateRect(hwnd, NULL, TRUE);
        }
            return 0;

        /* Tool menu items */
        case IDM_TOOL_BULLDOZER:
            SelectTool(bulldozerState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_BULLDOZER, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_ROAD:
            SelectTool(roadState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_ROAD, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_RAIL:
            SelectTool(railState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_RAIL, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_WIRE:
            SelectTool(wireState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_WIRE, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_PARK:
            SelectTool(parkState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_PARK, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_RESIDENTIAL:
            SelectTool(residentialState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_RESIDENTIAL, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_COMMERCIAL:
            SelectTool(commercialState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_COMMERCIAL, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_INDUSTRIAL:
            SelectTool(industrialState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_INDUSTRIAL, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_FIRESTATION:
            SelectTool(fireState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_FIRESTATION, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_POLICESTATION:
            SelectTool(policeState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_POLICESTATION, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_STADIUM:
            SelectTool(stadiumState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_STADIUM, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_SEAPORT:
            SelectTool(seaportState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_SEAPORT, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_POWERPLANT:
            SelectTool(powerState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_POWERPLANT, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_NUCLEAR:
            SelectTool(nuclearState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_NUCLEAR, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_AIRPORT:
            SelectTool(airportState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_AIRPORT, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_QUERY:
            SelectTool(queryState);
            isToolActive = TRUE;
            CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_QUERY, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        default:
            if (LOWORD(wParam) >= IDM_TILESET_BASE && LOWORD(wParam) < IDM_TILESET_MAX) {
                int index;
                char tilesetName[MAX_PATH];

                index = LOWORD(wParam) - IDM_TILESET_BASE;

                /* Compatible with Windows NT 4.0 */
                GetMenuString(hTilesetMenu, LOWORD(wParam), tilesetName, MAX_PATH - 1,
                              MF_BYCOMMAND);

                if (changeTileset(hwnd, tilesetName)) {
                    CheckMenuRadioItem(hTilesetMenu, 0, GetMenuItemCount(hTilesetMenu) - 1, index, MF_BYPOSITION);
                }
                return 0;
            }
        }
        break;

    case WM_TIMER:
        if (wParam == SIM_TIMER_ID) {
            BOOL needRedraw;

            /* Run the simulation frame */
            SimFrame();

            /* Always redraw to handle animations, but skip if paused */
            needRedraw = TRUE;

            /* Update the display */
            if (needRedraw) {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        break;

    case WM_QUERYNEWPALETTE: {
        /* Realize the palette when window gets focus */
        if (hPalette != NULL) {
            HDC hdc = GetDC(hwnd);
            SelectPalette(hdc, hPalette, FALSE);
            RealizePalette(hdc);
            InvalidateRect(hwnd, NULL, FALSE);
            ReleaseDC(hwnd, hdc);
            return TRUE;
        }
        return FALSE;
    }

    case WM_PALETTECHANGED: {
        /* Realize palette if it was changed by another window */
        if ((HWND)wParam != hwnd && hPalette != NULL) {
            HDC hdc = GetDC(hwnd);
            SelectPalette(hdc, hPalette, FALSE);
            RealizePalette(hdc);
            UpdateColors(hdc);
            ReleaseDC(hwnd, hdc);
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc;

        hdc = BeginPaint(hwnd, &ps);

        /* Select and realize palette for proper 8-bit color rendering */
        if (hPalette) {
            SelectPalette(hdc, hPalette, FALSE);
            RealizePalette(hdc);
        }

        if (hbmBuffer) {
            /* Draw everything to our offscreen buffer with the correct palette */
            if (hPalette) {
                SelectPalette(hdcBuffer, hPalette, FALSE);
                RealizePalette(hdcBuffer);
            }

            /* Draw the city to our buffer */
            drawCity(hdcBuffer);

            /* Copy the buffer to the screen with offset for toolbar */
            BitBlt(hdc, toolbarWidth, 0, cxClient - toolbarWidth, cyClient, hdcBuffer, 0, 0,
                   SRCCOPY);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);

        /* Skip toolbar area */
        if (xPos < toolbarWidth) {
            return 0;
        }

        if (isToolActive) {
            /* Apply the tool at this position */
            int result = HandleToolMouse(xPos, yPos, xOffset, yOffset);

            /* Display result if needed */
            if (result == TOOLRESULT_NO_MONEY) {
                MessageBox(hwnd, "Not enough money!", "Tool Error", MB_ICONEXCLAMATION | MB_OK);
            } else if (result == TOOLRESULT_NEED_BULLDOZE) {
                MessageBox(hwnd, "You need to bulldoze this area first!", "Tool Error",
                           MB_ICONEXCLAMATION | MB_OK);
            } else if (result == TOOLRESULT_FAILED) {
                MessageBox(hwnd, "Can't build there!", "Tool Error", MB_ICONEXCLAMATION | MB_OK);
            }
        } else {
            /* Regular map dragging */
            isMouseDown = TRUE;
            lastMouseX = xPos;
            lastMouseY = yPos;
            SetCapture(hwnd);
            SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        }

        return 0;
    }

    case WM_MOUSEMOVE: {
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);
        int mapX, mapY;

        /* Skip toolbar area */
        if (xPos < toolbarWidth) {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;
        }

        if (isMouseDown) {
            int dx = lastMouseX - xPos;
            int dy = lastMouseY - yPos;

            lastMouseX = xPos;
            lastMouseY = yPos;

            if (dx != 0 || dy != 0) {
                scrollView(dx, dy);
            }

            SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        } else if (isToolActive) {
            /* Convert mouse position to map coordinates for tool hover */
            ScreenToMap(xPos, yPos, &mapX, &mapY, xOffset, yOffset);

            /* Use normal cursor instead of crosshair */
            SetCursor(LoadCursor(NULL, IDC_ARROW));

            /* Force a partial redraw to show hover effect only if needed */
            {
                RECT updateRect;
                updateRect.left = toolbarWidth;
                updateRect.top = 0;
                updateRect.right = cxClient;
                updateRect.bottom = cyClient;
                InvalidateRect(hwnd, &updateRect, FALSE);
            }
        } else {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        isMouseDown = FALSE;
        ReleaseCapture();

        /* Always use arrow cursor */
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return 0;
    }

    case WM_RBUTTONDOWN: {
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);

        /* Skip toolbar area */
        if (xPos < toolbarWidth) {
            return 0;
        }

        /* Start map dragging with right button regardless of tool state */
        isMouseDown = TRUE;
        lastMouseX = xPos;
        lastMouseY = yPos;
        SetCapture(hwnd);
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));

        return 0;
    }

    case WM_RBUTTONUP: {
        isMouseDown = FALSE;
        ReleaseCapture();

        /* Always use arrow cursor */
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return 0;
    }

    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT) {
            if (isMouseDown) {
                SetCursor(LoadCursor(NULL, IDC_SIZEALL));
            } else if (isToolActive) {
                SetCursor(LoadCursor(NULL, IDC_ARROW)); /* Use normal cursor instead of cross */
            } else {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            }
            return TRUE;
        }
        break;
    }

    case WM_SIZE: {
        RECT rcClient;
        HWND hwndToolbarWnd;

        /* Get full client area */
        cxClient = LOWORD(lParam);
        cyClient = HIWORD(lParam);

        /* Get the toolbar window handle */
        hwndToolbarWnd = FindWindow("MicropolisToolbar", NULL);

        /* If the toolbar exists, adjust its position */
        if (hwndToolbarWnd) {
            MoveWindow(hwndToolbarWnd, 0, 0, toolbarWidth, cyClient, TRUE);
        } else {
            /* Create the toolbar if it doesn't exist yet */
            CreateToolbar(hwnd, 0, 0, toolbarWidth, cyClient);
        }

        /* Resize the drawing buffer to the client area less the toolbar */
        resizeBuffer(cxClient - toolbarWidth, cyClient);

        /* Adjust the xOffset to account for the toolbar */
        xOffset = toolbarWidth;

        return 0;
    }

    case WM_KEYDOWN: {
        switch (wParam) {
        case VK_LEFT:
            scrollView(-TILE_SIZE, 0);
            break;

        case VK_RIGHT:
            scrollView(TILE_SIZE, 0);
            break;

        case VK_UP:
            scrollView(0, -TILE_SIZE);
            break;

        case VK_DOWN:
            scrollView(0, TILE_SIZE);
            break;

        case 'O':
            if (GetKeyState(VK_CONTROL) < 0) {
                openCityDialog(hwnd);
            }
            break;

        case 'Q':
            if (GetKeyState(VK_CONTROL) < 0) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
            break;

        case 'I':
            if (hbmTiles) {
                BITMAP bm;
                char debugMsg[256];

                if (GetObject(hbmTiles, sizeof(BITMAP), &bm)) {
                    wsprintf(debugMsg, "Bitmap Info:\nDimensions: %dx%d\nBits/pixel: %d",
                             bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);
                    MessageBox(hwnd, debugMsg, "Bitmap Information", MB_OK);
                }
            }
            break;
		default:
			break;
        }
        return 0;
    }

    case WM_DESTROY:
        CleanupSimTimer(hwnd);
        cleanupGraphics();

        /* Clean up toolbar */
        if (FindWindow("MicropolisToolbar", NULL)) {
            DestroyWindow(FindWindow("MicropolisToolbar", NULL));
        }

        /* Clean up toolbar bitmaps */
        CleanupToolbarBitmaps();

        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void swapShorts(short *buf, int len) {
    int i;

    for (i = 0; i < len; i++) {
        buf[i] = ((buf[i] & 0xFF) << 8) | ((buf[i] & 0xFF00) >> 8);
    }
}

HPALETTE createSystemPalette(void) {
    LOGPALETTE *pLogPal;
    HPALETTE hPal;
    int i, r, g, b;
    int gray;
    PALETTEENTRY sysColors[16];

    /* Setup the standard 16 colors */
    sysColors[0].peRed = 0;
    sysColors[0].peGreen = 0;
    sysColors[0].peBlue = 0;
    sysColors[0].peFlags = 0; /* Black */
    sysColors[1].peRed = 128;
    sysColors[1].peGreen = 0;
    sysColors[1].peBlue = 0;
    sysColors[1].peFlags = 0; /* Dark Red */
    sysColors[2].peRed = 0;
    sysColors[2].peGreen = 128;
    sysColors[2].peBlue = 0;
    sysColors[2].peFlags = 0; /* Dark Green */
    sysColors[3].peRed = 128;
    sysColors[3].peGreen = 128;
    sysColors[3].peBlue = 0;
    sysColors[3].peFlags = 0; /* Dark Yellow */
    sysColors[4].peRed = 0;
    sysColors[4].peGreen = 0;
    sysColors[4].peBlue = 128;
    sysColors[4].peFlags = 0; /* Dark Blue */
    sysColors[5].peRed = 128;
    sysColors[5].peGreen = 0;
    sysColors[5].peBlue = 128;
    sysColors[5].peFlags = 0; /* Dark Magenta */
    sysColors[6].peRed = 0;
    sysColors[6].peGreen = 128;
    sysColors[6].peBlue = 128;
    sysColors[6].peFlags = 0; /* Dark Cyan */
    sysColors[7].peRed = 192;
    sysColors[7].peGreen = 192;
    sysColors[7].peBlue = 192;
    sysColors[7].peFlags = 0; /* Light Gray */
    sysColors[8].peRed = 128;
    sysColors[8].peGreen = 128;
    sysColors[8].peBlue = 128;
    sysColors[8].peFlags = 0; /* Dark Gray */
    sysColors[9].peRed = 255;
    sysColors[9].peGreen = 0;
    sysColors[9].peBlue = 0;
    sysColors[9].peFlags = 0; /* Red */
    sysColors[10].peRed = 0;
    sysColors[10].peGreen = 255;
    sysColors[10].peBlue = 0;
    sysColors[10].peFlags = 0; /* Green */
    sysColors[11].peRed = 255;
    sysColors[11].peGreen = 255;
    sysColors[11].peBlue = 0;
    sysColors[11].peFlags = 0; /* Yellow */
    sysColors[12].peRed = 0;
    sysColors[12].peGreen = 0;
    sysColors[12].peBlue = 255;
    sysColors[12].peFlags = 0; /* Blue */
    sysColors[13].peRed = 255;
    sysColors[13].peGreen = 0;
    sysColors[13].peBlue = 255;
    sysColors[13].peFlags = 0; /* Magenta */
    sysColors[14].peRed = 0;
    sysColors[14].peGreen = 255;
    sysColors[14].peBlue = 255;
    sysColors[14].peFlags = 0; /* Cyan */
    sysColors[15].peRed = 255;
    sysColors[15].peGreen = 255;
    sysColors[15].peBlue = 255;
    sysColors[15].peFlags = 0; /* White */

    /* Allocate memory for 256 color entries */
    pLogPal = (LOGPALETTE *)malloc(sizeof(LOGPALETTE) + 255 * sizeof(PALETTEENTRY));
    if (!pLogPal) {
        return NULL;
    }

    pLogPal->palVersion = 0x300;  /* Windows 3.0 */
    pLogPal->palNumEntries = 256; /* 256 colors (8-bit) */

    /* Copy system colors */
    for (i = 0; i < 16; i++) {
        pLogPal->palPalEntry[i] = sysColors[i];
    }

    /* Create a 6x6x6 color cube for the next 216 entries (6 levels each for R, G, B) */
    /* This is similar to the "Web Safe" palette */
    i = 16; /* Start after system colors */
    for (r = 0; r < 6; r++) {
        for (g = 0; g < 6; g++) {
            for (b = 0; b < 6; b++) {
                pLogPal->palPalEntry[i].peRed = r * 51; /* 51 is 255/5 */
                pLogPal->palPalEntry[i].peGreen = g * 51;
                pLogPal->palPalEntry[i].peBlue = b * 51;
                pLogPal->palPalEntry[i].peFlags = 0;
                i++;
            }
        }
    }

    /* Fill the remaining entries with grayscale values */
    for (; i < 256; i++) {
        gray = (i - 232) * 10 + 8; /* 24 grayscale entries, from light gray to near-white */
        if (gray > 255) {
            gray = 255;
        }

        pLogPal->palPalEntry[i].peRed = gray;
        pLogPal->palPalEntry[i].peGreen = gray;
        pLogPal->palPalEntry[i].peBlue = gray;
        pLogPal->palPalEntry[i].peFlags = 0;
    }

    hPal = CreatePalette(pLogPal);
    free(pLogPal);

    return hPal;
}

void initializeGraphics(HWND hwnd) {
    HDC hdc;
    RECT rect;
    char tilePath[MAX_PATH];
    int width;
    int height;
    BITMAPINFOHEADER bi;
    LPVOID bits;
    HBITMAP hbmOld;
    char errorMsg[256];
    DWORD error;

    hdc = GetDC(hwnd);

    /* Create our 256-color palette */
    if (hPalette == NULL) {
        hPalette = createSystemPalette();

        if (hPalette) {
            SelectPalette(hdc, hPalette, FALSE);
            RealizePalette(hdc);
        } else {
            OutputDebugString("Failed to create palette!");
        }
    }

    hdcBuffer = CreateCompatibleDC(hdc);

    if (hPalette) {
        SelectPalette(hdcBuffer, hPalette, FALSE);
        RealizePalette(hdcBuffer);
    }

    GetClientRect(hwnd, &rect);
    cxClient = rect.right - rect.left;
    cyClient = rect.bottom - rect.top;

    width = cxClient;
    height = cyClient;

    /* Setup 8-bit DIB section for our drawing buffer */
    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; /* Negative for top-down DIB */
    bi.biPlanes = 1;
    bi.biBitCount = 8; /* 8 bits = 256 colors */
    bi.biCompression = BI_RGB;

#if NEW32
    hbmBuffer = CreateDIBSection(hdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS, &bits, NULL, 0);
#else
	hbmBuffer = CreateDIBitmap(hdc, 
		&bi,				// Pointer to BITMAPINFOHEADER
		CBM_INIT,			// Initialize bitmap bits
		&bits,				// Pointer to actual bitmap bits (if any)
		(BITMAPINFO *)&bi,	// Pointer to BITMAPINFO
		DIB_RGB_COLORS);	// Color usage
#endif

    if (hbmBuffer == NULL) {
        error = GetLastError();

        wsprintf(errorMsg, "Failed to create buffer DIB Section: Error %d", error);
        OutputDebugString(errorMsg);
        ReleaseDC(hwnd, hdc);
        return;
    }

    hbmOld = SelectObject(hdcBuffer, hbmBuffer);

    /* Fill with black background */
    FillRect(hdcBuffer, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

    /* Use default.bmp tileset by default */
    strcpy(currentTileset, "default");
    wsprintf(tilePath, "tilesets\\%s.bmp", currentTileset);

    /* Load the tileset with our 8-bit palette */
    if (!loadTileset(tilePath)) {
        OutputDebugString("Failed to load default tileset! Trying classic as fallback...");
        /* Fallback to classic if default is not available */
        strcpy(currentTileset, "classic");
        wsprintf(tilePath, "tilesets\\%s.bmp", currentTileset);
        
        if (!loadTileset(tilePath)) {
            OutputDebugString("Failed to load classic tileset too!");
        }
    }

    ReleaseDC(hwnd, hdc);
}

HBITMAP loadBitmapFile(const char *filename) {
    HBITMAP hBitmap;
    DWORD error;
    char errorMsg[256];

    /* LR_CREATEDIBSECTION creates a DIB section bitmap that can be
       selected into a memory DC. Using this flag makes the bitmap
       compatible with our 8-bit/256 color rendering. */
	#if(WINVER >= 0x0400)
    hBitmap = LoadImage(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
#else
	hBitmap = LoadImage(filename);
#endif

    if (hBitmap == NULL) {
        /* Debug output to help diagnose bitmap loading errors */
        error = GetLastError();

        wsprintf(errorMsg, "Failed to load bitmap: %s, Error: %d", filename, error);
        OutputDebugString(errorMsg);
    }

    return hBitmap;
}

int loadTileset(const char *filename) {
    HDC hdc;
    char errorMsg[256];
    DWORD error;
    BITMAP bm;
    char debugMsg[256];

    if (hdcTiles) {
        DeleteDC(hdcTiles);
        hdcTiles = NULL;
    }

    if (hbmTiles) {
        DeleteObject(hbmTiles);
        hbmTiles = NULL;
    }

    /* Load the bitmap with explicit color control */
	hbmTiles = loadBitmapFile(filename);
    //hbmTiles = LoadImage(NULL, filename, IMAGE_BITMAP, 0, 0, 
    //                    LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_DEFAULTCOLOR);

    if (hbmTiles == NULL) {
        /* Output debug message */
        error = GetLastError();

        wsprintf(errorMsg, "Failed to load tileset: %s, Error: %d", filename, error);
        OutputDebugString(errorMsg);
        return 0;
    }

    /* Verify that the bitmap was loaded properly */
    if (GetObject(hbmTiles, sizeof(BITMAP), &bm)) {
        wsprintf(debugMsg, "Tileset Info: %dx%d, %d bits/pixel", bm.bmWidth, bm.bmHeight,
                 bm.bmBitsPixel);
        OutputDebugString(debugMsg);

        /* Warn if the bitmap is not 8-bit */
        if (bm.bmBitsPixel != 8) {
            wsprintf(debugMsg, "WARNING: Tileset is not 8-bit color (%d bits)", bm.bmBitsPixel);
            OutputDebugString(debugMsg);
        }
    }

    hdc = GetDC(hwndMain);
    hdcTiles = CreateCompatibleDC(hdc);

    /* Apply our palette to the tileset - be more forceful about it */
    if (hPalette) {
        SelectPalette(hdcTiles, hPalette, FALSE);
        RealizePalette(hdcTiles);
    }

    SelectObject(hdcTiles, hbmTiles);

    /* Force a background color update to use our palette */
    SetBkMode(hdcTiles, TRANSPARENT);
    
    ReleaseDC(hwndMain, hdc);
    return 1;
}

int changeTileset(HWND hwnd, const char *tilesetName) {
    char tilesetPath[MAX_PATH];
    char windowTitle[MAX_PATH];
    HDC hdc;
    char errorMsg[256];
    DWORD error;
    BITMAP bm;
    char debugMsg[256];

    wsprintf(tilesetPath, "tilesets\\%s.bmp", tilesetName);

    if (hdcTiles) {
        DeleteDC(hdcTiles);
        hdcTiles = NULL;
    }

    if (hbmTiles) {
        DeleteObject(hbmTiles);
        hbmTiles = NULL;
    }

    hbmTiles = loadBitmapFile(tilesetPath);
//        LoadImage(NULL, tilesetPath, IMAGE_BITMAP, 0, 0, 
//                 LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_DEFAULTCOLOR);

    if (hbmTiles == NULL) {
        /* Output debug message */
        error = GetLastError();

        wsprintf(errorMsg, "Failed to change tileset: %s, Error: %d", tilesetPath, error);
        OutputDebugString(errorMsg);
        
        /* Try fallback to default if requested tileset isn't 'default' */
        if (strcmp(tilesetName, "default") != 0) {
            wsprintf(errorMsg, "Trying to fallback to default tileset");
            OutputDebugString(errorMsg);
            
            wsprintf(tilesetPath, "tilesets\\default.bmp");
			hbmTiles = loadBitmapFile(tilesetPath);
//            hbmTiles = LoadImage(NULL, tilesetPath, IMAGE_BITMAP, 0, 0, 
//                              LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_DEFAULTCOLOR);
                              
            if (hbmTiles == NULL) {
                /* If default also fails, give up */
                error = GetLastError();
                wsprintf(errorMsg, "Failed to load default fallback: Error: %d", error);
                OutputDebugString(errorMsg);
                return 0;
            }
            
            /* Success with default tileset - update in the strcpy below */
            /* We can't modify tilesetName directly as it's a const parameter */
        } else {
            /* If we're already trying to load default and it failed, give up */
            return 0;
        }
    }

    /* Verify that the bitmap was loaded properly */
    if (GetObject(hbmTiles, sizeof(BITMAP), &bm)) {
        wsprintf(debugMsg, "Changed Tileset Info: %dx%d, %d bits/pixel", bm.bmWidth, bm.bmHeight,
                 bm.bmBitsPixel);
        OutputDebugString(debugMsg);
    }

    hdc = GetDC(hwndMain);
    hdcTiles = CreateCompatibleDC(hdc);

    /* Apply our palette to the tileset - be more forceful about it */
    if (hPalette) {
        SelectPalette(hdcTiles, hPalette, FALSE);
        RealizePalette(hdcTiles);
    }

    SelectObject(hdcTiles, hbmTiles);
    
    /* Force a background color update to use our palette */
    SetBkMode(hdcTiles, TRANSPARENT);

    /* If we used the fallback to default.bmp, use "default" as the tileset name */
    if (strcmp(tilesetPath, "tilesets\\default.bmp") == 0 && strcmp(tilesetName, "default") != 0) {
        strcpy(currentTileset, "default");
    } else {
        strcpy(currentTileset, tilesetName);
    }

    wsprintf(windowTitle, "MicropolisNT - Tileset: %s", currentTileset);
    SetWindowText(hwnd, windowTitle);

    /* Force a full redraw */
    InvalidateRect(hwnd, NULL, TRUE);

    ReleaseDC(hwndMain, hdc);
    return 1;
}

void cleanupGraphics(void) {
    if (hbmBuffer) {
        DeleteObject(hbmBuffer);
        hbmBuffer = NULL;
    }

    if (hdcBuffer) {
        DeleteDC(hdcBuffer);
        hdcBuffer = NULL;
    }

    if (hbmTiles) {
        DeleteObject(hbmTiles);
        hbmTiles = NULL;
    }

    if (hdcTiles) {
        DeleteDC(hdcTiles);
        hdcTiles = NULL;
    }

    if (hPalette) {
        DeleteObject(hPalette);
        hPalette = NULL;
    }
}

void resizeBuffer(int cx, int cy) {
    HDC hdc;
    HBITMAP hbmNew;
    RECT rcBuffer;
    BITMAPINFOHEADER bi;
    LPVOID bits;
    char errorMsg[256];
    DWORD error;

    if (cx <= 0 || cy <= 0) {
        return;
    }

    hdc = GetDC(hwndMain);

    /* Make sure our palette is selected into the DC */
    if (hPalette) {
        SelectPalette(hdc, hPalette, FALSE);
        RealizePalette(hdc);
    }

    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = cx;
    bi.biHeight = -cy; /* Negative for top-down DIB */
    bi.biPlanes = 1;
    bi.biBitCount = 8; /* 8 bits = 256 colors */
    bi.biCompression = BI_RGB;

    /* Create DIB section with our palette */
#if NEW32
    hbmNew = CreateDIBSection(hdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS, &bits, NULL, 0);
#else
	hbmNew = CreateDIBitmap(hdc, 
		&bi,				// Pointer to BITMAPINFOHEADER
		CBM_INIT,			// Initialize bitmap bits
		&bits,				// Pointer to actual bitmap bits (if any)
		(BITMAPINFO *)&bi,	// Pointer to BITMAPINFO
		DIB_RGB_COLORS);	// Color usage
#endif

    if (hbmNew == NULL) {
        /* Debug output for DIB creation failure */
        error = GetLastError();

        wsprintf(errorMsg, "Failed to create DIB Section: Error %d", error);
        OutputDebugString(errorMsg);
        ReleaseDC(hwndMain, hdc);
        return;
    }

    if (hbmBuffer) {
        DeleteObject(hbmBuffer);
    }

    hbmBuffer = hbmNew;
    SelectObject(hdcBuffer, hbmBuffer);

    /* Apply the palette to our buffer */
    if (hPalette) {
        SelectPalette(hdcBuffer, hPalette, FALSE);
        RealizePalette(hdcBuffer);
    }

    rcBuffer.left = 0;
    rcBuffer.top = 0;
    rcBuffer.right = cx;
    rcBuffer.bottom = cy;
    FillRect(hdcBuffer, &rcBuffer, (HBRUSH)GetStockObject(BLACK_BRUSH));

    ReleaseDC(hwndMain, hdc);

    InvalidateRect(hwndMain, NULL, FALSE);
}

void scrollView(int dx, int dy) {
    RECT rcClient;
    RECT updateRect;
    HRGN hRgn;

    /* Adjust offsets */
    xOffset += dx;
    yOffset += dy;

    /* Enforce bounds */
    if (xOffset < 0) {
        xOffset = 0;
    }
    if (yOffset < 0) {
        yOffset = 0;
    }

    if (xOffset > WORLD_X * TILE_SIZE - cxClient) {
        xOffset = WORLD_X * TILE_SIZE - cxClient;
    }
    if (yOffset > WORLD_Y * TILE_SIZE - cyClient) {
        yOffset = WORLD_Y * TILE_SIZE - cyClient;
    }

    /* Get client area without toolbar */
    GetClientRect(hwndMain, &rcClient);
    rcClient.left = toolbarWidth; /* Skip toolbar area */

    /* Create update region for the map area only */
    hRgn = CreateRectRgnIndirect(&rcClient);

    /* Update only the map region, without erasing the background */
    InvalidateRgn(hwndMain, hRgn, FALSE);
    DeleteObject(hRgn);
}

/* Internal function to load file data */
int loadFile(char *filename) {
    FILE *f;
    DWORD size;
    size_t readResult;

    f = fopen(filename, "rb");
    if (f == NULL) {
        return 0;
    }

    fseek(f, 0L, SEEK_END);
    size = ftell(f);
    fseek(f, 0L, SEEK_SET);

    /* The original Micropolis city files are 27120 bytes */
    if (size != 27120) {
        fclose(f);
        return 0;
    }

    readResult = fread(ResHis, sizeof(short), HISTLEN / 2, f);
    if (readResult != HISTLEN / 2) {
        goto read_error;
    }
    swapShorts(ResHis, HISTLEN / 2);

    readResult = fread(ComHis, sizeof(short), HISTLEN / 2, f);
    if (readResult != HISTLEN / 2) {
        goto read_error;
    }
    swapShorts(ComHis, HISTLEN / 2);

    readResult = fread(IndHis, sizeof(short), HISTLEN / 2, f);
    if (readResult != HISTLEN / 2) {
        goto read_error;
    }
    swapShorts(IndHis, HISTLEN / 2);

    readResult = fread(CrimeHis, sizeof(short), HISTLEN / 2, f);
    if (readResult != HISTLEN / 2) {
        goto read_error;
    }
    swapShorts(CrimeHis, HISTLEN / 2);

    readResult = fread(PollutionHis, sizeof(short), HISTLEN / 2, f);
    if (readResult != HISTLEN / 2) {
        goto read_error;
    }
    swapShorts(PollutionHis, HISTLEN / 2);

    readResult = fread(MoneyHis, sizeof(short), HISTLEN / 2, f);
    if (readResult != HISTLEN / 2) {
        goto read_error;
    }
    swapShorts(MoneyHis, HISTLEN / 2);

    readResult = fread(MiscHis, sizeof(short), MISCHISTLEN / 2, f);
    if (readResult != MISCHISTLEN / 2) {
        goto read_error;
    }
    swapShorts(MiscHis, MISCHISTLEN / 2);

    /* Original Micropolis stores map transposed compared to our array convention */
    {
        short tmpMap[WORLD_X][WORLD_Y];
        int x, y;

        readResult = fread(&tmpMap[0][0], sizeof(short), WORLD_X * WORLD_Y, f);
        if (readResult != WORLD_X * WORLD_Y) {
            goto read_error;
        }

        swapShorts((short *)tmpMap, WORLD_X * WORLD_Y);

        for (x = 0; x < WORLD_X; x++) {
            for (y = 0; y < WORLD_Y; y++) {
                Map[y][x] = tmpMap[x][y];
            }
        }
    }

    fclose(f);
    return 1;

read_error:
    fclose(f);
    return 0;
}

/* External function declarations */
extern int calcResPop(int zone);     /* Calculate residential zone population - from zone.c */
extern int calcComPop(int zone);     /* Calculate commercial zone population - from zone.c */
extern int calcIndPop(int zone);     /* Calculate industrial zone population - from zone.c */
extern void ClearCensus(void);       /* Reset census counters - from simulation.c */
extern void CityEvaluation(void);    /* Update city evaluation - from evaluation.c */
extern void TakeCensus(void);        /* Take a census - from simulation.c */
extern void CountSpecialTiles(void); /* Count special buildings - from evaluation.c */

/* Force a census calculation of the entire map */
void ForceFullCensus(void) {
    int x, y;
    short tile;
    int zoneTile;

    /* Reset census counts */
    ClearCensus();

    /* Scan entire map to count populations */
    for (y = 0; y < WORLD_Y; y++) {
        for (x = 0; x < WORLD_X; x++) {
            tile = Map[y][x];

            /* Check if this is a zone center */
            if (tile & ZONEBIT) {
                zoneTile = tile & LOMASK;

                /* Check zone type and add population accordingly */
                if (zoneTile >= RESBASE && zoneTile <= LASTRES) {
                    /* Residential zone */
                    ResPop += calcResPop(zoneTile);
                } else if (zoneTile >= COMBASE && zoneTile <= LASTCOM) {
                    /* Commercial zone */
                    ComPop += calcComPop(zoneTile);
                } else if (zoneTile >= INDBASE && zoneTile <= LASTIND) {
                    /* Industrial zone */
                    IndPop += calcIndPop(zoneTile);
                } else if (zoneTile == HOSPITAL) {
                    /* Hospital contributes to residential population */
                    ResPop += 30;
                } else if (zoneTile == CHURCH) {
                    /* Church contributes to residential population */
                    ResPop += 10;
                }

                /* Count other special zones */
                if (zoneTile == FIRESTATION) {
                    FirePop++;
                } else if (zoneTile == POLICESTATION) {
                    PolicePop++;
                } else if (zoneTile == STADIUM) {
                    StadiumPop++;
                    /* Stadium contributes to commercial population */
                    ComPop += 50;
                } else if (zoneTile == PORT) {
                    PortPop++;
                    /* Port contributes to industrial population */
                    IndPop += 40;
                } else if (zoneTile == AIRPORT) {
                    APortPop++;
                    /* Airport contributes to industrial population */
                    IndPop += 40;
                } else if (zoneTile == NUCLEAR) {
                    NuclearPop++;
                }
                /* Note: We don't count power plants here since CountSpecialTiles() in evaluation.c
                 * handles it */

                /* Count powered/unpowered zones */
                if (tile & POWERBIT) {
                    PwrdZCnt++;
                } else {
                    UnpwrdZCnt++;
                }
            }

            /* Count infrastructure */
            if ((tile & LOMASK) >= ROADBASE && (tile & LOMASK) <= LASTROAD) {
                RoadTotal++;
            } else if ((tile & LOMASK) >= RAILBASE && (tile & LOMASK) <= LASTRAIL) {
                RailTotal++;
            }
        }
    }

    /* Calculate total population */
    TotalPop = (ResPop + ComPop + IndPop) * 8;

    /* Also directly calculate CityPop to ensure it's set immediately */
    CityPop = ((ResPop) + (ComPop * 8) + (IndPop * 8)) * 20;

    /* Determine city class based on population */
    CityClass = 0; /* Village */
    if (CityPop > 2000) {
        CityClass++; /* Town */
    }
    if (CityPop > 10000) {
        CityClass++; /* City */
    }
    if (CityPop > 50000) {
        CityClass++; /* Capital */
    }
    if (CityPop > 100000) {
        CityClass++; /* Metropolis */
    }
    if (CityPop > 500000) {
        CityClass++; /* Megalopolis */
    }

    /* Count special buildings for city evaluation */
    CountSpecialTiles();

    /* Update the city evaluation based on the new population */
    CityEvaluation();

    /* Take census to update history graphs */
    TakeCensus();
}

int loadCity(char *filename) {
    /* Save previous population values in case we need them */
    int oldResPop;
    int oldComPop;
    int oldIndPop;
    QUAD oldCityPop;

    /* Initialize variables at the top of function for C89 compliance */
    oldResPop = ResPop;
    oldComPop = ComPop;
    oldIndPop = IndPop;
    oldCityPop = CityPop;

    /* Reset scenario ID */
    ScenarioID = 0;
    DisasterEvent = 0;
    DisasterWait = 0;

    lstrcpy(cityFileName, filename);

    if (!loadFile(filename)) {
        MessageBox(hwndMain, "Failed to load city file", "Error", MB_ICONERROR | MB_OK);
        return 0;
    }

    xOffset = (WORLD_X * TILE_SIZE - cxClient) / 2;
    yOffset = (WORLD_Y * TILE_SIZE - cyClient) / 2;
    if (xOffset < 0) {
        xOffset = 0;
    }
    if (yOffset < 0) {
        yOffset = 0;
    }

    /* First run a full census to calculate initial city population */
    ForceFullCensus();

    /* Check if we got a valid population */
    if (CityPop == 0 && (oldCityPop > 0)) {
        /* If no population detected, use values from previous city */
        ResPop = oldResPop;
        ComPop = oldComPop;
        IndPop = oldIndPop;
        TotalPop = (ResPop + ComPop + IndPop) * 8;
        CityPop = oldCityPop;

        /* Update city class based on restored population */
        CityClass = 0; /* Village */
        if (CityPop > 2000) {
            CityClass++; /* Town */
        }
        if (CityPop > 10000) {
            CityClass++; /* City */
        }
        if (CityPop > 50000) {
            CityClass++; /* Capital */
        }
        if (CityPop > 100000) {
            CityClass++; /* Metropolis */
        }
        if (CityPop > 500000) {
            CityClass++; /* Megalopolis */
        }
    }

    /* Now we can initialize the simulation but preserve population */
    DoSimInit();

    /* Force a final population census calculation for the loaded city */
    ForceFullCensus();

    /* Unpause simulation at medium speed */
    SetSimulationSpeed(hwndMain, SPEED_MEDIUM);

    /* Update the window title with city name */
    {
        char windowTitle[MAX_PATH];
        char *baseName;
        char *dot;

        baseName = cityFileName;

        if (strrchr(baseName, '\\')) {
            baseName = strrchr(baseName, '\\') + 1;
        }
        if (strrchr(baseName, '/')) {
            baseName = strrchr(baseName, '/') + 1;
        }

        lstrcpy(windowTitle, "MicropolisNT - ");
        lstrcat(windowTitle, baseName);

        /* Remove the extension if present */
        dot = strrchr(windowTitle, '.');
        if (dot) {
            *dot = '\0';
        }

        SetWindowText(hwndMain, windowTitle);
    }

    InvalidateRect(hwndMain, NULL, FALSE);

    /* Log the city load event */
    {
        char *baseName;

        baseName = cityFileName;

        if (strrchr(baseName, '\\')) {
            baseName = strrchr(baseName, '\\') + 1;
        }
        if (strrchr(baseName, '/')) {
            baseName = strrchr(baseName, '/') + 1;
        }

        /* Log the city loading information */
        addGameLog("City loaded: %s", baseName);
        addGameLog("City stats - Population: %d, Funds: $%d", (int)CityPop, (int)TotalFunds);
        addDebugLog("City class: %s, Year: %d, Month: %d", GetCityClassName(), CityYear,
                    CityMonth + 1);
        addDebugLog("R-C-I population: %d, %d, %d", ResPop, ComPop, IndPop);
    }

    return 1;
}

int getBaseFromTile(short tile) {
    tile &= LOMASK;

    if (tile >= TILE_WATER_LOW && tile <= TILE_WATER_HIGH) {
        return TILE_RIVER;
    }

    if (tile >= TILE_WOODS_LOW && tile <= TILE_WOODS_HIGH) {
        return TILE_TREEBASE;
    }

    if (tile >= TILE_ROADBASE && tile <= TILE_LASTROAD) {
        return TILE_ROADBASE;
    }

    if (tile >= TILE_POWERBASE && tile <= TILE_LASTPOWER) {
        return TILE_POWERBASE;
    }

    if (tile >= TILE_RAILBASE && tile <= TILE_LASTRAIL) {
        return TILE_RAILBASE;
    }

    if (tile >= TILE_RESBASE && tile <= TILE_LASTRES) {
        return TILE_RESBASE;
    }

    if (tile >= TILE_COMBASE && tile <= TILE_LASTCOM) {
        return TILE_COMBASE;
    }

    if (tile >= TILE_INDBASE && tile <= TILE_LASTIND) {
        return TILE_INDBASE;
    }

    if (tile >= TILE_FIREBASE && tile <= TILE_LASTFIRE) {
        return TILE_FIREBASE;
    }

    if (tile >= TILE_FLOOD && tile <= TILE_LASTFLOOD) {
        return TILE_FLOOD;
    }

    if (tile >= TILE_RUBBLE && tile <= TILE_LASTRUBBLE) {
        return TILE_RUBBLE;
    }

    return TILE_DIRT;
}

void drawTile(HDC hdc, int x, int y, short tileValue) {
    RECT rect;
    HBRUSH hBrush;
    HBRUSH hOldBrush;
    COLORREF color;
    int tileIndex;
    int srcX;
    int srcY;

    /* Don't treat negative values as special - they are valid tile values with the sign bit set
       In the original code, negative values in PowerMap indicated unpowered state,
       but in our implementation we need to still render the tile */

    tileIndex = tileValue & LOMASK;

    if (tileIndex >= TILE_TOTAL_COUNT) {
        tileIndex = 0;
    }

    /* Handle animated traffic tiles */
    if (tileValue & ANIMBIT) {
        /* Use low 2 bits of Fcycle for animation (0-3) */
        int frame = (Fcycle & 3);

        /* Light traffic animation range (80-127) */
        if (tileIndex >= 80 && tileIndex <= 127) {
            /* The way traffic works is:
               - Tiles 80-95 are frame 1 (use when frame=1)
               - Tiles 96-111 are frame 2 (use when frame=2)
               - Tiles 112-127 are frame 3 (use when frame=3)
               - Tiles 128-143 are frame 0 (use when frame=0)
             */
            int baseOffset = tileIndex & 0xF; /* Base road layout (0-15) */

            /* Get correct frame tiles */
            switch (frame) {
            case 0:
                tileIndex = 128 + baseOffset;
                break; /* Frame 0 */
            case 1:
                tileIndex = 80 + baseOffset;
                break; /* Frame 1 */
            case 2:
                tileIndex = 96 + baseOffset;
                break; /* Frame 2 */
            case 3:
                tileIndex = 112 + baseOffset;
                break; /* Frame 3 */
            }
        }

        /* Heavy traffic animation range (144-207) */
        if (tileIndex >= 144 && tileIndex <= 207) {
            /* Heavy traffic works the same way:
               - Tiles 144-159 are frame 1 (use when frame=1)
               - Tiles 160-175 are frame 2 (use when frame=2)
               - Tiles 176-191 are frame 3 (use when frame=3)
               - Tiles 192-207 are frame 0 (use when frame=0)
             */
            int baseOffset = tileIndex & 0xF; /* Base road layout (0-15) */

            /* Get correct frame tiles */
            switch (frame) {
            case 0:
                tileIndex = 192 + baseOffset;
                break; /* Frame 0 */
            case 1:
                tileIndex = 144 + baseOffset;
                break; /* Frame 1 */
            case 2:
                tileIndex = 160 + baseOffset;
                break; /* Frame 2 */
            case 3:
                tileIndex = 176 + baseOffset;
                break; /* Frame 3 */
            }
        }
    }

    rect.left = x;
    rect.top = y;
    rect.right = x + TILE_SIZE;
    rect.bottom = y + TILE_SIZE;

    if (hdcTiles && hbmTiles) {
        srcX = (tileIndex % TILES_IN_ROW) * TILE_SIZE;
        srcY = (tileIndex / TILES_IN_ROW) * TILE_SIZE;

        BitBlt(hdc, x, y, TILE_SIZE, TILE_SIZE, hdcTiles, srcX, srcY, SRCCOPY);
    } else {
        switch (getBaseFromTile(tileValue)) {
        case TILE_RIVER:
            color = RGB(0, 0, 128); /* Dark blue */
            break;
        case TILE_TREEBASE:
            color = RGB(0, 128, 0); /* Dark green */
            break;
        case TILE_ROADBASE:
            color = RGB(128, 128, 128); /* Gray */
            break;
        case TILE_RAILBASE:
            color = RGB(192, 192, 192); /* Light gray */
            break;
        case TILE_POWERBASE:
            color = RGB(255, 255, 0); /* Yellow */
            break;
        case TILE_RESBASE:
            color = RGB(0, 255, 0); /* Green */
            break;
        case TILE_COMBASE:
            color = RGB(0, 0, 255); /* Blue */
            break;
        case TILE_INDBASE:
            color = RGB(255, 255, 0); /* Yellow */
            break;
        case TILE_FIREBASE:
            color = RGB(255, 0, 0); /* Red */
            break;
        case TILE_FLOOD:
            color = RGB(0, 128, 255); /* Light blue */
            break;
        case TILE_RUBBLE:
            color = RGB(128, 128, 0); /* Olive */
            break;
        case TILE_DIRT:
        default:
            color = RGB(204, 102, 0); /* Orange-brown */
            break;
        }

        hBrush = CreateSolidBrush(color);
        hOldBrush = SelectObject(hdc, hBrush);

        FillRect(hdc, &rect, hBrush);

        SelectObject(hdc, hOldBrush);
        DeleteObject(hBrush);
    }

    /* Removed white frame for all zones to improve visual appearance */

    if ((tileValue & ZONEBIT) && !(tileValue & POWERBIT)) {
        /* Unpowered zones get a yellow frame */
        hBrush = CreateSolidBrush(RGB(255, 255, 0));
        FrameRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        /* Draw the lightning bolt power indicator in the center of the tile */
        if (hdcTiles && hbmTiles) {
            int srcX = (LIGHTNINGBOLT % TILES_IN_ROW) * TILE_SIZE;
            int srcY = (LIGHTNINGBOLT / TILES_IN_ROW) * TILE_SIZE;

            BitBlt(hdc, x, y, TILE_SIZE, TILE_SIZE, hdcTiles, srcX, srcY, SRCCOPY);
        }
    }
    /* Removed green frame for powered zones to improve visual appearance */

    /* Commented out power indicator dots for cleaner display
       else if ((tileValue & CONDBIT) && (tileValue & POWERBIT))
       {
        // Power lines and other conductors with power get a cyan dot
        int dotSize = 4;
        RECT dotRect;

        dotRect.left = rect.left + (TILE_SIZE - dotSize) / 2;
        dotRect.top = rect.top + (TILE_SIZE - dotSize) / 2;
        dotRect.right = dotRect.left + dotSize;
        dotRect.bottom = dotRect.top + dotSize;

        hBrush = CreateSolidBrush(RGB(0, 255, 255));
        FillRect(hdc, &dotRect, hBrush);
        DeleteObject(hBrush);
       }
     */

    /* Commented out traffic visualization dots for cleaner display
       {
        int tileBase = tileValue & LOMASK;
        Byte trafficLevel;

        if ((tileBase >= ROADBASE && tileBase <= LASTROAD) ||
            (tileBase >= RAILBASE && tileBase <= LASTRAIL))
        {
            // Get the traffic density for this location
            trafficLevel = TrfDensity[y/2][x/2];

            // Only display if there's significant traffic
            if (trafficLevel > 40) {
                COLORREF trafficColor;
                RECT trafficRect;
                int trafficSize;

                // Scale traffic visualization by density level
                if (trafficLevel < 100) {
                    trafficColor = RGB(255, 255, 0); // Yellow for light traffic
                    trafficSize = 2;
                } else if (trafficLevel < 200) {
                    trafficColor = RGB(255, 128, 0); // Orange for medium traffic
                    trafficSize = 3;
                } else {
                    trafficColor = RGB(255, 0, 0);   // Red for heavy traffic
                    trafficSize = 4;
                }

                // Draw traffic indicator
                trafficRect.left = rect.left + (TILE_SIZE - trafficSize) / 2;
                trafficRect.top = rect.top + (TILE_SIZE - trafficSize) / 2;
                trafficRect.right = trafficRect.left + trafficSize;
                trafficRect.bottom = trafficRect.top + trafficSize;

                hBrush = CreateSolidBrush(trafficColor);
                FillRect(hdc, &trafficRect, hBrush);
                DeleteObject(hBrush);
            }
        }
       }
     */
}

void drawCity(HDC hdc) {
    int x;
    int y;
    int screenX;
    int screenY;
    int startX;
    int startY;
    int endX;
    int endY;
    int barWidth;
    int maxHeight;
    int spacing;
    int rciStartX;
    int rciStartY;
    int rHeight;
    int cHeight;
    int iHeight;
    int localR;
    int localC;
    int localI;
    int cityMonth;
    int cityYear;
    int fundValue;
    int popValue;
    RECT rcClient;
    RECT rciRect;
    HBRUSH hResBrush;
    HBRUSH hComBrush;
    HBRUSH hIndBrush;
    HPEN hCenterPen;
    HPEN hOldPen;
    char *baseName;
    char *lastSlash;
    char *lastFwdSlash;
    char nameBuffer[MAX_PATH];
    char buffer[256];
    char *dot;

    /* Copy simulation values to local variables for display */
    cityMonth = CityMonth;
    cityYear = CityYear;
    fundValue = (int)TotalFunds;

    /* Display the current population */
    popValue = (int)CityPop;

    /* Calculate visible range */
    startX = xOffset / TILE_SIZE;
    startY = yOffset / TILE_SIZE;
    /* Adjust the width of the map view based on toolbar */
    endX = startX + ((cxClient - toolbarWidth) / TILE_SIZE) + 1;
    endY = startY + (cyClient / TILE_SIZE) + 1;

    /* Bounds check */
    if (startX < 0) {
        startX = 0;
    }
    if (startY < 0) {
        startY = 0;
    }
    if (endX > WORLD_X) {
        endX = WORLD_X;
    }
    if (endY > WORLD_Y) {
        endY = WORLD_Y;
    }

    /* Clear the background */
    rcClient.left = 0;
    rcClient.top = 0;
    rcClient.right = cxClient;
    rcClient.bottom = cyClient;
    FillRect(hdc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH));

    /* Draw the map tiles */
    for (y = startY; y < endY; y++) {
        for (x = startX; x < endX; x++) {
            screenX = x * TILE_SIZE - xOffset;
            screenY = y * TILE_SIZE - yOffset;

            drawTile(hdc, screenX, screenY, Map[y][x]);

            /* If power overlay is enabled, show power status with a transparent color overlay */
            if (powerOverlayEnabled) {
                RECT tileRect;
                HBRUSH hOverlayBrush;

                tileRect.left = screenX;
                tileRect.top = screenY;
                tileRect.right = screenX + TILE_SIZE;
                tileRect.bottom = screenY + TILE_SIZE;

                /* Skip power plants themselves */
                if ((Map[y][x] & LOMASK) != POWERPLANT && (Map[y][x] & LOMASK) != NUCLEAR) {
                    /* Create overlay effect - use red for unpowered zones and green for powered */
                    if (Map[y][x] & ZONEBIT) {
                        if (Map[y][x] & POWERBIT) {
                            /* Powered zones - bright green border */
                            hOverlayBrush = CreateSolidBrush(RGB(0, 255, 0));
                            /* Draw a thick green border for powered zones */
                            FrameRect(hdc, &tileRect, hOverlayBrush);
                            /* Add a small green power indicator in the corner */
                            Rectangle(hdc, tileRect.left + 2, tileRect.top + 2, tileRect.left + 6,
                                      tileRect.top + 6);
                            DeleteObject(hOverlayBrush);
                        } else {
                            /* Unpowered zones - red overlay */
                            hOverlayBrush = CreateSolidBrush(RGB(255, 0, 0));
                            /* Draw a thick red border for unpowered zones */
                            FrameRect(hdc, &tileRect, hOverlayBrush);
                            /* Add an X in the corner to indicate no power */
                            MoveToEx(hdc, tileRect.left + 2, tileRect.top + 2, NULL);
                            LineTo(hdc, tileRect.left + 6, tileRect.top + 6);
                            MoveToEx(hdc, tileRect.left + 6, tileRect.top + 2, NULL);
                            LineTo(hdc, tileRect.left + 2, tileRect.top + 6);
                            DeleteObject(hOverlayBrush);
                        }
                    } else if (PowerMap[y][x] == 1) {
                        /* Show power conducting elements (power lines, roads, etc.) clearly */
                        hOverlayBrush = CreateSolidBrush(RGB(0, 200, 0));
                        /* Show the power path with an overlay */
                        if ((Map[y][x] & LOMASK) >= POWERBASE &&
                            (Map[y][x] & LOMASK) < POWERBASE + 12) {
                            /* Power lines - make them bright */
                            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
                            HPEN hOldPen = SelectObject(hdc, hPen);
                            /* Draw a cross through the tile to indicate power flow */
                            MoveToEx(hdc, tileRect.left, tileRect.top, NULL);
                            LineTo(hdc, tileRect.right, tileRect.bottom);
                            MoveToEx(hdc, tileRect.right, tileRect.top, NULL);
                            LineTo(hdc, tileRect.left, tileRect.bottom);
                            SelectObject(hdc, hOldPen);
                            DeleteObject(hPen);
                        } else {
                            /* Other conductive tiles - highlight them */
                            Rectangle(hdc, tileRect.left + (TILE_SIZE / 2) - 1,
                                      tileRect.top + (TILE_SIZE / 2) - 1,
                                      tileRect.left + (TILE_SIZE / 2) + 2,
                                      tileRect.top + (TILE_SIZE / 2) + 2);
                        }
                        DeleteObject(hOverlayBrush);
                    }
                }

                /* Mark power plants with a yellow circle */
                if ((Map[y][x] & LOMASK) == POWERPLANT || (Map[y][x] & LOMASK) == NUCLEAR) {
                    HPEN hPen;
                    HPEN hOldPen;
                    HBRUSH hOldBrush;

                    hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 0));
                    hOldPen = SelectObject(hdc, hPen);
                    hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));

                    /* Draw a circle around the power plant */
                    Ellipse(hdc, screenX + 2, screenY + 2, screenX + TILE_SIZE - 2,
                            screenY + TILE_SIZE - 2);

                    SelectObject(hdc, hOldPen);
                    SelectObject(hdc, hOldBrush);
                    DeleteObject(hPen);
                }
            }
        }
    }

    /* Show legend for power overlay if enabled */
    if (powerOverlayEnabled) {
        RECT legendRect;
        RECT legendBackground;
        HBRUSH hLegendBrush;
        HBRUSH hBackgroundBrush;
        COLORREF oldTextColor;
        int legendX = (cxClient - toolbarWidth) - 300;
        int legendY = 10;
        int legendWidth = 150;
        int legendHeight = 120;

        /* Create semi-transparent background for the legend */
        legendBackground.left = legendX - 5;
        legendBackground.top = legendY - 5;
        legendBackground.right = legendX + legendWidth;
        legendBackground.bottom = legendY + legendHeight;

        hBackgroundBrush = CreateSolidBrush(RGB(50, 50, 50));
        FillRect(hdc, &legendBackground, hBackgroundBrush);
        DeleteObject(hBackgroundBrush);

        /* Set up text for legend */
        SetBkMode(hdc, TRANSPARENT);
        oldTextColor = SetTextColor(hdc, RGB(255, 255, 255));

        /* Header */
        TextOut(hdc, legendX, legendY, "Power Overlay Legend:", 21);
        legendY += 20;

        /* Powered zones */
        legendRect.left = legendX;
        legendRect.top = legendY;
        legendRect.right = legendX + 15;
        legendRect.bottom = legendY + 15;
        hLegendBrush = CreateSolidBrush(RGB(0, 255, 0));
        FrameRect(hdc, &legendRect, hLegendBrush);
        /* Add the green power indicator in corner */
        Rectangle(hdc, legendX + 2, legendY + 2, legendX + 6, legendY + 6);
        DeleteObject(hLegendBrush);
        TextOut(hdc, legendX + 20, legendY, "Powered Zones", 13);
        legendY += 20;

        /* Unpowered zones */
        legendRect.left = legendX;
        legendRect.top = legendY;
        legendRect.right = legendX + 15;
        legendRect.bottom = legendY + 15;
        hLegendBrush = CreateSolidBrush(RGB(255, 0, 0));
        FrameRect(hdc, &legendRect, hLegendBrush);
        /* Add an X to match visualization */
        MoveToEx(hdc, legendX + 2, legendY + 2, NULL);
        LineTo(hdc, legendX + 6, legendY + 6);
        MoveToEx(hdc, legendX + 6, legendY + 2, NULL);
        LineTo(hdc, legendX + 2, legendY + 6);
        DeleteObject(hLegendBrush);
        TextOut(hdc, legendX + 20, legendY, "Unpowered Zones", 15);
        legendY += 20;

        /* Power lines */
        legendRect.left = legendX;
        legendRect.top = legendY;
        legendRect.right = legendX + 15;
        legendRect.bottom = legendY + 15;
        hLegendBrush = CreateSolidBrush(RGB(0, 200, 0));
        FrameRect(hdc, &legendRect, hLegendBrush);

        /* Draw a cross through the tile to indicate power flow */
        {
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
            HPEN hOldPen = SelectObject(hdc, hPen);
            MoveToEx(hdc, legendX, legendY, NULL);
            LineTo(hdc, legendX + 15, legendY + 15);
            MoveToEx(hdc, legendX + 15, legendY, NULL);
            LineTo(hdc, legendX, legendY + 15);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
        }

        DeleteObject(hLegendBrush);
        TextOut(hdc, legendX + 20, legendY, "Power Lines", 11);
        legendY += 20;

        /* Other powered grid connections */
        legendRect.left = legendX;
        legendRect.top = legendY;
        legendRect.right = legendX + 15;
        legendRect.bottom = legendY + 15;
        hLegendBrush = CreateSolidBrush(RGB(0, 200, 0));
        FrameRect(hdc, &legendRect, hLegendBrush);
        /* Draw a dot in the center */
        Rectangle(hdc, legendX + 6, legendY + 6, legendX + 10, legendY + 10);
        DeleteObject(hLegendBrush);
        TextOut(hdc, legendX + 20, legendY, "Power Grid", 10);
        legendY += 20;

        /* Power plants */
        legendRect.left = legendX;
        legendRect.top = legendY;
        legendRect.right = legendX + 15;
        legendRect.bottom = legendY + 15;
        /* Use black brush for frame (not NULL_BRUSH which was causing a type error) */
        hLegendBrush = CreateSolidBrush(RGB(0, 0, 0));
        FrameRect(hdc, &legendRect, hLegendBrush);
        DeleteObject(hLegendBrush);

        /* Draw a yellow circle for power plants */
        {
            HPEN hPen;
            HPEN hOldPen;
            HBRUSH hOldBrush;

            hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 0));
            hOldPen = SelectObject(hdc, hPen);
            hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));

            Ellipse(hdc, legendX + 2, legendY + 2, legendX + 13, legendY + 13);

            SelectObject(hdc, hOldPen);
            SelectObject(hdc, hOldBrush);
            DeleteObject(hPen);
        }

        TextOut(hdc, legendX + 20, legendY, "Power Plants", 12);

        /* Restore text color */
        SetTextColor(hdc, oldTextColor);
    }

    /* Draw tool hover highlight if a tool is active */
    if (isToolActive) {
        /* Get mouse position */
        POINT mousePos;
        int mapX, mapY;

        GetCursorPos(&mousePos);
        ScreenToClient(hwndMain, &mousePos);

        /* Skip if mouse is outside client area or in toolbar */
        if (mousePos.x >= toolbarWidth && mousePos.y >= 0 && mousePos.x < cxClient &&
            mousePos.y < cyClient) {
            /* Convert to map coordinates */
            ScreenToMap(mousePos.x, mousePos.y, &mapX, &mapY, xOffset, yOffset);

            /* Draw the highlight box */
            DrawToolHover(hdc, mapX, mapY, GetCurrentTool(), xOffset, yOffset);
        }
    }

    /* Setup text drawing */
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
}

void openCityDialog(HWND hwnd) {
    OPENFILENAME ofn;
    char szFileName[MAX_PATH];

    szFileName[0] = '\0';

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "City Files (*.cty)\0*.cty\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
	#if(WINVER >= 0x0400)
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	#else
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	#endif
    ofn.lpstrDefExt = "cty";

    if (GetOpenFileName(&ofn)) {
        loadCity(szFileName);
    }
}

HMENU createMainMenu(void) {
    HMENU hMainMenu;
    HMENU hViewMenu;

    hMainMenu = CreateMenu();

    hFileMenu = CreatePopupMenu();
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_NEW, "&New...");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_OPEN, "&Open City...");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, "E&xit");

    hTilesetMenu = CreatePopupMenu();
    populateTilesetMenu(hTilesetMenu);

    hSimMenu = CreatePopupMenu();
    AppendMenu(hSimMenu, MF_STRING, IDM_SIM_PAUSE, "&Pause");
    AppendMenu(hSimMenu, MF_STRING, IDM_SIM_SLOW, "&Slow");
    AppendMenu(hSimMenu, MF_STRING, IDM_SIM_MEDIUM, "&Medium");
    AppendMenu(hSimMenu, MF_STRING, IDM_SIM_FAST, "&Fast");

    /* Default simulation speed is medium */
    CheckMenuRadioItem(hSimMenu, IDM_SIM_PAUSE, IDM_SIM_FAST, IDM_SIM_MEDIUM, MF_BYCOMMAND);

    /* Create scenario menu */
    hScenarioMenu = CreatePopupMenu();
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_DULLSVILLE, "&Dullsville (1900): Boredom");
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_SANFRANCISCO,
               "&San Francisco (1906): Earthquake");
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_HAMBURG, "&Hamburg (1944): Bombing");
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_BERN, "&Bern (1965): Traffic");
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_TOKYO, "&Tokyo (1957): Monster Attack");
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_DETROIT, "&Detroit (1972): Crime");
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_BOSTON, "&Boston (2010): Nuclear Meltdown");
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_RIO,
               "&Rio de Janeiro (2047): Coastal Flooding");

    /* Create tools menu */
    hToolMenu = CreatePopupMenu();

    /* Transportation Tools */
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_BULLDOZER, "&Bulldozer ($1)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_ROAD, "&Road ($10)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_RAIL, "Rail&road ($20)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_WIRE, "&Wire ($5)");
    AppendMenu(hToolMenu, MF_SEPARATOR, 0, NULL);

    /* Zone Tools */
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_RESIDENTIAL, "&Residential Zone ($100)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_COMMERCIAL, "&Commercial Zone ($100)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_INDUSTRIAL, "&Industrial Zone ($100)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_PARK, "Par&k ($10)");
    AppendMenu(hToolMenu, MF_SEPARATOR, 0, NULL);

    /* Public Services */
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_POLICESTATION, "Police &Station ($500)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_FIRESTATION, "&Fire Station ($500)");
    AppendMenu(hToolMenu, MF_SEPARATOR, 0, NULL);

    /* Special Buildings */
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_STADIUM, "S&tadium ($5000)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_SEAPORT, "Sea&port ($3000)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_AIRPORT, "&Airport ($10000)");
    AppendMenu(hToolMenu, MF_SEPARATOR, 0, NULL);

    /* Power */
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_POWERPLANT, "&Coal Power Plant ($3000)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_NUCLEAR, "&Nuclear Power Plant ($5000)");
    AppendMenu(hToolMenu, MF_SEPARATOR, 0, NULL);

    /* Query */
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_QUERY, "&Query");

    /* Default tool is bulldozer */
    CheckMenuRadioItem(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_BULLDOZER, MF_BYCOMMAND);

    /* Create View menu */
    hViewMenu = CreatePopupMenu();
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_INFOWINDOW, "&Info Window");
    /* Check it by default since the info window is shown on startup */
    CheckMenuItem(hViewMenu, IDM_VIEW_INFOWINDOW, MF_CHECKED);
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_LOGWINDOW, "&Log Window");
    /* Check it by default since the log window is now shown on startup */
    CheckMenuItem(hViewMenu, IDM_VIEW_LOGWINDOW, MF_CHECKED);
    AppendMenu(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_POWER_OVERLAY, "&Power Overlay");
    AppendMenu(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_DEBUG_LOGS, "Show &Debug Logs");
    /* Check it by default since debug logs are now enabled on startup */
    CheckMenuItem(hViewMenu, IDM_VIEW_DEBUG_LOGS, MF_CHECKED);

    AppendMenu(hMainMenu, MF_POPUP, (UINT)hFileMenu, "&File");
    AppendMenu(hMainMenu, MF_POPUP, (UINT)hScenarioMenu, "&Scenarios");
    AppendMenu(hMainMenu, MF_POPUP, (UINT)hTilesetMenu, "&Tileset");
    AppendMenu(hMainMenu, MF_POPUP, (UINT)hSimMenu, "&Speed");
    AppendMenu(hMainMenu, MF_POPUP, (UINT)hViewMenu, "&View");

    return hMainMenu;
}

void populateTilesetMenu(HMENU hSubMenu) {
    WIN32_FIND_DATA findData;
    HANDLE hFind;
    char searchPath[MAX_PATH];
    char fileName[MAX_PATH];
    char *dot;
    int menuId = IDM_TILESET_BASE;
    UINT menuFlags;

    strcpy(searchPath, "tilesets\\*.bmp");

    hFind = FindFirstFile(searchPath, &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) {
                continue;
            }

            strcpy(fileName, findData.cFileName);
            dot = strrchr(fileName, '.');
            if (dot != NULL) {
                *dot = '\0';
            }

            menuFlags = MF_STRING;
            if (strcmp(fileName, currentTileset) == 0) {
                menuFlags |= MF_CHECKED;
            }

            AppendMenu(hSubMenu, menuFlags, menuId++, fileName);

        } while (FindNextFile(hFind, &findData) && menuId < IDM_TILESET_MAX);

        FindClose(hFind);
    } else {
        AppendMenu(hSubMenu, MF_GRAYED, 0, "No tilesets found");
    }
}

/**
 * Creates a new empty map
 */
void createNewMap(HWND hwnd) {
    int x, y;
    
    /* Clear city filename since this is a new map */
    cityFileName[0] = '\0';
    
    /* Set default tileset for new maps */
    changeTileset(hwnd, "default");
    
    /* Update window title (override the tileset title) */
    SetWindowText(hwnd, "MicropolisNT - New City");
    
    /* Fill map with dirt */
    for (y = 0; y < WORLD_Y; y++) {
        for (x = 0; x < WORLD_X; x++) {
            Map[y][x] = TILE_DIRT;
        }
    }
    
    /* Reset scenario values */
    ScenarioID = 0;
    DisasterEvent = 0;
    DisasterWait = 0;
    
    /* Use fixed seed for consistent appearance */
    RandomlySeedRand();
    
    /* Reset simulation */
    DoSimInit();
    
    /* Reset funds to starting amount */
    TotalFunds = 50000;
    
    /* Start with medium speed */
    SetSimulationSpeed(hwnd, SPEED_MEDIUM);
    
    /* Set demand valves to initial values */
    SetValves(500, 300, 100);
    
    /* Add logging */
    addGameLog("Created new empty city");
    
    /* Force display update */
    InvalidateRect(hwnd, NULL, TRUE);
    
    /* Update info window */
    if (hwndInfo) {
        InvalidateRect(hwndInfo, NULL, TRUE);
    }
}
