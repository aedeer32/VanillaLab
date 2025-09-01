#pragma once

namespace yc {

    enum class CompKind { Simple, Compounded, Continuous };

    struct Compounding {
        CompKind kind{ CompKind::Simple };
        int m{ 1 }; // Compounded �̂Ƃ��̕p�x�i�N���񐔁j�BSimple/Continuous �ł͖��g�p�B

        static Compounding simple() { return { CompKind::Simple, 1 }; }
        static Compounding compounded(int freq) { return { CompKind::Compounded, freq }; }
        static Compounding continuous() { return { CompKind::Continuous, 1 }; }
    };

} // namespace yc
