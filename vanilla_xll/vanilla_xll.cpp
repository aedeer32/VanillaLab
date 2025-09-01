// vanilla_xll.cpp  — minimal helpers, no FRAMEWRK dependency

#include "pch.h"   // ← プロジェクトで PCH を使っていないなら、この行を削除 or /Y- に
#include <windows.h>
#include <string>
#include <algorithm>

#include "xlcall.h"
#pragma comment(lib, "xlcall32.lib")   // パス設定は別途必要（Additional Library Directories）

// --- your library ---
#include "yc/date.hpp"
#include "yc/daycount.hpp"
#include "yc/compounding.hpp"
#include "yc/discount.hpp"

using namespace yc;

//==================== Minimal helpers (instead of framewrk.h) ====================
struct XStr12 {
    XLOPER12 x{};
    std::wstring buf;  // buf[0]=len, buf[1..n]=chars
    explicit XStr12(const wchar_t* s) {
        size_t n = wcslen(s);
        buf.resize(n + 1);
        buf[0] = static_cast<wchar_t>(n);
        std::copy(s, s + n, buf.begin() + 1);
        x.xltype = xltypeStr;
        x.val.str = const_cast<wchar_t*>(buf.c_str());
    }
    operator LPXLOPER12() { return &x; }
};
inline XLOPER12 XNum12(double d) { XLOPER12 x; x.xltype = xltypeNum; x.val.num = d; return x; }

static void register_one(const wchar_t* proc,
    const wchar_t* shownName,
    const wchar_t* typeText,    // e.g. L"BBBB$"
    const wchar_t* argText,     // e.g. L"a,b,c"
    int macroType,              // 1=worksheet UDF
    const wchar_t* category,
    const wchar_t* funcHelp)
{
    XLOPER12 xDLL{}, xRes{};
    Excel12(xlGetName, &xDLL, 0);

    XStr12 p(proc), t(typeText), f(shownName), a(argText), c(category), h(funcHelp);
    XLOPER12 mt = XNum12(static_cast<double>(macroType));
    XStr12 empty(L"");

    LPXLOPER12 argv[] = { &xDLL, p, t, f, a, &mt, c, empty, empty, h };
    Excel12v(xlfRegister, &xRes, (int)_countof(argv), argv);

    Excel12(xlFree, 0, 1, &xDLL);
    Excel12(xlFree, 0, 1, &xRes);
}
//===============================================================================

//-------------------- Small helpers for yc enums --------------------
static DayCount map_dc(int dc) {
    switch (dc) {
    case 0: return DayCount::Actual360;
    case 1: return DayCount::Actual365Fixed;
    case 2: return DayCount::Thirty360US;
    default: return DayCount::Actual365Fixed;
    }
}
static Compounding map_comp(int kind, int m) {
    switch (kind) {
    case 0: return Compounding::simple();
    case 1: return Compounding::compounded(m > 0 ? m : 1);
    case 2: return Compounding::continuous();
    default: return Compounding::continuous();
    }
}

//============================= UDFs =============================

// =YC.YEARFRAC(DATE(y1,m1,d1), DATE(y2,m2,d2), dc)
// dc: 0=ACT/360, 1=ACT/365F, 2=30/360 US
extern "C" __declspec(dllexport)
double __stdcall YC_YEARFRAC(double y1, double m1, double d1,
    double y2, double m2, double d2,
    double dcCode)
{
    Date a{ (int)y1,(int)m1,(int)d1 };
    Date b{ (int)y2,(int)m2,(int)d2 };
    return year_fraction(a, b, map_dc((int)dcCode));
}

// =YC.DF_FROM_ZERO(r, t, kind, m)
// kind: 0=Simple, 1=Compounded(m), 2=Continuous
extern "C" __declspec(dllexport)
double __stdcall YC_DF_FROM_ZERO(double r, double t, double kind, double m)
{
    return discount_factor_from_zero(r, t, map_comp((int)kind, (int)m));
}

// =YC.ZERO_FROM_DF(df, t, kind, m)
extern "C" __declspec(dllexport)
double __stdcall YC_ZERO_FROM_DF(double df, double t, double kind, double m)
{
    return zero_rate_from_df(df, t, map_comp((int)kind, (int)m));
}

//============================ XLL entry points ============================

extern "C" __declspec(dllexport)
int __stdcall xlAutoOpen(void)
{
    // 型テキスト: 先頭が戻り値、その後が引数。B=double。末尾の $ はスレッドセーフ指定。
    register_one(L"YC_YEARFRAC", L"YC.YEARFRAC",
        L"BBBBBBBB$", L"y1,m1,d1,y2,m2,d2,dc",
        1, L"YC", L"Year fraction (dc:0=ACT/360,1=ACT/365F,2=30U)");

    register_one(L"YC_DF_FROM_ZERO", L"YC.DF_FROM_ZERO",
        L"BBBBBB$", L"r,t,kind,m",
        1, L"YC", L"Discount factor from zero rate");

    register_one(L"YC_ZERO_FROM_DF", L"YC.ZERO_FROM_DF",
        L"BBBBBB$", L"df,t,kind,m",
        1, L"YC", L"Zero rate from discount factor");

    return 1;
}

extern "C" __declspec(dllexport)
int __stdcall xlAutoClose(void)
{
    // ここでは特に解除処理なし（Excelが解放）
    return 1;
}

// （任意）アドイン マネージャ表示名
extern "C" __declspec(dllexport)
LPXLOPER12 __stdcall xlAddInManagerInfo12(LPXLOPER12)
{
    static XStr12 name(L"VanillaLib XLL");
    return name;
}
