/****************************************************************************

  Header file for MotorCtrl service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef MotorCtrl_H
#define MotorCtrl_H

#include "ES_Types.h"

//typedefs
typedef struct {
  int32_t  pos_ticks;       // current estimated position (ticks or degrees*10)
  int32_t  tgt_ticks;       // commanded
  int32_t  max_step;        // ticks per 20 ms, from difficulty
  int32_t  floor_ticks;     // crash threshold
  int32_t  ceiling_ticks;   // top limit
} Axis_t;

// State machine used to handle gear dispensing. typedef below has the states for it
typedef enum {
  GEAR_IDLE,
  GEAR_GOING_TO_DISPENSE,
  GEAR_GOING_TO_REST
} GearDispenseState_t;

static GearDispenseState_t GearState = GEAR_IDLE;

// Public Function Prototypes
void MC_SetDifficultyPercent(uint8_t pct);         // sets per-axis max_step
void MC_CommandRise(uint8_t idx);
void MC_CommandFall(uint8_t idx);
void MC_RaiseAllToTop(void);
void MC_DispenseTwoGearsOnce(void);

bool InitMotorCtrl(uint8_t Priority);
bool PostMotorCtrl(ES_Event_t ThisEvent);
ES_Event_t RunMotorCtrl(ES_Event_t ThisEvent);

#endif /* MotorCtrl_H */

