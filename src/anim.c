/* animation.c - Tile animation implementation for MicropolisNT
 * Based on original Micropolis code from MicropolisLegacy project
 */

#include "animtab.h"
#include "sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* Animation state flags */
static int AnimationEnabled = 1; /* Animation enabled by default */

/* Forward declarations */
static void DoCoalSmoke(int x, int y);
static void DoIndustrialSmoke(int x, int y);
static void DoStadiumAnimation(int x, int y);

/* Process animations for the entire map */
void AnimateTiles(void) {
    unsigned short tilevalue, tileflags;
    int x, y;
    static int debugCount = 0;

    /* Skip animation if disabled */
    if (!AnimationEnabled) {
        return;
    }

    /* Scan the entire map for tiles with the ANIMBIT set */
    for (y = 0; y < WORLD_Y; y++) {
        for (x = 0; x < WORLD_X; x++) {
            tilevalue = Map[y][x];

            /* Only process tiles with the animation bit set */
            if (tilevalue & ANIMBIT) {
                tileflags = tilevalue & MASKBITS; /* Save the flags */
                tilevalue &= LOMASK;              /* Extract base tile value */

                /* Debug animation (once every 100 frames) */
                if (debugCount == 0) {
                    /* Check for known animation types to debug them */
                    if (tilevalue >= TELEBASE && tilevalue <= TELELAST) {
                        char debugMsg[256];
                        wsprintf(debugMsg, "ANIMATION: Industrial smoke at (%d,%d) frame %d\n", 
                                 x, y, tilevalue);
                        OutputDebugString(debugMsg);
                    } else if (tilevalue == NUCLEAR_SWIRL) {
                        char debugMsg[256];
                        wsprintf(debugMsg, "ANIMATION: Nuclear reactor at (%d,%d)\n", x, y);
                        OutputDebugString(debugMsg);
                    } else if (tilevalue >= RADAR0 && tilevalue <= RADAR7) {
                        char debugMsg[256];
                        wsprintf(debugMsg, "ANIMATION: Airport radar animation at (%d,%d)\n", x, y);
                        OutputDebugString(debugMsg);
                    } else if (tilevalue == FOOTBALLGAME1 || tilevalue == FOOTBALLGAME2) {
                        char debugMsg[256];
                        wsprintf(debugMsg, "ANIMATION: Stadium game at (%d,%d)\n", x, y);
                        OutputDebugString(debugMsg);
                    }
                }

                /* Look up the next animation frame */
                tilevalue = aniTile[tilevalue];

                /* Reapply the flags */
                tilevalue |= tileflags;

                /* Update the map with the new tile */
                Map[y][x] = tilevalue;
            }
        }
    }
    
    /* Increment debug counter and wrap around */
    debugCount = (debugCount + 1) % 100;
}

/* Enable or disable animations */
void SetAnimationEnabled(int enabled) {
    AnimationEnabled = enabled;
}

/* Get animation enabled status */
int GetAnimationEnabled(void) {
    return AnimationEnabled;
}

/* Add smoke animation to coal power plants */
void SetSmoke(int x, int y) {
    int i;
    int xx, yy;

    /* Skip if the position is invalid */
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
        return;
    }

    /* Check if this is a power plant */
    if ((Map[y][x] & LOMASK) != POWERPLANT) {
        return;
    }

    /* Set smoke on the appropriate tiles */
    for (i = 0; i < 4; i++) {
        xx = x + smokeOffsetX[i];
        yy = y + smokeOffsetY[i];

        /* Make sure the smoke position is valid */
        if (xx >= 0 && xx < WORLD_X && yy >= 0 && yy < WORLD_Y) {
            /* Only set the tile if it doesn't already have the animation bit set
               or if it's not already a smoke tile. This avoids resetting the
               animation sequence and makes it flow better. */
            if (!(Map[yy][xx] & ANIMBIT) || (Map[yy][xx] & LOMASK) < COALSMOKE1 ||
                (Map[yy][xx] & LOMASK) > COALSMOKE4 + 4) {

                /* Set the appropriate smoke tile with animation */
                switch (i) {
                case 0:
                    Map[yy][xx] = COALSMOKE1 | ANIMBIT | CONDBIT | POWERBIT | BURNBIT;
                    break;
                case 1:
                    Map[yy][xx] = COALSMOKE2 | ANIMBIT | CONDBIT | POWERBIT | BURNBIT;
                    break;
                case 2:
                    Map[yy][xx] = COALSMOKE3 | ANIMBIT | CONDBIT | POWERBIT | BURNBIT;
                    break;
                case 3:
                    Map[yy][xx] = COALSMOKE4 | ANIMBIT | CONDBIT | POWERBIT | BURNBIT;
                    break;
                }
            }
        }
    }
}

/* Add smoke animation to industrial buildings */
static void DoIndustrialSmoke(int x, int y) {
    int i;
    int xx, yy;
    short baseTile;
    short smokeTile;

    /* Skip if the position is invalid */
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
        return;
    }

    /* Check if this industrial building has power */
    if (!(Map[y][x] & POWERBIT)) {
        return;
    }

    /* Get the base tile value */
    baseTile = Map[y][x] & LOMASK;

    /* Look for industrial buildings that need smoke */
    for (i = 0; i < 8; i++) {
        /* Skip if this isn't a valid smoke position */
        if (indSmokeTable[i] == 0) {
            continue;
        }

        /* Check if this industrial building type gets smoke */
        if (baseTile == indSmokeTable[i]) {
            /* Calculate the smoke position */
            xx = x + indOffsetX[i];
            yy = y + indOffsetY[i];

            /* Make sure the smoke position is valid */
            if (xx >= 0 && xx < WORLD_X && yy >= 0 && yy < WORLD_Y) {
                /* Choose the appropriate smoke stack tile */
                switch (i) {
                case 0: smokeTile = INDSMOKE1; break;
                case 2: smokeTile = INDSMOKE3; break;
                case 3: smokeTile = INDSMOKE4; break;
                case 6: smokeTile = INDSMOKE6; break;
                case 7: smokeTile = INDSMOKE7; break;
                default: smokeTile = INDSMOKE1; break;
                }

                /* Skip if already animated with the right tile - use TELEBASE range */
                if ((Map[yy][xx] & ANIMBIT) && (Map[yy][xx] & LOMASK) >= TELEBASE &&
                    (Map[yy][xx] & LOMASK) <= TELELAST) {
                    continue;
                }

                /* Set the smoke animation */
                Map[yy][xx] = smokeTile | ANIMBIT | CONDBIT | POWERBIT | BURNBIT;
            }
        }
    }
}

/* Update stadium animations for games */
static void DoStadiumAnimation(int x, int y) {
    int centerX, centerY;

    /* Skip if the position is invalid */
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
        return;
    }

    /* Check if this is a stadium with power */
    if ((Map[y][x] & LOMASK) != STADIUM || !(Map[y][x] & POWERBIT)) {
        return;
    }

    /* Only update periodically - games happen based on city time and position */
    if (!((CityTime + x + y) & 31)) {
        /* Stadium center coordinates to place the game field */
        centerX = x + 1;
        centerY = y;

        /* Make sure the position is valid */
        if (centerX >= 0 && centerX < WORLD_X && centerY >= 0 && centerY < WORLD_Y) {
            /* Set up the football game animation */
            Map[centerY][centerX] = FOOTBALLGAME1 | ANIMBIT | CONDBIT | BURNBIT;
            
            /* Set up the second part of the football game animation */
            if (centerY+1 >= 0 && centerY+1 < WORLD_Y) {
                Map[centerY+1][centerX] = FOOTBALLGAME2 | ANIMBIT | CONDBIT | BURNBIT;
            }
        }
    }

    /* Check if we need to end the game */
    if (((CityTime + x + y) & 7) == 0) {
        /* Scan for football game tiles and remove them */
        centerX = x + 1;
        centerY = y;

        if (centerX >= 0 && centerX < WORLD_X && centerY >= 0 && centerY < WORLD_Y) {
            short tileValue = Map[centerY][centerX] & LOMASK;
            
            /* If we find football game tiles, revert them */
            if (tileValue >= FOOTBALLGAME1 && tileValue <= FOOTBALLGAME1 + 16) {
                Map[centerY][centerX] &= ~ANIMBIT;
            }
            
            if (centerY+1 >= 0 && centerY+1 < WORLD_Y) {
                tileValue = Map[centerY+1][centerX] & LOMASK;
                
                if (tileValue >= FOOTBALLGAME2 && tileValue <= FOOTBALLGAME2 + 16) {
                    Map[centerY+1][centerX] &= ~ANIMBIT;
                }
            }
        }
    }
}

/* Update fire animations */
void UpdateFire(int x, int y) {
    /* Check bounds */
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
        return;
    }

    /* Set fire tiles to animate */
    if ((Map[y][x] & LOMASK) >= FIREBASE && (Map[y][x] & LOMASK) <= (FIREBASE + 7)) {
        Map[y][x] |= ANIMBIT;
    }
}

/* Update nuclear power plant animations */
void UpdateNuclearPower(int x, int y) {
    /* Check bounds */
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
        return;
    }

    /* Set the nuclear core to animate */
    if ((Map[y][x] & LOMASK) == NUCLEAR) {
        int xx, yy;

        /* Set animation bits on the nuclear plant's core */
        xx = x + 2;
        yy = y - 1;

        if (xx >= 0 && xx < WORLD_X && yy >= 0 && yy < WORLD_Y) {
            /* Set the nuclear swirl animation bit with appropriate flags */
            Map[yy][xx] = NUCLEAR_SWIRL | ANIMBIT | CONDBIT | POWERBIT | BURNBIT;
        }
    }
}

/* Update airport radar animation */
void UpdateAirportRadar(int x, int y) {
    /* Check bounds */
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
        return;
    }

    /* Check if this is an airport */
    if ((Map[y][x] & LOMASK) == AIRPORT) {
        int xx, yy;

        /* Set animation bit on the radar tower tile */
        xx = x + 1;
        yy = y - 1;

        if (xx >= 0 && xx < WORLD_X && yy >= 0 && yy < WORLD_Y) {
            /* Set the radar animation bit with appropriate flags */
            Map[yy][xx] = RADAR0 | ANIMBIT | CONDBIT | BURNBIT;
        }
    }
}

/* Update all special animations - called periodically from simulation */
void UpdateSpecialAnimations(void) {
    int x, y;
    short tileValue;

    /* Skip if animation is disabled */
    if (!AnimationEnabled) {
        return;
    }

    /* Scan entire map for special animations */
    for (y = 0; y < WORLD_Y; y++) {
        for (x = 0; x < WORLD_X; x++) {
            tileValue = Map[y][x] & LOMASK;

            /* Add smoke to power plants */
            if (tileValue == POWERPLANT && (Map[y][x] & ZONEBIT)) {
                SetSmoke(x, y);
            }

            /* Add smoke to industrial buildings */
            if (tileValue >= INDBASE && tileValue <= LASTIND && (Map[y][x] & ZONEBIT)) {
                DoIndustrialSmoke(x, y);
            }

            /* Animate nuclear plants */
            if (tileValue == NUCLEAR && (Map[y][x] & ZONEBIT)) {
                UpdateNuclearPower(x, y);
            }

            /* Animate airport radar */
            if (tileValue == AIRPORT && (Map[y][x] & ZONEBIT)) {
                UpdateAirportRadar(x, y);
            }
            
            /* Animate stadium */
            if (tileValue == STADIUM && (Map[y][x] & ZONEBIT)) {
                DoStadiumAnimation(x, y);
            }
        }
    }
}