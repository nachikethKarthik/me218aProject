/****************************************************************************
 Module
   BalloonCtrl.c

 Revision
   1.0.1

 Description
   This module implements the BalloonCtrl service.It implements the simple service under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 11/10/25   karthi24    started conversion into BalloonCtrl.c 
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "BalloonCtrl.h"

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/* prototypes for public functions for this service.They should be functions
   relevant to the behavior of this service
*/
void BC_SetDifficultyPercent(uint8_t pct);
void BC_CommandRise(uint8_t idx);
void BC_RaiseAllToTop(void);
void BC_CommandFall(uint8_t idx);
/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static Axis_t Ax[3]; // each axis has one balloon

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitBalloonCtrl

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 01/16/12, 10:00
****************************************************************************/
bool InitBalloonCtrl(uint8_t Priority)
{
    ES_Event_t e = { .EventType = ES_INIT };

    MyPriority = Priority;
    // map servo channels (OC3/OC4/OC5) and set PWM@50Hz via your PWM HAL
    
    ES_Timer_InitTimer(TID_BALLOON_20MS, 20);
    /********************************************
    in here you write your initialization code
    *******************************************/
    // post the initial transition event
    return ES_PostToService(MyPriority, e); 
}

/****************************************************************************
 Function
     PostBalloonCtrl

 Parameters
     EF_Event_t ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostBalloonCtrl(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunBalloonCtrl

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunBalloonCtrl(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  /********************************************
   in here you write your service code
   *******************************************/
  return ReturnEvent;
}

/***************************************************************************
 public functions
 ***************************************************************************/
void BC_SetDifficultyPercent(uint8_t pct){
  // map 0?100% ? max ticks per 20 ms (speed profile)
  // e.g., Ax[i].max_step = map(pct, 0,100, slow_step, fast_step);
}
void BC_CommandRise(uint8_t idx){ Ax[idx-1].tgt_ticks = Ax[idx-1].ceiling_ticks; }
void BC_CommandFall(uint8_t idx){ Ax[idx-1].tgt_ticks = Ax[idx-1].floor_ticks;   }
void BC_RaiseAllToTop(void){ for(uint8_t i=1;i<=3;i++) BC_CommandRise(i); }
/***************************************************************************
 private functions
 ***************************************************************************/

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

