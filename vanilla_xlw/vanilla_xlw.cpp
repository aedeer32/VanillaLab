#include <xlw/xlw.h>
using namespace xlw;

// ネイティブ関数（あとでVanillaLibの関数に置き換え）
static double PingImpl(double x) { return x + 1.0; }