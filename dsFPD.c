#include <stdio.h>
#include <string.h>
#include "dsFPD.h"

dsError_t dsFPInit(void)
{
    return dsERR_NONE;
}

dsError_t dsFPTerm(void)
{
    return dsERR_NONE;
}

dsError_t dsSetFPBlink (dsFPDIndicator_t eIndicator, unsigned int uBlinkDuration, unsigned int uBlinkIterations)
{
    return dsERR_OPERATION_NOT_SUPPORTED;
}

dsError_t dsSetFPBrightness (dsFPDIndicator_t eIndicator, dsFPDBrightness_t eBrightness)
{
    return dsERR_NONE;
}

dsError_t dsGetFPBrightness (dsFPDIndicator_t eIndicator, dsFPDBrightness_t *pBrightness)
{
    return dsERR_NONE;
}

dsError_t dsSetFPColor (dsFPDIndicator_t eIndicator, dsFPDColor_t eColor)
{
    return dsERR_OPERATION_NOT_SUPPORTED;
}

dsError_t dsSetFPTime (dsFPDTimeFormat_t eTimeFormat, const unsigned int uHour, const unsigned int uMinutes)
{
    return dsERR_NONE;
}

dsError_t dsSetFPText(const char* pText)
{
    return dsERR_NONE;
}

dsError_t dsSetFPScroll (unsigned int uScrollHoldOnDur, unsigned int uHorzScrollIterations, unsigned int uVertScrollIterations)
{
    return dsERR_NONE;
}


dsError_t dsSetFPTextBrightness (dsFPDTextDisplay_t eIndicator, dsFPDBrightness_t eBrightness)
{
    return dsERR_NONE;
}
dsError_t dsGetFPTextBrightnes  (dsFPDTextDisplay_t eIndicator, dsFPDBrightness_t *pBrightness)
{
    return dsERR_NONE;
}

dsError_t dsFPEnableCLockDisplay (int enable)
{
    return dsERR_NONE;
}

dsError_t dsSetFPState (dsFPDIndicator_t eIndicator, dsFPDState_t state)
{
    return dsERR_NONE;
}
dsError_t dsGetFPTextBrightness (dsFPDTextDisplay_t eIndicator, dsFPDBrightness_t *pBrightness)
{
    return dsERR_NONE;
}

dsError_t dsSetFPDBrightness(dsFPDIndicator_t eIndicator, dsFPDBrightness_t eBrightness,bool toPersist)
{
    return dsERR_GENERAL;
}
dsError_t dsSetFPDColor (dsFPDIndicator_t eIndicator, dsFPDColor_t eColor,bool toPersist)
{
    return dsERR_GENERAL;
}
dsError_t dsGetFPColor (dsFPDIndicator_t eIndicator, dsFPDColor_t *pColor)
{
    return dsERR_GENERAL;
}

