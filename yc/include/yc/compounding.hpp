#pragma once

namespace yc {

    enum class CompKind { Simple, Compounded, Continuous };

    struct Compounding {
        CompKind kind{ CompKind::Simple };
        int m{ 1 }; // Compounded のときの頻度（年内回数）。Simple/Continuous では未使用。

        static Compounding simple() { return { CompKind::Simple, 1 }; }
        static Compounding compounded(int freq) { return { CompKind::Compounded, freq }; }
        static Compounding continuous() { return { CompKind::Continuous, 1 }; }
    };

} // namespace yc
