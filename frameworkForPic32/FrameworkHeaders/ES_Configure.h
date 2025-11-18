/****************************************************************************
 Module
     ES_Configure.h
 Description
     This file contains macro definitions that are edited by the user to
     adapt the Events and Services framework to a particular application.
 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 11/10/25   karthi24     began modification for me218a project
 12/19/16 20:19  jec     removed EVENT_CHECK_HEADER definition. This goes with
                         the V2.3 move to a single wrapper for event checking
                         headers
  10/11/15 18:00 jec     added new event type ES_SHORT_TIMEOUT
  10/21/13 20:54 jec     lots of added entries to bring the number of timers
                         and services up to 16 each
 08/06/13 14:10 jec      removed PostKeyFunc stuff since we are moving that
                         functionality out of the framework and putting it
                         explicitly into the event checking functions
 01/15/12 10:03 jec      started coding
*****************************************************************************/

#ifndef ES_CONFIGURE_H
#define ES_CONFIGURE_H

/****************************************************************************/
// The maximum number of services sets an upper bound on the number of
// services that the framework will handle. Reasonable values are 8 and 16
// corresponding to an 8-bit(uint8_t) and 16-bit(uint16_t) Ready variable size

#define MAX_NUM_SERVICES 16

/****************************************************************************/
// This macro determines that number of services that are *actually* used in
// a particular application. It will vary in value from 1 to MAX_NUM_SERVICES

// Service 0: TestHarness service
// Service 0: GameSM
// Service 1: MotorCtrl

#define NUM_SERVICES 4

/****************************************************************************/
// These are the definitions for Service 0, the lowest priority service.
// Every Events and Services application must have a Service 0. Further
// services are added in numeric sequence (1,2,3,...) with increasing
// priorities
// the header file with the public function prototypes
#define SERV_0_HEADER "TestHarnessService0.h"
// the name of the Init function
#define SERV_0_INIT InitTestHarnessService0
// the name of the run function
#define SERV_0_RUN RunTestHarnessService0
// How big should this services Queue be?
#define SERV_0_QUEUE_SIZE 3

/****************************************************************************/
// The following sections are used to define the parameters for each of the
// services. You only need to fill out as many as the number of services
// defined by NUM_SERVICES
/****************************************************************************/
// These are the definitions for Service 1
#if NUM_SERVICES > 1
    #define SERV_1_HEADER      "GameSM.h"
    #define SERV_1_INIT        InitGameSM
    #define SERV_1_RUN         RunGameSM
    #define SERV_1_QUEUE_SIZE  5
#endif

/****************************************************************************/
// These are the definitions for Service 2
#if NUM_SERVICES > 2

    #define SERV_2_HEADER "MotorCtrl.h"
    #define SERV_2_INIT   InitMotorCtrl
    #define SERV_2_RUN    RunMotorCtrl
    #define SERV_2_QUEUE_SIZE 5

#endif

/****************************************************************************/
// These are the definitions for Service 3
#if NUM_SERVICES > 3

#define SERV_3_HEADER      "LEDService.h"
#define SERV_3_INIT        InitLEDService
#define SERV_3_RUN         RunLEDService
#define SERV_3_QUEUE_SIZE  5

#endif

/****************************************************************************/
// These are the definitions for Service 4
#if NUM_SERVICES > 4
// the header file with the public function prototypes
#define SERV_4_HEADER "TestHarnessService3.h"
// the name of the Init function
#define SERV_4_INIT InitTestHarnessService3
// the name of the run function
#define SERV_4_RUN RunTestHarnessService3
// How big should this services Queue be?
#define SERV_4_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 5
#if NUM_SERVICES > 5
// the header file with the public function prototypes
#define SERV_5_HEADER "TestHarnessService5.h"
// the name of the Init function
#define SERV_5_INIT InitTestHarnessService5
// the name of the run function
#define SERV_5_RUN RunTestHarnessService5
// How big should this services Queue be?
#define SERV_5_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 6
#if NUM_SERVICES > 6
// the header file with the public function prototypes
#define SERV_6_HEADER "TestHarnessService6.h"
// the name of the Init function
#define SERV_6_INIT InitTestHarnessService6
// the name of the run function
#define SERV_6_RUN RunTestHarnessService6
// How big should this services Queue be?
#define SERV_6_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 7
#if NUM_SERVICES > 7
// the header file with the public function prototypes
#define SERV_7_HEADER "TestHarnessService7.h"
// the name of the Init function
#define SERV_7_INIT InitTestHarnessService7
// the name of the run function
#define SERV_7_RUN RunTestHarnessService7
// How big should this services Queue be?
#define SERV_7_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 8
#if NUM_SERVICES > 8
// the header file with the public function prototypes
#define SERV_8_HEADER "TestHarnessService8.h"
// the name of the Init function
#define SERV_8_INIT InitTestHarnessService8
// the name of the run function
#define SERV_8_RUN RunTestHarnessService8
// How big should this services Queue be?
#define SERV_8_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 9
#if NUM_SERVICES > 9
// the header file with the public function prototypes
#define SERV_9_HEADER "TestHarnessService9.h"
// the name of the Init function
#define SERV_9_INIT InitTestHarnessService9
// the name of the run function
#define SERV_9_RUN RunTestHarnessService9
// How big should this services Queue be?
#define SERV_9_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 10
#if NUM_SERVICES > 10
// the header file with the public function prototypes
#define SERV_10_HEADER "TestHarnessService10.h"
// the name of the Init function
#define SERV_10_INIT InitTestHarnessService10
// the name of the run function
#define SERV_10_RUN RunTestHarnessService10
// How big should this services Queue be?
#define SERV_10_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 11
#if NUM_SERVICES > 11
// the header file with the public function prototypes
#define SERV_11_HEADER "TestHarnessService11.h"
// the name of the Init function
#define SERV_11_INIT InitTestHarnessService11
// the name of the run function
#define SERV_11_RUN RunTestHarnessService11
// How big should this services Queue be?
#define SERV_11_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 12
#if NUM_SERVICES > 12
// the header file with the public function prototypes
#define SERV_12_HEADER "TestHarnessService12.h"
// the name of the Init function
#define SERV_12_INIT InitTestHarnessService12
// the name of the run function
#define SERV_12_RUN RunTestHarnessService12
// How big should this services Queue be?
#define SERV_12_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 13
#if NUM_SERVICES > 13
// the header file with the public function prototypes
#define SERV_13_HEADER "TestHarnessService13.h"
// the name of the Init function
#define SERV_13_INIT InitTestHarnessService13
// the name of the run function
#define SERV_13_RUN RunTestHarnessService13
// How big should this services Queue be?
#define SERV_13_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 14
#if NUM_SERVICES > 14
// the header file with the public function prototypes
#define SERV_14_HEADER "TestHarnessService14.h"
// the name of the Init function
#define SERV_14_INIT InitTestHarnessService14
// the name of the run function
#define SERV_14_RUN RunTestHarnessService14
// How big should this services Queue be?
#define SERV_14_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 15
#if NUM_SERVICES > 15
// the header file with the public function prototypes
#define SERV_15_HEADER "TestHarnessService15.h"
// the name of the Init function
#define SERV_15_INIT InitTestHarnessService15
// the name of the run function
#define SERV_15_RUN RunTestHarnessService15
// How big should this services Queue be?
#define SERV_15_QUEUE_SIZE 3
#endif

/****************************************************************************/
// Name/define the events of interest
// Universal events occupy the lowest entries, followed by user-defined events
typedef enum
{
    ES_NO_EVENT = 0,
    ES_ERROR,                 /* framework error */
    ES_INIT,                  /* pseudo-state init */
    ES_TIMEOUT,               /* timer expired */
    ES_SHORT_TIMEOUT,         /* short timer expired */

    /* User-defined events */
    ES_NEW_KEY,               // 5  from UART test harness
    ES_HAND_WAVE_DETECTED,    // 6  beam-break -> start game
    ES_DIFFICULTY_CHANGED,    // 7  param: 0?100 %
    DIRECT_HIT_B1,            // 8
    DIRECT_HIT_B2,            // 9
    DIRECT_HIT_B3,            // 10
    NO_HIT_B1,                // 11
    NO_HIT_B2,                // 12
    NO_HIT_B3,                // 13
    ES_OBJECT_CRASHED,        // 14 any balloon hit floor
    
    ES_LED_SHOW_MESSAGE,      // 15 EventParam: LED_MessageID_t
    ES_LED_SHOW_SCORE,        // 16 EventParam: score (uint16_t)
    ES_LED_SHOW_COUNTDOWN,    // 17 EventParam: seconds (0?60)
    ES_LED_SHOW_DIFFICULTY,   // 18 EventParam: 0?100 %
    ES_LED_PUSH_STEP          // 19 Internal LED row-push
} ES_EventType_t;

/****************************************************************************/
// These are the definitions for the Distribution lists. Each definition
// should be a comma separated list of post functions to indicate which
// services are on that distribution list.
#define NUM_DIST_LISTS 0
#if NUM_DIST_LISTS > 0
#define DIST_LIST0 PostTestHarnessService0, PostTestHarnessService0
#endif
#if NUM_DIST_LISTS > 1
#define DIST_LIST1 PostTestHarnessService1, PostTestHarnessService1
#endif
#if NUM_DIST_LISTS > 2
#define DIST_LIST2 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 3
#define DIST_LIST3 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 4
#define DIST_LIST4 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 5
#define DIST_LIST5 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 6
#define DIST_LIST6 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 7
#define DIST_LIST7 PostTemplateFSM
#endif

/****************************************************************************/
// This is the list of event checking functions
#define EVENT_CHECK_LIST Check4LaserHits, Check4HandWave,Check4Difficulty, Check4Keystroke  

/****************************************************************************/
// These are the definitions for the post functions to be executed when the
// corresponding timer expires. All 16 must be defined. If you are not using
// a timer, then you should use TIMER_UNUSED
// Unlike services, any combination of timers may be used and there is no
// priority in servicing them
#define TIMER_UNUSED ((pPostFunc)0)
#define TIMER0_RESP_FUNC PostGameSM   // 60s gameplay
#define TIMER1_RESP_FUNC PostGameSM   // 20s inactivity
#define TIMER2_RESP_FUNC PostGameSM   // 1s tick
#define TIMER3_RESP_FUNC PostGameSM   // 3s mode end
#define TIMER4_RESP_FUNC PostMotorCtrl // balloon update tick
#define TIMER5_RESP_FUNC PostMotorCtrl // gear servo dwell
#define TIMER6_RESP_FUNC TIMER_UNUSED 
#define TIMER7_RESP_FUNC TIMER_UNUSED
#define TIMER8_RESP_FUNC TIMER_UNUSED
#define TIMER9_RESP_FUNC TIMER_UNUSED
#define TIMER10_RESP_FUNC TIMER_UNUSED
#define TIMER11_RESP_FUNC TIMER_UNUSED
#define TIMER12_RESP_FUNC TIMER_UNUSED
#define TIMER13_RESP_FUNC TIMER_UNUSED
#define TIMER14_RESP_FUNC TIMER_UNUSED
#define TIMER15_RESP_FUNC PostTestHarnessService0

/****************************************************************************/
// Give the timer numbers symbolc names to make it easier to move them
// to different timers if the need arises. Keep these definitions close to the
// definitions for the response functions to make it easier to check that
// the timer number matches where the timer event will be routed
// These symbolic names should be changed to be relevant to your application


// === Symbolic timer IDs ===
#define TID_GAME_60S        0   // TIMER0
#define TID_INACTIVITY_20S  1   // TIMER1
#define TID_TICK_1S         2   // TIMER2
#define TID_MODE_3S         3   // TIMER3
#define TID_BALLOON_UPDATE  4   // TIMER4
#define TID_GEAR_SERVO      5   // TIMER5
#define SERVICE0_TIMER 15

#endif /* ES_CONFIGURE_H */
