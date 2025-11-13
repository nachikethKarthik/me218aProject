/****************************************************************************

  Header file for Game State Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef GameSM_H
#define GameSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// ======= PIN MAP:(PIC32MX170F256B) =======

// --- Beam-break --- // idle = 1 (beam unbroken), active = 0 (beam broken)
#define BEAM_BREAK_TRIS      TRISBbits.TRISB8
#define BEAM_BREAK_PORT      PORTBbits.RB8
#define BEAM_BREAK_CNPU      CNPUBbits.CNPUB8   // enable internal pull-up for clean idle-high


// --- Slider (difficulty) on AN11 / RPB13 ---
#define SLIDER_TRIS          TRISBbits.TRISB13
#define SLIDER_ANSEL         ANSELBbits.ANSB13

// --- ALS-PT19 sensors: B1=AN12/RPB12, B2=AN5/RPB3, B3=AN4/RPB2 ---
#define ALS1_TRIS            TRISBbits.TRISB12
#define ALS1_ANSEL           ANSELBbits.ANSB12

#define ALS2_TRIS            TRISBbits.TRISB3
#define ALS2_ANSEL           ANSELBbits.ANSB3

#define ALS3_TRIS            TRISBbits.TRISB2
#define ALS3_ANSEL           ANSELBbits.ANSB2

#define ADC_CHANSET ( (1<<4) /*AN4*/ | (1<<5) /*AN5*/ | (1<<11) /*AN11*/ | (1<<12) /*AN12*/ )

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  GS_InitPState, GS_Welcome, GS_WaitingForHandWave, GS_Gameplay,
  GS_CompletingMode, GS_LosingMode, GS_NoUserInput
}GameState_t;

// Public Function Prototypes

bool InitGameSM(uint8_t Priority);
bool PostGameSM(ES_Event_t ThisEvent);
ES_Event_t RunGameSM(ES_Event_t ThisEvent);
GameState_t QueryGameSM(void);

#endif /* GameSM_H */

