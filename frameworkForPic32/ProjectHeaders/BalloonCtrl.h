/****************************************************************************

  Header file for BalloonCtrl service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef BalloonCtrl_H
#define BalloonCtrl_H

#include "ES_Types.h"

//typedefs
typedef struct {
  int32_t  pos_ticks;       // current estimated position (ticks or degrees*10)
  int32_t  tgt_ticks;       // commanded
  int32_t  max_step;        // ticks per 20 ms, from difficulty
  int32_t  floor_ticks;     // crash threshold
  int32_t  ceiling_ticks;   // top limit
} Axis_t;

// Public Function Prototypes
void BC_SetDifficultyPercent(uint8_t pct);         // sets per-axis max_step
void BC_CommandRise(uint8_t idx);
void BC_CommandFall(uint8_t idx);
void BC_RaiseAllToTop(void);

bool InitBalloonCtrl(uint8_t Priority);
bool PostBalloonCtrl(ES_Event_t ThisEvent);
ES_Event_t RunBalloonCtrl(ES_Event_t ThisEvent);

#endif /* BalloonCtrl_H */

