#pragma once
#include <variant>
#include "date.hpp"
#include "daycount.hpp"
#include "types.hpp"
#include "swap.hpp"   // OIS�̃p�����[�^�iFixed/Float�j�ɃA�N�Z�X���邽��

namespace yc {

    // ---- �f�|�i�P�������ʁj ----
    // PV=0����: DF(0, S) - (1 + r*��) * DF(0, E_pay) = 0
    struct DepoQuote {
        Date start;
        Date end;              // �ʏ�: start < end
        double rate;           // �N���N�H�[�g�i��: 0.012 = 1.2%�j
        DayCount dc;           // ��: Actual360
        int payment_lag_days{ 0 }; // �ʏ� 0�iT+0�j�B�K�v�Ȃ�T+2�Ȃ�
    };

    // ---- FRA�����i�Z���t�H���[�h�j ----
    // ����: F(S,E) = r_quote,  ������ F = (DF(S)/DF(E)-1)/��
    // �`���IFRA��S���ς����A�p���e�B��͏�̓���������DF(E)�����܂�
    struct FraQuote {
        Date start;
        Date end;      // �V�m�[�h�� E �ɒu��
        double rate;   // �N���N�H�[�g
        DayCount dc;   // ��: Actual360
    };

    // ---- OIS�i�����j ----
    struct OisParQuote {
        Date end;
        double fixed_rate;
    };

    // �� �ǉ�: IBOR�X���b�v�̃p�[���[�g
    // ������API�����ŗ^����i�X�|�b�g�Ȃǁj�B�Œ�/�����̕p�x��DC�̓e���v���[�g�Ŏw��B
    struct IborSwapQuote {
        Date end;          // �X���b�v����
        double fixed_rate; // �N���i�� 0.0125�j
    };

    // ���݃C���X�g�D�������g
    using Instrument = std::variant<DepoQuote, FraQuote, OisParQuote, IborSwapQuote>;

} // namespace yc
