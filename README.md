# VanillaLib (yc) — Lightweight C++17 Yield Curve & Swaps

**Goal**  
A small, dependency-free C++17 library that implements just the essentials for building yield curves and valuing plain-vanilla interest rate swaps, inspired by QuantLib but intentionally minimal.

- Dates & Day Count: `Actual/360`, `Actual/365F`, `30/360 US`
- Compounding rules and DF ⇄ zero-rate conversions (simple / m-times / continuous)
- Discount curve with **log-linear on DF** interpolation (flat end extrapolation)
- Simple schedule generator (month add with end-of-month preservation)
- Fixed & floating legs  
  - **RFR OIS, compounded in arrears**  
  - Observation lag (lookback/observation shift), **lockout**, **payment lag**
- Bootstrapping
  - OIS par swaps → **discount curve**
  - Mixed short end: **Depos / FRAs / OIS**
  - **Multi-curve**: discount (OIS) separated from projection (IBOR)

> Business-day calendars / holiday adjustments are intentionally omitted for clarity and will be added later.

---

## Layout

<solution_root>/
├─ yc/ # Library (Static .lib)
│ ├─ include/yc/
│ │ date.hpp
│ │ daycount.hpp
│ │ compounding.hpp
│ │ discount.hpp
│ │ curve.hpp
│ │ schedule.hpp
│ │ cashflow.hpp
│ │ swap.hpp
│ │ instrument.hpp
│ │ bootstrap.hpp
│ └─ src/
│ daycount.cpp
│ discount.cpp
│ curve.cpp
│ schedule.cpp
│ swap.cpp
│ bootstrap.cpp
├─ examples/ # One console app project per example (recommended)
│ ├─ ex-ois/ # OIS (RFR compounded in arrears, with lags)
│ ├─ ex-boot/ # OIS bootstrapping
│ ├─ ex-boot-mixed/ # Depos / FRA / OIS mixed bootstrap
│ └─ ex-multicurve/ # Discount(OIS) + Projection(IBOR)
└─ tests/
└─ smoke.cpp

yaml
Copy code

---

## Requirements

- **Visual Studio 2022 Community** (Desktop development with C++)
- C++ standard: **C++17**

Recommended unified output dirs in VS:
- **Configuration Properties → General → Output Directory**  
  `$(SolutionDir)bin\$(Platform)\$(Configuration)\`
- **Intermediate Directory**  
  `$(SolutionDir)obj\$(ProjectName)\$(Platform)\$(Configuration)\`

---

## Build & Run (Visual Studio)

1. Open the solution.
2. Make `yc` a **Static Library (.lib)**; set C++ standard to C++17.
3. For each example (e.g., `examples/ex-ois`), create a **Console App** project:
   - Add a **Project Reference** to `yc`.
   - Add `$(SolutionDir)yc\include` to **Additional Include Directories**.
   - Set Output/Intermediate directories as above.
4. Set the example project as **Startup Project**.
5. Choose `x64 / Debug` (or Release) and run with **Ctrl+F5** (or **F5**).

Artifacts are placed under `bin\<Platform>\<Configuration>\`.

---

## Quick Start

```cpp
#include "yc/date.hpp"
#include "yc/daycount.hpp"
#include "yc/curve.hpp"
#include "yc/swap.hpp"
#include "yc/bootstrap.hpp"
using namespace yc;

int main() {
  Date ref{2025,9,1}, start{2025,9,3};

  // 1) Build an OIS discount curve from par quotes
  std::vector<OisParQuote> ois = { {Date{2026,9,3},0.011}, {Date{2027,9,3},0.0125} };

  FixedLegParams ois_fix; 
  ois_fix.freq=Frequency::Annual; ois_fix.dc=DayCount::Thirty360US; 
  ois_fix.pay=true; ois_fix.payment_lag_days=2;

  FloatLegParams ois_flt; 
  ois_flt.freq=Frequency::SemiAnnual; ois_flt.dc=DayCount::Actual360; 
  ois_flt.pay=false; ois_flt.style=FloatStyle::CompoundedInArrears; 
  ois_flt.observation_lag_days=2; ois_flt.payment_lag_days=2;

  DiscountCurve disc = bootstrap_ois_curve(
      ref, start, ois, ois_fix, ois_flt, DayCount::Actual365Fixed);

  // 2) Build an IBOR projection curve using Depo/FRA/IBOR swaps (discount = OIS)
  std::vector<Instrument> proj = {
    DepoQuote{ start, Date{2025,9,10}, 0.0110, DayCount::Actual360, 0 },
    FraQuote { Date{2025,10,3}, Date{2026,1,3}, 0.0120, DayCount::Actual360 },
    IborSwapQuote{ Date{2027,9,3}, 0.0140 }
  };
  FixedLegParams ib_fix; 
  ib_fix.freq=Frequency::Annual; ib_fix.dc=DayCount::Thirty360US; 
  ib_fix.pay=true; ib_fix.payment_lag_days=2;

  FloatLegParams ib_flt; 
  ib_flt.freq=Frequency::Quarterly; ib_flt.dc=DayCount::Actual360; 
  ib_flt.pay=false; ib_flt.style=FloatStyle::Simple; 
  ib_flt.payment_lag_days=2;

  DiscountCurve proj_curve = bootstrap_projection_curve(
      ref, start, disc, proj, ib_fix, ib_flt, DayCount::Actual365Fixed);

  // 3) Query DFs
  double t2y = year_fraction(ref, Date{2027,9,3}, DayCount::Actual365Fixed);
  auto df_disc_2y = disc.df(t2y);
  auto df_proj_2y = proj_curve.df(t2y);
  (void)df_disc_2y; (void)df_proj_2y;
}
