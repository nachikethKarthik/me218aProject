/****************************************************************************
 Module
     EventCheckers.h
 Description
     header file for the event checking functions
 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 10/18/15 11:50 jec      added #include for stdint & stdbool
 08/06/13 14:37 jec      started coding
*****************************************************************************/

#ifndef EventCheckers_H
#define EventCheckers_H

// the common headers for C99 types
//#include <stdint.h>
#include <stdbool.h>
#include "PIC32_AD_Lib.h"
// prototypes for event checkers

bool Check4Keystroke(void);

bool Check4HandWave(void);
bool Check4Difficulty(void);
bool Check4LaserHits(void);


void Targets_SetBaselines(uint16_t b12, uint16_t b5, uint16_t b4);

#endif /* EventCheckers_H */
