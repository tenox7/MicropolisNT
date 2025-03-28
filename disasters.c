/* disasters.c - Disaster implementation for MicropolisNT
 * Based on original Micropolis code from MicropolisLegacy project
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "simulation.h"

/* External functions */
extern int SimRandom(int range);

/* External variables */
extern HWND hwndMain;
extern short Map[WORLD_Y][WORLD_X];

/* Common movement direction arrays */
static const short xDelta[4] = {0, 1, 0, -1};
static const short yDelta[4] = {-1, 0, 1, 0};

/* These constants are from simulation.h, defining them here to avoid redeclaration issues */
#define FIRE        56      /* Fire tile base - from simulation.h */
#define ANIMBIT     0x0800  /* Animation bit - from simulation.h */
#define RUBBLE      44      /* Rubble tile - from simulation.h */
#define BULLBIT     0x1000  /* Bulldozable bit - from simulation.h */
#define BURNBIT     0x2000  /* Burnable bit - from simulation.h */
#define FLOOD       48      /* Flood tile - from simulation.h */
#define RADTILE     52      /* Radiation tile - from simulation.h */
#define DIRT        0       /* Dirt tile - from simulation.h */
#define ZONEBIT     0x0400  /* Zone center bit - from simulation.h */

/* Zone ranges */
#define RESBASE     240     /* Residential zone base */
#define LASTRES     420     /* Last residential zone */
#define COMBASE     423     /* Commercial zone base */
#define LASTCOM     611     /* Last commercial zone */
#define INDBASE     612     /* Industrial zone base */
#define LASTIND     692     /* Last industrial zone */
#define PORTBASE    693     /* Seaport base */
#define LASTPORT    708     /* Last seaport */
#define AIRPORTBASE 709     /* Airport base */
#define LASTAIRPORT 744     /* Last airport */
#define NUCLEAR     816     /* Nuclear power plant */
#define LASTZONE    LASTAIRPORT /* Last zone tile for disaster purposes */

/* Trigger an earthquake disaster */
void doEarthquake(void)
{
    int x, y, z;
    int time;
    short tile, tileValue;
    int epicenterX, epicenterY;
    char buf[256];
    
    /* Set epicenter to center of the map */
    epicenterX = WORLD_X / 2;
    epicenterY = WORLD_Y / 2;
    
    /* Notify user */
    wsprintf(buf, "Earthquake reported at %d,%d!", epicenterX, epicenterY);
    MessageBox(hwndMain, buf, "Disaster", MB_ICONEXCLAMATION | MB_OK);
    
    /* Random earthquake damage - with reasonable limits */
    time = SimRandom(700) + 300;
    if (time > 1000) time = 1000;  /* Cap to prevent excessive processing */
    
    for (z = 0; z < time; z++) {
        /* Get random coordinates but ensure they are within bounds */
        x = SimRandom(WORLD_X);
        y = SimRandom(WORLD_Y);
        
        if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y)
            continue;
            
        /* Check if tile is vulnerable */
        tile = Map[y][x];
        tileValue = tile & LOMASK;
        
        if ((tileValue >= RESBASE) && (tileValue <= LASTZONE) && !(tile & ZONEBIT)) {
            if (z & 0x3) {
                /* Create rubble (every 4th iteration) */
                Map[y][x] = (RUBBLE + BULLBIT) + (SimRandom(4));
            } else {
                /* Create fire */
                Map[y][x] = (FIRE + ANIMBIT) + (SimRandom(8));
            }
        }
    }
    
    /* Force redraw */
    InvalidateRect(hwndMain, NULL, FALSE);
}

/* Create an explosion */
void makeExplosion(int x, int y)
{
    int dir, tx, ty;
    char buf[256];
    
    /* Validate coordinates first */
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y)
        return;
    
    /* Create fire at explosion center */
    Map[y][x] = (FIRE + ANIMBIT) + (SimRandom(8));
    
    /* Create fire in surrounding tiles (N, E, S, W) */
    for (dir = 0; dir < 4; dir++) {
        tx = x + xDelta[dir];
        ty = y + yDelta[dir];
        
        /* Check bounds for each surrounding tile */
        if (tx >= 0 && tx < WORLD_X && ty >= 0 && ty < WORLD_Y) {
            /* Only set fire if not a zone center */
            if (!(Map[ty][tx] & ZONEBIT)) {
                Map[ty][tx] = (FIRE + ANIMBIT) + (SimRandom(8));
            }
        }
    }
    
    /* Notify user */
    wsprintf(buf, "Explosion reported at %d,%d!", x, y);
    MessageBox(hwndMain, buf, "Disaster", MB_ICONEXCLAMATION | MB_OK);
    
    /* Force redraw */
    InvalidateRect(hwndMain, NULL, FALSE);
}

/* Start a fire at the given location */
void makeFire(int x, int y)
{
    char buf[256];
    
    /* Validate coordinates first */
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y)
        return;
    
    /* Create fire tile with animation and random frame */
    Map[y][x] = (FIRE + ANIMBIT) + (SimRandom(8));
    
    /* Notify user */
    wsprintf(buf, "Fire reported at %d,%d!", x, y);
    MessageBox(hwndMain, buf, "Disaster", MB_ICONEXCLAMATION | MB_OK);
    
    /* Force redraw */
    InvalidateRect(hwndMain, NULL, FALSE);
}

/* Create a monster (Godzilla-like) disaster */
void makeMonster(void)
{
    int x, y;
    int found = 0;
    int attempts = 0;
    int i, tx, ty, dir;
    short tile;
    
    /* Try to find a valid starting position for the monster */
    while (!found && attempts < 100) {
        /* Generate random position within world bounds */
        x = SimRandom(WORLD_X);
        y = SimRandom(WORLD_Y);
        
        /* Make sure position is valid */
        if (x >= 0 && x < WORLD_X && y >= 0 && y < WORLD_Y) {
            tile = Map[y][x] & LOMASK;
            
            /* Only place monster on non-dirt tiles */
            if (tile != 0) {
                /* Create fire at monster's starting position */
                Map[y][x] = (FIRE + ANIMBIT) + (SimRandom(8));
                found = 1;
                
                /* Monster moves randomly destroying things */
                for (i = 0; i < 100; i++) {
                    dir = SimRandom(4);
                    
                    /* Move in a random direction */
                    switch (dir) {
                        case 0: tx = x; ty = y - 1; break; /* North */
                        case 1: tx = x + 1; ty = y; break; /* East */
                        case 2: tx = x; ty = y + 1; break; /* South */
                        case 3: tx = x - 1; ty = y; break; /* West */
                        default: tx = x; ty = y; break;    /* Error case - stay put */
                    }
                    
                    /* Check bounds before setting new position */
                    if (tx >= 0 && tx < WORLD_X && ty >= 0 && ty < WORLD_Y) {
                        x = tx;
                        y = ty;
                        Map[y][x] = (FIRE + ANIMBIT) + (SimRandom(8));
                    }
                }
            }
        }
        attempts++;
    }
    
    /* Notify user */
    {
        char buf[256];
        wsprintf(buf, "Monster attack reported in the city!");
        MessageBox(hwndMain, buf, "Disaster", MB_ICONEXCLAMATION | MB_OK);
    }
    
    /* Force redraw */
    InvalidateRect(hwndMain, NULL, FALSE);
}

/* Create a flood disaster */
void makeFlood(void)
{
    int x, y, xx, yy, tx, ty;
    int waterFound = 0;
    int attempts = 0;
    int t, i, j;
    char buf[256];
    short tileValue;
    
    /* Try to find water edge to start flood, with a reasonable attempt limit */
    while (!waterFound && attempts < 300) {
        /* Generate random coordinates within world bounds */
        x = SimRandom(WORLD_X);
        y = SimRandom(WORLD_Y);
        
        /* Validate coordinates */
        if (x >= 0 && x < WORLD_X && y >= 0 && y < WORLD_Y) {
            tileValue = Map[y][x] & LOMASK;
            
            /* Look for river/water tiles */
            if (tileValue > 4 && tileValue < 21) {
                /* Check all four adjacent tiles */
                for (t = 0; t < 4; t++) {
                    xx = x + xDelta[t];
                    yy = y + yDelta[t];
                    
                    /* Check if adjacent tile is in bounds and floodable */
                    if (xx >= 0 && xx < WORLD_X && yy >= 0 && yy < WORLD_Y) {
                        if (Map[yy][xx] == DIRT || 
                            ((Map[yy][xx] & BULLBIT) && (Map[yy][xx] & BURNBIT))) {
                            
                            /* Create initial flood tile */
                            Map[yy][xx] = FLOOD;
                            waterFound = 1;
                            
                            /* Notify user */
                            wsprintf(buf, "Flooding reported at %d,%d!", xx, yy);
                            MessageBox(hwndMain, buf, "Disaster", MB_ICONEXCLAMATION | MB_OK);
                            
                            /* Start spreading the flood - limit to 100 iterations */
                            for (i = 0; i < 100; i++) {
                                /* Check adjacent tiles */
                                for (j = 0; j < 4; j++) {
                                    tx = xx + xDelta[j];
                                    ty = yy + yDelta[j];
                                    
                                    /* Only flood tiles that are in bounds and floodable */
                                    if (tx >= 0 && tx < WORLD_X && ty >= 0 && ty < WORLD_Y) {
                                        if (Map[ty][tx] == DIRT || 
                                            ((Map[ty][tx] & BULLBIT) && (Map[ty][tx] & BURNBIT))) {
                                            Map[ty][tx] = FLOOD;
                                        }
                                    }
                                }
                                
                                /* Also try a random position near the original water source */
                                tx = x + SimRandom(10) - 5;
                                ty = y + SimRandom(10) - 5;
                                
                                if (tx >= 0 && tx < WORLD_X && ty >= 0 && ty < WORLD_Y) {
                                    if (Map[ty][tx] == DIRT || 
                                        ((Map[ty][tx] & BULLBIT) && (Map[ty][tx] & BURNBIT))) {
                                        Map[ty][tx] = FLOOD;
                                    }
                                }
                            }
                            
                            break;
                        }
                    }
                }
            }
        }
        attempts++;
    }
    
    /* Force redraw */
    InvalidateRect(hwndMain, NULL, FALSE);
}

/* Create nuclear meltdown disaster */
void makeMeltdown(void)
{
    int x, y, tx, ty, i;
    int found = 0;
    char buf[256];
    
    /* Find nuclear power plant */
    for (x = 0; x < WORLD_X; x++) {
        for (y = 0; y < WORLD_Y; y++) {
            if ((Map[y][x] & LOMASK) == NUCLEAR) {
                /* Found nuclear plant - trigger meltdown */
                
                /* Notify user */
                wsprintf(buf, "Nuclear meltdown reported at %d,%d!", x, y);
                MessageBox(hwndMain, buf, "Disaster", MB_ICONEXCLAMATION | MB_OK);
                
                /* Create radiation in a 20x20 area around the plant */
                for (i = 0; i < 40; i++) {
                    /* Get random position within 10 tiles of plant */
                    tx = x + SimRandom(20) - 10;
                    ty = y + SimRandom(20) - 10;
                    
                    /* Ensure positions are within bounds */
                    if (tx >= 0 && tx < WORLD_X && ty >= 0 && ty < WORLD_Y) {
                        /* Add radiation tiles */
                        Map[ty][tx] = RADTILE;
                    }
                }
                
                /* Create fire at power plant location */
                if (x >= 0 && x < WORLD_X && y >= 0 && y < WORLD_Y) {
                    Map[y][x] = (FIRE + ANIMBIT) + (SimRandom(8));
                }
                
                found = 1;
                break;
            }
        }
        if (found) break;
    }
    
    /* Force redraw */
    InvalidateRect(hwndMain, NULL, FALSE);
}