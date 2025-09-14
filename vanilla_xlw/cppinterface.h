//<xlw:libraryname=vanilla_xlw>
#include <string>

//<xlw:export>
double YC_YearFraction(int y1, int m1, int d1,
    int y2, int m2, int d2,
    const std::string& dc);

//<xlw:export>
double YC_YearFractionSerial(int start_excel_serial,
    int end_excel_serial,
    const std::string& dc);
