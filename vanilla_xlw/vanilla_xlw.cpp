// vanilla_xlw.cpp
//
// Excel(XLW) から呼ぶ薄いラッパーを 1 ファイルに集約。
// - 依存：yc/ (date.hpp, daycount.hpp, curve.hpp など)
// - 方針：Excel 側は「文字列・数値・Excel シリアル日付(long)」のみを受け取り、
//         変換→ yc API 呼び出し → 結果を double/long で返す。
// - 注意：DayCount などの列挙は Excel 側に露出させず、string で受けてパース。
//         （_xlwcppinterface.cpp の “invalid literal suffix 'ACT'” を回避）
//
// 推奨：関数の宣言（エクスポート指定や <xlw:...> タグ）は同名のヘッダ
//       VanillaXlw.h に置いて再生成する運用。
//       ここでは実装のみを提供。

#include <stdexcept>
#include <string>
#include <cstdint>
#include <cmath>
#include <limits>

#include "yc/date.hpp"
#include "yc/daycount.hpp"
// 必要に応じて：#include "yc/curve.hpp"
// 必要に応じて：#include "yc/discount.hpp"

using std::string;

namespace xlw_bridge {

    //------------------------------------------------------------
    // Excel 1900 シリアル日付 ←→ yc::Date 変換ヘルパ
    //------------------------------------------------------------
    //
    // Excel(1900) は「1900年をうるう年と誤認」する歴史的仕様を持ち、
    // シリアル 60 が存在（1900-02-29）。ここでは業界で一般的な以下の扱い：
    //   - 基準日：1899-12-31 を 1 とする（実質 1900-01-01 が 1）
    //   - シリアル 60 はダミーとして通過させ、日付化では 1900-03-01 へずらす
    //
    // 参考：多くの実装は「シリアル >= 61 のとき 1 日引く」方式で補正。
    //
    static inline yc::Date from_excel_serial(long serial)
    {
        if (serial <= 0) {
            throw std::invalid_argument("Excel serial must be positive.");
        }
        // 1899-12-31 を day 0 と考える
        // 1900-01-01 = 1, ..., 1900-03-01 = 61（ただし 60 はダミー）
        long days = serial;

        // 1900-02-29 のダミー（serial=60）以降を 1 日引いて実日付に合わせる
        if (serial >= 61) {
            days -= 1;
        }
        else if (serial == 60) {
            // ダミー日付：慣例的に 1900-03-01 と見なす
            return yc::Date{ 1900, 3, 1 };
        }

        // ユリウス日などを使わず簡易加算：yc::Date に日加算ヘルパがなければ
        // 一旦 y/m/d に分解して計算…だが簡潔のため、yc 側に day-shift があればそれを利用。
        // ここでは自前でグレゴリオ暦加算を実装（最小限）。
        auto is_leap = [](int y) {
            return (y % 400 == 0) || ((y % 4 == 0) && (y % 100 != 0));
        };
        int y = 1899, m = 12, d = 31;
        static const int mdays_norm[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

        // days 回だけ 1 日ずつ進める（性能が必要ならユリウス日換算に置換）
        for (long i = 0; i < days; ++i) {
            int md = mdays_norm[m - 1] + ((m == 2 && is_leap(y)) ? 1 : 0);
            if (d < md) { ++d; }
            else {
                d = 1;
                if (m < 12) { ++m; }
                else { m = 1; ++y; }
            }
        }
        return yc::Date{ y, m, d };
    }

    static inline long to_excel_serial(const yc::Date& date)
    {
        // 1899-12-31 を 0 とし、以降の経過日数 + leap-bug 補正
        auto is_leap = [](int y) {
            return (y % 400 == 0) || ((y % 4 == 0) && (y % 100 != 0));
        };
        auto days_in_month = [&](int y, int m) {
            static const int mdays_norm[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
            return mdays_norm[m - 1] + ((m == 2 && is_leap(y)) ? 1 : 0);
        };

        int y = 1899, m = 12, d = 31;
        long serial = 0;

        // 1899-12-31 から date まで 1 日ずつ進める（高速化は必要なら後日）
        while (true) {
            if (y == date.y && m == date.m && d == date.d) break;
            int md = days_in_month(y, m);
            if (d < md) { ++d; }
            else {
                d = 1;
                if (m < 12) { ++m; }
                else { m = 1; ++y; }
            }
            ++serial;
        }

        // 1900-02-28(=59) の次にダミー 60(1900-02-29) を挿入するため、
        // それ以降は +1 して Excel のシリアルへ。
        // 実日付 1900-03-01 以降 → +1
        if (serial >= 60) serial += 1;
        return serial;
    }

    //------------------------------------------------------------
    // DayCount 文字列 → yc::DayCount
    //------------------------------------------------------------
    static inline yc::DayCount parse_dc(const std::string& s_raw)
    {
        // 大文字化とスラッシュ/空白の正規化（最小限）
        string s;
        s.reserve(s_raw.size());
        for (char c : s_raw) {
            if (c == ' ' || c == '-') continue;
            s.push_back(static_cast<char>(::toupper(static_cast<unsigned char>(c))));
        }

        // よく使う別名も吸収
        if (s == "ACT360" || s == "ACT/360" || s == "ACTUAL360" || s == "A360")
            return yc::DayCount::Actual360;
        if (s == "ACT365" || s == "ACT/365" || s == "ACT/365F" || s == "ACTUAL365F" || s == "A365F")
            return yc::DayCount::Actual365Fixed;
        if (s == "30U360" || s == "30/360US" || s == "30_360US" || s == "30E360US" || s == "30US360")
            return yc::DayCount::Thirty360US;

        throw std::invalid_argument("Unsupported DayCount: " + s_raw);
    }

} // namespace xlw_bridge


//------------------------------------------------------------
// ここから Excel に公開する関数の実装
//------------------------------------------------------------
//
// 重要：XLW の自動生成は「宣言（VanillaXlw.h）」を参照します。
//       - 関数名・引数順・型は宣言と一致させてください
//       - DayCount は string で受け取り、本ファイルで parse_dc します
//
// 提供関数（最初の最小セット）
//   1) YearFraction(start_serial, end_serial, dc_str) -> double
//   2) DateAddDays(excel_serial, days) -> excel_serial
//   3) DateToYMD(excel_serial) -> 文字列 "YYYY-MM-DD"（利便関数：検証用）
//
// 将来追加（例）
//   - BuildDiscountCurve(name, ref_serial, knot_serials[], dfs[])
//   - DF(name, pay_serial, dc_str)
//   - ZeroRate(name, pay_serial, dc_str, compounding, m)
//
// まずは “純粋関数系” から運用し、次にカーブ登録系を足すのが安全です。

extern "C" {

    // Year fraction between two Excel serial dates, given a daycount string.
    double YearFraction(long start_excel_serial, long end_excel_serial, const char* dc_cstr)
    {
        try {
            if (dc_cstr == nullptr) throw std::invalid_argument("DayCount string is null.");
            std::string dc_str(dc_cstr);

            const yc::Date d1 = xlw_bridge::from_excel_serial(start_excel_serial);
            const yc::Date d2 = xlw_bridge::from_excel_serial(end_excel_serial);
            const yc::DayCount dc = xlw_bridge::parse_dc(dc_str);

            return yc::year_fraction(d1, d2, dc);
        }
        catch (const std::exception& e) {
            // Excel には NaN を返し、別セルでエラーメッセージ表示できるようにする運用が無難
            // （必要ならログへ出す）
            (void)e;
            return std::numeric_limits<double>::quiet_NaN();
        }
    }

    // Add N days to an Excel serial date and return Excel serial date.
    long DateAddDays(long excel_serial, int days)
    {
        try {
            yc::Date d = xlw_bridge::from_excel_serial(excel_serial);

            // 最小限の加算（高速化は後日）：days>=0 で日々インクリメント
            auto is_leap = [](int y) {
                return (y % 400 == 0) || ((y % 4 == 0) && (y % 100 != 0));
            };
            auto days_in_month = [&](int y, int m) {
                static const int mdays_norm[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
                return mdays_norm[m - 1] + ((m == 2 && is_leap(y)) ? 1 : 0);
            };

            int y = d.y, m = d.m, dd = d.d;

            if (days >= 0) {
                for (int i = 0; i < days; ++i) {
                    int md = days_in_month(y, m);
                    if (dd < md) ++dd;
                    else { dd = 1; if (m < 12) ++m; else { m = 1; ++y; } }
                }
            }
            else {
                for (int i = 0; i < -days; ++i) {
                    if (dd > 1) --dd;
                    else { if (m > 1) --m; else { m = 12; --y; } dd = days_in_month(y, m); }
                }
            }

            return xlw_bridge::to_excel_serial(yc::Date{ y, m, dd });
        }
        catch (...) {
            return -1; // Excel 側で #VALUE! 扱いにしてもらう
        }
    }

    // Convenience: convert an Excel serial to "YYYY-MM-DD" text (for debugging).
    // Excel からの呼び出し：=DateToYMD(A1)
    __declspec(dllexport) const char* DateToYMD(long excel_serial)
    {
        // 注意：静的バッファはスレッドセーフではない。XLL は通常シングルスレッドだが
        // 必要に応じて TLS に移行。
        static thread_local std::string buf;
        try {
            yc::Date d = xlw_bridge::from_excel_serial(excel_serial);
            char tmp[32];
            std::snprintf(tmp, sizeof(tmp), "%04d-%02d-%02d", d.y, d.m, d.d);
            buf = tmp;
            return buf.c_str();
        }
        catch (...) {
            buf = "ERR";
            return buf.c_str();
        }
    }

} // extern "C"


//------------------------------------------------------------
// 将来拡張のスケルトン（コメントアウト例）
//------------------------------------------------------------
/*

// 例：割引カーブを Excel から登録（name で保持）
#include <vector>
#include <unordered_map>

namespace {
    struct CurveHolder {
        yc::Date ref;
        // 実際の yc::DiscountCurve のコンストラクタに合わせて保持形を調整
        // ここでは説明用ダミー：
        // yc::DiscountCurve curve;
    };

    std::unordered_map<std::string, CurveHolder> g_curves;
}

extern "C" {

// Excel 配列の受け渡しは XLW の型に合わせる必要あり。
// ここでは説明用にポインタ＋長さだが、実際は xlw::XlfOper / FP12 等を利用。
double BuildDiscountCurve(
    const char* name_cstr,
    long ref_excel_serial,
    const double* knot_serials, int knot_len,
    const double* dfs,          int dfs_len,
    const char* dc_cstr)
{
    // …ここで yc の実コンストラクタに合わせて nodes を組み立てて保持
    return 1.0; // 成功なら 1.0 など
}

double DF(const char* name_cstr, long pay_excel_serial, const char* dc_cstr)
{
    // g_curves[name].curve.df(t) を返す等
    return std::numeric_limits<double>::quiet_NaN();
}

} // extern "C"

*/

// 追加：YC_YearFraction の実体（_xlwcppinterface.cpp がこれを呼ぶ）
double YC_YearFraction(int y1, int m1, int d1,
    int y2, int m2, int d2,
    const std::string& dc_str)
{
    try {
        const yc::Date d_start{ y1, m1, d1 };
        const yc::Date d_end{ y2, m2, d2 };
        const yc::DayCount dc = xlw_bridge::parse_dc(dc_str);
        return yc::year_fraction(d_start, d_end, dc);
    }
    catch (...) {
        return std::numeric_limits<double>::quiet_NaN();
    }
}

// 新規（Excelシリアル版）
double YC_YearFractionSerial(int start_excel_serial,
    int end_excel_serial,
    const std::string& dc_str)
{
    try {
        const yc::Date d1 = xlw_bridge::from_excel_serial(start_excel_serial);
        const yc::Date d2 = xlw_bridge::from_excel_serial(end_excel_serial);
        const yc::DayCount dc = xlw_bridge::parse_dc(dc_str);
        return yc::year_fraction(d1, d2, dc);
    }
    catch (...) {
        return std::numeric_limits<double>::quiet_NaN();
    }
}
