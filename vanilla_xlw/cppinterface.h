//
//
//                                                                    Test.h
//

#ifndef TEST_H
#define TEST_H

#include <xlw/MyContainers.h>
#include <xlw/CellMatrix.h>
#include <xlw/DoubleOrNothing.h>
#include <xlw/ArgList.h>  
    
using namespace xlw;

//<xlw:libraryname=VanillaLib

double // Adds 1 to x
Ping(double x // argument
);

short // echoes a short
EchoShort(short x // number to be echoed
       );

#endif
