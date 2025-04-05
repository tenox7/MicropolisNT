/* evaluation.c - City evaluation system for MicropolisNT
 * Based on original Micropolis code from MicropolisLegacy project
 */

#include "sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* External log functions */
extern void addGameLog(const char *format, ...);
extern void addDebugLog(const char *format, ...);

/* Constants */
#define PROBNUM 8 /* Number of city problems tracked */

/* Problem categories */
#define PROB_CRIME 0
#define PROB_POLLUTION 1
#define PROB_HOUSING 2
#define PROB_TAXES 3
#define PROB_TRAFFIC 4
#define PROB_UNEMPLOYMENT 5
#define PROB_FIRE 6
#define PROB_NONE 7

/* Internal state variables */
static short EvalValid;             /* Is evaluation valid? */
static short ProblemTable[PROBNUM]; /* Problem scores */
static short ProblemTaken[PROBNUM]; /* Problems already processed */
static short ProblemVotes[PROBNUM]; /* Problem votes */
static short ProblemOrder[4];       /* Top 4 problems, in order */
static long deltaCityPop;           /* Population change */
static QUAD CityAssValue;           /* City assessed value */
static short AverageCityScore;      /* Average score over time */
static int HospPop;                 /* Hospital population count */
static int NuclearPop;              /* Nuclear plant count */
static int CoalPop;                 /* Coal plant count */

/* Function prototypes */
static void GetAssValue(void);
static void DoPopNum(void);
static void DoProblems(void);
static void GetScore(void);
static void DoVotes(void);
static void VoteProblems(void);
static int AverageTrf(void);
static int GetUnemployment(void);
static int GetFire(void);

/* Initialize the evaluation system */
void EvalInit(void) {
    int x;
    int preservePop;
    int preserveClass;

    /* Save population values */
    preservePop = CityPop;
    preserveClass = CityClass;

    /* Clear variables */
    CityYes = 0;
    CityNo = 0;
    /* Don't clear CityPop here */
    deltaCityPop = 0;
    CityAssValue = 0;
    /* Don't clear CityClass here */
    CityScore = 500;
    deltaCityScore = 0;
    AverageCityScore = 500;
    EvalValid = 1;

    /* Restore population values if they were non-zero */
    if (preservePop > 0) {
        CityPop = preservePop;
        CityClass = preserveClass;
    }

    /* Clear problem arrays */
    for (x = 0; x < PROBNUM; x++) {
        ProblemVotes[x] = 0;
    }

    /* Clear problem order */
    for (x = 0; x < 4; x++) {
        ProblemOrder[x] = 0;
    }
}

/* Calculate the assessed value of the city */
static void GetAssValue(void) {
    QUAD z;

    /* Value of transportation */
    z = RoadTotal * 5;
    z += RailTotal * 10;

    /* Value of emergency services */
    z += PolicePop * 1000;
    z += FirePop * 1000;
    z += HospPop * 400;

    /* Value of entertainment and transportation */
    z += StadiumPop * 3000;
    z += PortPop * 5000;
    z += APortPop * 10000;

    /* Value of power plants */
    z += CoalPop * 3000;
    z += NuclearPop * 6000;

    /* Final value in thousands */
    CityAssValue = z * 1000;
}

/* Calculate population figures and city class */
static void DoPopNum(void) {
    QUAD OldCityPop;

    /* Save old population */
    OldCityPop = CityPop;

    /* CityPop is already calculated in TakeCensus since we need it
       updated more frequently, but calculate it here too for consistency */
    if (ResPop > 0 || ComPop > 0 || IndPop > 0) {
        CityPop = ((ResPop) + (ComPop * 8) + (IndPop * 8)) * 20;
    } else if (CityPop == 0) {
        CityPop = 100; /* Minimum population display */
    }

    /* If first time, set old pop to current */
    if (OldCityPop == 0) {
        OldCityPop = CityPop;
    }

    /* Calculate population change */
    deltaCityPop = CityPop - OldCityPop;

    /* Determine city class based on population - also done in TakeCensus
       but repeated here for consistency */
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

/* Evaluate problem levels in the city */
static void DoProblems(void) {
    int x, z;
    int ThisProb, Max;

    /* Clear problem table */
    for (z = 0; z < PROBNUM; z++) {
        ProblemTable[z] = 0;
    }

    /* Set up problems */
    ProblemTable[PROB_CRIME] = CrimeAverage;
    ProblemTable[PROB_POLLUTION] = PollutionAverage;
    ProblemTable[PROB_HOUSING] = (int)(LVAverage * 0.7);
    ProblemTable[PROB_TAXES] = TaxRate * 10;
    ProblemTable[PROB_TRAFFIC] = AverageTrf();
    ProblemTable[PROB_UNEMPLOYMENT] = GetUnemployment();
    ProblemTable[PROB_FIRE] = GetFire();

    /* Calculate problem votes */
    VoteProblems();

    /* Clear problems taken array */
    for (z = 0; z < PROBNUM; z++) {
        ProblemTaken[z] = 0;
    }

    /* Find top 4 problems */
    for (z = 0; z < 4; z++) {
        Max = 0;

        /* Find highest untaken problem */
        for (x = 0; x < 7; x++) {
            if ((ProblemVotes[x] > Max) && (!ProblemTaken[x])) {
                ThisProb = x;
                Max = ProblemVotes[x];
            }
        }

        /* If problem found, mark it taken and record it */
        if (Max) {
            ProblemTaken[ThisProb] = 1;
            ProblemOrder[z] = ThisProb;
        } else {
            /* No problem found, use NONE */
            ProblemOrder[z] = PROB_NONE;
            ProblemTable[PROB_NONE] = 0;
        }
    }
}

/* Calculate votes for each problem */
static void VoteProblems(void) {
    int x, z, count;

    /* Clear vote counts */
    for (z = 0; z < PROBNUM; z++) {
        ProblemVotes[z] = 0;
    }

    /* Initialize loop variables */
    x = 0;
    z = 0;
    count = 0;

    /* Vote tallying loop */
    while ((z < 100) && (count < 600)) {
        /* Random vote based on problem score */
        if (SimRandom(300) < ProblemTable[x]) {
            ProblemVotes[x]++;
            z++;
        }

        /* Move to next problem */
        x++;
        if (x >= PROBNUM) {
            x = 0;
        }

        /* Increment counter to avoid infinite loop */
        count++;
    }
}

/* Calculate average traffic */
static int AverageTrf(void) {
    QUAD TrfTotal;
    int x, y, count;

    TrfTotal = 0;
    count = 1; /* Start at 1 to avoid division by zero */

    /* Sum up traffic in developed areas */
    for (x = 0; x < WORLD_X / 2; x++) {
        for (y = 0; y < WORLD_Y / 2; y++) {
            if (LandValueMem[y][x]) {
                TrfTotal += TrfDensity[y][x];
                count++;
            }
        }
    }

    /* Calculate average with scaling */
    TrafficAverage = (int)((TrfTotal / count) * 2.4);
    return TrafficAverage;
}

/* Calculate unemployment rate */
static int GetUnemployment(void) {
    float r;
    int b;

    /* Calculate jobs (commercial + industrial) */
    b = (ComPop + IndPop) << 3;

    if (b) {
        /* Ratio of residential to jobs */
        r = ((float)ResPop) / b;
    } else {
        return 0;
    }

    /* Convert to unemployment score (where r=1 is perfect balance) */
    b = (int)((r - 1.0) * 255.0);

    /* Cap at maximum */
    if (b > 255) {
        b = 255;
    }

    return b;
}

/* Calculate fire severity */
static int GetFire(void) {
    int z;

    /* Scale fire count to score */
    z = FirePop * 5;

    /* Cap at maximum */
    if (z > 255) {
        return 255;
    } else {
        return z;
    }
}

/* Calculate city score */
static void GetScore(void) {
    int x, z;
    short OldCityScore;
    float SM, TM;

    /* Save old score */
    OldCityScore = CityScore;

    /* Sum all problems */
    x = 0;
    for (z = 0; z < 7; z++) {
        x += ProblemTable[z];
    }

    /* Average problems */
    x = x / 3;
    if (x > 256) {
        x = 256;
    }

    /* Convert to score (higher = better) */
    z = (256 - x) * 4;
    if (z > 1000) {
        z = 1000;
    }
    if (z < 0) {
        z = 0;
    }

    /* Apply various penalties */

    /* Residential, commercial, industrial caps */
    if (ResCap) {
        z = (int)(z * 0.85);
    }
    if (ComCap) {
        z = (int)(z * 0.85);
    }
    if (IndCap) {
        z = (int)(z * 0.85);
    }

    /* Poor road maintenance */
    if (RoadEffect < 32) {
        z = z - (32 - RoadEffect);
    }

    /* Poor police/fire coverage */
    if (PoliceEffect < 1000) {
        z = (int)(z * (0.9 + (PoliceEffect / 10000.1)));
    }
    if (FireEffect < 1000) {
        z = (int)(z * (0.9 + (FireEffect / 10000.1)));
    }

    /* Negative growth factors */
    if (RValve < -1000) {
        z = (int)(z * 0.85);
    }
    if (CValve < -1000) {
        z = (int)(z * 0.85);
    }
    if (IValve < -1000) {
        z = (int)(z * 0.85);
    }

    /* Apply population growth factor */
    SM = 1.0f;
    if ((CityPop == 0) || (deltaCityPop == 0)) {
        SM = 1.0f;
    } else if (deltaCityPop == CityPop) {
        SM = 1.0f;
    } else if (deltaCityPop > 0) {
        SM = ((float)deltaCityPop / CityPop) + 1.0f;
    } else if (deltaCityPop < 0) {
        SM = 0.95f + ((float)deltaCityPop / (CityPop - deltaCityPop));
    }
    z = (int)(z * SM);

    /* Penalize for fires */
    z = z - GetFire();

    /* Penalize for high taxes */
    z = z - TaxRate;

    /* Penalize for unpowered zones */
    TM = (float)(UnpwrdZCnt + PwrdZCnt);
    if (TM) {
        SM = (float)PwrdZCnt / TM;
    } else {
        SM = 1.0f;
    }
    z = (int)(z * SM);

    /* Ensure score is in valid range */
    if (z > 1000) {
        z = 1000;
    }
    if (z < 0) {
        z = 0;
    }

    /* Smooth score by averaging with previous */
    CityScore = (CityScore + z) / 2;

    /* Calculate score change */
    deltaCityScore = CityScore - OldCityScore;

    /* Update average city score */
    AverageCityScore = (AverageCityScore + CityScore) / 2;
}

/* Calculate vote results */
static void DoVotes(void) {
    int z;

    /* Reset vote counts */
    CityYes = 0;
    CityNo = 0;

    /* Tally 100 votes based on city score */
    for (z = 0; z < 100; z++) {
        /* Higher score = more yes votes */
        if (SimRandom(1000) < CityScore) {
            CityYes++;
        } else {
            CityNo++;
        }
    }
}

/* Perform a city evaluation */
void CityEvaluation(void) {
    short problems[4];
    int i;

    /* Indicate evaluation is in progress */
    EvalValid = 0;

    /* Only evaluate if there are people */
    if (TotalPop) {
        GetAssValue();
        DoPopNum();
        DoProblems();
        GetScore();
        DoVotes();

        /* Log evaluation results */
        addDebugLog("City evaluation: Score=%d, Approval=%d%%", CityScore, CityYes);
        addDebugLog("City assessment: $%d", (int)(CityAssValue / 1000));

        /* Log city problems */
        GetTopProblems(problems);
        addGameLog("City status report:");
        addGameLog("Population: %d - %s", (int)CityPop, GetCityClassName());

        /* Log top problems if they exist */
        if (problems[0] != PROB_NONE) {
            addGameLog("Top problems:");
            for (i = 0; i < 4; i++) {
                if (problems[i] != PROB_NONE) {
                    addGameLog("- %s", GetProblemText(problems[i]));
                }
            }
        } else {
            addGameLog("No significant problems detected");
        }
    } else {
        /* No population, reset values */
        EvalInit();
    }

    /* Indicate evaluation is complete */
    EvalValid = 1;
}

/* Count the number of each special building type */
void CountSpecialTiles(void) {
    int x, y;
    short tile;

    /* Reset counters */
    HospPop = 0;
    CoalPop = 0;
    NuclearPop = 0;

    /* Scan the map for special tiles */
    for (x = 0; x < WORLD_X; x++) {
        for (y = 0; y < WORLD_Y; y++) {
            tile = Map[y][x] & LOMASK;

            /* Check for zone centers */
            if (Map[y][x] & ZONEBIT) {
                /* Check tile types */
                if (tile == HOSPITAL) {
                    HospPop++;
                } else if (tile == POWERPLANT) {
                    CoalPop++;
                } else if (tile == NUCLEAR) {
                    NuclearPop++;
                }
            }
        }
    }
}

/* Get problem description by index */
const char *GetProblemText(int problemIndex) {
    switch (problemIndex) {
    case PROB_CRIME:
        return "Crime";
    case PROB_POLLUTION:
        return "Pollution";
    case PROB_HOUSING:
        return "Housing";
    case PROB_TAXES:
        return "Taxes";
    case PROB_TRAFFIC:
        return "Traffic";
    case PROB_UNEMPLOYMENT:
        return "Unemployment";
    case PROB_FIRE:
        return "Fire";
    case PROB_NONE:
    default:
        return "None";
    }
}

/* Get the city class description */
const char *GetCityClassName(void) {
    switch (CityClass) {
    case 0:
        return "Village";
    case 1:
        return "Town";
    case 2:
        return "City";
    case 3:
        return "Capital";
    case 4:
        return "Metropolis";
    case 5:
        return "Megalopolis";
    default:
        return "Village";
    }
}

/* Get top problems array */
void GetTopProblems(short problems[4]) {
    int i;

    /* Copy problem order array */
    for (i = 0; i < 4; i++) {
        problems[i] = ProblemOrder[i];
    }
}

/* Get problem vote count for a specific problem */
int GetProblemVotes(int problemIndex) {
    if (problemIndex >= 0 && problemIndex < PROBNUM) {
        return ProblemVotes[problemIndex];
    }
    return 0;
}

/* Get city assessment */
QUAD GetCityAssessedValue(void) {
    return CityAssValue;
}

/* Is evaluation data valid */
int IsEvaluationValid(void) {
    return EvalValid;
}

/* Get city average score */
int GetAverageCityScore(void) {
    return AverageCityScore;
}