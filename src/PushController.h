#ifndef PUSH_CONTROLLER_H_
#define PUSH_CONTROLLER_H_


///////////////
//  DEFINES  //
///////////////

#define PUSH_CONTROLLER_NAME "Controller"
#define SYNTH_NAME "Synth"

///////////
// TYPES //
///////////

#define TESTING_PKT 123
typedef struct TestingPacket
{
    int test;
} TestingPacket;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

void PushController_run(void);

#endif