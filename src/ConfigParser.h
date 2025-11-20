#ifndef CONFIG_PARSER_H_
#define CONFIG_PARSER_H_

#include "comm/Common.h"
#include "modules/Module.h"

//////////////
// DEFINES  //
//////////////

/////////////
//  TYPES  //
/////////////

typedef struct {
    char inPort[32];
    char otherModule[32];
    char otherOutPort[32];
} ConnectionInfo;

typedef struct {
    char controlName[32];
    float value;
} ControlInfo;

typedef struct {
    char name[32];
    char type[32];
    
    int connectionCount;
    ConnectionInfo connections[64];

    int controlCount;
    ControlInfo controls[64];
} ModuleConfig;

typedef struct {
    int moduleCount;
    ModuleConfig modules[MAX_RACK_SIZE];
} FullConfig;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

bool ConfigParser_Parse(FullConfig* cfg, char* fname);
void ConfigParser_Write(FullConfig* cfg, char* fname);
void ConfigParser_Print(FullConfig* cfg);

#endif