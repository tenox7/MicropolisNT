/* scenarios.c - Scenario implementation for MicropolisNT
 * Based on original Micropolis code from MicropolisLegacy project
 */

#include "sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#ifndef MB_ICONWARNING
#define MB_ICONWARNING	MB_ICONASTERISK
#define MB_ICONERROR	MB_ICONSTOP
#endif

/* External log functions */
extern void addGameLog(const char *format, ...);
extern void addDebugLog(const char *format, ...);

/* Scenario variables */
short ScenarioID = 0;    /* Current scenario ID (0 = none) */
short DisasterEvent = 0; /* Current disaster type */
short DisasterWait = 0;  /* Countdown to next disaster */
short ScoreType = 0;     /* Score type for scenario */
short ScoreWait = 0;     /* Score wait for scenario */

/* External functions needed from other modules */
extern int SimRandom(int range);     /* From simulation.c */
extern int loadFile(char *filename); /* From main.c */

/* Disaster functions from disasters.c */
extern void doEarthquake(void);          /* Earthquake disaster */
extern void makeFlood(void);             /* Flooding disaster */
extern void makeFire(int x, int y);      /* Fire disaster at specific location */
extern void makeMonster(void);           /* Monster attack disaster */
extern void makeExplosion(int x, int y); /* Explosion at specific location */
extern void makeMeltdown(void);          /* Nuclear meltdown disaster */

/* External functions from main.c */
extern void ForceFullCensus(void); /* Census calculation function */

/* External variables */
extern HWND hwndMain;
extern char cityFileName[MAX_PATH];

/* Disaster timing arrays */
static short DisTab[9] = {0, 2, 10, 5, 20, 3, 5, 5, 2 * 48};
static short ScoreWaitTab[9] = {0,      30 * 48, 5 * 48, 5 * 48, 10 * 48,
                                5 * 48, 10 * 48, 5 * 48, 10 * 48};

/* Load scenario based on ID */
int loadScenario(int scenarioId) {
    char *name = NULL;
    char *fname = NULL;
    char path[MAX_PATH];
    int startYear = 1900;
    FILE *f;

    /* Reset city filename */
    cityFileName[0] = '\0';

    GameLevel = 0; /* Set game level to easy */

    /* Validate scenario ID range */
    if ((scenarioId < 1) || (scenarioId > 8)) {
        MessageBox(hwndMain, "Invalid scenario ID! Using Dullsville (1) instead.", "Warning",
                   MB_ICONWARNING | MB_OK);
        scenarioId = 1;
    }

    /* Check if scenario files are available - construct filename based on ID */
    switch (scenarioId) {
    case 1:
        strcpy(path, "cities\\dullsville.scn");
        break;
    case 2:
        strcpy(path, "cities\\sanfrancisco.scn");
        break;
    case 3:
        strcpy(path, "cities\\hamburg.scn");
        break;
    case 4:
        strcpy(path, "cities\\bern.scn");
        break;
    case 5:
        strcpy(path, "cities\\tokyo.scn");
        break;
    case 6:
        strcpy(path, "cities\\detroit.scn");
        break;
    case 7:
        strcpy(path, "cities\\boston.scn");
        break;
    case 8:
        strcpy(path, "cities\\rio.scn");
        break;
    default:
        strcpy(path, "cities\\dullsville.scn");
        break;
    }

    f = fopen(path, "rb");
    if (f == NULL) {
        MessageBox(hwndMain,
                   "Scenario files not found!\nPlease copy scenario files to the cities directory.",
                   "Error", MB_ICONERROR | MB_OK);
        return 0;
    }
    fclose(f);

    switch (scenarioId) {
    case 1:
        name = "Dullsville";
        fname = "dullsville.scn";
        ScenarioID = 1;
        startYear = 1900;
        TotalFunds = 5000;
        break;
    case 2:
        name = "San Francisco";
        fname = "sanfrancisco.scn";
        ScenarioID = 2;
        startYear = 1906;
        TotalFunds = 20000;
        break;
    case 3:
        name = "Hamburg";
        fname = "hamburg.scn";
        ScenarioID = 3;
        startYear = 1944;
        TotalFunds = 20000;
        break;
    case 4:
        name = "Bern";
        fname = "bern.scn";
        ScenarioID = 4;
        startYear = 1965;
        TotalFunds = 20000;
        break;
    case 5:
        name = "Tokyo";
        fname = "tokyo.scn";
        ScenarioID = 5;
        startYear = 1957;
        TotalFunds = 20000;
        break;
    case 6:
        name = "Detroit";
        fname = "detroit.scn";
        ScenarioID = 6;
        startYear = 1972;
        TotalFunds = 20000;
        break;
    case 7:
        name = "Boston";
        fname = "boston.scn";
        ScenarioID = 7;
        startYear = 2010;
        TotalFunds = 20000;
        break;
    case 8:
        name = "Rio de Janeiro";
        fname = "rio.scn";
        ScenarioID = 8;
        startYear = 2047;
        TotalFunds = 20000;
        break;
    }

    /* Set up scenario initial conditions */
    strcpy(cityFileName, name);
    CityYear = startYear;
    CityMonth = 0;

    /* Update window title with scenario name */
    {
        char winTitle[256];
        wsprintf(winTitle, "MicropolisNT - Scenario: %s", name);
        SetWindowText(hwndMain, winTitle);

        /* Log the scenario load */
        addGameLog("SCENARIO: %s loaded", name);
        addGameLog("Year: %d, Initial funds: $%d", startYear, (int)TotalFunds);

        /* Add scenario-specific descriptions */
        switch (scenarioId) {
        case 1:
            addGameLog("Dullsville: A sleepy town with room to grow");
            addDebugLog("Scenario goal: Build a bigger city");
            break;
        case 2:
            addGameLog("San Francisco 1906: Earthquake-prone metropolis");
            addGameLog("WARNING: Earthquake disaster expected!");
            addDebugLog("Scenario goal: Recover from earthquake");
            break;
        case 3:
            addGameLog("Hamburg 1944: War-torn city requires rebuilding");
            addGameLog("WARNING: Expect fire bombing attacks!");
            addDebugLog("Scenario goal: Rebuild after fire bombing");
            break;
        case 4:
            addGameLog("Bern 1965: Beautiful Swiss capital");
            addDebugLog("Scenario goal: Manage traffic and growth");
            break;
        case 5:
            addGameLog("Tokyo 1957: Dense Japanese metropolis");
            addGameLog("WARNING: Monster attack imminent!");
            addDebugLog("Scenario goal: Recover from monster disaster");
            break;
        case 6:
            addGameLog("Detroit 1972: Struggling with economic decline");
            addDebugLog("Scenario goal: Reverse economic decay");
            break;
        case 7:
            addGameLog("Boston 2010: Home to high-tech industry");
            addGameLog("WARNING: Nuclear accident risk detected!");
            addDebugLog("Scenario goal: Manage nuclear disaster");
            break;
        case 8:
            addGameLog("Rio 2047: Coastal city threatened by flooding");
            addGameLog("WARNING: Flood risk is high!");
            addDebugLog("Scenario goal: Handle rising water levels");
            break;
        }

        addDebugLog("Scenario ID: %d, Starting population ~%d", scenarioId, (int)CityPop);
    }

    /* Set disaster info */
    DisasterEvent = ScenarioID;
    /* Ensure bounds checking before array access */
    if (DisasterEvent >= 0 && DisasterEvent < 9) {
        DisasterWait = DisTab[DisasterEvent];
        ScoreWait = ScoreWaitTab[DisasterEvent];
    } else {
        DisasterWait = 0;
        ScoreWait = 0;
    }
    ScoreType = ScenarioID;

    /* Load scenario file */
    wsprintf(path, "cities\\%s", fname);
    if (!loadFile(path)) {
        MessageBox(hwndMain, "Failed to load scenario file", "Error", MB_ICONERROR | MB_OK);
        return 0;
    }

    /* CRITICAL: Set the flag to prevent ClearCensus from erasing population */
    SkipCensusReset = 1;

    /* Set simulation to medium speed */
    SimSpeed = SPEED_MEDIUM;
    SimPaused = 0;

    /* IMPORTANT: SKIP DoSimInit() for scenarios, as it would reset population!
       Instead, do a directed initialization that preserves scenario values */

    /* Initialize various simulation subsystems */
    /* These are the essential parts of DoSimInit() */

    /* Set up random number generator */
    RandomlySeedRand();

    /* Initialize simulation counters */
    Scycle = 0;
    Fcycle = 0;
    Spdcycle = 0;

    /* Set higher growth demand to encourage population increase */
    SetValves(900, 800, 700);
    ValveFlag = 1;

    /* Initialize budget system without resetting population */
    InitBudget();

    /* Generate a random disaster wait period */
    DisasterWait = SimRandom(51) + 49;

    /* CRITICAL: Add initial population even before census to avoid zero values */
    {
        int x, y;
        short tileValue;
        int resCount = 0, comCount = 0, indCount = 0;

        /* Count residential, commercial, and industrial zones */
        for (y = 0; y < WORLD_Y; y++) {
            for (x = 0; x < WORLD_X; x++) {
                tileValue = Map[y][x] & LOMASK;
                if (Map[y][x] & ZONEBIT) {
                    if (tileValue >= RESBASE && tileValue <= LASTRES) {
                        resCount++;
                    } else if (tileValue >= COMBASE && tileValue <= LASTCOM) {
                        comCount++;
                    } else if (tileValue >= INDBASE && tileValue <= LASTIND) {
                        indCount++;
                    }
                }
            }
        }

        /* Set higher initial population based on zones for better gameplay */
        if (resCount > 0) {
            ResPop = resCount * 16; /* Doubled from 8 */
        }
        if (comCount > 0) {
            ComPop = comCount * 8; /* Doubled from 4 */
        }
        if (indCount > 0) {
            IndPop = indCount * 8; /* Doubled from 4 */
        }

        /* Initialize total population */
        TotalPop = (ResPop + ComPop + IndPop) * 8;

        /* Initialize city population with higher multiplier */
        CityPop = ((ResPop) + (ComPop * 8) + (IndPop * 8)) * 25; /* Increased from 20 */

        /* Initialize class based on population */
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

        /* Ensure we have some population */
        if (CityPop < 100) {
            ResPop = 10;
            TotalPop = ResPop * 8;
            CityPop = ResPop * 20;
        }

        /* Save this value for debugging */
        PrevCityPop = CityPop;
    }

    /* Force a full census calculation */
    /* This is critical for correctly calculating population */
    ForceFullCensus();

    /* After census, explicitly update history graphs */
    TakeCensus();

    /* Make sure population is preserved */
    if (CityPop == 0 && PrevCityPop > 0) {
        /* Restore from previous value if we lost it */
        CityPop = PrevCityPop;
    }

    /* We no longer need to change SkipCensusReset flag as we've modified ClearCensus
       to always reset population counters which allows them to be properly recounted
       during each map scan. This enables growth while preventing population from disappearing. */
    /* Note: SkipCensusReset is now only used for debugging purposes */

    /* Redraw screen */
    InvalidateRect(hwndMain, NULL, TRUE);

    return 1;
}

/* Process scenario disasters */
void scenarioDisaster(void) {
    static int disasterX, disasterY;

    /* Early return if no disaster or waiting period is over */
    if (DisasterEvent == 0 || DisasterWait == 0) {
        return;
    }

    /* Ensure DisasterEvent is in valid range */
    if (DisasterEvent < 1 || DisasterEvent > 8) {
        DisasterEvent = 0;
        return;
    }

    switch (DisasterEvent) {
    case 1: /* Dullsville */
        break;
    case 2: /* San Francisco */
        if (DisasterWait == 1) {
            addGameLog("SCENARIO EVENT: San Francisco earthquake is happening now!");
            addGameLog("Significant damage reported throughout the city!");
            doEarthquake();
        }
        break;
    case 3: /* Hamburg */
        /* Drop fire bombs */
        if (DisasterWait % 10 == 0) {
            disasterX = SimRandom(WORLD_X);
            disasterY = SimRandom(WORLD_Y);
            if (DisasterWait == 20) {
                addGameLog("SCENARIO EVENT: Hamburg firebombing attack has begun!");
                addGameLog("Multiple fires are breaking out across the city!");
            }
            addGameLog("Explosion reported at %d,%d", disasterX, disasterY);
            addDebugLog("Firebomb at coordinates %d,%d, %d bombs remaining", disasterX, disasterY,
                        DisasterWait / 10);
            makeExplosion(disasterX, disasterY);
        }
        break;
    case 4: /* Bern */
        /* No disaster in Bern scenario */
        break;
    case 5: /* Tokyo */
        if (DisasterWait == 1) {
            addGameLog("SCENARIO EVENT: Tokyo monster attack is underway!");
            addGameLog("Giant creature is destroying buildings in its path!");
            makeMonster();
        }
        break;
    case 6: /* Detroit */
        /* No disaster in Detroit scenario */
        break;
    case 7: /* Boston */
        if (DisasterWait == 1) {
            addGameLog("SCENARIO EVENT: Boston nuclear meltdown is happening!");
            addGameLog("Nuclear power plant has suffered a catastrophic failure!");
            makeMeltdown();
        }
        break;
    case 8: /* Rio */
        if ((DisasterWait % 24) == 0) {
            if (DisasterWait == 48) {
                addGameLog("SCENARIO EVENT: Rio flood disaster is starting!");
                addGameLog("Ocean levels are rising - coastal areas at risk!");
            } else {
                addGameLog("Flood waters continue to spread!");
            }
            addDebugLog("Flood event triggered - %d hours until flood peak", DisasterWait);
            makeFlood();
        }
        break;
    default:
        /* Invalid disaster event */
        DisasterEvent = 0;
        return;
    }

    if (DisasterWait) {
        DisasterWait--;

        /* Log when disaster ends */
        if (DisasterWait == 0) {
            addGameLog("Scenario disaster has ended");
            addDebugLog("Scenario disaster ID %d complete", DisasterEvent);
            DisasterEvent = 0;
        }
    } else {
        DisasterEvent = 0;
    }
}
