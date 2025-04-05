/* simulation.c - Core simulation implementation for MicropolisNT
 * Based on original Micropolis code from MicropolisLegacy project
 */

#include "sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#ifndef MB_ICONWARNING
#define MB_ICONWARNING	MB_ICONASTERISK
#define MB_ICONERROR	MB_ICONSTOP
#endif

/* External log functions */
extern void addGameLog(const char *format, ...);
extern void addDebugLog(const char *format, ...);

/* Map data */
Byte PopDensity[WORLD_Y / 2][WORLD_X / 2];
Byte TrfDensity[WORLD_Y / 2][WORLD_X / 2];
Byte PollutionMem[WORLD_Y / 2][WORLD_X / 2];
Byte LandValueMem[WORLD_Y / 2][WORLD_X / 2];
Byte CrimeMem[WORLD_Y / 2][WORLD_X / 2];
short PowerMap[WORLD_Y][WORLD_X];

/* Quarter-sized maps for effects */
Byte TerrainMem[WORLD_Y / 4][WORLD_X / 4];
Byte FireStMap[WORLD_Y / 4][WORLD_X / 4];
Byte FireRate[WORLD_Y / 4][WORLD_X / 4];
Byte PoliceMap[WORLD_Y / 4][WORLD_X / 4];
Byte PoliceMapEffect[WORLD_Y / 4][WORLD_X / 4];

/* Commercial development score */
short ComRate[WORLD_Y / 4][WORLD_X / 4];

/* Runtime simulation state */
int SimSpeed = SPEED_MEDIUM;
int SimSpeedMeta = 0;
int SimPaused = 1;
int CityTime = 0;
int CityYear = 1900;
int CityMonth = 0;
QUAD TotalFunds = 5000;
int TaxRate = 7;
int SkipCensusReset = 0;  /* Flag to skip census reset after loading a scenario */
int DebugCensusReset = 0; /* Debug counter for tracking census resets */
int PrevResPop = 0;       /* Debug tracker for last residential population value */
int PrevCityPop = 0;      /* Debug tracker for last city population value */

/* External declarations for scenario variables */
extern short ScenarioID;
extern short DisasterEvent;
extern short DisasterWait;
extern void scenarioDisaster(void);

/* Counters */
int Scycle = 0;
int Fcycle = 0;
int Spdcycle = 0;

/* Game evaluation */
int CityYes = 0;
int CityNo = 0;
QUAD CityPop = 0;
int CityScore = 500;
int deltaCityScore = 0;
int CityClass = 0;
int CityLevel = 0;
int CityLevelPop = 0;
int GameLevel = 0;
int ResCap = 0;
int ComCap = 0;
int IndCap = 0;

/* City statistics */
int ResPop = 0;
int ComPop = 0;
int IndPop = 0;
int TotalPop = 0;
int LastTotalPop = 0;
float Delta = 1.0f;

/* Infrastructure counts */
int PwrdZCnt = 0;
int UnpwrdZCnt = 0;
int RoadTotal = 0;
int RailTotal = 0;
int FirePop = 0;
int PolicePop = 0;
int StadiumPop = 0;
int PortPop = 0;
int APortPop = 0;
int NuclearPop = 0;

/* External effects */
int RoadEffect = 0;
int PoliceEffect = 0;
int FireEffect = 0;
int TrafficAverage = 0;
int PollutionAverage = 0;
int CrimeAverage = 0;
int LVAverage = 0;

/* Growth rates */
short RValve = 0;
short CValve = 0;
short IValve = 0;
int ValveFlag = 0;

/* Disasters */
extern short DisasterEvent; /* Defined in scenarios.c */
extern short DisasterWait;  /* Defined in scenarios.c */
int DisasterLevel = 0;

/* Internal work variables - also used by power.c */
int SMapX, SMapY; /* Current map position (no longer static, needed by power.c) */
static int TMapX, TMapY;
static short CChr;
static short CChr9;

/* Random number generator - Windows compatible */
void RandomlySeedRand(void) {
    /* Using a fixed seed of 12345 gives more consistent results while still allowing variation */
    static int fixedSeed = 12345;
    srand((unsigned int)fixedSeed);
}

/* Public random number function - available to other modules */
int SimRandom(int range) {
    return (rand() % range);
}

void DoSimInit(void) {
    int x, y;
    int oldResPop, oldComPop, oldIndPop, oldTotalPop, oldCityClass;
    QUAD oldCityPop;
    int centerX, centerY, distance, value;

    /* Save previous population values */
    oldResPop = ResPop;
    oldComPop = ComPop;
    oldIndPop = IndPop;
    oldTotalPop = TotalPop;
    oldCityPop = CityPop;
    oldCityClass = CityClass;

    /* Clear all the density maps */
    memset(PopDensity, 0, sizeof(PopDensity));
    memset(TrfDensity, 0, sizeof(TrfDensity));
    memset(PollutionMem, 0, sizeof(PollutionMem));
    memset(LandValueMem, 0, sizeof(LandValueMem));
    memset(CrimeMem, 0, sizeof(CrimeMem));

    /* Clear all the effect maps */
    memset(TerrainMem, 0, sizeof(TerrainMem));
    memset(FireStMap, 0, sizeof(FireStMap));
    memset(FireRate, 0, sizeof(FireRate));
    memset(PoliceMap, 0, sizeof(PoliceMap));
    memset(PoliceMapEffect, 0, sizeof(PoliceMapEffect));
    memset(ComRate, 0, sizeof(ComRate));

    /* Initialize land values to make simulation more visually interesting */
    centerX = WORLD_X / 4;
    centerY = WORLD_Y / 4;

    for (y = 0; y < WORLD_Y / 2; y++) {
        for (x = 0; x < WORLD_X / 2; x++) {
            /* Create a gradient of land values */
            distance = (x - centerX) * (x - centerX) + (y - centerY) * (y - centerY);
            value = 250 - (distance / 10);
            if (value < 1) {
                value = 1;
            }
            if (value > 250) {
                value = 250;
            }

            LandValueMem[y][x] = (Byte)value;
        }
    }

    /* Clear power map and set to unpowered (0) */
    for (y = 0; y < WORLD_Y; y++) {
        for (x = 0; x < WORLD_X; x++) {
            PowerMap[y][x] = 0; /* Use 0 instead of -1 */
            /* Also clear the power bit in the map */
            Map[y][x] &= ~POWERBIT;
        }
    }

    /* Set up random number generator */
    RandomlySeedRand();

    /* Initialize simulation counters */
    Scycle = 0;
    Fcycle = 0;
    Spdcycle = 0;

    /* Default settings */
    SimSpeed = SPEED_MEDIUM;
    SimPaused = 0; /* Set to running by default */
    CityTime = 0;
    CityYear = 1900;
    CityMonth = 0;

    /* Population counters */
    /* For a new city, set minimum population */
    if (oldCityPop == 0) {
        /* Set minimum default values */
        ResPop = 1;
        ComPop = 1;
        IndPop = 1;
        TotalPop = 3;
        CityPop = 100; /* Minimum city population to display */
        LastTotalPop = 3;
    } else {
        /* Preserve population from previous values */
        ResPop = oldResPop > 0 ? oldResPop : 1;
        ComPop = oldComPop > 0 ? oldComPop : 1;
        IndPop = oldIndPop > 0 ? oldIndPop : 1;
        TotalPop = oldTotalPop > 0 ? oldTotalPop : 3;
        CityPop = oldCityPop > 0 ? oldCityPop : 100;
        CityClass = oldCityClass;
        LastTotalPop = oldTotalPop > 0 ? oldTotalPop : 3;
    }

    /* Set initial growth demand */
    SetValves(500, 300, 100);
    ValveFlag = 1;

    /* Set initial funds and tax rate */
    TotalFunds = 50000; /* Give more starting money */
    TaxRate = 7;

    /* Initial game state */
    CityScore = 500;
    DisasterEvent = 0;
    DisasterWait = 0;

    /* Initialize evaluation system */
    EvalInit();

    /* Initialize budget system */
    InitBudget();

    /* Generate a random disaster wait period */
    DisasterWait = SimRandom(51) + 49;

    /* Force an initial census to populate values */
    TakeCensus();
}

void SimFrame(void) {
    /* Main simulation frame entry point */

    if (SimPaused) {
        return;
    }

    /* Update the speed cycle counter */
    Spdcycle = (Spdcycle + 1) & (SPEEDCYCLE - 1);

    /* Execute simulation steps based on speed */
    switch (SimSpeed) {
    case SPEED_PAUSED:
        /* Do nothing - simulation is paused */
        break;

    case SPEED_SLOW:
        /* Slow speed - process every 5th frame */
        if ((Spdcycle % 5) == 0) {
            Fcycle = (Fcycle + 1) & 1023;
            Simulate(Fcycle & 15);
        }
        break;

    case SPEED_MEDIUM:
        /* Medium speed - process every 3rd frame */
        if ((Spdcycle % 3) == 0) {
            Fcycle = (Fcycle + 1) & 1023;
            Simulate(Fcycle & 15);
        }
        break;

    case SPEED_FAST:
        /* Fast speed - process every frame */
        Fcycle = (Fcycle + 1) & 1023;
        Simulate(Fcycle & 15);
        break;
    }
}

void Simulate(int mod16) {
    /* Main simulation logic */

    Scycle = (Scycle + 1) & 1023;

    /* Perform different actions based on the cycle position (mod 16) */
    switch (mod16) {
    case 0:
        /* Increment time, check for disasters, process valve changes */
        DoTimeStuff();

        /* Adjust valves when needed */
        if (ValveFlag) {
            SetValves(RValve, CValve, IValve);
            ValveFlag = 0;

            /* Log valves change */
            addDebugLog("Demand adjusted: R=%d C=%d I=%d", RValve, CValve, IValve);
        }

        /* DIRECT FIX: Run the power scan at the start of each major cycle
           to ensure power distribution happens frequently enough */
        DoPowerScan();

        /* Process tile animations */
        AnimateTiles();
        break;

    case 1:
        /* Clear census before starting a new scan cycle */
        ClearCensus();
        /* FALLTHROUGH to start map scanning */

    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        /* Scan map in 8 different segments (1/8th each time) */
        {
            int xs = (mod16 - 1) * (WORLD_X / 8);
            int xe = xs + (WORLD_X / 8);
            MapScan(xs, xe, 0, WORLD_Y);
        }
        break;

    case 9:
        /* Process taxes, maintenance, city evaluation if needed */
        /* ClearCensus is now done in case 1 before map scanning */

        /* Every 4 cycles, take census for graphs */
        if ((Scycle % CENSUSRATE) == 0) {
            TakeCensus();
        }

        /* Every 48 cycles, do tax collection and evaluation */
        if ((Scycle % TAXFREQ) == 0) {
            CollectTax();        /* Collect taxes based on population */
            CountSpecialTiles(); /* Count special buildings */
            CityEvaluation();    /* Evaluate city conditions */
        }
        break;

    case 10:
        /* Process traffic decrease & other tile updates */
        DecTrafficMap();

        /* Calculate traffic average periodically */
        if ((Scycle % 4) == 0) {
            CalcTrafficAverage();

            /* Log traffic */
            if (TrafficAverage > 100) {
                addDebugLog("Traffic level: %d (Heavy)", TrafficAverage);
            } else if (TrafficAverage > 50) {
                addDebugLog("Traffic level: %d (Moderate)", TrafficAverage);
            }
        }

        /* Update city population more frequently than just at census time */
        if (ResPop > 0 || ComPop > 0 || IndPop > 0) {
            CityPop = ((ResPop) + (ComPop * 8) + (IndPop * 8)) * 20;
        } else if (CityPop == 0) {
            CityPop = 100; /* Minimum population display */
        }

        /* Run animations for smoother motion */
        AnimateTiles();
        break;

    case 11:
        /* Process power grid updates */
        DoPowerScan();

        /* Check if population has gone to zero (but not initially) */
        if (TotalPop > 0 || LastTotalPop == 0) {
            LastTotalPop = TotalPop;
        } else if (TotalPop == 0 && LastTotalPop != 0) {
            /* ToDo: DoShowPicture(POPULATIONLOST_BIT); */
            LastTotalPop = 0;

            /* Log catastrophic population decline */
            addGameLog("CRISIS: All citizens have left the city!");
        }

        /* Update city class based on population */
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

        /* Log city class - only done when population changes */
        addDebugLog("City class: %s (Pop: %d)", GetCityClassName(), (int)CityPop);
        break;

    case 12:
        /* Process pollution spread (at a reduced rate) */
        if ((Scycle % 16) == 0) {
            PTLScan(); /* Do pollution, terrain, and land value */

            /* Log pollution and land value */
            addDebugLog("Pollution average: %d", PollutionAverage);
            addDebugLog("Land value average: %d", LVAverage);
        }

        /* Update special animations (power plants, etc.) - increased frequency for faster
         * animations */
        if ((Scycle % 2) == 0) {
            UpdateSpecialAnimations();
        }

        /* Process tile animations more frequently for smoother motion */
        AnimateTiles();
        break;

    case 13:
        /* Process crime spread (at a reduced rate) */
        if ((Scycle % 4) == 0) {
            CrimeScan(); /* Do crime map analysis */

            /* Log crime level */
            if (CrimeAverage > 100) {
                addGameLog("WARNING: Crime level is very high (%d)", CrimeAverage);
            } else if (CrimeAverage > 50) {
                addDebugLog("Crime average: %d (Moderate)", CrimeAverage);
            }
        }
        break;

    case 14:
        /* Process population density (at a reduced rate) */
        if ((Scycle % 16) == 0) {
            PopDenScan();   /* Do population density scan */
            FireAnalysis(); /* Update fire protection effect */
        }
        break;

    case 15:
        /* Process fire analysis and disasters (at a reduced rate) */
        if ((Scycle % 4) == 0) {
            /* Process fire spreading */
            spreadFire();

            /* Log fire information */
            if (FirePop > 0) {
                addDebugLog("Active fires: %d", FirePop);
            }
        }

        /* Process disasters */
        if (DisasterEvent) {
            /* Process scenario-based disasters */
            scenarioDisaster();
        }

        /* Process tile animations again at the end of the cycle */
        AnimateTiles();
        break;
    }
}

void DoTimeStuff(void) {
    /* For milestone tracking */
    static int lastMilestone = 0;
    static int lastCityClass = 0;
    int currentMilestone;

    /* Process time advancement */
    CityTime++;

    CityMonth++;
    if (CityMonth > 11) {
        CityMonth = 0;
        CityYear++;

        /* Log the new year */
        addGameLog("New year: %d", CityYear);

        /* Log population milestones */
        currentMilestone = ((int)CityPop / 10000) * 10000;

        if (CityPop > 0 && currentMilestone > lastMilestone) {
            if (currentMilestone == 10000) {
                addGameLog("Population milestone: 10,000 citizens!");
            } else if (currentMilestone == 50000) {
                addGameLog("Population milestone: 50,000 citizens!");
            } else if (currentMilestone == 100000) {
                addGameLog("Population milestone: 100,000 citizens!");
            } else if (currentMilestone == 500000) {
                addGameLog("Population milestone: 500,000 citizens!");
            } else if (currentMilestone >= 1000000 && (currentMilestone % 1000000) == 0) {
                addGameLog("Population milestone: %d Million citizens!",
                           currentMilestone / 1000000);
            } else if (currentMilestone > 0) {
                addGameLog("Population milestone: %d citizens", currentMilestone);
            }

            lastMilestone = currentMilestone;
        }

        /* Check for city class changes */
        if (CityClass > lastCityClass) {
            addGameLog("City upgraded to %s!", GetCityClassName());
            lastCityClass = CityClass;
        }

        /* Make the simulation more dynamic - adjust valves more frequently */
        /* Increased from every 2 years to every year */
        {
            int rDelta, cDelta, iDelta;

            /* Much stronger bias toward positive growth */
            rDelta = SimRandom(600) - 100; /* Bias toward positive values */
            cDelta = SimRandom(600) - 100;
            iDelta = SimRandom(600) - 100;

            RValve += rDelta;
            CValve += cDelta;
            IValve += iDelta;

            /* Force valves to stay positive most of the time */
            if (RValve < 0 && SimRandom(4) < 3) {
                RValve = 200 + SimRandom(300);
            }
            if (CValve < 0 && SimRandom(4) < 3) {
                CValve = 200 + SimRandom(300);
            }
            if (IValve < 0 && SimRandom(4) < 3) {
                IValve = 200 + SimRandom(300);
            }

            /* Ensure valves stay in reasonable ranges */
            if (RValve < -1000) {
                RValve = -1000;
            }
            if (RValve > 2000) {
                RValve = 2000;
            }
            if (CValve < -1000) {
                CValve = -1000;
            }
            if (CValve > 2000) {
                CValve = 2000;
            }
            if (IValve < -1000) {
                IValve = -1000;
            }
            if (IValve > 2000) {
                IValve = 2000;
            }

            /* Debug valve changes */
            {
                char debugMsg[256];
                wsprintf(debugMsg, "VALVES: R=%d C=%d I=%d (Year %d)\n", RValve, CValve, IValve,
                         CityYear);
                OutputDebugString(debugMsg);

                /* Add to log window */
                addDebugLog("Growth rates: Residential=%d Commercial=%d Industrial=%d", RValve,
                            CValve, IValve);
            }
        }

        /* Add some funds periodically to keep things interesting */
        TotalFunds += 1000;
    }

    /* Manage disasters */
    if (DisasterEvent) {
        DisasterWait = 0;
    } else {
        if (DisasterWait > 0) {
            DisasterWait--;
        } else {
            /* Check for random disasters when counter reaches zero */
            if (GameLevel > 0) {
                if (SimRandom(9 - GameLevel) == 0) {
                    /* Choose a disaster type */
                    switch (SimRandom(8)) {
                    case 0:
                    case 1:
                        /* ToDo: MakeFlood(); */
                        break;
                    case 2:
                    case 3:
                        /* ToDo: MakeFire(); */
                        break;
                    case 4:
                    case 5:
                        /* ToDo: MakeAirCrash(); */
                        break;
                    case 6:
                        /* ToDo: MakeTornado(); */
                        break;
                    case 7:
                        /* ToDo: MakeEarthquake(); */
                        break;
                    }
                }
            }
            /* Reset disaster wait period */
            DisasterWait = SimRandom(51) + 49;
        }
    }
}

void SetValves(int res, int com, int ind) {
    /* Set the development rate valves */

    if (res < -2000) {
        res = -2000;
    }
    if (com < -2000) {
        com = -2000;
    }
    if (ind < -2000) {
        ind = -2000;
    }

    if (res > 2000) {
        res = 2000;
    }
    if (com > 2000) {
        com = 2000;
    }
    if (ind > 2000) {
        ind = 2000;
    }

    RValve = res;
    CValve = com;
    IValve = ind;

    /* Update the toolbar to reflect the new RCI values */
    UpdateToolbar();
}

void ClearCensus(void) {
    /* CRITICAL: Save previous population values BEFORE resetting */
    QUAD oldCityPop;
    int oldResPop;
    int oldComPop;
    int oldIndPop;
    int oldTotalPop;

    oldCityPop = CityPop;
    oldResPop = ResPop;
    oldComPop = ComPop;
    oldIndPop = IndPop;
    oldTotalPop = TotalPop;

    /* DEBUG: Track previous population values */
    PrevResPop = ResPop;
    PrevCityPop = (int)CityPop;

    /* Log infrastructure counts before resetting */
    addDebugLog("Infrastructure: Roads=%d Rail=%d Fire=%d Police=%d", RoadTotal, RailTotal, FirePop,
                PolicePop);
    addDebugLog("Special zones: Stadium=%d Port=%d Airport=%d Nuclear=%d", StadiumPop, PortPop,
                APortPop, NuclearPop);
    addDebugLog("Power: Powered=%d Unpowered=%d", PwrdZCnt, UnpwrdZCnt);

    /* Infrastructure counts always need resetting */
    RoadTotal = 0;
    RailTotal = 0;
    FirePop = 0;
    PolicePop = 0;
    StadiumPop = 0;
    PortPop = 0;
    APortPop = 0;
    NuclearPop = 0;
    PwrdZCnt = 0;
    UnpwrdZCnt = 0;

    /* Population MUST be reset to allow recounting and growth/decline */
    /* This is the key difference: always reset these values regardless of SkipCensusReset */
    ResPop = 0;
    ComPop = 0;
    IndPop = 0;
    TotalPop = 0;

    /* DEBUG: Increment counter to track census resets */
    DebugCensusReset++;
}

void TakeCensus(void) {
    /* Store city statistics in the history arrays */
    int i;
    QUAD newCityPop;

    /* CRITICAL: Make sure we have valid population counts even if they're small */
    if (ResPop <= 0 && (Map[4][4] & LOMASK) == RESBASE) {
        ResPop = 50;  /* Set a minimal initial population */
    }

    /* Calculate total population - normalize for simulation */
    TotalPop = ResPop + ComPop + IndPop;

    /* Calculate new city population from zone populations using the official formula */
    newCityPop = ((ResPop) + (ComPop * 8) + (IndPop * 8)) * 20;

    /* If we have real zone population, update CityPop with the new value */
    if (ResPop > 0 || ComPop > 0 || IndPop > 0) {
        CityPop = newCityPop;
    }
    /* If we don't have zone population but have previous CityPop, maintain it */
    else if (CityPop == 0 && PrevCityPop > 0) {
        CityPop = PrevCityPop;
    }

    /* CRITICAL: Make sure we have some population value if there are zones */
    if (CityPop < 100 && (PwrdZCnt > 0 || UnpwrdZCnt > 0)) {
        /* Set a minimum population if there are any zones */
        CityPop = 100;
    }

    /* DEBUG: Output current population state */
    {
        char debugMsg[256];
        wsprintf(debugMsg,
                 "DEBUG Population: Res=%d Com=%d Ind=%d Total=%d CityPop=%d (Prev=%d) Resets=%d\n",
                 ResPop, ComPop, IndPop, TotalPop, (int)CityPop, PrevCityPop, DebugCensusReset);
        OutputDebugString(debugMsg);

        /* Add to log window */
        addDebugLog("Census: R=%d C=%d I=%d Total=%d CityPop=%d", ResPop, ComPop, IndPop, TotalPop,
                    (int)CityPop);
    }

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

    /* Make sure CityPop is never zero if we have zones */
    if (CityPop == 0) {
        if (ResPop > 0 || ComPop > 0 || IndPop > 0) {
            /* Calculate from zone counts */
            CityPop = ((ResPop) + (ComPop * 8) + (IndPop * 8)) * 20;

            /* If still zero, use minimum value */
            if (CityPop == 0) {
                CityPop = 100; /* Minimum village size */
                CityClass = 0; /* Village */
            }
        } else if (PrevCityPop > 0) {
            /* If we lost all zones but had population before, maintain it */
            CityPop = PrevCityPop;
        } else {
            /* Absolute minimum to prevent zero display */
            CityPop = 100;
            CityClass = 0;
        }
    }

    /* Track population changes for growth rate calculations */
    if (CityPop > PrevCityPop) {
        /* Population is growing */
        char debugMsg[256];
        int growth;

        growth = (int)(CityPop - PrevCityPop);
        wsprintf(debugMsg, "GROWTH: Population increased from %d to %d (+%d)\n", PrevCityPop,
                 (int)CityPop, growth);
        OutputDebugString(debugMsg);

        /* Add to log window - use regular log for population growth */
        if (growth > 100) {
            addGameLog("Population growing: +%d citizens", growth);
        }
    } else if (CityPop < PrevCityPop) {
        /* Population is declining */
        char debugMsg[256];
        int decline;

        decline = (int)(PrevCityPop - CityPop);
        wsprintf(debugMsg, "DECLINE: Population decreased from %d to %d (-%d)\n", PrevCityPop,
                 (int)CityPop, decline);
        OutputDebugString(debugMsg);

        /* Add to log window - use regular log for population decline */
        if (decline > 100) {
            addGameLog("Population declining: -%d citizens", decline);
        }
    }

    /* Save current value for next comparison */
    PrevCityPop = (int)CityPop;

    /* Update graph history */
    for (i = 0; i < HISTLEN / 2 - 1; i++) {
        ResHis[i] = ResHis[i + 1];
        ComHis[i] = ComHis[i + 1];
        IndHis[i] = IndHis[i + 1];
        CrimeHis[i] = CrimeHis[i + 1];
        PollutionHis[i] = PollutionHis[i + 1];
        MoneyHis[i] = MoneyHis[i + 1];
    }

    /* Update miscellaneous history */
    for (i = 0; i < MISCHISTLEN / 2 - 1; i++) {
        MiscHis[i] = MiscHis[i + 1];
    }

    /* Record current values in history */
    ResHis[HISTLEN / 2 - 1] = ResPop * 8;
    ComHis[HISTLEN / 2 - 1] = ComPop * 8;
    IndHis[HISTLEN / 2 - 1] = IndPop * 8;
    CrimeHis[HISTLEN / 2 - 1] = CrimeAverage * 8;
    PollutionHis[HISTLEN / 2 - 1] = PollutionAverage * 8;
    MoneyHis[HISTLEN / 2 - 1] = (short)(TotalFunds / 100);

    /* Note: MiscHis will be updated in the specific subsystem implementations */
}

void MapScan(int x1, int x2, int y1, int y2) {
    /* Scan a section of the map for zone processing */
    int x, y;

    if (x1 < 0 || x2 > WORLD_X || y1 < 0 || y2 > WORLD_Y) {
        return;
    }

    for (x = x1; x < x2; x++) {
        for (y = y1; y < y2; y++) {
            SMapX = x;
            SMapY = y;
            CChr = Map[y][x] & LOMASK;

            /* Process all zone centers, whether powered or not */
            if (Map[y][x] & ZONEBIT) {
                DoZone(x, y, CChr);
            }
        }
    }
}

int GetPValue(int x, int y) {
    /* Get power status at a given position */
    if (x >= 0 && x < WORLD_X && y >= 0 && y < WORLD_Y) {
        return (Map[y][x] & POWERBIT) != 0;
    }
    return 0;
}

/* Check if coordinates are within map bounds */
int TestBounds(int x, int y) {
    return (x >= 0 && x < WORLD_X && y >= 0 && y < WORLD_Y);
}

/* Timer ID for simulation */
#define SIM_TIMER_ID 1

/* Timer interval in milliseconds */
#define SIM_TIMER_INTERVAL 50

/* External function declaration for the UI update */
extern void UpdateSimulationMenu(HWND hwnd, int speed);

/* Global timer ID to track between function calls */
static UINT SimTimerID = 0;

void SetSimulationSpeed(HWND hwnd, int speed) {
    /* Update the simulation speed */
    SimSpeed = speed;

    /* Update UI (menu checkmarks) */
    UpdateSimulationMenu(hwnd, speed);

    /* If paused, stop the timer */
    if (speed == SPEED_PAUSED) {
        SimPaused = 1;
        if (SimTimerID) {
            KillTimer(hwnd, SIM_TIMER_ID);
            SimTimerID = 0;
        }
    } else {
        /* Otherwise, make sure the timer is running */
        SimPaused = 0;
        if (!SimTimerID) {
            SimTimerID = SetTimer(hwnd, SIM_TIMER_ID, SIM_TIMER_INTERVAL, NULL);

            /* Check for timer creation failure */
            if (!SimTimerID) {
                MessageBox(hwnd, "Failed to create simulation timer", "Error",
                           MB_ICONERROR | MB_OK);
            }
        }
    }
}

/* Cleanup simulation timer when program exits */
void CleanupSimTimer(HWND hwnd) {
    if (SimTimerID) {
        KillTimer(hwnd, SIM_TIMER_ID);
        SimTimerID = 0;
    }
}
