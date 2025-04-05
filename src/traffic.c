/* traffic.c - Traffic simulation implementation for MicropolisNT
 * Based on original Micropolis code from MicropolisLegacy project
 */

#include "sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* Maximum distance for a trip */
#define MAXDIS 30

/* Traffic position stack for pathfinding */
static short PosStackN;
static short SMapXStack[MAXDIS + 1];
static short SMapYStack[MAXDIS + 1];

/* Traffic state variables */
static short LDir;     /* Last direction traveled */
static short Zsource;  /* Source zone type */
static short TrafMaxX; /* Traffic density peak X */
static short TrafMaxY; /* Traffic density peak Y */

/* Direction offsets for perimeter search */
static short PerimX[12] = {-1, 0, 1, 2, 2, 2, 1, 0, -1, -2, -2, -2};
static short PerimY[12] = {-2, -2, -2, -1, 0, 1, 2, 2, 2, 1, 0, -1};

/* Function prototypes */
static int FindPRoad(void);
static int TryDrive(void);
static int TryGo(int z);
static int DriveDone(void);
static int RoadTest(int x);
static int GetFromMap(int x);
static void PushPos(void);
static void PullPos(void);
static void SetTrafMem(void);

/* Road test - check if a tile is part of the road/rail network */
static int RoadTest(int x) {
    x = x & LOMASK;

    /* Not a road or rail if it's outside the valid ranges */
    if (x < ROADBASE || x > LASTRAIL || (x >= POWERBASE && x < RAILBASE)) {
        return 0;
    }

    return 1;
}

/* Get the type of a tile in a given direction */
static int GetFromMap(int x) {
    switch (x) {
    case 0: /* North */
        return (SMapY > 0) ? (Map[SMapY - 1][SMapX] & LOMASK) : 0;

    case 1: /* East */
        return (SMapX < (WORLD_X - 1)) ? (Map[SMapY][SMapX + 1] & LOMASK) : 0;

    case 2: /* South */
        return (SMapY < (WORLD_Y - 1)) ? (Map[SMapY + 1][SMapX] & LOMASK) : 0;

    case 3: /* West */
        return (SMapX > 0) ? (Map[SMapY][SMapX - 1] & LOMASK) : 0;

    default:
        return 0;
    }
}

/* Push current position onto stack */
static void PushPos(void) {
    PosStackN++;
    SMapXStack[PosStackN] = SMapX;
    SMapYStack[PosStackN] = SMapY;
}

/* Pull position from stack */
static void PullPos(void) {
    SMapX = SMapXStack[PosStackN];
    SMapY = SMapYStack[PosStackN];
    PosStackN--;
}

/* Look for a road on the perimeter of a zone */
static int FindPRoad(void) {
    int tx, ty, z;

    for (z = 0; z < 12; z++) {
        tx = SMapX + PerimX[z];
        ty = SMapY + PerimY[z];
        if (TestBounds(tx, ty)) {
            if (RoadTest(Map[ty][tx])) {
                SMapX = tx;
                SMapY = ty;
                return 1;
            }
        }
    }
    return 0;
}

/* Check if we've reached the destination */
static int DriveDone(void) {
    static short TARGL[3] = {COMBASE, INDBASE, RESBASE}; /* Destinations (low range) */
    static short TARGH[3] = {LASTCOM, LASTIND, LASTRES}; /* Destinations (high range) */
    int z, l, h;

    l = TARGL[Zsource];
    h = TARGH[Zsource];

    /* Check north */
    if (SMapY > 0) {
        z = Map[SMapY - 1][SMapX] & LOMASK;
        if ((z >= l) && (z <= h)) {
            return 1;
        }
    }

    /* Check east */
    if (SMapX < (WORLD_X - 1)) {
        z = Map[SMapY][SMapX + 1] & LOMASK;
        if ((z >= l) && (z <= h)) {
            return 1;
        }
    }

    /* Check south */
    if (SMapY < (WORLD_Y - 1)) {
        z = Map[SMapY + 1][SMapX] & LOMASK;
        if ((z >= l) && (z <= h)) {
            return 1;
        }
    }

    /* Check west */
    if (SMapX > 0) {
        z = Map[SMapY][SMapX - 1] & LOMASK;
        if ((z >= l) && (z <= h)) {
            return 1;
        }
    }

    return 0;
}

/* Try to make a move in the pathfinding */
static int TryGo(int z) {
    int x, rdir, realdir;

    /* Start in a random direction */
    rdir = SimRandom(4);

    /* Try all four directions, starting at random */
    for (x = 0; x < 4; x++) {
        realdir = (rdir + x) & 3;

        /* Skip last direction (don't go back) */
        if (realdir == LDir) {
            continue;
        }

        /* Check if we can move in this direction */
        if (!RoadTest(GetFromMap(realdir))) {
            continue;
        }

        /* Move in this direction */
        switch (realdir) {
        case 0:
            SMapY--;
            break;
        case 1:
            SMapX++;
            break;
        case 2:
            SMapY++;
            break;
        case 3:
            SMapX--;
            break;
        }

        /* Remember the direction we came from */
        LDir = (realdir + 2) & 3;

        /* Save position every other move */
        if (z & 1) {
            PushPos();
        }

        return 1;
    }

    return 0;
}

/* Try to drive to a destination */
static int TryDrive(void) {
    int z;

    /* Invalid direction to start with */
    LDir = 5;

    /* Try to drive for maximum distance */
    for (z = 0; z < MAXDIS; z++) {
        /* Try to make a move */
        if (TryGo(z)) {
            /* Check if we've reached the destination */
            if (DriveDone()) {
                return 1;
            }

            /* Continue to next step */
            continue;
        }

        /* Handle dead end */
        if (PosStackN) {
            /* Back up and skip a few steps */
            PosStackN--;
            z += 3;
            continue;
        }

        /* Give up at start if can't move */
        return 0;
    }

    /* Gone maximum distance without success */
    return 0;
}

/* Set traffic density along the path taken */
static void SetTrafMem(void) {
    int x, z;
    int tx, ty;

    for (x = PosStackN; x > 0; x--) {
        PullPos();
        if (TestBounds(SMapX, SMapY)) {
            z = Map[SMapY][SMapX] & LOMASK;
            if ((z >= ROADBASE) && (z < POWERBASE)) {
                /* Calculate traffic density map index - divide by 2 since density
                   map is half the size of the main map */
                tx = SMapX >> 1;
                ty = SMapY >> 1;

                /* Increase traffic density */
                z = TrfDensity[ty][tx];
                z += 50;

                /* Cap at maximum value */
                if (z > 240) {
                    z = 240;
                    TrafMaxX = SMapX;
                    TrafMaxY = SMapY;
                    /* TODO: Add police car sprite at congestion point */
                }

                TrfDensity[ty][tx] = (Byte)z;
            }
        }
    }
}

/* Make a trip from a specific zone type */
int MakeTraffic(int zoneType) {
    short xtem, ytem;

    /* Check for valid zone type (0=res, 1=com, 2=ind) */
    if (zoneType < 0 || zoneType > 2) {
        return -1;
    }

    /* Check coordinates for map bounds */
    if (!TestBounds(SMapX, SMapY)) {
        return -1;
    }

    /* Save original position */
    xtem = SMapX;
    ytem = SMapY;
    Zsource = zoneType;
    PosStackN = 0;

    /* Look for a road on the zone perimeter */
    if (FindPRoad()) {
        /* Attempt to drive somewhere */
        if (TryDrive()) {
            /* If successful, increase traffic density */
            SetTrafMem();
            SMapX = xtem;
            SMapY = ytem;
            return 1; /* Traffic passed */
        }
        SMapX = xtem;
        SMapY = ytem;
        return 0; /* Traffic failed */
    } else {
        return -1; /* No road found */
    }
}

/* Decrease traffic values over time */
void DecTrafficMap(void) {
    int x, y;
    int fullX, fullY; /* Full-sized map coordinates */
    int dx, dy;       /* Offsets within density cell */
    int mapX, mapY;   /* Actual map coordinates */
    int tile;         /* Tile value */

    for (y = 0; y < WORLD_Y / 2; y++) {
        for (x = 0; x < WORLD_X / 2; x++) {
            if (TrfDensity[y][x] > 0) {
                /* Gradually decrease traffic */
                TrfDensity[y][x] = (Byte)(TrfDensity[y][x] - (TrfDensity[y][x] / 8 + 1));

                /* If traffic decreases to zero, make sure to remove animation bits */
                if (TrfDensity[y][x] == 0) {
                    /* Clear animation bits in corresponding full-size map tiles */
                    fullX = x * 2;
                    fullY = y * 2;

                    for (dy = 0; dy < 2; dy++) {
                        for (dx = 0; dx < 2; dx++) {
                            mapX = fullX + dx;
                            mapY = fullY + dy;

                            if (mapX < WORLD_X && mapY < WORLD_Y) {
                                tile = Map[mapY][mapX] & LOMASK;

                                /* Only update road tiles */
                                if (tile >= ROADBASE && tile <= LASTROAD) {
                                    /* If this is a heavy traffic tile, convert back to normal road
                                     */
                                    if (tile >= HTRFBASE) {
                                        Map[mapY][mapX] = (tile - HTRFBASE + ROADBASE) & ~ANIMBIT;
                                    } else {
                                        /* Otherwise just clear animation bit */
                                        Map[mapY][mapX] &= ~ANIMBIT;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Calculate traffic density average */
void CalcTrafficAverage(void) {
    int x, y;
    long total = 0;
    int count = 0;
    int fullX, fullY; /* Full-sized map coordinates */
    int dx, dy;       /* Offsets within density cell */
    int mapX, mapY;   /* Actual map coordinates */
    int tile;         /* Tile value */

    for (y = 0; y < WORLD_Y / 2; y++) {
        for (x = 0; x < WORLD_X / 2; x++) {
            if (TrfDensity[y][x] > 0) {
                total += TrfDensity[y][x];
                count++;

                /* Apply animation bit to corresponding map tiles with traffic */
                fullX = x * 2;
                fullY = y * 2;

                /* Check all four tiles in the full-size map that this density tile covers */
                for (dy = 0; dy < 2; dy++) {
                    for (dx = 0; dx < 2; dx++) {
                        mapX = fullX + dx;
                        mapY = fullY + dy;

                        if (mapX < WORLD_X && mapY < WORLD_Y) {
                            tile = Map[mapY][mapX] & LOMASK;

                            /* Only set ANIMBIT on road tiles */
                            if (tile >= ROADBASE && tile <= LASTROAD) {
                                /* Heavy traffic */
                                if (TrfDensity[y][x] > 40) {
                                    /* Set animation bit and add HTRFBASE offset */
                                    Map[mapY][mapX] = (tile - ROADBASE + HTRFBASE) | ANIMBIT;
                                }
                                /* Light traffic - randomly animate some tiles */
                                else if (TrfDensity[y][x] > 10 && ((Fcycle & 3) == 0)) {
                                    /* Set animation bit but keep at ROADBASE */
                                    Map[mapY][mapX] |= ANIMBIT;
                                }
                                /* No traffic or very light - clear animation */
                                else {
                                    /* Clear animation bit */
                                    Map[mapY][mapX] &= ~ANIMBIT;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (count > 0) {
        TrafficAverage = (int)(total / count);
    } else {
        TrafficAverage = 0;
    }
}
