#ifndef __DSCONFIG_H
#define __DSCONFIG_H
#include "dsError.h"

typedef dsError_t (*port_allocation_fp)(size_t);
typedef dsError_t (*port_initialization_fp)(size_t index,char* propName,char* Values);
char* getProperty(char* prop,size_t index,char* portType);
char* dsGetValue(char* property);
char* dsGetValidStringFrmCfg(FILE* fptr);
size_t dsGetIndexFrmCfg(char* indexString);
char* dsGetPropertyFrmCfg(char* prop,size_t index,char* portType);
dsError_t dsReadCfgFile(size_t index,char* portString,port_initialization_fp init);

#endif

