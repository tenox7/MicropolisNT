/* tools.c - Tool handling code for MicropolisNT (Windows NT version)
 * Based on original Micropolis code from MicropolisLegacy project
 */

#include "tools.h"
#include "sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifndef MB_ICONWARNING
#define MB_ICONWARNING  MB_ICONASTERISK
#define MB_ICONERROR    MB_ICONSTOP
#endif

/* External reference to the toolbar width */
extern int toolbarWidth;

/* External reference to RCI values */
extern short RValve, CValve, IValve;

/* Constants for boolean values */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* External reference to main window handle */
extern HWND hwndMain;

/* External reference to Map array */
extern short Map[WORLD_Y][WORLD_X];

/* 
 * Tile connection tables for road, rail, and wire
 * Index is a 4-bit mask representing connections:
 * Bit 0 (value 1): North connection
 * Bit 1 (value 2): East connection
 * Bit 2 (value 4): South connection
 * Bit 3 (value 8): West connection
 */
/* MicropolisJS RoadTable exact mapping */
static short RoadTable[16] = {
    ROADS,         /* 0000 - No connections */
    ROADS2,        /* 0001 - North only */
    ROADS,         /* 0010 - East only */
    ROADS3,        /* 0011 - North, East */
    ROADS2,        /* 0100 - South only */
    ROADS2,        /* 0101 - North, South */
    ROADS4,        /* 0110 - East, South */
    ROADS8,        /* 0111 - North, East, South */
    ROADS,         /* 1000 - West only */
    ROADS6,        /* 1001 - North, West */
    ROADS,         /* 1010 - East, West */
    ROADS7,        /* 1011 - North, East, West */
    ROADS5,        /* 1100 - South, West */
    ROADS10,       /* 1101 - North, South, West */
    ROADS9,        /* 1110 - East, South, West */
    INTERSECTION   /* 1111 - All connections */
};

/* MicropolisJS RailTable exact mapping */
static short RailTable[16] = {
    LHRAIL,       /* 0000 - No connections */
    LVRAIL,       /* 0001 - North only */
    LHRAIL,       /* 0010 - East only */
    LVRAIL2,      /* 0011 - North, East */
    LVRAIL,       /* 0100 - South only */
    LVRAIL,       /* 0101 - North, South */
    LVRAIL3,      /* 0110 - East, South */
    LVRAIL7,      /* 0111 - North, East, South */
    LHRAIL,       /* 1000 - West only */
    LVRAIL5,      /* 1001 - North, West */
    LHRAIL,       /* 1010 - East, West */
    LVRAIL6,      /* 1011 - North, East, West */
    LVRAIL4,      /* 1100 - South, West */
    LVRAIL9,      /* 1101 - North, South, West */
    LVRAIL8,      /* 1110 - East, South, West */
    LVRAIL10      /* 1111 - All connections */
};

/* MicropolisJS WireTable exact mapping */
static short WireTable[16] = {
    LHPOWER,      /* 0000 - No connections */
    LVPOWER,      /* 0001 - North only */
    LHPOWER,      /* 0010 - East only */
    LVPOWER2,     /* 0011 - North, East */
    LVPOWER,      /* 0100 - South only */
    LVPOWER,      /* 0101 - North, South */
    LVPOWER3,     /* 0110 - East, South */
    LVPOWER7,     /* 0111 - North, East, South */
    LHPOWER,      /* 1000 - West only */
    LVPOWER5,     /* 1001 - North, West */
    LHPOWER,      /* 1010 - East, West */
    LVPOWER6,     /* 1011 - North, East, West */
    LVPOWER4,     /* 1100 - South, West */
    LVPOWER9,     /* 1101 - North, South, West */
    LVPOWER8,     /* 1110 - East, South, West */
    LVPOWER10     /* 1111 - All connections */
};

/* Tool cost constants */
#define TOOL_BULLDOZER_COST 1
#define TOOL_ROAD_COST 10
#define TOOL_RAIL_COST 20
#define TOOL_WIRE_COST 5
#define TOOL_PARK_COST 10
#define TOOL_RESIDENTIAL_COST 100
#define TOOL_COMMERCIAL_COST 100
#define TOOL_INDUSTRIAL_COST 100
#define TOOL_FIRESTATION_COST 500
#define TOOL_POLICESTATION_COST 500
#define TOOL_STADIUM_COST 5000
#define TOOL_SEAPORT_COST 3000
#define TOOL_POWERPLANT_COST 3000
#define TOOL_NUCLEAR_COST 5000
#define TOOL_AIRPORT_COST 10000
#define TOOL_NETWORK_COST 1000

/* Road and bridge cost constants */
#define ROAD_COST 10
#define BRIDGE_COST 50
#define RAIL_COST 20
#define TUNNEL_COST 100
#define WIRE_COST 5
#define UNDERWATER_WIRE_COST 25

/* Tool result constants */
#define TOOLRESULT_OK 0
#define TOOLRESULT_FAILED 1
#define TOOLRESULT_NO_MONEY 2
#define TOOLRESULT_NEED_BULLDOZE 3

/* Constants needed for tools.c */
#define TILE_SIZE 16  /* Size of each tile in pixels */
#define LASTTILE 960  /* Last possible tile value */

/* Special tiles not in simulation.h */
#define HANDBALL 5    /* Bridge helper tile */
#define LHBALL 6      /* Bridge helper tile */

/* Building and zone constants with TILE_ prefix for consistent naming */
#define TILE_WOODS WOODS
#define TILE_FIRESTBASE FIRESTBASE
#define TILE_FIRESTATION FIRESTATION
#define TILE_POLICESTBASE POLICESTBASE
#define TILE_POLICESTATION POLICESTATION
#define TILE_COALBASE COALBASE
#define TILE_POWERPLANT POWERPLANT
#define TILE_NUCLEARBASE NUCLEARBASE
#define TILE_NUCLEAR NUCLEAR
#define TILE_STADIUMBASE STADIUMBASE
#define TILE_STADIUM STADIUM
#define TILE_PORTBASE PORTBASE
#define TILE_PORT PORT
#define TILE_AIRPORTBASE AIRPORTBASE
#define TILE_AIRPORT AIRPORT

/* Using constants from simulation.h for zone ranges */
#define LASTAIRPORT LASTPORT     /* Last airport tile */
#define LASTFIRESTATION POLICESTBASE - 1 /* Last fire station tile */
#define LASTPOLICESTATION STADIUMBASE - 1 /* Last police station tile */
#define LASTNUCLEAR LASTZONE     /* Last nuclear plant tile */
#define LASTSTADIUM FULLSTADIUM  /* Last stadium tile */

/* Forward declarations for tile connection functions */
int ConnectTile(int x, int y, short *tilePtr, int command);
int LayDoze(int x, int y, short *tilePtr);
int LayRoad(int x, int y, short *tilePtr);
int LayRail(int x, int y, short *tilePtr);
int LayWire(int x, int y, short *tilePtr);
void FixZone(int x, int y, short *tilePtr);
void FixSingle(int x, int y);
short NormalizeRoad(short tile);

/* Zone deletion functions */
int checkSize(short tileValue);
int checkBigZone(short tileValue, int *deltaHPtr, int *deltaVPtr);
void put3x3Rubble(int x, int y);
void put4x4Rubble(int x, int y);
void put6x6Rubble(int x, int y);

/* Main connection function used for roads, rails, and wires */
int ConnectTile(int x, int y, short *tilePtr, int command) {
    short tile;
    int result = 1;

    /* Make sure the array subscripts are in bounds */
    if (!TestBounds(x, y)) {
        return 0;
    }

    /* AutoBulldoze feature - if trying to build road, rail, or wire */
    if (command >= 2 && command <= 4) {
        /* Check if we have funds and if the tile can be bulldozed */
        if (TotalFunds > 0 && ((tile = (*tilePtr)) & BULLBIT)) {
            /* Can bulldoze small objects and rubble */
            tile &= LOMASK;
            if (tile >= RUBBLE && tile <= LASTRUBBLE) {
                Spend(1);
                *tilePtr = DIRT;
            }
        }
    }

    /* Execute the appropriate command and fix zone */
    switch (command) {
    case 0: /* Fix zone */
        FixZone(x, y, tilePtr);
        break;

    case 1: /* Bulldoze */
        result = LayDoze(x, y, tilePtr);
        FixZone(x, y, tilePtr);
        break;

    case 2: /* Lay Road */
        result = LayRoad(x, y, tilePtr);
        FixZone(x, y, tilePtr);
        break;

    case 3: /* Lay Rail */
        result = LayRail(x, y, tilePtr);
        FixZone(x, y, tilePtr);
        break;

    case 4: /* Lay Wire */
        result = LayWire(x, y, tilePtr);
        FixZone(x, y, tilePtr);
        break;
    }

    return result;
}

/* Check size of a zone centered at this tile */
int checkSize(short tileValue) {
    /* Check for the normal com, resl, ind 3x3 zones & the fireDept & PoliceDept */
    if ((tileValue >= (RESBASE - 1) && tileValue <= (PORTBASE - 1)) ||
        (tileValue >= (LASTPOWERPLANT + 1) && tileValue <= (POLICESTATION + 4))) {
        return 3;
    }

    /* 4x4 zone buildings */
    if ((tileValue >= PORTBASE && tileValue <= LASTPORT) ||
        (tileValue >= COALBASE && tileValue <= LASTPOWERPLANT) ||
        (tileValue >= STADIUMBASE && tileValue <= LASTSTADIUM)) {
        return 4;
    }

    /* 6x6 zone buildings (airport) */
    if (tileValue >= AIRPORTBASE && tileValue <= LASTAIRPORT) {
        return 6;
    }

    return 0;
}

/* Check if this tile is part of a big zone and find its center */
int checkBigZone(short id, int *deltaHPtr, int *deltaVPtr) {
    /* Handle zone center tiles */
    switch (id) {
    case 750: /* POWERPLANT / TILE_POWERPLANT */
    case 698: /* PORT / TILE_PORT */
    case 816: /* NUCLEAR / TILE_NUCLEAR */
    case 785: /* STADIUM / TILE_STADIUM */
        *deltaHPtr = 0;
        *deltaVPtr = 0;
        return 4;

    case 716: /* AIRPORT / TILE_AIRPORT */
        *deltaHPtr = 0;
        *deltaVPtr = 0;
        return 6;
    }

    /* Coal power plant parts (745-754) */
    if (id == TILE_COALBASE + 1) {
        *deltaHPtr = -1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_COALBASE + 2) {
        *deltaHPtr = 0;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_COALBASE + 3) {
        *deltaHPtr = -1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_COALBASE + 4) {
        *deltaHPtr = 1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_COALBASE + 5) {
        *deltaHPtr = 1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_COALBASE + 6) {
        *deltaHPtr = 0;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_COALBASE + 7) {
        *deltaHPtr = -1;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_COALBASE + 8) {
        *deltaHPtr = 1;
        *deltaVPtr = 1;
        return 4;
    }
    /* Nuclear power plant parts (811-826) */
    else if (id == TILE_NUCLEARBASE + 1) {
        *deltaHPtr = -1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_NUCLEARBASE + 2) {
        *deltaHPtr = 0;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_NUCLEARBASE + 3) {
        *deltaHPtr = -1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_NUCLEARBASE + 4) {
        *deltaHPtr = 1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_NUCLEARBASE + 5) {
        *deltaHPtr = 1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_NUCLEARBASE + 6) {
        *deltaHPtr = 0;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_NUCLEARBASE + 7) {
        *deltaHPtr = -1;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_NUCLEARBASE + 8) {
        *deltaHPtr = 1;
        *deltaVPtr = 1;
        return 4;
    }
    /* Stadium parts (779-799) */
    else if (id == TILE_STADIUMBASE + 1) {
        *deltaHPtr = -1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_STADIUMBASE + 2) {
        *deltaHPtr = 0;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_STADIUMBASE + 3) {
        *deltaHPtr = -1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_STADIUMBASE + 4) {
        *deltaHPtr = 1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_STADIUMBASE + 5) {
        *deltaHPtr = 1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_STADIUMBASE + 6) {
        *deltaHPtr = 0;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_STADIUMBASE + 7) {
        *deltaHPtr = -1;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_STADIUMBASE + 8) {
        *deltaHPtr = 1;
        *deltaVPtr = 1;
        return 4;
    }
    /* Seaport parts (693-708) */
    else if (id == TILE_PORTBASE + 1) {
        *deltaHPtr = -1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_PORTBASE + 2) {
        *deltaHPtr = 0;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_PORTBASE + 3) {
        *deltaHPtr = -1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_PORTBASE + 4) {
        *deltaHPtr = 1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_PORTBASE + 5) {
        *deltaHPtr = 1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_PORTBASE + 6) {
        *deltaHPtr = 0;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_PORTBASE + 7) {
        *deltaHPtr = -1;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_PORTBASE + 8) {
        *deltaHPtr = 1;
        *deltaVPtr = 1;
        return 4;
    }
    /* Airport parts (709-744) */
    else if (id == TILE_AIRPORTBASE) {
        *deltaHPtr = -2;
        *deltaVPtr = -2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 1) {
        *deltaHPtr = -1;
        *deltaVPtr = -2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 2) {
        *deltaHPtr = 0;
        *deltaVPtr = -2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 3) {
        *deltaHPtr = 1;
        *deltaVPtr = -2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 4) {
        *deltaHPtr = 2;
        *deltaVPtr = -2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 5) {
        *deltaHPtr = 3;
        *deltaVPtr = -2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 6) {
        *deltaHPtr = -2;
        *deltaVPtr = -1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 7) {
        *deltaHPtr = -1;
        *deltaVPtr = -1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 8) {
        *deltaHPtr = 0;
        *deltaVPtr = -1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 9) {
        *deltaHPtr = 1;
        *deltaVPtr = -1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 10) {
        *deltaHPtr = 2;
        *deltaVPtr = -1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 11) {
        *deltaHPtr = 3;
        *deltaVPtr = -1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 12) {
        *deltaHPtr = -2;
        *deltaVPtr = 0;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 13) {
        *deltaHPtr = -1;
        *deltaVPtr = 0;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 14) {
        *deltaHPtr = 1;
        *deltaVPtr = 0;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 15) {
        *deltaHPtr = 2;
        *deltaVPtr = 0;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 16) {
        *deltaHPtr = 3;
        *deltaVPtr = 0;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 17) {
        *deltaHPtr = -2;
        *deltaVPtr = 1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 18) {
        *deltaHPtr = -1;
        *deltaVPtr = 1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 19) {
        *deltaHPtr = 0;
        *deltaVPtr = 1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 20) {
        *deltaHPtr = 1;
        *deltaVPtr = 1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 21) {
        *deltaHPtr = 2;
        *deltaVPtr = 1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 22) {
        *deltaHPtr = 3;
        *deltaVPtr = 1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 23) {
        *deltaHPtr = -2;
        *deltaVPtr = 2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 24) {
        *deltaHPtr = -1;
        *deltaVPtr = 2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 25) {
        *deltaHPtr = 0;
        *deltaVPtr = 2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 26) {
        *deltaHPtr = 1;
        *deltaVPtr = 2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 27) {
        *deltaHPtr = 2;
        *deltaVPtr = 2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 28) {
        *deltaHPtr = 3;
        *deltaVPtr = 2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 29) {
        *deltaHPtr = -2;
        *deltaVPtr = 3;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 30) {
        *deltaHPtr = -1;
        *deltaVPtr = 3;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 31) {
        *deltaHPtr = 0;
        *deltaVPtr = 3;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 32) {
        *deltaHPtr = 1;
        *deltaVPtr = 3;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 33) {
        *deltaHPtr = 2;
        *deltaVPtr = 3;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 34) {
        *deltaHPtr = 3;
        *deltaVPtr = 3;
        return 6;
    }
    /* Also handle base tiles */
    else if (id == TILE_COALBASE) {
        *deltaHPtr = 0;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_NUCLEARBASE) {
        *deltaHPtr = 0;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_STADIUMBASE) {
        *deltaHPtr = 0;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_PORTBASE) {
        *deltaHPtr = 0;
        *deltaVPtr = 0;
        return 4;
    }

    /* Not found */
    return 0;
}

/* Create 3x3 rubble */
void put3x3Rubble(int x, int y) {
    int xx, yy;
    short zz;
    short mask;

    /* First pass - clear all zone bits to avoid confusion during bulldozing */
    for (yy = y - 1; yy <= y + 1; yy++) {
        for (xx = x - 1; xx <= x + 1; xx++) {
            if (TestBounds(xx, yy)) {
                /* Clear the zone bit but preserve other flags */
                mask = Map[yy][xx] & ~ZONEBIT;
                Map[yy][xx] = mask;
            }
        }
    }

    /* Second pass - create the rubble */
    for (yy = y - 1; yy <= y + 1; yy++) {
        for (xx = x - 1; xx <= x + 1; xx++) {
            if (TestBounds(xx, yy)) {
                zz = Map[yy][xx] & LOMASK;
                if ((zz != RADTILE) && (zz != 0)) {
                    Map[yy][xx] = SOMETINYEXP | ANIMBIT | BULLBIT;
                }
            }
        }
    }
}

/* Create 4x4 rubble */
void put4x4Rubble(int x, int y) {
    int xx, yy;
    short zz;
    short mask;

    /* First pass - clear all zone bits to avoid confusion during bulldozing */
    for (yy = y - 1; yy <= y + 2; yy++) {
        for (xx = x - 1; xx <= x + 2; xx++) {
            if (TestBounds(xx, yy)) {
                /* Clear the zone bit but preserve other flags */
                mask = Map[yy][xx] & ~ZONEBIT;
                Map[yy][xx] = mask;
            }
        }
    }

    /* Second pass - create the rubble */
    for (yy = y - 1; yy <= y + 2; yy++) {
        for (xx = x - 1; xx <= x + 2; xx++) {
            if (TestBounds(xx, yy)) {
                zz = Map[yy][xx] & LOMASK;
                if ((zz != RADTILE) && (zz != 0)) {
                    Map[yy][xx] = SOMETINYEXP | ANIMBIT | BULLBIT;
                }
            }
        }
    }
}

/* Create 6x6 rubble */
void put6x6Rubble(int x, int y) {
    int xx, yy;
    short zz;
    short mask;

    /* First pass - clear all zone bits to avoid confusion during bulldozing */
    for (yy = y - 2; yy <= y + 3; yy++) {
        for (xx = x - 2; xx <= x + 3; xx++) {
            if (TestBounds(xx, yy)) {
                /* Clear the zone bit but preserve other flags */
                mask = Map[yy][xx] & ~ZONEBIT;
                Map[yy][xx] = mask;
            }
        }
    }

    /* Second pass - create the rubble */
    for (yy = y - 2; yy <= y + 3; yy++) {
        for (xx = x - 2; xx <= x + 3; xx++) {
            if (TestBounds(xx, yy)) {
                zz = Map[yy][xx] & LOMASK;
                if ((zz != RADTILE) && (zz != 0)) {
                    Map[yy][xx] = SOMETINYEXP | ANIMBIT | BULLBIT;
                }
            }
        }
    }
}

/* Bulldoze a tile */
int LayDoze(int x, int y, short *tilePtr) {
    short tile;
    int zoneSize = 0;
    int deltaH = 0;
    int deltaV = 0;

    /* If not in bounds, we cannot bulldoze */
    if (!TestBounds(x, y)) {
        return 0;
    }

    tile = *tilePtr & LOMASK;

    /* Empty land - nothing to bulldoze, return success */
    if (tile == DIRT) {
        return 1;
    }

    /* If funds are too low (we need at least $1) */
    if (TotalFunds < 1) {
        return 0;
    }

    /* Special case for water-related structures which cost more */
    if ((tile == HANDBALL || tile == LHBALL || tile == HBRIDGE || tile == VBRIDGE || tile == BRWH ||
         tile == BRWV) &&
        TotalFunds < 5) {
        return 0;
    }

    /* If this is a zone center, handle it specially */
    if (*tilePtr & ZONEBIT) {
        /* Get the zone size based on the tile value */
        zoneSize = checkSize(tile);

        switch (zoneSize) {
        case 3:
            /* Small 3x3 zone */
            Spend(1);
            put3x3Rubble(x, y);
            return 1;

        case 4:
            /* Medium 4x4 zone */
            Spend(1);
            put4x4Rubble(x, y);
            return 1;

        case 6:
            /* Large 6x6 zone (airport) */
            Spend(1);
            put6x6Rubble(x, y);
            return 1;

        default:
            /* Unknown zone type - convert to rubble instead of just clearing */
            Spend(1);
            *tilePtr = RUBBLE | BULLBIT;
            return 1;
        }
    }
    /* If not a zone center, check if it's part of a big zone */
    else if ((zoneSize = checkBigZone(tile, &deltaH, &deltaV))) {
        /* Adjust coordinates to the zone center */
        int centerX = x + deltaH;
        int centerY = y + deltaV;

        /* Verify new coords are in bounds */
        if (TestBounds(centerX, centerY)) {
            switch (zoneSize) {
            case 3:
                Spend(1);
                put3x3Rubble(centerX, centerY);
                return 1;

            case 4:
                Spend(1);
                put4x4Rubble(centerX, centerY);
                return 1;

            case 6:
                Spend(1);
                put6x6Rubble(centerX, centerY);
                return 1;

            default:
                /* For unknown zone size types, just bulldoze normally */
                break;
            }
        }
    }

    /* Can't bulldoze lakes, rivers, radiated tiles */
    if ((tile == RIVER) || (tile == REDGE) || (tile == CHANNEL) || (tile == RADTILE)) {
        return 0;
    }

    /* Bulldoze water-related structures back to water */
    if (tile == HANDBALL || tile == LHBALL || tile == HBRIDGE || tile == VBRIDGE || tile == BRWH ||
        tile == BRWV) {
        Spend(5);
        *tilePtr = RIVER;
        return 1;
    }

    /* General bulldozing of other tiles */
    Spend(1);
    *tilePtr = DIRT;
    return 1;
}

/* Lay road */
int LayRoad(int x, int y, short *tilePtr) {
    short cost;
    short tile = *tilePtr & LOMASK;

    if (tile == RIVER || tile == REDGE || tile == CHANNEL) {
        /* Build a bridge over water */
        cost = BRIDGE_COST;

        if (TotalFunds < cost) {
            return 0;
        }

        Spend(cost);
        if (((y > 0) && (Map[y - 1][x] & LOMASK) == VRAIL) ||
            ((y < WORLD_Y - 1) && (Map[y + 1][x] & LOMASK) == VRAIL)) {
            *tilePtr = VRAILROAD | BULLBIT;
        } else {
            *tilePtr = HBRIDGE | BULLBIT;
        }
        return 1;
    }
    
    /* Handle crossing a power line */
    if (tile >= POWERBASE && (tile <= LASTPOWER)) {
        cost = ROAD_COST;
        if (TotalFunds < cost) {
            return 0;
        }
        Spend(cost);
        /* Use the built-in road/power crossing tiles */
        if (y > 0 && y < WORLD_Y - 1 && 
            (Map[y-1][x] & LOMASK) >= POWERBASE && (Map[y-1][x] & LOMASK) <= LASTPOWER &&
            (Map[y+1][x] & LOMASK) >= POWERBASE && (Map[y+1][x] & LOMASK) <= LASTPOWER) {
            /* Vertical power line needs horizontal road crossing */
            *tilePtr = HROADPOWER | CONDBIT | BULLBIT | BURNBIT;
        } else {
            /* Horizontal power line needs vertical road crossing */
            *tilePtr = VROADPOWER | CONDBIT | BULLBIT | BURNBIT;
        }
        return 1;
    }

    if (tile == DIRT || (tile >= TINYEXP && tile <= LASTTINYEXP)) {
        /* Pave road on dirt */
        cost = ROAD_COST;

        if (TotalFunds < cost) {
            return 0;
        }

        Spend(cost);
        *tilePtr = ROADS | BULLBIT | BURNBIT;
        return 1;
    }

    /* Cannot build road on radioactive tiles or other pre-existing buildings */
    return 0;
}

/* Lay rail */
int LayRail(int x, int y, short *tilePtr) {
    short cost;
    short tile = *tilePtr & LOMASK;

    if (tile == RIVER || tile == REDGE || tile == CHANNEL) {
        /* Build a rail tunnel over water */
        cost = TUNNEL_COST;

        if (TotalFunds < cost) {
            return 0;
        }

        Spend(cost);
        *tilePtr = HRAIL | BULLBIT;
        return 1;
    }
    
    /* Handle crossing a power line */
    if (tile >= POWERBASE && (tile <= LASTPOWER)) {
        cost = RAIL_COST;
        if (TotalFunds < cost) {
            return 0;
        }
        Spend(cost);
        /* Use the built-in rail/power crossing tiles */
        if (y > 0 && y < WORLD_Y - 1 && 
            (Map[y-1][x] & LOMASK) >= POWERBASE && (Map[y-1][x] & LOMASK) <= LASTPOWER &&
            (Map[y+1][x] & LOMASK) >= POWERBASE && (Map[y+1][x] & LOMASK) <= LASTPOWER) {
            /* Vertical power line needs horizontal rail crossing */
            *tilePtr = RAILHPOWERV | CONDBIT | BULLBIT | BURNBIT;
        } else {
            /* Horizontal power line needs vertical rail crossing */
            *tilePtr = RAILVPOWERH | CONDBIT | BULLBIT | BURNBIT;
        }
        return 1;
    }

    if (tile == DIRT || (tile >= TINYEXP && tile <= LASTTINYEXP)) {
        /* Lay rail on dirt */
        cost = RAIL_COST;

        if (TotalFunds < cost) {
            return 0;
        }

        Spend(cost);
        *tilePtr = RAILBASE | BULLBIT | BURNBIT;
        return 1;
    }

    /* Cannot build rail on radioactive tiles or other pre-existing buildings */
    return 0;
}

/* Lay power lines */
int LayWire(int x, int y, short *tilePtr) {
    short cost;
    short tile;
    short connectMask;
    
    tile = *tilePtr & LOMASK;
    connectMask = 0;

    if (tile == RIVER || tile == REDGE || tile == CHANNEL) {
        /* Build underwater power lines */
        cost = UNDERWATER_WIRE_COST;

        if (TotalFunds < cost) {
            return 0;
        }

        Spend(cost);
        
        /* Build the connection mask manually for underwater power lines */
        /* Check North */
        if (y > 0 && ((Map[y-1][x] & CONDBIT) || 
            ((Map[y-1][x] & LOMASK) >= POWERBASE && (Map[y-1][x] & LOMASK) <= LASTPOWER))) {
            connectMask |= 1;
        }
        
        /* Check East */
        if (x < WORLD_X - 1 && ((Map[y][x+1] & CONDBIT) || 
            ((Map[y][x+1] & LOMASK) >= POWERBASE && (Map[y][x+1] & LOMASK) <= LASTPOWER))) {
            connectMask |= 2;
        }
        
        /* Check South */
        if (y < WORLD_Y - 1 && ((Map[y+1][x] & CONDBIT) || 
            ((Map[y+1][x] & LOMASK) >= POWERBASE && (Map[y+1][x] & LOMASK) <= LASTPOWER))) {
            connectMask |= 4;
        }
        
        /* Check West */
        if (x > 0 && ((Map[y][x-1] & CONDBIT) || 
            ((Map[y][x-1] & LOMASK) >= POWERBASE && (Map[y][x-1] & LOMASK) <= LASTPOWER))) {
            connectMask |= 8;
        }

        if (connectMask != 0) {
            /* Use the proper tile from the wire table */
            *tilePtr = WireTable[connectMask & 15] | CONDBIT | BULLBIT;
        } else if ((x > 0 && x < WORLD_X - 1) && 
                  (Map[y][x-1] & CONDBIT) && (Map[y][x+1] & CONDBIT)) {
            /* Horizontal connection needed */
            *tilePtr = HPOWER | CONDBIT | BULLBIT;
        } else {
            /* Default to vertical power line for underwater */
            *tilePtr = VPOWER | CONDBIT | BULLBIT; 
        }
        return 1;
    }

    /* Handle crossing a road */
    if (tile >= ROADBASE && tile <= LASTROAD) {
        cost = WIRE_COST;
        if (TotalFunds < cost) {
            return 0;
        }
        Spend(cost);
        /* Use the built-in road/power crossing tiles */
        if (x > 0 && x < WORLD_X - 1 && 
            (Map[y][x-1] & LOMASK) >= ROADBASE && (Map[y][x-1] & LOMASK) <= LASTROAD &&
            (Map[y][x+1] & LOMASK) >= ROADBASE && (Map[y][x+1] & LOMASK) <= LASTROAD) {
            /* Horizontal road needs vertical power line crossing */
            *tilePtr = VROADPOWER | CONDBIT | BULLBIT | BURNBIT;
        } else {
            /* Vertical road needs horizontal power line crossing */
            *tilePtr = HROADPOWER | CONDBIT | BULLBIT | BURNBIT;
        }
        return 1;
    }
    
    /* Handle crossing a rail */
    if (tile >= RAILBASE && tile <= LASTRAIL) {
        cost = WIRE_COST;
        if (TotalFunds < cost) {
            return 0;
        }
        Spend(cost);
        /* Use the built-in rail/power crossing tiles */
        if (x > 0 && x < WORLD_X - 1 && 
            (Map[y][x-1] & LOMASK) >= RAILBASE && (Map[y][x-1] & LOMASK) <= LASTRAIL &&
            (Map[y][x+1] & LOMASK) >= RAILBASE && (Map[y][x+1] & LOMASK) <= LASTRAIL) {
            /* Horizontal rail needs vertical power line crossing */
            *tilePtr = RAILVPOWERH | CONDBIT | BULLBIT | BURNBIT;
        } else {
            /* Vertical rail needs horizontal power line crossing */
            *tilePtr = RAILHPOWERV | CONDBIT | BULLBIT | BURNBIT;
        }
        return 1;
    }

    if (tile == DIRT || (tile >= TINYEXP && tile <= LASTTINYEXP)) {
        /* Lay wire on dirt */
        short connectMask;
        
        cost = WIRE_COST;

        if (TotalFunds < cost) {
            return 0;
        }

        Spend(cost);
        
        /* Build a connection mask to determine the proper wire tile */
        connectMask = 0;
        
        /* Check North */
        if (y > 0 && ((Map[y-1][x] & CONDBIT) || 
            ((Map[y-1][x] & LOMASK) >= POWERBASE && (Map[y-1][x] & LOMASK) <= LASTPOWER))) {
            connectMask |= 1;
        }
        
        /* Check East */
        if (x < WORLD_X - 1 && ((Map[y][x+1] & CONDBIT) || 
            ((Map[y][x+1] & LOMASK) >= POWERBASE && (Map[y][x+1] & LOMASK) <= LASTPOWER))) {
            connectMask |= 2;
        }
        
        /* Check South */
        if (y < WORLD_Y - 1 && ((Map[y+1][x] & CONDBIT) || 
            ((Map[y+1][x] & LOMASK) >= POWERBASE && (Map[y+1][x] & LOMASK) <= LASTPOWER))) {
            connectMask |= 4;
        }
        
        /* Check West */
        if (x > 0 && ((Map[y][x-1] & CONDBIT) || 
            ((Map[y][x-1] & LOMASK) >= POWERBASE && (Map[y][x-1] & LOMASK) <= LASTPOWER))) {
            connectMask |= 8;
        }
        
        /* If we have connections, choose the appropriate power line tile */
        if (connectMask != 0) {
            /* Use the proper tile from the wire table */
            *tilePtr = WireTable[connectMask & 15] | CONDBIT | BULLBIT | BURNBIT;
        } else {
            /* Special case for vertical alignment - if this is a second vertical tile, 
               use VPOWER instead of LHPOWER to avoid the upside-down L issue */
            if (y > 0 && (Map[y-1][x] & LOMASK) == VPOWER) {
                *tilePtr = VPOWER | CONDBIT | BULLBIT | BURNBIT;
            } else if (y < WORLD_Y - 1 && (Map[y+1][x] & LOMASK) == VPOWER) {
                *tilePtr = VPOWER | CONDBIT | BULLBIT | BURNBIT;
            } else {
                /* Default to LHPOWER (horizontal power line) */
                *tilePtr = LHPOWER | CONDBIT | BULLBIT | BURNBIT;
            }
        }
        return 1;
    }

    /* Cannot build power lines on radioactive tiles or other pre-existing buildings */
    return 0;
}

/* Fix zone - update connections of surrounding tiles */
void FixZone(int x, int y, short *tilePtr) {
    /* Check that coordinates are valid */
    if (!TestBounds(x, y)) {
        return;
    }

    FixSingle(x, y);

    if (TestBounds(x - 1, y)) {
        FixSingle(x - 1, y);
    }

    if (TestBounds(x + 1, y)) {
        FixSingle(x + 1, y);
    }

    if (TestBounds(x, y - 1)) {
        FixSingle(x, y - 1);
    }

    if (TestBounds(x, y + 1)) {
        FixSingle(x, y + 1);
    }
}

/* NormalizeRoad function - standardizes road tile values for comparison 
 * This is equivalent to MicropolisJS's normalizeRoad function
 */
short NormalizeRoad(short tile) {
    /* For roads, convert them to a standard set of tiles */
    if (tile >= ROADBASE && tile <= LASTROAD) {
        return (tile & 15) + ROADS;
    }
    return tile;
}

/* Fix a single tile - update its connections 
 * Closely follows MicropolisJS's fixSingle implementation
 */
void FixSingle(int x, int y) {
    short tile;
    short adjTile = 0; /* Adjacency bitmask */
    short mapValue;

    /* Verify the coordinates */
    if (!TestBounds(x, y)) {
        return;
    }

    tile = Map[y][x] & LOMASK;

    /* Skip some types of tiles */
    if (tile < 1 || tile >= LASTTILE) {
        return;
    }

    /* Normalize the current tile for comparison */
    tile = NormalizeRoad(tile);

    /* Check for road connections */
    if (tile >= ROADS && tile <= INTERSECTION) {
        /* Check the north side */
        if (y > 0) {
            mapValue = Map[y - 1][x] & LOMASK;
            mapValue = NormalizeRoad(mapValue);

            if ((mapValue == HRAILROAD || (mapValue >= ROADBASE && mapValue <= VROADPOWER)) &&
                mapValue != HROADPOWER && mapValue != VRAILROAD &&
                mapValue != ROADBASE) {
                adjTile |= 1;  /* North connection */
            }
        }

        /* Check the east side */
        if (x < WORLD_X - 1) {
            mapValue = Map[y][x + 1] & LOMASK;
            mapValue = NormalizeRoad(mapValue);

            if ((mapValue == VRAILROAD || (mapValue >= ROADBASE && mapValue <= VROADPOWER)) &&
                mapValue != VROADPOWER && mapValue != HRAILROAD &&
                mapValue != VBRIDGE) {
                adjTile |= 2;  /* East connection */
            }
        }

        /* Check the south side */
        if (y < WORLD_Y - 1) {
            mapValue = Map[y + 1][x] & LOMASK;
            mapValue = NormalizeRoad(mapValue);

            if ((mapValue == HRAILROAD || (mapValue >= ROADBASE && mapValue <= VROADPOWER)) &&
                mapValue != HROADPOWER && mapValue != VRAILROAD &&
                mapValue != ROADBASE) {
                adjTile |= 4;  /* South connection */
            }
        }

        /* Check the west side */
        if (x > 0) {
            mapValue = Map[y][x - 1] & LOMASK;
            mapValue = NormalizeRoad(mapValue);

            if ((mapValue == VRAILROAD || (mapValue >= ROADBASE && mapValue <= VROADPOWER)) &&
                mapValue != VROADPOWER && mapValue != HRAILROAD &&
                mapValue != VBRIDGE) {
                adjTile |= 8;  /* West connection */
            }
        }

        /* Update the road tile with proper connections */
        tile = RoadTable[adjTile];
        Map[y][x] = (Map[y][x] & MASKBITS) | tile | BULLBIT | BURNBIT;
        return;
    }

    /* Check for rail connections */
    if (tile >= LHRAIL && tile <= LVRAIL10) {
        /* Check the north side */
        if (y > 0) {
            mapValue = Map[y - 1][x] & LOMASK;
            mapValue = NormalizeRoad(mapValue);
            if (mapValue >= RAILHPOWERV && mapValue <= VRAILROAD &&
                mapValue != RAILHPOWERV && mapValue != HRAILROAD &&
                mapValue != HRAIL) {
                adjTile |= 1;  /* North connection */
            }
        }

        /* Check the east side */
        if (x < WORLD_X - 1) {
            mapValue = Map[y][x + 1] & LOMASK;
            mapValue = NormalizeRoad(mapValue);
            if (mapValue >= RAILHPOWERV && mapValue <= VRAILROAD &&
                mapValue != RAILVPOWERH && mapValue != VRAILROAD &&
                mapValue != VRAIL) {
                adjTile |= 2;  /* East connection */
            }
        }

        /* Check the south side */
        if (y < WORLD_Y - 1) {
            mapValue = Map[y + 1][x] & LOMASK;
            mapValue = NormalizeRoad(mapValue);
            if (mapValue >= RAILHPOWERV && mapValue <= VRAILROAD &&
                mapValue != RAILHPOWERV && mapValue != HRAILROAD &&
                mapValue != HRAIL) {
                adjTile |= 4;  /* South connection */
            }
        }

        /* Check the west side */
        if (x > 0) {
            mapValue = Map[y][x - 1] & LOMASK;
            mapValue = NormalizeRoad(mapValue);
            if (mapValue >= RAILHPOWERV && mapValue <= VRAILROAD &&
                mapValue != RAILVPOWERH && mapValue != VRAILROAD &&
                mapValue != VRAIL) {
                adjTile |= 8;  /* West connection */
            }
        }

        /* Update the rail tile with proper connections */
        tile = RailTable[adjTile];
        Map[y][x] = (Map[y][x] & MASKBITS) | tile | BULLBIT | BURNBIT;
        return;
    }

    /* Check for wire connections */
    if (tile >= LHPOWER && tile <= LVPOWER10) {
        /* Check the north side */
        if (y > 0) {
            if ((Map[y - 1][x] & CONDBIT) != 0) {
                mapValue = Map[y - 1][x] & LOMASK;
                mapValue = NormalizeRoad(mapValue);
                if (mapValue != VPOWER && mapValue != VROADPOWER && mapValue != RAILVPOWERH) {
                    adjTile |= 1;  /* North connection */
                }
            }
        }

        /* Check the east side */
        if (x < WORLD_X - 1) {
            if ((Map[y][x + 1] & CONDBIT) != 0) {
                mapValue = Map[y][x + 1] & LOMASK;
                mapValue = NormalizeRoad(mapValue);
                if (mapValue != HPOWER && mapValue != HROADPOWER && mapValue != RAILHPOWERV) {
                    adjTile |= 2;  /* East connection */
                }
            }
        }

        /* Check the south side */
        if (y < WORLD_Y - 1) {
            if ((Map[y + 1][x] & CONDBIT) != 0) {
                mapValue = Map[y + 1][x] & LOMASK;
                mapValue = NormalizeRoad(mapValue);
                if (mapValue != VPOWER && mapValue != VROADPOWER && mapValue != RAILVPOWERH) {
                    adjTile |= 4;  /* South connection */
                }
            }
        }

        /* Check the west side */
        if (x > 0) {
            if ((Map[y][x - 1] & CONDBIT) != 0) {
                mapValue = Map[y][x - 1] & LOMASK;
                mapValue = NormalizeRoad(mapValue);
                if (mapValue != HPOWER && mapValue != HROADPOWER && mapValue != RAILHPOWERV) {
                    adjTile |= 8;  /* West connection */
                }
            }
        }

        /* Update the wire tile with proper connections */
        tile = WireTable[adjTile];
        Map[y][x] = (Map[y][x] & MASKBITS) | tile | BULLBIT | BURNBIT | CONDBIT;
        return;
    }
    
    /* Handle the special crossing tiles */
    if (tile == HROADPOWER || tile == VROADPOWER || 
        tile == RAILHPOWERV || tile == RAILVPOWERH ||
        tile == HRAILROAD || tile == VRAILROAD) {
        /* These special tiles should trigger updates for surrounding tiles */
        if (y > 0) {
            FixSingle(x, y - 1);
        }
        if (x < WORLD_X - 1) {
            FixSingle(x + 1, y);
        }
        if (y < WORLD_Y - 1) {
            FixSingle(x, y + 1);
        }
        if (x > 0) {
            FixSingle(x - 1, y);
        }
        return;
    }
}

/* Toolbar state variables */
static int currentTool = bulldozerState;
static int toolResult = TOOLRESULT_OK;
static int toolCost = 0;

/* Last mouse position for hover effect */
static int lastMouseMapX = -1;
static int lastMouseMapY = -1;

/* Mapping of toolbar indices to tool states - this maps from toolbar position (0-17) to the
 * simulation.h tool state constants */
static const int toolbarToStateMapping[17] = {
    residentialState, /* 0 - Residential in toolbar position 0 */
    commercialState,  /* 1 - Commercial in toolbar position 1 */
    industrialState,  /* 2 - Industrial in toolbar position 2 */
    fireState,        /* 3 - Fire Station in toolbar position 3 */
    policeState,      /* 4 - Police Station in toolbar position 4 */
    wireState,        /* 5 - Wire in toolbar position 5 */
    roadState,        /* 6 - Road in toolbar position 6 */
    railState,        /* 7 - Rail in toolbar position 7 */
    parkState,        /* 8 - Park in toolbar position 8 */
    stadiumState,     /* 9 - Stadium in toolbar position 9 */
    seaportState,     /* 10 - Seaport in toolbar position 10 */
    powerState,       /* 11 - Power Plant in toolbar position 11 */
    nuclearState,     /* 12 - Nuclear Plant in toolbar position 12 */
    airportState,     /* 13 - Airport in toolbar position 13 */
    bulldozerState,   /* 14 - Bulldozer in toolbar position 14 */
    queryState,       /* 15 - Query in toolbar position 15 */
    noToolState       /* 16 - No Tool in toolbar position 16 */
};

/* Reverse mapping from tool state to toolbar position for fast lookups */
static const int stateToToolbarMapping[19] = {
    0,  /* residentialState (0) -> position 0 */
    1,  /* commercialState (1) -> position 1 */
    2,  /* industrialState (2) -> position 2 */
    3,  /* fireState (3) -> position 3 */
    4,  /* policeState (4) -> position 4 */
    5,  /* wireState (5) -> position 5 */
    6,  /* roadState (6) -> position 6 */
    7,  /* railState (7) -> position 7 */
    8,  /* parkState (8) -> position 8 */
    9,  /* stadiumState (9) -> position 9 */
    10, /* seaportState (10) -> position 10 */
    11, /* powerState (11) -> position 11 */
    12, /* nuclearState (12) -> position 12 */
    13, /* airportState (13) -> position 13 */
    0,  /* networkState (14) - not used in toolbar */
    14, /* bulldozerState (15) -> position 14 */
    15, /* queryState (16) -> position 15 */
    16, /* windowState (17) - not used in toolbar */
    16  /* noToolState (18) -> position 16 */
};

/* Tool active flag - needs to be exportable to main.c */
int isToolActive = 0; /* 0 = FALSE, 1 = TRUE */

/* Helper function to check 3x3 area for zone placement */
int Check3x3Area(int x, int y, int *cost) {
    int dx, dy;
    short tile;
    int clearCost = 0;

    /* Check bounds for a 3x3 zone */
    if (x < 1 || x >= WORLD_X - 1 || y < 1 || y >= WORLD_Y - 1) {
        return 0;
    }

    /* Check all tiles in the 3x3 area */
    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            tile = Map[y + dy][x + dx] & LOMASK;

            /* Check if tile is clear or can be bulldozed */
            if (tile != DIRT) {
                /* Can be bulldozed at additional cost */
                if (tile == RUBBLE || (tile >= TINYEXP && tile <= LASTTINYEXP)) {
                    clearCost += 1;
                } else {
                    /* Not buildable */
                    return 0;
                }
            }
        }
    }

    *cost = clearCost;
    return 1;
}

/* Helper function to check 4x4 area for zone placement */
int Check4x4Area(int x, int y, int *cost) {
    int dx, dy;
    short tile;
    int clearCost = 0;

    /* Check bounds for a 4x4 zone */
    if (x < 1 || x >= WORLD_X - 2 || y < 1 || y >= WORLD_Y - 2) {
        return 0;
    }

    /* Check all tiles in the 4x4 area */
    for (dy = -1; dy <= 2; dy++) {
        for (dx = -1; dx <= 2; dx++) {
            tile = Map[y + dy][x + dx] & LOMASK;

            /* Check if tile is clear or can be bulldozed */
            if (tile != DIRT) {
                /* Can be bulldozed at additional cost */
                if (tile == RUBBLE || (tile >= TINYEXP && tile <= LASTTINYEXP)) {
                    clearCost += 1;
                } else {
                    /* Not buildable */
                    return 0;
                }
            }
        }
    }

    *cost = clearCost;
    return 1;
}

/* Helper function to check 6x6 area for zone placement */
int Check6x6Area(int x, int y, int *cost) {
    int dx, dy;
    short tile;
    int clearCost = 0;

    /* Check bounds for a 6x6 zone */
    if (x < 2 || x >= WORLD_X - 3 || y < 2 || y >= WORLD_Y - 3) {
        return 0;
    }

    /* Check all tiles in the 6x6 area */
    for (dy = -2; dy <= 3; dy++) {
        for (dx = -2; dx <= 3; dx++) {
            tile = Map[y + dy][x + dx] & LOMASK;

            /* Check if tile is clear or can be bulldozed */
            if (tile != DIRT) {
                /* Can be bulldozed at additional cost */
                if (tile == RUBBLE || (tile >= TINYEXP && tile <= LASTTINYEXP)) {
                    clearCost += 1;
                } else {
                    /* Not buildable */
                    return 0;
                }
            }
        }
    }

    *cost = clearCost;
    return 1;
}

/* Toolbar button constants */
#define TB_BULLDOZER 100
#define TB_ROAD 101
#define TB_RAIL 102
#define TB_WIRE 103
#define TB_PARK 104
#define TB_RESIDENTIAL 105
#define TB_COMMERCIAL 106
#define TB_INDUSTRIAL 107
#define TB_FIRESTATION 108
#define TB_POLICESTATION 109
#define TB_STADIUM 110
#define TB_SEAPORT 111
#define TB_POWERPLANT 112
#define TB_NUCLEAR 113
#define TB_AIRPORT 114
#define TB_QUERY 115

static HWND hwndToolbar = NULL; /* Toolbar window handle */
static int toolButtonSize = 36; /* Size of each tool button, increased for better spacing */
static int toolbarWidth = 108;  /* Width of the toolbar (3 columns) */
static int toolbarColumns = 3;  /* Number of tool columns */

/* Tool bitmap handles */
static HBITMAP hToolBitmaps[17]; /* Tool bitmaps */

/* File names for tool bitmaps - order matches toolbar position */
static const char *toolBitmapFiles[17] = {
    "resident",		/* 0 - Residential */
    "commerce",		/* 1 - Commercial */
    "industrl",		/* 2 - Industrial */
    "firest",		/* 3 - Fire Station */
    "policest",		/* 4 - Police Station */
    "powerln",		/* 5 - Wire */
    "road",			/* 6 - Road */
    "rail",         /* 7 - Rail */
    "park",         /* 8 - Park */
    "stadium",      /* 9 - Stadium */
    "seaport",      /* 10 - Seaport */
    "powerpl",		/* 11 - Coal Power Plant */
    "nuclear",      /* 12 - Nuclear Power Plant */
    "airport",      /* 13 - Airport */
    "bulldzr",		/* 14 - Bulldozer */
    "query",        /* 15 - Query */
    "bulldzr"		/* 16 - No Tool (use a hand icon if available, otherwise bulldozer) */
};

/* Function to process toolbar button clicks */
LRESULT CALLBACK ToolbarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    int buttonY;
    int toolId;
    int i;
    int row;
    int col;
    int buttonX;
    int isSelected;
    int mouseX, mouseY;
    int toolIndex;
    HPEN hRedPen;
    HPEN hOldPen;

    switch (msg) {
    case WM_CREATE:
        return 0;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);

        /* Fill the background */
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, (HBRUSH)GetStockObject(LTGRAY_BRUSH));

        /* Draw the buttons in a 3-column grid */
        for (i = 0; i < 17; i++) {
            row = i / toolbarColumns;
            col = i % toolbarColumns;
            buttonX = col * toolButtonSize;
            buttonY = row * toolButtonSize;

            /* Set up button rect */
            rect.left = buttonX;
            rect.top = buttonY;
            rect.right = buttonX + toolButtonSize;
            rect.bottom = buttonY + toolButtonSize;

            /* Determine if this button is selected */
            isSelected = (GetCurrentTool() == toolbarToStateMapping[i]);

            /* Create a flat gray background for all buttons */
            FillRect(hdc, &rect, (HBRUSH)GetStockObject(LTGRAY_BRUSH));

            /* Draw a thin dark gray border */
            FrameRect(hdc, &rect, (HBRUSH)GetStockObject(DKGRAY_BRUSH));

            /* Draw the tool icon, with special handling for noToolState */
            if (toolbarToStateMapping[i] == noToolState) {
                /* Draw the tool icon */
                DrawToolIcon(hdc, bulldozerState, buttonX, buttonY, isSelected);

                /* Add a slash symbol to indicate "no tool" */

                /* Create a red pen for the slash */
                hRedPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
                hOldPen = SelectObject(hdc, hRedPen);

                /* Draw a diagonal line (slash) */
                MoveToEx(hdc, buttonX + 8, buttonY + 8, NULL);
                LineTo(hdc, buttonX + toolButtonSize - 8, buttonY + toolButtonSize - 8);

                /* Clean up */
                SelectObject(hdc, hOldPen);
                DeleteObject(hRedPen);
            } else {
                /* Draw the normal tool icon */
                DrawToolIcon(hdc, toolbarToStateMapping[i], buttonX, buttonY, isSelected);
            }
        }

        /* Draw RCI bars below the tool buttons */
        {
            /* Constants for RCI display - position below the last row of tools */
            int barWidth = 24;
            int maxHeight = 50;
            int spacing = 6;
            int rciStartX = 12;          /* Center in the toolbar (108px wide) */
            int rciStartY = 6 * 36 + 40; /* Position RCI below the last row of tools */
            int localR, localC, localI;
            int rHeight, cHeight, iHeight;
            RECT rciRect;
            HBRUSH hResBrush, hComBrush, hIndBrush;
            HPEN hCenterPen, hOldPen;

            /* Copy RCI values to local variables */
            localR = RValve;
            localC = CValve;
            localI = IValve;

            /* Limit values to range for drawing */
            if (localR > 2000) {
                localR = 2000;
            }
            if (localR < -2000) {
                localR = -2000;
            }
            if (localC > 2000) {
                localC = 2000;
            }
            if (localC < -2000) {
                localC = -2000;
            }
            if (localI > 2000) {
                localI = 2000;
            }
            if (localI < -2000) {
                localI = -2000;
            }

            /* Create brushes using system palette colors */
            hResBrush = CreateSolidBrush(RGB(0, 128, 0));   /* Dark Green */
            hComBrush = CreateSolidBrush(RGB(0, 0, 128));   /* Dark Blue */
            hIndBrush = CreateSolidBrush(RGB(128, 128, 0)); /* Dark Yellow */

            /* Error check for failed resource creation */
            if (!hResBrush || !hComBrush || !hIndBrush) {
                /* Clean up any resources that were created */
                if (hResBrush) {
                    DeleteObject(hResBrush);
                }
                if (hComBrush) {
                    DeleteObject(hComBrush);
                }
                if (hIndBrush) {
                    DeleteObject(hIndBrush);
                }
                EndPaint(hwnd, &ps);
                return 0;
            }

            /* Draw the horizontal center line */
            hCenterPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
            if (hCenterPen) {
                hOldPen = (HPEN)SelectObject(hdc, hCenterPen);
                MoveToEx(hdc, rciStartX - 5, rciStartY, NULL);
                LineTo(hdc, rciStartX + barWidth * 3 + spacing * 2 + 5, rciStartY);
                SelectObject(hdc, hOldPen);
            }

            /* Residential demand bar */
            if (localR >= 0) {
                rHeight = localR * maxHeight / 2000;
                rciRect.left = rciStartX;
                rciRect.right = rciStartX + barWidth;
                rciRect.bottom = rciStartY;
                rciRect.top = rciStartY - rHeight;
            } else {
                rHeight = -localR * maxHeight / 2000;
                rciRect.left = rciStartX;
                rciRect.right = rciStartX + barWidth;
                rciRect.top = rciStartY;
                rciRect.bottom = rciStartY + rHeight;
            }
            FillRect(hdc, &rciRect, hResBrush);

            /* Commercial demand bar */
            if (localC >= 0) {
                cHeight = localC * maxHeight / 2000;
                rciRect.left = rciStartX + barWidth + spacing;
                rciRect.right = rciStartX + barWidth * 2 + spacing;
                rciRect.bottom = rciStartY;
                rciRect.top = rciStartY - cHeight;
            } else {
                cHeight = -localC * maxHeight / 2000;
                rciRect.left = rciStartX + barWidth + spacing;
                rciRect.right = rciStartX + barWidth * 2 + spacing;
                rciRect.top = rciStartY;
                rciRect.bottom = rciStartY + cHeight;
            }
            FillRect(hdc, &rciRect, hComBrush);

            /* Industrial demand bar */
            if (localI >= 0) {
                iHeight = localI * maxHeight / 2000;
                rciRect.left = rciStartX + barWidth * 2 + spacing * 2;
                rciRect.right = rciStartX + barWidth * 3 + spacing * 2;
                rciRect.bottom = rciStartY;
                rciRect.top = rciStartY - iHeight;
            } else {
                iHeight = -localI * maxHeight / 2000;
                rciRect.left = rciStartX + barWidth * 2 + spacing * 2;
                rciRect.right = rciStartX + barWidth * 3 + spacing * 2;
                rciRect.top = rciStartY;
                rciRect.bottom = rciStartY + iHeight;
            }
            FillRect(hdc, &rciRect, hIndBrush);

            /* Add labels for RCI bars */
            SetTextColor(hdc, RGB(0, 0, 0)); /* Black text */
            SetBkMode(hdc, TRANSPARENT);
            TextOut(hdc, rciStartX + (barWidth / 2) - 4, rciStartY + 5, "R", 1);
            TextOut(hdc, rciStartX + barWidth + spacing + (barWidth / 2) - 4, rciStartY + 5, "C",
                    1);
            TextOut(hdc, rciStartX + barWidth * 2 + spacing * 2 + (barWidth / 2) - 4, rciStartY + 5,
                    "I", 1);

            /* Clean up GDI resources properly */
            if (hResBrush) {
                DeleteObject(hResBrush);
            }
            if (hComBrush) {
                DeleteObject(hComBrush);
            }
            if (hIndBrush) {
                DeleteObject(hIndBrush);
            }
            if (hCenterPen) {
                DeleteObject(hCenterPen);
            }
        }

        /* Show current tool information if a tool is active */
        if (isToolActive) {
            const char *toolName;
            int toolCost;
            char buffer[256];
            int textY = 6 * 36 + 55; /* Position below the RCI labels (rciStartY + 15) */

            /* Get the tool name */
            switch (GetCurrentTool()) {
            case bulldozerState:
                toolName = "Bulldozer";
                break;
            case roadState:
                toolName = "Road";
                break;
            case railState:
                toolName = "Rail";
                break;
            case wireState:
                toolName = "Wire";
                break;
            case parkState:
                toolName = "Park";
                break;
            case residentialState:
                toolName = "Residential Zone";
                break;
            case commercialState:
                toolName = "Commercial Zone";
                break;
            case industrialState:
                toolName = "Industrial Zone";
                break;
            case fireState:
                toolName = "Fire Station";
                break;
            case policeState:
                toolName = "Police Station";
                break;
            case stadiumState:
                toolName = "Stadium";
                break;
            case seaportState:
                toolName = "Seaport";
                break;
            case powerState:
                toolName = "Coal Power Plant";
                break;
            case nuclearState:
                toolName = "Nuclear Power Plant";
                break;
            case airportState:
                toolName = "Airport";
                break;
            case queryState:
                toolName = "Query";
                break;
            default:
                toolName = "Unknown Tool";
                break;
            }

            toolCost = GetToolCost();

            /* Show the tool name and cost in the toolbar */
            SetTextColor(hdc, RGB(0, 0, 0)); /* Black text */
            SetBkMode(hdc, TRANSPARENT);
            wsprintf(buffer, "Tool: %s", toolName);
            TextOut(hdc, 10, textY, buffer, lstrlen(buffer));

            if (toolCost > 0) {
                wsprintf(buffer, "Cost: $%d", toolCost);
                TextOut(hdc, 10, textY + 15, buffer, lstrlen(buffer));
            }
        }

        EndPaint(hwnd, &ps);
        return 0;

    case WM_LBUTTONDOWN: {
        /* Get mouse coordinates */
        mouseX = LOWORD(lParam);
        mouseY = HIWORD(lParam);

        /* Calculate row and column */
        col = mouseX / toolButtonSize;
        row = mouseY / toolButtonSize;

        /* Ensure col is within bounds */
        if (col >= toolbarColumns) {
            col = toolbarColumns - 1;
        }

        /* Calculate the tool index */
        toolIndex = row * toolbarColumns + col;

        if (toolIndex >= 0 && toolIndex < 17) {
            /* Select the corresponding tool using the mapping */
            SelectTool(toolbarToStateMapping[toolIndex]);

            /* Redraw the toolbar */
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }

    case WM_DESTROY:
        hwndToolbar = NULL;
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

/* Load all toolbar bitmap resources */
void LoadToolbarBitmaps(void) {
    int i;
    char filename[MAX_PATH];

    /* Load the renamed bitmaps directly from the images folder */
    for (i = 0; i < 17; i++) {
        /* Use only the new bitmap files with descriptive names */
        wsprintf(filename, "images\\%s.bmp", toolBitmapFiles[i]);

        /* Load the bitmap */
        hToolBitmaps[i] =
			loadBitmapFile(filename);
//            LoadImage(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);

        if (hToolBitmaps[i] == NULL) {
            /* Log error if bitmap loading fails */
            OutputDebugString("Failed to load bitmap: ");
            OutputDebugString(filename);
        }
    }
}

/* Clean up toolbar bitmap resources */
void CleanupToolbarBitmaps(void) {
    int i;

    /* Delete all bitmap handles */
    for (i = 0; i < 17; i++) {
        if (hToolBitmaps[i]) {
            DeleteObject(hToolBitmaps[i]);
            hToolBitmaps[i] = NULL;
        }
    }
}

/* Draw a tool icon using bitmap resources */
void DrawToolIcon(HDC hdc, int toolType, int x, int y, int isSelected) {
    HDC hdcMem;
    HBITMAP hbmOld;
    int toolIndex;
    BITMAP bm;
    int width, height;
    int centerX, centerY;
    RECT toolRect;
    HPEN hYellowPen;
    HPEN hOldPen;
    char debugMsg[100];
    char indexStr[8];
    RECT rect;
    int margin; /* Margin around tool icons */

    /* Get the toolbar index using the reverse mapping */
    if (toolType >= 0 && toolType < 17) {
        toolIndex = stateToToolbarMapping[toolType];
    } else {
        toolIndex = 0;
    }

    /* Make sure index is in range */
    if (toolIndex < 0 || toolIndex >= 16) {
        return;
    }

    /* Create a memory DC */
    hdcMem = CreateCompatibleDC(hdc);

    /* Select the bitmap (we only have one version now) */
    if (hToolBitmaps[toolIndex]) {
        hbmOld = SelectObject(hdcMem, hToolBitmaps[toolIndex]);

        /* Get the bitmap dimensions */
        if (GetObject(hToolBitmaps[toolIndex], sizeof(BITMAP), &bm) == 0) {
            /* If GetObject fails, use default dimensions */
            bm.bmWidth = 24;
            bm.bmHeight = 24;

            /* Log the error */
            wsprintf(debugMsg, "GetObject failed for bitmap %d", toolIndex);
            OutputDebugString(debugMsg);
        }
    } else {
        /* Failed to load bitmap - fall back to drawing a rectangle with tool name */
        wsprintf(debugMsg, "No bitmap for tool %d", toolIndex);
        OutputDebugString(debugMsg);

        rect.left = x + 4;
        rect.top = y + 4;
        rect.right = x + 28;
        rect.bottom = y + 28;

        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

        /* Show tool index as a visual indicator */
        wsprintf(indexStr, "%d", toolIndex);
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        TextOut(hdc, x + 12, y + 12, indexStr, lstrlen(indexStr));

        DeleteDC(hdcMem);
        return;
    }

    width = bm.bmWidth;
    height = bm.bmHeight;

    /* Calculate centering position within the button with minimum margin of 4 pixels */
    margin = 4;

    /* Ensure we don't exceed button size minus margin */
    if (width > toolButtonSize - 2 * margin) {
        width = toolButtonSize - 2 * margin;
    }
    if (height > toolButtonSize - 2 * margin) {
        height = toolButtonSize - 2 * margin;
    }

    /* Calculate centered position */
    centerX = x + (toolButtonSize - width) / 2;
    centerY = y + (toolButtonSize - height) / 2;

    /* Make sure we respect minimum margins */
    if (centerX < x + margin) {
        centerX = x + margin;
    }
    if (centerY < y + margin) {
        centerY = y + margin;
    }

    /* Draw the bitmap with proper sizing */
    StretchBlt(hdc, centerX, centerY, width, height, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight,
               SRCCOPY);

    /* If this tool is selected, draw a thick yellow box around it */
    if (isSelected) {
        /* Draw the highlight box using the BUTTON boundaries, not the icon */
        toolRect.left = x + 2;
        toolRect.top = y + 2;
        toolRect.right = x + toolButtonSize - 2;
        toolRect.bottom = y + toolButtonSize - 2;

        /* Create a yellow pen for the outline */
        hYellowPen = CreatePen(PS_SOLID, 3, RGB(255, 255, 0));
        hOldPen = SelectObject(hdc, hYellowPen);

        /* Draw the rectangle */
        SelectObject(hdc, GetStockObject(NULL_BRUSH)); /* Transparent interior */
        Rectangle(hdc, toolRect.left, toolRect.top, toolRect.right, toolRect.bottom);

        /* Clean up the pen */
        SelectObject(hdc, hOldPen);
        DeleteObject(hYellowPen);
    }

    /* Clean up */
    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
}

/* Create toolbar window */
void CreateToolbar(HWND hwndParent, int x, int y, int width, int height) {
    WNDCLASS wc;
    RECT clientRect;

    /* Load the tool bitmaps */
    LoadToolbarBitmaps();

    /* Register the toolbar window class if not already done */
    if (!GetClassInfo(NULL, "MicropolisToolbar", &wc)) {
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = ToolbarProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = "MicropolisToolbar";

        RegisterClass(&wc);
    }

    /* Create the toolbar window */
    GetClientRect(hwndParent, &clientRect);

    hwndToolbar = CreateWindowEx(
        WS_EX_DLGMODALFRAME, /* Using dialog frame style for a raised appearance */
        "MicropolisToolbar", NULL,
        WS_CHILD | WS_VISIBLE, /* Removed WS_BORDER as DLGMODALFRAME provides its own border */
        0, 0,                  /* x, y - will be adjusted below */
        toolbarWidth, clientRect.bottom, hwndParent, NULL, GetModuleHandle(NULL), NULL);

    /* Set the tool to bulldozer by default */
    SelectTool(bulldozerState);

    /* Force a redraw of the toolbar */
    if (hwndToolbar) {
        InvalidateRect(hwndToolbar, NULL, TRUE);
    }
}

/* Select a tool - set the current tool and activate it */
void SelectTool(int toolType) {
    /* Update the current tool */
    currentTool = toolType;

    /* Set tool active state based on tool type */
    isToolActive = (toolType != noToolState);

    /* Only redraw the toolbar - the main window doesn't need to be redrawn completely */
    if (hwndToolbar) {
        InvalidateRect(hwndToolbar, NULL, TRUE);
    }

    /* Update tool cost */
    switch (currentTool) {
    case bulldozerState:
        toolCost = TOOL_BULLDOZER_COST;
        break;

    case roadState:
        toolCost = TOOL_ROAD_COST;
        break;

    case railState:
        toolCost = TOOL_RAIL_COST;
        break;

    case wireState:
        toolCost = TOOL_WIRE_COST;
        break;

    case parkState:
        toolCost = TOOL_PARK_COST;
        break;

    case residentialState:
        toolCost = TOOL_RESIDENTIAL_COST;
        break;

    case commercialState:
        toolCost = TOOL_COMMERCIAL_COST;
        break;

    case industrialState:
        toolCost = TOOL_INDUSTRIAL_COST;
        break;

    case fireState:
        toolCost = TOOL_FIRESTATION_COST;
        break;

    case policeState:
        toolCost = TOOL_POLICESTATION_COST;
        break;

    case stadiumState:
        toolCost = TOOL_STADIUM_COST;
        break;

    case seaportState:
        toolCost = TOOL_SEAPORT_COST;
        break;

    case powerState:
        toolCost = TOOL_POWERPLANT_COST;
        break;

    case nuclearState:
        toolCost = TOOL_NUCLEAR_COST;
        break;

    case airportState:
        toolCost = TOOL_AIRPORT_COST;
        break;

    case queryState:
        toolCost = 0; /* Query tool is free */
        break;

    default:
        toolCost = 0;
        break;
    }
}

/* Check if we have enough funds for the current tool */
int CheckFunds(int cost) {
    /* Don't charge in certain situations */
    if (cost <= 0) {
        return 1;
    }

    if (TotalFunds >= cost) {
        return 1;
    }

    return 0;
}

/* Apply the bulldozer tool */
int DoBulldozer(int mapX, int mapY) {
    short result;
    short tile;
    int zoneSize;
    int deltaH = 0;
    int deltaV = 0;

    /* Check bounds */
    if (mapX < 0 || mapX >= WORLD_X || mapY < 0 || mapY >= WORLD_Y) {
        return TOOLRESULT_FAILED;
    }

    /* Get current tile */
    tile = Map[mapY][mapX] & LOMASK;

    /* If this is empty land, just return success - nothing to do */
    if (tile == DIRT) {
        return TOOLRESULT_OK;
    }

    /* If funds are too low (we need at least $1) */
    if (TotalFunds < 1) {
        return TOOLRESULT_NO_MONEY;
    }

    /* Special case for water-related structures which cost more */
    if ((tile == RIVER || tile == REDGE || tile == CHANNEL || tile == HANDBALL || tile == LHBALL ||
         tile == HBRIDGE || tile == VBRIDGE || tile == BRWH || tile == BRWV) &&
        TotalFunds < 5) {
        return TOOLRESULT_NO_MONEY;
    }

    /* First check if it's part of a zone or big building */
    if (Map[mapY][mapX] & ZONEBIT) {
        /* Direct center-tile bulldozing */
        zoneSize = checkSize(tile);
        switch (zoneSize) {
        case 3:
            Spend(1);
            put3x3Rubble(mapX, mapY);
            return TOOLRESULT_OK;

        case 4:
            Spend(1);
            put4x4Rubble(mapX, mapY);
            return TOOLRESULT_OK;

        case 6:
            Spend(1);
            put6x6Rubble(mapX, mapY);
            return TOOLRESULT_OK;

        default:
            /* Fall through to normal bulldozing for unknown zone types */
            break;
        }
    }
    /* Check if it's part of a large zone but not the center */
    else if ((zoneSize = checkBigZone(tile, &deltaH, &deltaV))) {
        /* Part of a large zone, adjust to the center */
        int centerX = mapX + deltaH;
        int centerY = mapY + deltaV;

        /* Make sure the adjusted coordinates are within bounds */
        if (TestBounds(centerX, centerY)) {
            switch (zoneSize) {
            case 3:
                Spend(1);
                put3x3Rubble(centerX, centerY);
                return TOOLRESULT_OK;

            case 4:
                Spend(1);
                put4x4Rubble(centerX, centerY);
                return TOOLRESULT_OK;

            case 6:
                Spend(1);
                put6x6Rubble(centerX, centerY);
                return TOOLRESULT_OK;
            }
        }
    }

    /* Regular tile bulldozing - direct approach without ConnectTile */
    if (tile == RIVER || tile == REDGE || tile == CHANNEL) {
        /* Can't bulldoze natural water features */
        return TOOLRESULT_FAILED;
    } else if (tile == HANDBALL || tile == LHBALL || tile == HBRIDGE || tile == VBRIDGE ||
               tile == BRWH || tile == BRWV) {
        /* Water-related structures cost $5 */
        Spend(5);
        Map[mapY][mapX] = RIVER; /* Convert back to water */
    } else if (tile == RADTILE) {
        /* Can't bulldoze radiation */
        return TOOLRESULT_FAILED;
    } else {
        /* All other tiles cost $1 */
        Spend(1);
        Map[mapY][mapX] = DIRT;
    }

    /* Fix neighboring tiles after bulldozing */
    FixZone(mapX, mapY, &Map[mapY][mapX]);

    return TOOLRESULT_OK;
}

/* Apply the road tool */
int DoRoad(int mapX, int mapY) {
    short result;
    short baseTile;

    /* Check bounds */
    if (mapX < 0 || mapX >= WORLD_X || mapY < 0 || mapY >= WORLD_Y) {
        return TOOLRESULT_FAILED;
    }

    baseTile = Map[mapY][mapX] & LOMASK;

    /* Check if we need to bulldoze first - now allows roads over power lines */
    if (baseTile != DIRT && baseTile != RIVER && baseTile != REDGE && baseTile != CHANNEL &&
        !(baseTile >= TINYEXP && baseTile <= LASTTINYEXP) &&
        !(baseTile >= ROADBASE && baseTile <= LASTROAD) &&
        !(baseTile >= RAILBASE && baseTile <= LASTRAIL) &&
        !(baseTile >= POWERBASE && baseTile <= LASTPOWER)) {
        return TOOLRESULT_NEED_BULLDOZE;
    }

    /* Check if we have enough money - maximum cost case */
    if ((baseTile == RIVER || baseTile == REDGE || baseTile == CHANNEL) &&
        !CheckFunds(BRIDGE_COST)) {
        return TOOLRESULT_NO_MONEY;
    } else if (!CheckFunds(ROAD_COST)) {
        return TOOLRESULT_NO_MONEY;
    }

    /* Use Connect tile to build and connect the road (command 2) */
    result = ConnectTile(mapX, mapY, &Map[mapY][mapX], 2);

    if (result == 0) {
        return TOOLRESULT_FAILED;
    }

    return TOOLRESULT_OK;
}

/* Apply the rail tool */
int DoRail(int mapX, int mapY) {
    short result;
    short baseTile;

    /* Check bounds */
    if (mapX < 0 || mapX >= WORLD_X || mapY < 0 || mapY >= WORLD_Y) {
        return TOOLRESULT_FAILED;
    }

    baseTile = Map[mapY][mapX] & LOMASK;

    /* Check if we need to bulldoze first - now allows rails over power lines */
    if (baseTile != DIRT && baseTile != RIVER && baseTile != REDGE && baseTile != CHANNEL &&
        !(baseTile >= TINYEXP && baseTile <= LASTTINYEXP) &&
        !(baseTile >= ROADBASE && baseTile <= LASTROAD) &&
        !(baseTile >= RAILBASE && baseTile <= LASTRAIL) &&
        !(baseTile >= POWERBASE && baseTile <= LASTPOWER)) {
        return TOOLRESULT_NEED_BULLDOZE;
    }

    /* Check if we have enough money - maximum cost case */
    if ((baseTile == RIVER || baseTile == REDGE || baseTile == CHANNEL) &&
        !CheckFunds(TUNNEL_COST)) {
        return TOOLRESULT_NO_MONEY;
    } else if (!CheckFunds(RAIL_COST)) {
        return TOOLRESULT_NO_MONEY;
    }

    /* Use Connect tile to build and connect the rail (command 3) */
    result = ConnectTile(mapX, mapY, &Map[mapY][mapX], 3);

    if (result == 0) {
        return TOOLRESULT_FAILED;
    }

    return TOOLRESULT_OK;
}

/* Apply the wire tool */
int DoWire(int mapX, int mapY) {
    short result;
    short baseTile;

    /* Check bounds */
    if (mapX < 0 || mapX >= WORLD_X || mapY < 0 || mapY >= WORLD_Y) {
        return TOOLRESULT_FAILED;
    }

    baseTile = Map[mapY][mapX] & LOMASK;

    /* Check if we need to bulldoze first - now allows power over roads and rails */
    if (baseTile != DIRT && baseTile != RIVER && baseTile != REDGE && baseTile != CHANNEL &&
        !(baseTile >= TINYEXP && baseTile <= LASTTINYEXP) &&
        !(baseTile >= ROADBASE && baseTile <= LASTROAD) &&
        !(baseTile >= RAILBASE && baseTile <= LASTRAIL)) {
        return TOOLRESULT_NEED_BULLDOZE;
    }

    /* Check if we have enough money - maximum cost case */
    if ((baseTile == RIVER || baseTile == REDGE || baseTile == CHANNEL) &&
        !CheckFunds(UNDERWATER_WIRE_COST)) {
        return TOOLRESULT_NO_MONEY;
    } else if (!CheckFunds(WIRE_COST)) {
        return TOOLRESULT_NO_MONEY;
    }

    /* Use Connect tile to build and connect the wire (command 4) */
    result = ConnectTile(mapX, mapY, &Map[mapY][mapX], 4);

    if (result == 0) {
        return TOOLRESULT_FAILED;
    }

    return TOOLRESULT_OK;
}

/* Apply the park tool */
int DoPark(int mapX, int mapY) {
    short tile;
    int randval;

    /* Check bounds */
    if (mapX < 0 || mapX >= WORLD_X || mapY < 0 || mapY >= WORLD_Y) {
        return TOOLRESULT_FAILED;
    }

    tile = Map[mapY][mapX] & LOMASK;

    /* Parks can only be built on clear land */
    if (tile != TILE_DIRT) {
        return TOOLRESULT_NEED_BULLDOZE;
    }

    /* Check if we have enough money */
    if (!CheckFunds(TOOL_PARK_COST)) {
        return TOOLRESULT_NO_MONEY;
    }

    Spend(TOOL_PARK_COST); /* Deduct cost */

    /* Random park type */
    randval = SimRandom(4);

    /* Set the tile to a random park tile */
    Map[mapY][mapX] = (randval + TILE_WOODS) | BURNBIT | BULLBIT;

    return TOOLRESULT_OK;
}

/* Helper function for placing a 3x3 zone */
int PlaceZone(int mapX, int mapY, int baseValue, int totalCost) {
    int dx, dy;
    int index = 0;
    int bulldozeCost = 0;

    /* Check if we can build here */
    if (!Check3x3Area(mapX, mapY, &bulldozeCost)) {
        return TOOLRESULT_FAILED;
    }

    /* Total cost includes zone cost plus any clearing costs */
    totalCost += bulldozeCost;

    /* Check if we have enough money */
    if (!CheckFunds(totalCost)) {
        return TOOLRESULT_NO_MONEY;
    }

    /* Clear area if needed and charge for it */
    if (bulldozeCost > 0) {
        for (dy = -1; dy <= 1; dy++) {
            for (dx = -1; dx <= 1; dx++) {
                short tile = Map[mapY + dy][mapX + dx] & LOMASK;
                if (tile != DIRT && (tile == RUBBLE || (tile >= TINYEXP && tile <= LASTTINYEXP))) {
                    Map[mapY + dy][mapX + dx] = DIRT;
                }
            }
        }
        Spend(bulldozeCost);
    }

    /* Charge for the zone */
    Spend(totalCost - bulldozeCost);

    /* Place the 3x3 zone with proper power conductivity for ALL tiles */
    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) {
                /* Center tile gets ZONEBIT, CONDBIT, and BULLBIT */
                Map[mapY][mapX] = baseValue + 4 | ZONEBIT | BULLBIT | CONDBIT;
            } else {
                /* All other tiles in the zone get CONDBIT too for proper power distribution */
                Map[mapY + dy][mapX + dx] = baseValue + index | BULLBIT | CONDBIT;
            }
            index++;
        }
    }

    /* Fix the zone edges to connect with neighbors */
    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            FixZone(mapX + dx, mapY + dy, &Map[mapY + dy][mapX + dx]);
        }
    }

    return TOOLRESULT_OK;
}

/* Apply the residential zone tool */
int DoResidential(int mapX, int mapY) {
    /* Use the zone helper for residential zones */
    return PlaceZone(mapX, mapY, RESBASE, TOOL_RESIDENTIAL_COST);
}

/* Apply the commercial zone tool */
int DoCommercial(int mapX, int mapY) {
    /* Use the zone helper for commercial zones */
    return PlaceZone(mapX, mapY, COMBASE, TOOL_COMMERCIAL_COST);
}

/* Apply the industrial zone tool */
int DoIndustrial(int mapX, int mapY) {
    /* Use the zone helper for industrial zones */
    return PlaceZone(mapX, mapY, INDBASE, TOOL_INDUSTRIAL_COST);
}

/* Apply the fire station tool */
int DoFireStation(int mapX, int mapY) {
    /* Use the zone helper for fire stations */
    return PlaceZone(mapX, mapY, TILE_FIRESTBASE, TOOL_FIRESTATION_COST);
}

/* Apply the police station tool */
int DoPoliceStation(int mapX, int mapY) {
    /* Use the zone helper for police stations */
    return PlaceZone(mapX, mapY, TILE_POLICESTBASE, TOOL_POLICESTATION_COST);
}

/* Helper function for placing a 4x4 building */
int Place4x4Building(int mapX, int mapY, int baseValue, int centerTile, int totalCost) {
    int dx, dy;
    int index = 0;
    int bulldozeCost = 0;

    /* Check if we can build here */
    if (!Check4x4Area(mapX, mapY, &bulldozeCost)) {
        return TOOLRESULT_FAILED;
    }

    /* Total cost includes building cost plus any clearing costs */
    totalCost += bulldozeCost;

    /* Check if we have enough money */
    if (!CheckFunds(totalCost)) {
        return TOOLRESULT_NO_MONEY;
    }

    /* Clear area if needed and charge for it */
    if (bulldozeCost > 0) {
        for (dy = -1; dy <= 2; dy++) {
            for (dx = -1; dx <= 2; dx++) {
                short tile = Map[mapY + dy][mapX + dx] & LOMASK;
                if (tile != DIRT && (tile == RUBBLE || (tile >= TINYEXP && tile <= LASTTINYEXP))) {
                    Map[mapY + dy][mapX + dx] = DIRT;
                }
            }
        }
        Spend(bulldozeCost);
    }

    /* Charge for the building */
    Spend(totalCost - bulldozeCost);

    /* Place the 4x4 building */
    index = 0;
    for (dy = -1; dy <= 2; dy++) {
        for (dx = -1; dx <= 2; dx++) {
            if (dx == 0 && dy == 0) {
                /* Center tile gets ZONEBIT */
                Map[mapY][mapX] = centerTile | ZONEBIT | BULLBIT;
            } else {
                /* Skip the center index (5) */
                if (index == 5) {
                    index++;
                }
                Map[mapY + dy][mapX + dx] = baseValue + index | BULLBIT;
            }
            index++;
        }
    }

    /* Fix the building edges to connect with neighbors */
    for (dy = -1; dy <= 2; dy++) {
        for (dx = -1; dx <= 2; dx++) {
            FixZone(mapX + dx, mapY + dy, &Map[mapY + dy][mapX + dx]);
        }
    }

    return TOOLRESULT_OK;
}

/* Apply the coal power plant tool */
int DoPowerPlant(int mapX, int mapY) {
    /* Use the building helper for power plants */
    return Place4x4Building(mapX, mapY, TILE_COALBASE, TILE_POWERPLANT, TOOL_POWERPLANT_COST);
}

/* Apply the nuclear power plant tool */
int DoNuclearPlant(int mapX, int mapY) {
    /* Use the building helper for nuclear plants */
    return Place4x4Building(mapX, mapY, TILE_NUCLEARBASE, TILE_NUCLEAR, TOOL_NUCLEAR_COST);
}

/* Apply the stadium tool */
int DoStadium(int mapX, int mapY) {
    /* Use the building helper for stadiums */
    return Place4x4Building(mapX, mapY, TILE_STADIUMBASE, TILE_STADIUM, TOOL_STADIUM_COST);
}

/* Apply the seaport tool */
int DoSeaport(int mapX, int mapY) {
    /* Use the building helper for seaports */
    return Place4x4Building(mapX, mapY, TILE_PORTBASE, TILE_PORT, TOOL_SEAPORT_COST);
}

/* Helper function for placing a 6x6 building like an airport */
int Place6x6Building(int mapX, int mapY, int baseValue, int centerTile, int totalCost) {
    int dx, dy;
    int index = 0;
    int bulldozeCost = 0;

    /* Check if we can build here */
    if (!Check6x6Area(mapX, mapY, &bulldozeCost)) {
        return TOOLRESULT_FAILED;
    }

    /* Total cost includes building cost plus any clearing costs */
    totalCost += bulldozeCost;

    /* Check if we have enough money */
    if (!CheckFunds(totalCost)) {
        return TOOLRESULT_NO_MONEY;
    }

    /* Clear area if needed and charge for it */
    if (bulldozeCost > 0) {
        for (dy = -2; dy <= 3; dy++) {
            for (dx = -2; dx <= 3; dx++) {
                short tile = Map[mapY + dy][mapX + dx] & LOMASK;
                if (tile != DIRT && (tile == RUBBLE || (tile >= TINYEXP && tile <= LASTTINYEXP))) {
                    Map[mapY + dy][mapX + dx] = DIRT;
                }
            }
        }
        Spend(bulldozeCost);
    }

    /* Charge for the building */
    Spend(totalCost - bulldozeCost);

    /* Place the 6x6 building */
    index = 0;
    for (dy = -2; dy <= 3; dy++) {
        for (dx = -2; dx <= 3; dx++) {
            if (dx == 0 && dy == 0) {
                /* Center tile gets ZONEBIT */
                Map[mapY][mapX] = centerTile | ZONEBIT | BULLBIT;
            } else {
                /* Skip the center index (14) */
                if (index == 14) {
                    index++;
                }
                Map[mapY + dy][mapX + dx] = baseValue + index | BULLBIT;
            }
            index++;
        }
    }

    /* Fix the building edges to connect with neighbors */
    for (dy = -2; dy <= 3; dy++) {
        for (dx = -2; dx <= 3; dx++) {
            FixZone(mapX + dx, mapY + dy, &Map[mapY + dy][mapX + dx]);
        }
    }

    return TOOLRESULT_OK;
}

/* Apply the airport tool */
int DoAirport(int mapX, int mapY) {
    /* Use the building helper for airports */
    return Place6x6Building(mapX, mapY, TILE_AIRPORTBASE, TILE_AIRPORT, TOOL_AIRPORT_COST);
}

/* Helper function to get zone name from tile */
const char *GetZoneName(short tile) {
    short baseTile = tile & LOMASK;

    if (baseTile >= RESBASE && baseTile <= LASTRES) {
        return "Residential Zone";
    } else if (baseTile >= COMBASE && baseTile <= LASTCOM) {
        return "Commercial Zone";
    } else if (baseTile >= INDBASE && baseTile <= LASTIND) {
        return "Industrial Zone";
    } else if (baseTile >= PORTBASE && baseTile <= LASTPORT) {
        return "Seaport";
    } else if (baseTile >= AIRPORTBASE && baseTile <= LASTAIRPORT) {
        return "Airport";
    } else if (baseTile >= COALBASE && baseTile <= LASTPOWERPLANT) {
        return "Coal Power Plant";
    } else if (baseTile >= NUCLEARBASE && baseTile <= LASTNUCLEAR) {
        return "Nuclear Power Plant";
    } else if (baseTile >= FIRESTBASE && baseTile <= LASTFIRESTATION) {
        return "Fire Station";
    } else if (baseTile >= POLICESTBASE && baseTile <= LASTPOLICESTATION) {
        return "Police Station";
    } else if (baseTile >= STADIUMBASE && baseTile <= LASTSTADIUM) {
        return "Stadium";
    } else if (baseTile >= ROADBASE && baseTile <= LASTROAD) {
        return "Road";
    } else if (baseTile >= RAILBASE && baseTile <= LASTRAIL) {
        return "Rail";
    } else if (baseTile >= POWERBASE && baseTile <= LASTPOWER) {
        return "Power Line";
    } else if (baseTile == RIVER || baseTile == REDGE || baseTile == CHANNEL) {
        return "Water";
    } else if (baseTile >= RUBBLE && baseTile <= LASTRUBBLE) {
        return "Rubble";
    } else if (baseTile >= TREEBASE && baseTile <= LASTTREE) {
        return "Trees";
    } else if (baseTile == RADTILE) {
        return "Radiation";
    } else if (baseTile >= FIREBASE && baseTile <= LASTFIRE) {
        return "Fire";
    } else if (baseTile >= FLOOD && baseTile <= LASTFLOOD) {
        return "Flood";
    }

    return "Clear Land";
}

/* Apply the query tool - shows information about the tile */
int DoQuery(int mapX, int mapY) {
    short tile;
    char message[256];
    const char *zoneName;

    /* Check bounds */
    if (mapX < 0 || mapX >= WORLD_X || mapY < 0 || mapY >= WORLD_Y) {
        return TOOLRESULT_FAILED;
    }

    tile = Map[mapY][mapX];

    /* Get zone name from tile */
    zoneName = GetZoneName(tile);

    /* Prepare message */
    wsprintf(message, "Location: %d, %d\nTile Type: %s\nHas Power: %s", mapX, mapY, zoneName,
             (tile & POWERBIT) ? "Yes" : "No");

    /* Display the message box with information */
    MessageBox(hwndMain, message, "Zone Info", MB_OK | MB_ICONINFORMATION);

    return TOOLRESULT_OK;
}

/* Apply the current tool at the given coordinates */
int ApplyTool(int mapX, int mapY) {
    int result = TOOLRESULT_FAILED;

    switch (currentTool) {
    case bulldozerState:
        result = DoBulldozer(mapX, mapY);
        break;

    case roadState:
        result = DoRoad(mapX, mapY);
        break;

    case railState:
        result = DoRail(mapX, mapY);
        break;

    case wireState:
        result = DoWire(mapX, mapY);
        break;

    case parkState:
        result = DoPark(mapX, mapY);
        break;

    case residentialState:
        result = DoResidential(mapX, mapY);
        break;

    case commercialState:
        result = DoCommercial(mapX, mapY);
        break;

    case industrialState:
        result = DoIndustrial(mapX, mapY);
        break;

    case fireState:
        result = DoFireStation(mapX, mapY);
        break;

    case policeState:
        result = DoPoliceStation(mapX, mapY);
        break;

    case stadiumState:
        result = DoStadium(mapX, mapY);
        break;

    case seaportState:
        result = DoSeaport(mapX, mapY);
        break;

    case powerState:
        result = DoPowerPlant(mapX, mapY);
        break;

    case nuclearState:
        result = DoNuclearPlant(mapX, mapY);
        break;

    case airportState:
        result = DoAirport(mapX, mapY);
        break;

    case queryState:
        result = DoQuery(mapX, mapY);
        break;

    default:
        result = TOOLRESULT_FAILED;
        break;
    }

    /* Store the result for later display */
    toolResult = result;

    /* Force redraw of map */
    InvalidateRect(hwndMain, NULL, FALSE);

    return result;
}

/* Get the current tool */
int GetCurrentTool(void) {
    return currentTool;
}

/* Function to update the toolbar display (including RCI bars) */
void UpdateToolbar(void) {
    if (hwndToolbar) {
        InvalidateRect(hwndToolbar, NULL, TRUE);
    }
}

/* Get the last tool result */
int GetToolResult(void) {
    return toolResult;
}

/* Get the current tool cost */
int GetToolCost(void) {
    return toolCost;
}

/* Get the size of a tool (1x1, 3x3, 4x4, or 6x6) */
int GetToolSize(int toolType) {
    switch (toolType) {
    case residentialState:
    case commercialState:
    case industrialState:
    case fireState:
    case policeState:
        return TOOL_SIZE_3X3; /* 3x3 zones */

    case stadiumState:
    case seaportState:
    case powerState:
    case nuclearState:
        return TOOL_SIZE_4X4; /* 4x4 buildings */

    case airportState:
        return TOOL_SIZE_6X6; /* 6x6 buildings */

    case roadState:
    case railState:
    case wireState:
    case parkState:
    case bulldozerState:
    case queryState:
    default:
        return TOOL_SIZE_1X1; /* Single tile tools */
    }
}

/* Draw the tool hover highlight at the given map position */
void DrawToolHover(HDC hdc, int mapX, int mapY, int toolType, int xOffset, int yOffset) {
    int size = GetToolSize(toolType);
    int screenX, screenY;
    int startX, startY;
    int width, height;
    HPEN hPen;
    HPEN hOldPen;
    HBRUSH hOldBrush;

    /* Calculate the starting position based on tool size */
    switch (size) {
    case TOOL_SIZE_3X3:
        /* Center 3x3 highlight at cursor */
        startX = mapX - 1;
        startY = mapY - 1;
        width = 3;
        height = 3;
        break;

    case TOOL_SIZE_4X4:
        /* Center 4x4 highlight at cursor */
        startX = mapX - 1;
        startY = mapY - 1;
        width = 4;
        height = 4;
        break;

    case TOOL_SIZE_6X6:
        /* Center 6x6 highlight at cursor */
        startX = mapX - 2;
        startY = mapY - 2;
        width = 6;
        height = 6;
        break;

    case TOOL_SIZE_1X1:
    default:
        /* Single tile highlight */
        startX = mapX;
        startY = mapY;
        width = 1;
        height = 1;
        break;
    }

    /* Convert map coordinates to screen coordinates */
    screenX = (startX * TILE_SIZE) - xOffset;
    screenY = (startY * TILE_SIZE) - yOffset;

    /* Create a white pen for the outline */
    hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));

    if (!hPen) {
        return;
    }

    /* Select pen and null brush (for hollow rectangle) */
    hOldPen = SelectObject(hdc, hPen);
    hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));

    /* Draw the rectangle */
    Rectangle(hdc, screenX, screenY, screenX + (width * TILE_SIZE), screenY + (height * TILE_SIZE));

    /* Clean up */
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);

    /* Save the last position for efficiency */
    lastMouseMapX = mapX;
    lastMouseMapY = mapY;
}

/* Convert screen coordinates to map coordinates */
void ScreenToMap(int screenX, int screenY, int *mapX, int *mapY, int xOffset, int yOffset) {
    /*
     * Convert mouse position to map coordinates.
     * Add xOffset to account for the map scrolling,
     * no need to adjust for toolbar as screenX is already relative to the left of client area
     */
    *mapX = (screenX + xOffset - toolbarWidth) / TILE_SIZE;
    *mapY = (screenY + yOffset) / TILE_SIZE;
}

/* Mouse handler for tools - converts mouse coordinates to map coordinates and applies the current
 * tool */
int HandleToolMouse(int mouseX, int mouseY, int xOffset, int yOffset) {
    int mapX, mapY;

    /* Convert screen coordinates to map coordinates */
    ScreenToMap(mouseX, mouseY, &mapX, &mapY, xOffset, yOffset);

    /* Apply the current tool */
    return ApplyTool(mapX, mapY);
}

#if(WINVER <= 0x0351)
void CheckMenuRadioItem(){}

// Function to load a bitmap from a file and handle different palette sizes
HBITMAP LoadImage(LPCSTR filePath) {
    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bmiHeader;
    HBITMAP hBitmap = NULL;
    RGBQUAD *colorTable = NULL;
    BYTE *bitmapBits = NULL;
    FILE *file;
	HDC hdc;

	hdc = GetDC(hwndMain); // Get a handle to the screen DC

    // Open the file in binary mode
    file = fopen(filePath, "rb");
    if (file == NULL) {
        MessageBox(NULL, "Failed to open the file!", "Error", MB_OK | MB_ICONERROR);
	addDebugLog("LoadBitmapFromFile failed to load %s\n",filePath);
        return NULL;
    }

    // Read the BITMAPFILEHEADER
    fread(&bmfHeader, sizeof(BITMAPFILEHEADER), 1, file);
    if (bmfHeader.bfType != 0x4D42) { // Check for 'BM' signature
        MessageBox(NULL, "Not a valid BMP file!", "Error", MB_OK | MB_ICONERROR);
        fclose(file);
        return NULL;
    }

    // Read the BITMAPINFOHEADER
    fread(&bmiHeader, sizeof(BITMAPINFOHEADER), 1, file);

    // Handle indexed color bitmaps (palettes)
    if (bmiHeader.biBitCount <= 8) {
        // Calculate the size of the palette
        size_t colorTableSize = (1 << bmiHeader.biBitCount) * sizeof(RGBQUAD);
        colorTable = (RGBQUAD *)malloc(colorTableSize);
        if (colorTable == NULL) {
            MessageBox(NULL, "Failed to allocate memory for color table!", "Error", MB_OK | MB_ICONERROR);
            fclose(file);
            return NULL;
        }
        fread(colorTable, colorTableSize, 1, file);
    }
addDebugLog("bmiHeader.biBitCount is %d\n",bmiHeader.biBitCount);

    // Allocate memory for the bitmap bits
    bitmapBits = (BYTE *)malloc(bmiHeader.biSizeImage);
    if (bitmapBits == NULL) {
        MessageBox(NULL, "Failed to allocate memory for bitmap bits!", "Error", MB_OK | MB_ICONERROR);
        free(colorTable);
        fclose(file);
        return NULL;
    }

    // Move file pointer to the start of bitmap data and read it
    fseek(file, bmfHeader.bfOffBits, SEEK_SET);
    fread(bitmapBits, 1, bmiHeader.biSizeImage, file);
    fclose(file);

    // Create the HBITMAP
    hBitmap = CreateDIBitmap(hdc, &bmiHeader, CBM_INIT, bitmapBits, 
                             (BITMAPINFO *)&bmiHeader, DIB_RGB_COLORS);

    // Clean up allocated memory
    free(colorTable);
    free(bitmapBits);

    if (hBitmap == NULL) {
        MessageBox(NULL, "Failed to create HBITMAP!", "Error", MB_OK | MB_ICONERROR);
    }
addDebugLog("LoadBitmapFromFile loaded %s\n",filePath);
    return hBitmap;
}

#endif
