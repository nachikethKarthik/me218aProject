/****************************************************************************

  Header file for MotorCtrl service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef MotorCtrl_H
#define MotorCtrl_H

#include "ES_Types.h"


// Servo channel mapping
#define GEAR_SERVO_CHANNEL  1u      // OC1 -> RPB15 ->
#define B1_SERVO_CHANNEL    3u      // OC3 -> RPA3 -> pin 10
#define B2_SERVO_CHANNEL    4u      // OC4 -> RPA4 -> pin 
#define B3_SERVO_CHANNEL    5u      // OC5 -> RPA2

//typedefs
typedef struct {
  int32_t  pos_ticks;       // current estimated position (ticks or degrees*10)
  int32_t  tgt_ticks;       // commanded
  int32_t  max_step;        // ticks per 20 ms, from difficulty
  int32_t  floor_ticks;     // crash threshold
  int32_t  ceiling_ticks;   // top limit
} Axis_t;


// Public Function Prototypes
void MC_SetDifficultyPercent(uint8_t pct); // sets per-axis max_step
void MC_CommandRise(uint8_t idx);
void MC_CommandFall(uint8_t idx);
void MC_RaiseAllToTop(void);
void MC_DispenseTwoGearsOnce(void);
void MC_DebugPrintAxes(void);
uint8_t MC_CountBalloonsAboveDangerline(void);

bool InitMotorCtrl(uint8_t Priority);
bool PostMotorCtrl(ES_Event_t ThisEvent);
ES_Event_t RunMotorCtrl(ES_Event_t ThisEvent);

#endif /* MotorCtrl_H */

