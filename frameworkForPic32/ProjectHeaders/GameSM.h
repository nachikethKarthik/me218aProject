/****************************************************************************

  Header file for Game State Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef GameSM_H
#define GameSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

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


void LED_ShowWelcome(void);
void LED_ShowCountdown(uint8_t seconds_remaining);
void SetAllBalloonsToTop(void);
void CommandBalloonRise(uint8_t idx);     // talk to BalloonCtrl
void CommandBalloonFall(uint8_t idx);
void DispenseTwoGearsOnce(void);          // uses GearDispenser helper

#endif /* GameSM_H */

