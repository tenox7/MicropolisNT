/* budget.c - Budget management system for MicropolisNT
 * Based on original Micropolis code from MicropolisLegacy project
 */

#include "simulation.h"

/* Budget values */
float RoadPercent = 1.0;    /* Road funding percentage (0.0-1.0) */
float PolicePercent = 1.0;  /* Police funding percentage (0.0-1.0) */
float FirePercent = 1.0;    /* Fire funding percentage (0.0-1.0) */

QUAD RoadFund = 0;      /* Required road funding amount */
QUAD PoliceFund = 0;    /* Required police funding amount */
QUAD FireFund = 0;      /* Required fire funding amount */

QUAD RoadSpend = 0;     /* Actual road spending */
QUAD PoliceSpend = 0;   /* Actual police spending */
QUAD FireSpend = 0;     /* Actual fire spending */

QUAD TaxFund = 0;       /* Tax income for current year */
int AutoBudget = 1;     /* Auto-budget enabled flag */

/* Budget initialization */
void InitBudget(void)
{
    /* Set initial percentages to 100% */
    FirePercent = 1.0;
    PolicePercent = 1.0;
    RoadPercent = 1.0;
    
    /* Default to auto-budget */
    AutoBudget = 1;
    
    /* Reset all spending values */
    RoadFund = 0;
    PoliceFund = 0;
    FireFund = 0;
    RoadSpend = 0;
    PoliceSpend = 0;
    FireSpend = 0;
    TaxFund = 0;
}

/* Tax collection function - called yearly */
void CollectTax(void)
{
    int z;
    int taxable;
    float r;
    QUAD income;
    
    /* No income initially */
    TaxFund = 0;
    
    /* Calculate taxable amount */
    taxable = (ResPop + ComPop + IndPop) / 3;
    if (taxable > 0) {
        /* Apply difficulty level factor to taxes */
        if (GameLevel == 0) {
            r = 1.4f;     /* Easy level - more tax income */
        } else if (GameLevel == 1) {
            r = 1.2f;     /* Medium level */
        } else {
            r = 0.8f;     /* Hard level - less tax income */
        }
        
        /* Calculate tax income based on tax rate and population */
        income = (QUAD)(taxable * TaxRate * r);
        TaxFund = income;
        
        /* Add funds to treasury */
        Spend(-TaxFund);
    }
    
    /* Calculate funding requirements */
    RoadFund = RoadTotal * 4;    /* $4 per road tile */
    FireFund = FirePop * 100;    /* $100 per fire station */
    PoliceFund = PolicePop * 100; /* $100 per police station */
    
    /* Update budget to allocate available funds */
    DoBudget();
}

/* Spend money (negative means income) */
void Spend(QUAD amount)
{
    /* Add to treasury - negative values increase funds */
    TotalFunds -= amount;
    
    /* Ensure funds never go below zero */
    if (TotalFunds < 0) {
        TotalFunds = 0;
    }
}

/* Auto-budget processing */
void DoBudget(void)
{
    QUAD total;
    QUAD yumDuckets;
    QUAD fireInt;
    QUAD policeInt;
    QUAD roadInt;
    
    /* Calculate desired allocation based on percentages */
    fireInt = (QUAD)(((float)FireFund) * FirePercent);
    policeInt = (QUAD)(((float)PoliceFund) * PolicePercent);
    roadInt = (QUAD)(((float)RoadFund) * RoadPercent);
    
    total = fireInt + policeInt + roadInt;
    yumDuckets = TaxFund + TotalFunds;
    
    /* If we have enough money for full funding */
    if (yumDuckets >= total) {
        FireSpend = fireInt;
        PoliceSpend = policeInt;
        RoadSpend = roadInt;
    } 
    /* If we don't have enough money, allocate in priority order */
    else if (total > 0) {
        /* Try to fund roads first */
        if (yumDuckets > roadInt) {
            RoadSpend = roadInt;
            yumDuckets -= roadInt;
            
            /* Then try to fund fire protection */
            if (yumDuckets > fireInt) {
                FireSpend = fireInt;
                yumDuckets -= fireInt;
                
                /* Finally fund police if money remains */
                if (yumDuckets > policeInt) {
                    PoliceSpend = policeInt;
                    yumDuckets -= policeInt;
                } else {
                    /* Partial police funding */
                    PoliceSpend = yumDuckets;
                    if (yumDuckets > 0) {
                        PolicePercent = ((float)yumDuckets) / ((float)PoliceFund);
                    } else {
                        PolicePercent = 0.0;
                    }
                }
            } else {
                /* Partial fire funding */
                FireSpend = yumDuckets;
                PoliceSpend = 0;
                PolicePercent = 0.0;
                if (yumDuckets > 0) {
                    FirePercent = ((float)yumDuckets) / ((float)FireFund);
                } else {
                    FirePercent = 0.0;
                }
            }
        } else {
            /* Partial road funding */
            RoadSpend = yumDuckets;
            if (yumDuckets > 0) {
                RoadPercent = ((float)yumDuckets) / ((float)RoadFund);
            } else {
                RoadPercent = 0.0;
            }
            
            FireSpend = 0;
            PoliceSpend = 0;
            FirePercent = 0.0;
            PolicePercent = 0.0;
        }
    } else {
        /* No required funding */
        FireSpend = 0;
        PoliceSpend = 0;
        RoadSpend = 0;
        FirePercent = 1.0;
        PolicePercent = 1.0;
        RoadPercent = 1.0;
    }
    
    /* Calculate effective rates */
    fireInt = FireFund > 0 ? FireSpend * 100 / FireFund : 100;
    policeInt = PoliceFund > 0 ? PoliceSpend * 100 / PoliceFund : 100;
    roadInt = RoadFund > 0 ? RoadSpend * 100 / RoadFund : 100;
    
    /* Apply effect based on funding level */
    if (FireEffect != (int)fireInt) {
        FireEffect = (int)fireInt;
        /* Effect on fire coverage */
    }
    
    if (PoliceEffect != (int)policeInt) {
        PoliceEffect = (int)policeInt;
        /* Effect on police coverage */
    }
    
    if (RoadEffect != (int)roadInt) {
        RoadEffect = (int)roadInt;
        /* Effect on traffic and roads */
    }
    
    /* Spend budget money */
    total = FireSpend + PoliceSpend + RoadSpend;
    Spend(total);
}

/* Gets current tax income */
QUAD GetTaxIncome(void) 
{
    return TaxFund;
}

/* Gets current budget balance (after spending) */
QUAD GetBudgetBalance(void) 
{
    QUAD total = FireSpend + PoliceSpend + RoadSpend;
    return TaxFund - total;
}

/* Returns the effective road maintenance percentage */
int GetRoadEffect(void) 
{
    return RoadEffect;
}

/* Returns the effective police funding percentage */
int GetPoliceEffect(void) 
{
    return PoliceEffect;
}

/* Returns the effective fire department funding percentage */
int GetFireEffect(void) 
{
    return FireEffect;
}

/* Sets road funding percentage */
void SetRoadPercent(float percent) 
{
    if (percent < 0.0f) {
        percent = 0.0f;
    } else if (percent > 1.0f) {
        percent = 1.0f;
    }
    
    RoadPercent = percent;
    DoBudget();
}

/* Sets police funding percentage */
void SetPolicePercent(float percent) 
{
    if (percent < 0.0f) {
        percent = 0.0f;
    } else if (percent > 1.0f) {
        percent = 1.0f;
    }
    
    PolicePercent = percent;
    DoBudget();
}

/* Sets fire department funding percentage */
void SetFirePercent(float percent) 
{
    if (percent < 0.0f) {
        percent = 0.0f;
    } else if (percent > 1.0f) {
        percent = 1.0f;
    }
    
    FirePercent = percent;
    DoBudget();
}