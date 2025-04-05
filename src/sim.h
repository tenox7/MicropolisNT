/* simulation.h - Core simulation structures and functions for MicropolisNT
 * Based on original Micropolis code from MicropolisLegacy project
 */

#ifndef _SIMULATION_H
#define _SIMULATION_H

#include <windows.h>

/* External declaration for UpdateToolbar function */
extern void UpdateToolbar(void);

/* Basic type definitions */
typedef unsigned char Byte;
typedef long QUAD;

/* Constants */
#define WORLD_X         120
#define WORLD_Y         100
#define WORLD_W         WORLD_X
#define WORLD_H         WORLD_Y

#define SmX             (WORLD_X >> 1)
#define SmY             (WORLD_Y >> 1)

/* Game levels */
#define LEVEL_EASY      0
#define LEVEL_MEDIUM    1
#define LEVEL_HARD      2

/* Maximum size for arrays */
#define HISTLEN         480
#define MISCHISTLEN     240

/* Masks for the various tile properties */
#define LOMASK          0x03ff  /* Mask for the low 10 bits = 1023 decimal */
#define ANIMBIT         0x0800  /* bit 11, tile is animated */
#define BURNBIT         0x2000  /* bit 13, tile can be lit */
#define BULLBIT         0x1000  /* bit 12, tile is bulldozable */
#define CONDBIT         0x4000  /* bit 14, tile can conduct electricity */
#define ZONEBIT         0x0400  /* bit 10, tile is the center of a zone */
#define POWERBIT        0x8000  /* bit 15, tile has power */
#define MASKBITS        (~LOMASK)  /* Mask for just the bits */
#define BNCNBIT         0x0400  /* Center bit */
#define ALLBITS         0xFFFF  /* All bits */

/*
 * MICROPOLIS TILESET LAYOUT - 16×16 PIXEL TILES IN A GRID
 * --------------------------------------------------------
 * Tile numbers are arranged in a 32×30 grid (32 columns, 30 rows)
 * The tileset BMP files are 512x480 pixels.
 *
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 | 29 | 30 | 31 | Row 0: Dirt, River, River edges, Trees
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * | 32 | 33 | 34 | 35 | 36 | 37 | 38 | 39 | 40 | 41 | 42 | 43 | 44 | 45 | 46 | 47 | 48 | 49 | 50 | 51 | 52 | 53 | 54 | 55 | 56 | 57 | 58 | 59 | 60 | 61 | 62 | 63 | Row 1: Trees, Woods, Rubble, Flood, Radiation, Fire
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * | 64 | 65 | 66 | 67 | 68 | 69 | 70 | 71 | 72 | 73 | 74 | 75 | 76 | 77 | 78 | 79 | 80 | 81 | 82 | 83 | 84 | 85 | 86 | 87 | 88 | 89 | 90 | 91 | 92 | 93 | 94 | 95 | Row 2: Roads, Bridges, Crossings, Traffic
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * | 96 | 97 | 98 | 99 |100 |101 |102 |103 |104 |105 |106 |107 |108 |109 |110 |111 |112 |113 |114 |115 |116 |117 |118 |119 |120 |121 |122 |123 |124 |125 |126 |127 | Row 3: Traffic (continued)
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * |128 |129 |130 |131 |132 |133 |134 |135 |136 |137 |138 |139 |140 |141 |142 |143 |144 |145 |146 |147 |148 |149 |150 |151 |152 |153 |154 |155 |156 |157 |158 |159 | Row 4: Traffic (continued)
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * |160 |161 |162 |163 |164 |165 |166 |167 |168 |169 |170 |171 |172 |173 |174 |175 |176 |177 |178 |179 |180 |181 |182 |183 |184 |185 |186 |187 |188 |189 |190 |191 | Row 5: Traffic (continued)
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * |192 |193 |194 |195 |196 |197 |198 |199 |200 |201 |202 |203 |204 |205 |206 |207 |208 |209 |210 |211 |212 |213 |214 |215 |216 |217 |218 |219 |220 |221 |222 |223 | Row 6: Traffic, Power Lines
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * |224 |225 |226 |227 |228 |229 |230 |231 |232 |233 |234 |235 |236 |237 |238 |239 |240 |241 |242 |243 |244 |245 |246 |247 |248 |249 |250 |251 |252 |253 |254 |255 | Row 7: Rail, Rail crossings, Residential zones
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... |... | Rows 8-29: Residential, Commercial, Industrial zones
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * |928 |929 |930 |931 |932 |933 |934 |935 |936 |937 |938 |939 |940 |941 |942 |943 |944 |945 |946 |947 |948 |949 |950 |951 |952 |953 |954 |955 |956 |957 |958 |959 | Row 29: Stadium games, Bridge parts
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 *
 * Special crossing tiles:
 * - HROADPOWER (77): Horizontal road with vertical power line (╥)
 * - VROADPOWER (78): Vertical road with horizontal power line (╞)
 * - RAILHPOWERV (221): Horizontal rail with vertical power line
 * - RAILVPOWERH (222): Vertical rail with horizontal power line
 * - HRAILROAD (237): Horizontal rail with vertical road
 * - VRAILROAD (238): Vertical rail with horizontal road
 */

 #define DIRT		0
 #define RIVER		2
 #define REDGE		3
 #define CHANNEL		4
 #define FIRSTRIVEDGE	5
 #define LASTRIVEDGE	20
 #define TREEBASE	21
 #define LASTTREE	36
 #define WOODS		37
 #define UNUSED_TRASH1	38
 #define UNUSED_TRASH2	39
 #define WOODS2		40
 #define WOODS3		41
 #define WOODS4		42
 #define WOODS5		43
 #define RUBBLE		44
 #define LASTRUBBLE	47
 #define FLOOD		48
 #define LASTFLOOD	51
 #define RADTILE		52
 #define UNUSED_TRASH3	53
 #define UNUSED_TRASH4	54
 #define UNUSED_TRASH5	55
 #define FIRE		56
 #define FIREBASE	56
 #define LASTFIRE	63
 #define ROADBASE	64
 #define HBRIDGE		64
 #define VBRIDGE		65
 /* Road tiles - positioned according to RoadTable[16] index 0-15 */
 #define ROADS		66  /* East-West road (no connections) - RoadTable[0] */
 #define ROADS2		67  /* North-South road (no connections) - RoadTable[1] */
 #define ROADS3		68  /* East-West road with West connection - RoadTable[2] */
 #define ROADS4		69  /* North-South road with South connection - RoadTable[3] */
 #define ROADS5		70  /* East-West road with East connection - RoadTable[4] */
 #define ROADS6		71  /* North-West corner - RoadTable[5] */
 #define ROADS7		72  /* North-East corner - RoadTable[6] */
 #define ROADS8		73  /* South-East corner - RoadTable[7] */
 #define ROADS9		74  /* South-West corner - RoadTable[8] */
 #define ROADS10		75  /* T-junction East-South-West - RoadTable[9] */
 #define ROADS11		76  /* T-junction North-South-West - RoadTable[10] */
 #define ROADS12		77  /* T-junction North-East-South - RoadTable[11] */
 #define ROADS13		78  /* T-junction North-East-West - RoadTable[12] */
 #define ROADS14		79  /* North-South road with North connection - RoadTable[13] */
 #define ROADS15		80  /* East-West road with East and West connections - RoadTable[14] */
 #define ROADS16		81  /* North-South road with North and South connections - RoadTable[15] */
 #define INTERSECTION	82  /* All connections (North, East, South, West) */
 #define HROADPOWER	83  /* Horizontal road with vertical power line */
 #define VROADPOWER	84  /* Vertical road with horizontal power line */
 #define BRWH		79
 #define LTRFBASE	80
 #define BRWV		95
 #define BRWXXX1		111
 #define BRWXXX2		127
 #define BRWXXX3		143
 #define HTRFBASE	144
 #define BRWXXX4		159
 #define BRWXXX5		175
 #define BRWXXX6		191
 #define LASTROAD	206
 #define BRWXXX7		207
 #define POWERBASE	208
 #define HPOWER		208
 #define VPOWER		209
 /* Power line tiles - positioned according to WireTable[16] index 0-15 */
 #define LHPOWER		210  /* Horizontal power line (no connections) - WireTable[0] */
 #define LVPOWER		211  /* Vertical power line (no connections) - WireTable[1] */
 #define LVPOWER2	212  /* Horizontal power line with West connection - WireTable[2] */
 #define LVPOWER3	213  /* Vertical power line with South connection - WireTable[3] */
 #define LVPOWER4	214  /* Horizontal power line with East connection - WireTable[4] */
 #define LVPOWER5	215  /* North-West corner power line - WireTable[5] */
 #define LVPOWER6	216  /* North-East corner power line - WireTable[6] */
 #define LVPOWER7	217  /* South-East corner power line - WireTable[7] */
 #define LVPOWER8	218  /* South-West corner power line - WireTable[8] */
 #define LVPOWER9	219  /* T-junction E-S-W power line - WireTable[9] */
 #define LVPOWER10	220  /* T-junction N-S-W power line - WireTable[10] */
 #define RAILHPOWERV	221  /* Horizontal rail with vertical power line */
 #define RAILVPOWERH	222  /* Vertical rail with horizontal power line */
 #define LASTPOWER	222
 #define UNUSED_TRASH6	223
 #define RAILBASE	224
 #define HRAIL		224  /* Horizontal rail (no connections) - RailTable[0] */
 #define VRAIL		225  /* Vertical rail (no connections) - RailTable[1] */
 /* Rail tiles - positioned according to RailTable[16] index 0-15 */
 #define LHRAIL		226  /* Horizontal rail (no connections) - RailTable[0] */
 #define LVRAIL		227  /* Vertical rail (no connections) - RailTable[1] */
 #define LVRAIL2		228  /* Horizontal rail with West connection - RailTable[2] */
 #define LVRAIL3		229  /* Vertical rail with South connection - RailTable[3] */
 #define LVRAIL4		230  /* Horizontal rail with East connection - RailTable[4] */
 #define LVRAIL5		231  /* North-West corner rail - RailTable[5] */
 #define LVRAIL6		232  /* North-East corner rail - RailTable[6] */
 #define LVRAIL7		233  /* South-East corner rail - RailTable[7] */
 #define LVRAIL8		234  /* South-West corner rail - RailTable[8] */
 #define LVRAIL9		235  /* T-junction E-S-W rail - RailTable[9] */
 #define LVRAIL10	236  /* T-junction N-S-W rail - RailTable[10] */
 #define HRAILROAD	237  /* Horizontal rail with vertical road */
 #define VRAILROAD	238  /* Vertical rail with horizontal road */
 #define LASTRAIL	238
 #define ROADVPOWERH	239 /* bogus? */
 #define RESBASE		240
 #define FREEZ		244
 #define HOUSE		249
 #define LHTHR		249
 #define HHTHR		260
 #define RZB		265
 #define HOSPITAL	409
 #define CHURCH		418
 #define COMBASE		423
 #define COMCLR		427
 #define CZB		436
 #define INDBASE		612
 #define INDCLR		616
 #define LASTIND		620
 #define IND1		621
 #define IZB		625
 #define IND2		641
 #define IND3		644
 #define IND4		649
 #define IND5		650
 #define IND6		676
 #define IND7		677
 #define IND8		686
 #define IND9		689
 #define PORTBASE	693
 #define PORT		698
 #define LASTPORT	708
 #define AIRPORTBASE	709
 #define RADAR		711
 #define AIRPORT		716
 #define COALBASE	745
 #define POWERPLANT	750
 #define LASTPOWERPLANT	760
 #define FIRESTBASE	761
 #define FIRESTATION	765
 #define POLICESTBASE	770
 #define POLICESTATION	774
 #define STADIUMBASE	779
 #define STADIUM		784
 #define FULLSTADIUM	800
 #define NUCLEARBASE	811
 #define NUCLEAR		816
 #define LASTZONE	826
 #define LIGHTNINGBOLT	827
 #define HBRDG0		828
 #define HBRDG1		829
 #define HBRDG2		830
 #define HBRDG3		831
 #define RADAR0		832
 #define RADAR1		833
 #define RADAR2		834
 #define RADAR3		835
 #define RADAR4		836
 #define RADAR5		837
 #define RADAR6		838
 #define RADAR7		839
 #define FOUNTAIN	840
 #define INDBASE2	844
 #define TELEBASE	844
 #define TELELAST	851
 #define SMOKEBASE	852
 #define TINYEXP		860
 #define SOMETINYEXP	864
 #define LASTTINYEXP	867
 #define COALSMOKE1	916
 #define COALSMOKE2	920
 #define COALSMOKE3	924
 #define COALSMOKE4	928
 #define FOOTBALLGAME1	932
 #define FOOTBALLGAME2	940
 #define VBRDG0		948
 #define VBRDG1		949
 #define VBRDG2		950
 #define VBRDG3		951

 #define TILE_COUNT	960

/* Other constants */
#define BSIZE           8       /* Building size? */
#define TILE_DIRT       0       /* Dirt tile (same as DIRT) */
#define PWRBIT          0x8000  /* Power bit (same as POWERBIT) */

/* Zone base and limit values */
#define RESBASE         240     /* Start of residential zones */
#define LASTRES         420     /* End of residential zones */
#define COMBASE         423     /* Start of commercial zones */
#define LASTCOM         611     /* End of commercial zones */
#define INDBASE         612     /* Start of industrial zones */
#define LIGHTNINGBOLT   827     /* No-power indicator */

/* Game simulation rate constants */
#define SPEEDCYCLE      1024  /* The number of cycles before the speed counter loops from 0-1023 */
#define CENSUSRATE      4     /* Census update rate (once per 4 passes) */
#define TAXFREQ         48    /* Tax assessment frequency (once per 48 passes) */
#define VALVEFREQ       16    /* Valve adjustment frequency (once every 16 passes) */

/* Tool states */
#define residentialState    0
#define commercialState     1
#define industrialState     2
#define fireState           3
#define policeState         4
#define wireState           5
#define roadState           6
#define railState           7
#define parkState           8
#define stadiumState        9
#define seaportState       10
#define powerState         11
#define nuclearState       12
#define airportState       13
#define networkState       14
#define bulldozerState     15
#define queryState         16
#define windowState        17
#define noToolState        18

/* Simulation speed settings */
#define SPEED_PAUSED     0
#define SPEED_SLOW       1
#define SPEED_MEDIUM     2
#define SPEED_FAST       3

/* Structures */
extern short Map[WORLD_Y][WORLD_X];      /* The main map */
extern Byte PopDensity[WORLD_Y/2][WORLD_X/2]; /* Population density map (half size) */
extern Byte TrfDensity[WORLD_Y/2][WORLD_X/2]; /* Traffic density map (half size) */
extern Byte PollutionMem[WORLD_Y/2][WORLD_X/2]; /* Pollution density map (half size) */
extern Byte LandValueMem[WORLD_Y/2][WORLD_X/2]; /* Land value map (half size) */
extern Byte CrimeMem[WORLD_Y/2][WORLD_X/2];   /* Crime map (half size) */
extern short PowerMap[WORLD_Y][WORLD_X];  /* Power connectivity map */

/* Quarter-sized maps for effects */
extern Byte TerrainMem[WORLD_Y/4][WORLD_X/4];  /* Terrain memory (quarter size) */
extern Byte FireStMap[WORLD_Y/4][WORLD_X/4];   /* Fire station map (quarter size) */
extern Byte FireRate[WORLD_Y/4][WORLD_X/4];    /* Fire coverage rate (quarter size) */
extern Byte PoliceMap[WORLD_Y/4][WORLD_X/4];   /* Police station map (quarter size) */
extern Byte PoliceMapEffect[WORLD_Y/4][WORLD_X/4]; /* Police station effect (quarter size) */

/* Commercial development score */
extern short ComRate[WORLD_Y/4][WORLD_X/4];    /* Commercial score (quarter size) */

/* Historical data for graphs */
extern short ResHis[HISTLEN/2];      /* Residential history */
extern short ComHis[HISTLEN/2];      /* Commercial history */
extern short IndHis[HISTLEN/2];      /* Industrial history */
extern short CrimeHis[HISTLEN/2];    /* Crime history */
extern short PollutionHis[HISTLEN/2]; /* Pollution history */
extern short MoneyHis[HISTLEN/2];    /* Cash flow history */
extern short MiscHis[MISCHISTLEN/2]; /* Miscellaneous history */

/* Runtime simulation state */
extern int SimSpeed;     /* 0=pause, 1=slow, 2=med, 3=fast */
extern int SimSpeedMeta; /* Counter for adjusting sim speed, 0-3 */
extern int SimPaused;    /* 1 if paused, 0 otherwise */
extern int CityTime;     /* City time from 0 to ~32 depending on scenario */
extern int CityYear;     /* City year from 1900 onwards */
extern int CityMonth;    /* City month from Jan to Dec */
extern QUAD TotalFunds;  /* City operating funds */
extern int TaxRate;      /* City tax rate 0-20 */
extern int SkipCensusReset; /* Flag to skip census reset after loading a scenario */
extern int DebugCensusReset; /* Debug counter for tracking census resets */
extern int PrevResPop;      /* Debug tracker for last residential population value */
extern int PrevCityPop;     /* Debug tracker for last city population value */

/* Counters */
extern int Scycle;       /* Simulation cycle counter (0-1023) */
extern int Fcycle;       /* Frame counter (0-1023) */
extern int Spdcycle;     /* Speed cycle counter (0-1023) */

/* Game evaluation */
extern int CityYes;        /* Positive sentiment votes */
extern int CityNo;         /* Negative sentiment votes */
extern QUAD CityPop;       /* Population assessment */
extern int CityScore;      /* City score */
extern int deltaCityScore; /* Score change */
extern int CityClass;      /* City class (village, town, city, etc.) */
extern int CityLevel;      /* Mayor level (0-5, 0 is worst) */
extern int CityLevelPop;   /* Population threshold for level */
extern int GameLevel;      /* Game level (0=easy, 1=medium, 2=hard) */
extern int ResCap;         /* Residential capacity reached */
extern int ComCap;         /* Commercial capacity reached */
extern int IndCap;         /* Industrial capacity reached */

/* City statistics */
extern int ResPop;       /* Residential population */
extern int ComPop;       /* Commercial population */
extern int IndPop;       /* Industrial population */
extern int TotalPop;     /* Total population */
extern int LastTotalPop; /* Previous total population */
extern float Delta;      /* Population change coefficient */

/* Infrastructure counts */
extern int PwrdZCnt;     /* Number of powered zones */
extern int UnpwrdZCnt;   /* Number of unpowered zones */
extern int RoadTotal;    /* Number of road tiles */
extern int RailTotal;    /* Number of rail tiles */
extern int FirePop;      /* Number of fire station zones */
extern int PolicePop;    /* Number of police station zones */
extern int StadiumPop;   /* Number of stadium tiles */
extern int PortPop;      /* Number of seaport tiles */
extern int APortPop;     /* Number of airport tiles */
extern int NuclearPop;   /* Number of nuclear plant tiles */

/* External effects */
extern int RoadEffect;   /* Road maintenance effectiveness (a function of funding) */
extern int PoliceEffect; /* Police effectiveness */
extern int FireEffect;   /* Fire department effectiveness */
extern int TrafficAverage; /* Average traffic */
extern int PollutionAverage; /* Average pollution */
extern int CrimeAverage; /* Average crime */
extern int LVAverage;    /* Average land value */

/* Growth rates */
extern short RValve;     /* Residential development rate */
extern short CValve;     /* Commercial development rate */
extern short IValve;     /* Industrial development rate */
extern int ValveFlag;    /* Set to 1 when valves change */

/* Disasters */
extern short DisasterEvent; /* Current disaster type (0=none) - defined in scenarios.c */
extern short DisasterWait;  /* Countdown to next disaster - defined in scenarios.c */
extern int DisasterLevel;   /* Disaster level */

/* Core simulation functions */
void DoSimInit(void);
void SimFrame(void);
void Simulate(int mod16);
void DoTimeStuff(void);
void SetValves(int res, int com, int ind);
void ClearCensus(void);
void TakeCensus(void);
void MapScan(int x1, int x2, int y1, int y2);
int GetPValue(int x, int y);
int TestBounds(int x, int y);
void SetSimulationSpeed(HWND hwnd, int speed);
void CleanupSimTimer(HWND hwnd);

/* Functions implemented in zone.c */
void DoZone(int Xloc, int Yloc, int pos);
int calcResPop(int zone);   /* Calculate residential zone population */
int calcComPop(int zone);   /* Calculate commercial zone population */
int calcIndPop(int zone);   /* Calculate industrial zone population */

/* Power-related variables and functions - power.c */
extern int SMapX;             /* Current map X position for power scan */
extern int SMapY;             /* Current map Y position for power scan */
void CountPowerPlants(void);
void QueuePowerPlant(int x, int y);
void FindPowerPlants(void);
void DoPowerScan(void);

/* Traffic-related functions - traffic.c */
int MakeTraffic(int zoneType);
void DecTrafficMap(void);
void CalcTrafficAverage(void);
void RandomlySeedRand(void); /* Initialize random number generator */
int SimRandom(int range);  /* Random number function used by traffic system */

/* Scanner-related functions - scanner.c */
void FireAnalysis(void);    /* Fire station effect analysis */
void PopDenScan(void);      /* Population density scan */
void PTLScan(void);         /* Pollution/terrain/land value scan */
void CrimeScan(void);       /* Crime level scan */

/* Evaluation-related functions - evaluation.c */
void EvalInit(void);           /* Initialize evaluation system */
void CityEvaluation(void);     /* Perform city evaluation */
void CountSpecialTiles(void);  /* Count special building types */
const char* GetProblemText(int problemIndex);  /* Get problem description */
const char* GetCityClassName(void);            /* Get city class name */
void GetTopProblems(short problems[4]);        /* Get top problems list */
int GetProblemVotes(int problemIndex);         /* Get problem vote count */
QUAD GetCityAssessedValue(void);               /* Get city assessed value */
int IsEvaluationValid(void);                   /* Is evaluation data valid */
int GetAverageCityScore(void);                 /* Get average city score */

/* Budget-related variables and functions - budget.c */
extern float RoadPercent;      /* Road funding percentage (0.0-1.0) */
extern float PolicePercent;    /* Police funding percentage (0.0-1.0) */
extern float FirePercent;      /* Fire funding percentage (0.0-1.0) */
extern QUAD RoadFund;          /* Required road funding amount */
extern QUAD PoliceFund;        /* Required police funding amount */
extern QUAD FireFund;          /* Required fire funding amount */
extern QUAD RoadSpend;         /* Actual road spending */
extern QUAD PoliceSpend;       /* Actual police spending */
extern QUAD FireSpend;         /* Actual fire spending */
extern QUAD TaxFund;           /* Tax income for current year */
extern int AutoBudget;         /* Auto-budget enabled flag */

void InitBudget(void);         /* Initialize budget system */
void CollectTax(void);         /* Calculate and collect taxes */
void Spend(QUAD amount);       /* Spend funds (negative = income) */
void DoBudget(void);           /* Process budget allocation */
QUAD GetTaxIncome(void);       /* Get current tax income */
QUAD GetBudgetBalance(void);   /* Get current budget balance */
int GetRoadEffect(void);       /* Get road effectiveness */
int GetPoliceEffect(void);     /* Get police effectiveness */
int GetFireEffect(void);       /* Get fire department effectiveness */
void SetRoadPercent(float percent);      /* Set road funding percentage */
void SetPolicePercent(float percent);    /* Set police funding percentage */
void SetFirePercent(float percent);      /* Set fire department funding percentage */

/* Scenario functions (scenarios.c) */
int loadScenario(int scenarioId);        /* Load a scenario by ID */
void scenarioDisaster(void);             /* Process scenario disasters */

/* Disaster functions (disasters.c) */
void doEarthquake(void);                 /* Create an earthquake */
void makeFlood(void);                    /* Create a flood */
void makeFire(int x, int y);             /* Start a fire */
void spreadFire(void);                   /* Check for and spread fires */
void makeMonster(void);                  /* Create a monster */
void makeExplosion(int x, int y);        /* Create an explosion */
void makeMeltdown(void);                 /* Create a nuclear meltdown */

/* File I/O functions (main.c) */
int loadFile(char *filename);    /* Load city file data */

/* Animation functions (animation.c) */
void AnimateTiles(void);            /* Process animations for the entire map */
void SetAnimationEnabled(int enabled);  /* Enable or disable animations */
int GetAnimationEnabled(void);      /* Get animation enabled status */
void SetSmoke(int x, int y);        /* Set smoke animation for coal plants */
void UpdateFire(int x, int y);      /* Update fire animations */
void UpdateNuclearPower(int x, int y);  /* Update nuclear power plant animations */
void UpdateAirportRadar(int x, int y);  /* Update airport radar animation */
void UpdateSpecialAnimations(void); /* Update all special animations */

#endif /* _SIMULATION_H */