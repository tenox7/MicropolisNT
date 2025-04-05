/* scanner.c - Map scanning and effects spreading for MicropolisNT
 * Based on original Micropolis code from MicropolisLegacy project
 */

#include "sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* Internal state variables */
static short CCx, CCy;             /* City center X and Y coordinates */
static short CCx2, CCy2;           /* City center coordinates, divided by 2 */
static short PolMaxX, PolMaxY;     /* Coordinates of highest pollution */
static short CrimeMaxX, CrimeMaxY; /* Coordinates of highest crime */

/* Temporary arrays for smoothing operations */
static Byte tem[WORLD_X / 2][WORLD_Y / 2];  /* Temp array 1 for smoothing */
static Byte tem2[WORLD_X / 2][WORLD_Y / 2]; /* Temp array 2 for smoothing */
static Byte STem[WORLD_X / 4][WORLD_Y / 4]; /* Small temp array for fire/police map */
static Byte Qtem[WORLD_X / 4][WORLD_Y / 4]; /* Quarter-size temp array */

/* Function prototypes */
static void ClrTemArray(void);
static void DoSmooth(void);
static void DoSmooth2(void);
static void SmoothPSMap(void);
static void SmoothFSMap(void);
static void SmoothTerrain(void);
static int GetDisCC(int x, int y);
static int GetPValueLocal(int loc);
static int GetPDen(int zone);
static void DistIntMarket(void);

/* Clear temporary array (tem) */
static void ClrTemArray(void) {
    int x, y;

    for (x = 0; x < WORLD_X / 2; x++) {
        for (y = 0; y < WORLD_Y / 2; y++) {
            tem[x][y] = 0;
        }
    }
}

/* Smoothing algorithm - smooths tem into tem2 */
static void DoSmooth(void) {
    int x, y, z;

    for (x = 0; x < WORLD_X / 2; x++) {
        for (y = 0; y < WORLD_Y / 2; y++) {
            z = 0;

            /* Get average of nearby cells */
            if (x > 0) {
                z += tem[x - 1][y];
            }
            if (x < (WORLD_X / 2 - 1)) {
                z += tem[x + 1][y];
            }
            if (y > 0) {
                z += tem[x][y - 1];
            }
            if (y < (WORLD_Y / 2 - 1)) {
                z += tem[x][y + 1];
            }

            /* Average with central cell */
            z = (z + tem[x][y]) >> 2;
            if (z > 255) {
                z = 255;
            }
            tem2[x][y] = (Byte)z;
        }
    }
}

/* Second smoothing algorithm - smooths tem2 into tem */
static void DoSmooth2(void) {
    int x, y, z;

    for (x = 0; x < WORLD_X / 2; x++) {
        for (y = 0; y < WORLD_Y / 2; y++) {
            z = 0;

            /* Get average of nearby cells */
            if (x > 0) {
                z += tem2[x - 1][y];
            }
            if (x < (WORLD_X / 2 - 1)) {
                z += tem2[x + 1][y];
            }
            if (y > 0) {
                z += tem2[x][y - 1];
            }
            if (y < (WORLD_Y / 2 - 1)) {
                z += tem2[x][y + 1];
            }

            /* Average with central cell */
            z = (z + tem2[x][y]) >> 2;
            if (z > 255) {
                z = 255;
            }
            tem[x][y] = (Byte)z;
        }
    }
}

/* Smooth the Police Station effect map */
static void SmoothPSMap(void) {
    int x, y, edge;

    /* Quarter size map */
    for (x = 0; x < WORLD_X / 4; x++) {
        for (y = 0; y < WORLD_Y / 4; y++) {
            edge = 0;

            /* Add up surrounding cells */
            if (x > 0) {
                edge += PoliceMap[x - 1][y];
            }
            if (x < (WORLD_X / 4 - 1)) {
                edge += PoliceMap[x + 1][y];
            }
            if (y > 0) {
                edge += PoliceMap[x][y - 1];
            }
            if (y < (WORLD_Y / 4 - 1)) {
                edge += PoliceMap[x][y + 1];
            }

            /* Weighted average */
            edge = (edge >> 2) + PoliceMap[x][y];
            STem[x][y] = (Byte)(edge >> 1);
        }
    }

    /* Copy back to original map */
    for (x = 0; x < WORLD_X / 4; x++) {
        for (y = 0; y < WORLD_Y / 4; y++) {
            PoliceMap[x][y] = STem[x][y];
        }
    }
}

/* Smooth the Fire Station effect map */
static void SmoothFSMap(void) {
    int x, y, edge;

    /* Quarter size map */
    for (x = 0; x < WORLD_X / 4; x++) {
        for (y = 0; y < WORLD_Y / 4; y++) {
            edge = 0;

            /* Add up surrounding cells */
            if (x > 0) {
                edge += FireStMap[x - 1][y];
            }
            if (x < (WORLD_X / 4 - 1)) {
                edge += FireStMap[x + 1][y];
            }
            if (y > 0) {
                edge += FireStMap[x][y - 1];
            }
            if (y < (WORLD_Y / 4 - 1)) {
                edge += FireStMap[x][y + 1];
            }

            /* Weighted average */
            edge = (edge >> 2) + FireStMap[x][y];
            STem[x][y] = (Byte)(edge >> 1);
        }
    }

    /* Copy back to original map */
    for (x = 0; x < WORLD_X / 4; x++) {
        for (y = 0; y < WORLD_Y / 4; y++) {
            FireStMap[x][y] = STem[x][y];
        }
    }
}

/* Smooth terrain map */
static void SmoothTerrain(void) {
    int x, y, z;

    for (x = 0; x < WORLD_X / 4; x++) {
        for (y = 0; y < WORLD_Y / 4; y++) {
            z = 0;

            /* Get average of surrounding cells */
            if (x > 0) {
                z += Qtem[x - 1][y];
            }
            if (x < (WORLD_X / 4 - 1)) {
                z += Qtem[x + 1][y];
            }
            if (y > 0) {
                z += Qtem[x][y - 1];
            }
            if (y < (WORLD_Y / 4 - 1)) {
                z += Qtem[x][y + 1];
            }

            /* Average with central value */
            TerrainMem[x][y] = (Byte)(((z >> 2) + Qtem[x][y]) >> 1);
        }
    }
}

/* Calculate distance to city center */
static int GetDisCC(int x, int y) {
    int xdis, ydis, z;

    /* Calculate Manhattan distance to city center */
    xdis = (x > CCx2) ? (x - CCx2) : (CCx2 - x);
    ydis = (y > CCy2) ? (y - CCy2) : (CCy2 - y);

    z = xdis + ydis;

    /* Cap at 32 */
    return (z > 32) ? 32 : z;
}

/* Get pollution value for a tile type */
static int GetPValueLocal(int loc) {
    /* Road/rail traffic pollution */
    if (loc < POWERBASE) {
        /* Heavy traffic */
        if (loc >= ROADBASE + 16) {
            return 75;
        }

        /* Light traffic */
        if (loc >= ROADBASE) {
            return 50;
        }

        /* Specials under ROADBASE */
        if (loc < ROADBASE) {
            /* Fire */
            if (loc > FIREBASE) {
                return 90;
            }

            /* Radioactive */
            if (loc >= RADTILE) {
                return 255;
            }
        }
        return 0;
    }

    /* Industrial pollution */
    if (loc <= LASTIND) {
        return 0;
    }

    /* Industry */
    if (loc < PORTBASE) {
        return 50;
    }

    /* Port, airport, power plant */
    if (loc <= POWERPLANT + 10) {
        return 100;
    }

    return 0;
}

/* Get population density for a zone type */
static int GetPDen(int zone) {
    /* Residential population */
    if (zone < COMBASE) {
        return calcResPop(zone);
    }

    /* Commercial population (higher weight) */
    if (zone < INDBASE) {
        return calcComPop(zone) << 3;
    }

    /* Industrial population (higher weight) */
    if (zone < PORTBASE) {
        return calcIndPop(zone) << 3;
    }

    return 0;
}

/* Calculate commercial rate based on distance to center */
static void DistIntMarket(void) {
    int x, y, z;

    for (x = 0; x < WORLD_X / 4; x++) {
        for (y = 0; y < WORLD_Y / 4; y++) {
            /* Get Manhattan distance to city center */
            z = GetDisCC(x << 2, y << 2);

            /* Convert to commercial rate - closer to center is better */
            z = z << 2;
            z = 64 - z;

            /* Set commercial rate */
            ComRate[x][y] = z;
        }
    }
}

/* Fire effect analysis - spread fire station coverage */
void FireAnalysis(void) {
    int x, y;

    /* Smooth the fire station map three times to spread coverage */
    SmoothFSMap();
    SmoothFSMap();
    SmoothFSMap();

    /* Copy to fire rate map */
    for (x = 0; x < WORLD_X / 4; x++) {
        for (y = 0; y < WORLD_Y / 4; y++) {
            FireRate[x][y] = FireStMap[x][y];
        }
    }
}

/* Do population density scan */
void PopDenScan(void) {
    QUAD Xtot, Ytot, Ztot;
    int x, y, z;

    ClrTemArray();
    Xtot = 0;
    Ytot = 0;
    Ztot = 0;

    /* Scan the map for populated zones */
    for (x = 0; x < WORLD_X; x++) {
        for (y = 0; y < WORLD_Y; y++) {
            z = Map[y][x];
            if (z & ZONEBIT) {
                z = z & LOMASK;
                SMapX = x;
                SMapY = y;
                z = GetPDen(z) << 3;
                if (z > 254) {
                    z = 254;
                }

                /* Add to temporary density map */
                tem[x >> 1][y >> 1] = (Byte)z;

                /* Track population center of mass */
                Xtot += x;
                Ytot += y;
                Ztot++;
            }
        }
    }

    /* Triple-smooth the population density */
    DoSmooth();  /* tem -> tem2 */
    DoSmooth2(); /* tem2 -> tem */
    DoSmooth();  /* tem -> tem2 */

    /* Copy to population density map */
    for (x = 0; x < WORLD_X / 2; x++) {
        for (y = 0; y < WORLD_Y / 2; y++) {
            PopDensity[y][x] = (Byte)(tem2[x][y] << 1);
        }
    }

    /* Set commercial rate based on center of city */
    DistIntMarket();

    /* Calculate center of mass of the city */
    if (Ztot) {
        CCx = (short)(Xtot / Ztot);
        CCy = (short)(Ytot / Ztot);
    } else {
        /* If population is zero, center of map is center */
        CCx = WORLD_X / 2;
        CCy = WORLD_Y / 2;
    }

    /* Store center divided by 2 for calculations */
    CCx2 = CCx >> 1;
    CCy2 = CCy >> 1;
}

/* Calculate and scan pollution, terrain, and land value */
void PTLScan(void) {
    QUAD ptot, LVtot;
    int x, y, z, dis;
    int Plevel, LVflag, LVnum, pnum, pmax;
    int zx, zy, Mx, My;

    /* Initialize terrain map */
    for (x = 0; x < WORLD_X / 4; x++) {
        for (y = 0; y < WORLD_Y / 4; y++) {
            Qtem[x][y] = 0;
        }
    }

    /* Initialize land value counters */
    LVtot = 0;
    LVnum = 0;

    /* Scan the map for pollution and land value */
    for (x = 0; x < WORLD_X / 2; x++) {
        for (y = 0; y < WORLD_Y / 2; y++) {
            Plevel = 0;
            LVflag = 0;

            /* Each half-cell checks four full cells */
            zx = x << 1;
            zy = y << 1;

            for (Mx = zx; Mx <= zx + 1; Mx++) {
                for (My = zy; My <= zy + 1; My++) {
                    if (Mx < WORLD_X && My < WORLD_Y) {
                        int loc = Map[My][Mx] & LOMASK;

                        if (loc) {
                            if (loc < RUBBLE) {
                                /* Terrain (trees, water) increases terrain value */
                                Qtem[x >> 1][y >> 1] += 15;
                                continue;
                            }

                            /* Get pollution value for this tile */
                            Plevel += GetPValueLocal(loc);

                            /* If there's development, track it for land value */
                            if (loc >= ROADBASE) {
                                LVflag++;
                            }
                        }
                    }
                }
            }

            /* Cap pollution level at max */
            if (Plevel > 255) {
                Plevel = 255;
            }

            /* Store in temporary array */
            tem[x][y] = (Byte)Plevel;

            /* Calculate land value if there are developed tiles */
            if (LVflag) {
                /* Land value equation */
                dis = 34 - GetDisCC(x, y);
                dis = dis << 2;
                dis += (TerrainMem[x >> 1][y >> 1]);
                dis -= (PollutionMem[y][x]);

                /* Crime reduces land value */
                if (CrimeMem[y][x] > 190) {
                    dis -= 20;
                }

                /* Cap land value and ensure minimum */
                if (dis > 250) {
                    dis = 250;
                }
                if (dis < 1) {
                    dis = 1;
                }

                /* Store land value */
                LandValueMem[y][x] = (Byte)dis;

                /* Track for average */
                LVtot += dis;
                LVnum++;
            } else {
                LandValueMem[y][x] = 0;
            }
        }
    }

    /* Calculate land value average */
    if (LVnum) {
        LVAverage = (int)(LVtot / LVnum);
    } else {
        LVAverage = 0;
    }

    /* Smooth the pollution */
    DoSmooth();
    DoSmooth2();

    /* Find maximum pollution and calculate average */
    pmax = 0;
    pnum = 0;
    ptot = 0;

    for (x = 0; x < WORLD_X / 2; x++) {
        for (y = 0; y < WORLD_Y / 2; y++) {
            z = tem[x][y];
            PollutionMem[y][x] = (Byte)z;

            if (z) {
                /* Add to average pollution */
                pnum++;
                ptot += z;

                /* Find location of maximum pollution (for monster) */
                if ((z > pmax) || ((z == pmax) && (SimRandom(4) == 0))) {
                    pmax = z;
                    PolMaxX = x << 1;
                    PolMaxY = y << 1;
                }
            }
        }
    }

    /* Calculate pollution average */
    if (pnum) {
        PollutionAverage = (int)(ptot / pnum);
    } else {
        PollutionAverage = 0;
    }

    /* Smooth terrain */
    SmoothTerrain();
}

/* Scan crime map */
void CrimeScan(void) {
    int numz, cmax;
    QUAD totz;
    int x, y, z;

    /* Smooth police station effect map three times */
    SmoothPSMap();
    SmoothPSMap();
    SmoothPSMap();

    totz = 0;
    numz = 0;
    cmax = 0;

    for (x = 0; x < WORLD_X / 2; x++) {
        for (y = 0; y < WORLD_Y / 2; y++) {
            /* Only consider areas with land value */
            if (z = LandValueMem[y][x]) {
                /* Count tiles */
                ++numz;

                /* Crime equation */
                z = 128 - z;
                z += PopDensity[y][x];

                /* Cap crime before police effect */
                if (z > 300) {
                    z = 300;
                }

                /* Police stations reduce crime */
                z -= PoliceMap[x >> 2][y >> 2];

                /* Ensure crime values are in range 0-250 */
                if (z > 250) {
                    z = 250;
                }
                if (z < 0) {
                    z = 0;
                }

                /* Store crime value */
                CrimeMem[y][x] = (Byte)z;

                /* Track total for average */
                totz += z;

                /* Find maximum crime location */
                if ((z > cmax) || ((z == cmax) && (SimRandom(4) == 0))) {
                    cmax = z;
                    CrimeMaxX = x << 1;
                    CrimeMaxY = y << 1;
                }
            } else {
                /* No land value = no crime */
                CrimeMem[y][x] = 0;
            }
        }
    }

    /* Calculate crime average */
    if (numz) {
        CrimeAverage = (int)(totz / numz);
    } else {
        CrimeAverage = 0;
    }

    /* Copy police map to effect map */
    for (x = 0; x < WORLD_X / 4; x++) {
        for (y = 0; y < WORLD_Y / 4; y++) {
            PoliceMapEffect[x][y] = PoliceMap[x][y];
        }
    }
}
