/****************************************************************************
 Module
   MotorCtrl.c

 Revision
   1.0.1

 Description
   This module implements the MotorCtrl service.It implements the simple service under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 11/12/25   karthi24    started conversion into final code
 11/11/25   karthi24    continued adding pseudocode for functionality
 11/10/25   karthi24    started conversion into BalloonCtrl.c 
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "EventCheckers.h"
#include "ES_Framework.h"
#include "DM_Display.h"
#include "GameSM.h"
#include "MotorCtrl.h"
#include "PIC32_AD_Lib.h"
#include "PIC32_SPI_HAL.h"
#include "PWM_PIC32.h"

/*----------------------------- Module Defines ----------------------------*/
// PBCLK = 20 MHz, /8 = 2.5 MHz -> 0.4 µs per tick
#define SERVO_US_TO_TICKS(us)   ((uint16_t)(((us) * 5u) / 2u))

// Balloon servo range (tunable parameter)
#define BALLOON_MIN_US    1000u
#define BALLOON_MAX_US    2000u

// Gear servo positions (tunable parameter)
#define GEAR_SERVO_REST_US      1500u
#define GEAR_SERVO_DISPENSE_US  2100u

// Servo channel mapping
#define GEAR_SERVO_CHANNEL  1u      // OC1 -> RPB15
#define B1_SERVO_CHANNEL    3u      // OC3 -> RPA3
#define B2_SERVO_CHANNEL    4u      // OC4 -> RPA4
#define B3_SERVO_CHANNEL    5u      // OC5 -> RPA2

// Gear servo positions & dwell (tunable parameter)
#define GEAR_SERVO_REST_US      1500u
#define GEAR_SERVO_DISPENSE_US  2100u
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

static void MotorHW_InitServos(void);

/* prototypes for public functions for this service.They should be functions
   relevant to the behavior of this service
*/
void MC_SetDifficultyPercent(uint8_t pct);
void MC_CommandRise(uint8_t idx);
void MC_RaiseAllToTop(void);
void MC_CommandFall(uint8_t idx);
/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static Axis_t Ax[3]; // each axis has one balloon

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMotorCtrl

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
bool InitMotorCtrl(uint8_t Priority)
{
    ES_Event_t e = { .EventType = ES_INIT };

    MyPriority = Priority;
   
    ES_Timer_InitTimer(TID_BALLOON_UPDATE, 500); // Tunable parameter for the update rate of the servos
    
    MotorHW_InitServos();
    
    // Init balloon axes (you?ll tune these values)
    for (uint8_t i = 0; i < 3; i++){
        Ax[i].pos_ticks     = 0;
        Ax[i].tgt_ticks     = 0;
        Ax[i].max_step      = 1;      // will be set from difficulty
        Ax[i].floor_ticks   = 0;      // TODO: set real floor : tunable parameter
        Ax[i].ceiling_ticks = 1000;   // TODO: set real ceiling : tunable parameter
    }

    GearState = GEAR_IDLE;
    /********************************************
    in here you write your initialization code
    *******************************************/
    // post the initial transition event
    return ES_PostToService(MyPriority, e); 
}

/****************************************************************************
 Function
     PostMotorCtrl

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
bool PostMotorCtrl(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunMotorCtrl

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
ES_Event_t RunMotorCtrl(ES_Event_t ThisEvent)
{
    ES_Event_t ret = { .EventType=ES_NO_EVENT };
    /********************************************
     in here you write your service code
     *******************************************/
    if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == TID_BALLOON_UPDATE){
        
        // Slew each axis toward target
        for (uint8_t i=0;i<3;i++){
            int32_t delta = Ax[i].tgt_ticks - Ax[i].pos_ticks;
            if (delta >  Ax[i].max_step) delta =  Ax[i].max_step;
            if (delta < -Ax[i].max_step) delta = -Ax[i].max_step;
            Ax[i].pos_ticks += delta;

            // Convert pos_ticks -> PWM us and write via PWM HAL
            // (use your PWM_PIC32 helpers)

            // Crash detect
            if (Ax[i].pos_ticks <= Ax[i].floor_ticks){
            ES_Event_t crash = { .EventType = ES_OBJECT_CRASHED };
            PostGameSM(crash);
            }
        }
        
        ES_Timer_InitTimer(TID_BALLOON_UPDATE, 300);
        return ret;
    }
// Handling gear dispensing state machine alone
    if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == TID_GEAR_SERVO){

        uint16_t restTicks     = SERVO_US_TO_TICKS(GEAR_SERVO_REST_US);
        uint16_t dispenseTicks = SERVO_US_TO_TICKS(GEAR_SERVO_DISPENSE_US);

        switch (GearState){

          case GEAR_GOING_TO_DISPENSE:
            // Move to DISPENSE and schedule return to REST
            PWMOperate_SetPulseWidthOnChannel(dispenseTicks, GEAR_SERVO_CHANNEL);
            GearState = GEAR_GOING_TO_REST;
            ES_Timer_InitTimer(TID_GEAR_SERVO, 300);   // 300ms dwell at DISPENSE
            break;

          case GEAR_GOING_TO_REST:
            // Move back to REST and finish
            PWMOperate_SetPulseWidthOnChannel(restTicks, GEAR_SERVO_CHANNEL);
            GearState = GEAR_IDLE;
            break;

          default:
            GearState = GEAR_IDLE;
            break;
        }

        return ret;
    }

    return ret;
}

/***************************************************************************
 public functions
 ***************************************************************************/
void MC_SetDifficultyPercent(uint8_t pct){
    // map 0?100% ? max ticks per 20 ms (speed profile)
    // e.g., Ax[i].max_step = map(pct, 0,100, slow_step, fast_step);
    // Tunable speed range (ticks per 20 ms frame)
    // You will later choose pos_ticks units so that these feel reasonable.
    const int32_t MIN_STEP_TICKS = 5;    // very easy / slow motion tunable parameter
    const int32_t MAX_STEP_TICKS = 80;   // very hard / fast motion tunable parameter

    // Map 1?100% linearly into [MIN_STEP_TICKS .. MAX_STEP_TICKS]
    // Using 99 in the denominator so that pct=1 ? MIN, pct=100 ? MAX exactly.
    int32_t step = MIN_STEP_TICKS +
                   ((int32_t)(pct - 1) * (MAX_STEP_TICKS - MIN_STEP_TICKS)) / 99;

    // Apply to all 3 balloon axes
    for (uint8_t i = 0; i < 3; i++){
        Ax[i].max_step = step;
    }
}

void MC_DispenseTwoGearsOnce(void){
    if (GearState != GEAR_IDLE){
        // already running a sweep; ignore
        return;
    }

    uint16_t restTicks = SERVO_US_TO_TICKS(GEAR_SERVO_REST_US);

    // Start from REST
    PWMOperate_SetPulseWidthOnChannel(restTicks, GEAR_SERVO_CHANNEL);

    // First transition will move to DISPENSE
    GearState = GEAR_GOING_TO_DISPENSE;
    ES_Timer_InitTimer(TID_GEAR_SERVO, 300);   // 300ms before first move
}

void MC_CommandRise(uint8_t idx){ Ax[idx-1].tgt_ticks = Ax[idx-1].ceiling_ticks; }

void MC_CommandFall(uint8_t idx){ Ax[idx-1].tgt_ticks = Ax[idx-1].floor_ticks;}

void MC_RaiseAllToTop(void){ for(uint8_t i=1;i<=3;i++) MC_CommandRise(i);}
/***************************************************************************
 private functions
 ***************************************************************************/
static void MotorHW_InitServos(void){
    // We will use channels 1, 3, 4, 5 (OC1, OC3, OC4, OC5)
    PWMSetup_BasicConfig(5);   // init all 5 channels in the library

    // All servos share the same 50 Hz timebase on Timer3
    PWMSetup_AssignChannelToTimer(1, _Timer3_);  // gear:   OC1
    // Channel 2 / OC2 unused
    PWMSetup_AssignChannelToTimer(3, _Timer3_);  // balloon 1: OC3
    PWMSetup_AssignChannelToTimer(4, _Timer3_);  // balloon 2: OC4
    PWMSetup_AssignChannelToTimer(5, _Timer3_);  // balloon 3: OC5

    // Map channels to your chosen physical pins:
    //   OC1 -> RPB15  (gear)
    //   OC3 -> RPA3   (balloon 1)
    //   OC4 -> RPA4   (balloon 2)
    //   OC5 -> RPA2   (balloon 3)
    PWMSetup_MapChannelToOutputPin(1, PWM_RPB15);  // gear
    PWMSetup_MapChannelToOutputPin(3, PWM_RPA3);   // B1
    PWMSetup_MapChannelToOutputPin(4, PWM_RPA4);   // B2
    PWMSetup_MapChannelToOutputPin(5, PWM_RPA2);   // B3
}
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

