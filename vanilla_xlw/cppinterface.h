#ifndef TEST_H
#define TEST_H

#include <xlw/MyContainers.h>
#include <xlw/CellMatrix.h>
#include <xlw/DoubleOrNothing.h>
#include <xlw/ArgList.h>  
    
using namespace xlw;

//<xlw:libraryname=VanillaLib

double               // Year fraction between two dates under a day-count
YC_YearFraction(
    int y1, int m1, int d1,   // start date (YYYY,MM,DD)
    int y2, int m2, int d2,   // end date   (YYYY,MM,DD)
    const std::string& dc     // Allowed: ACT/360, ACT/365F, 30/360US
);

#endif
