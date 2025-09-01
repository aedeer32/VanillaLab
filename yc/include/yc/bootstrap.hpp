#pragma once
#include <vector>
#include <utility>
#include <algorithm>
#include "date.hpp"
#include "daycount.hpp"
#include "curve.hpp"
#include "swap.hpp"
#include "types.hpp"
#include "instrument.hpp"

namespace yc {

    // OIS�J�[�u�E�u�[�g�X�g���b�v�F
    // - ref        : �Ȑ��̎Q�Ɠ��it=0�j
    // - start      : �eOIS�̊����i�P�����F�S�N�H�[�g����̊����j
    // - quotes     : ���������i�����Ƀ\�[�g����Ă��Ȃ���΃\�[�g���܂��j
    // - fp_tmpl    : �Œ背�b�O�̐��`�inotional/freq/dc/pay/lag�j��fixed_rate��quote�ŏ㏑��
    // - lp_tmpl    : �������b�O�̐��`�iSOFR/TONA������ style=CompoundedInArrears �𐄏��j
    // - t_dc       : �Ȑ��� t[�N] �����Ƃ��� DayCount�i�� A/365F�j
    // �Ԃ�l�F�����Ȑ��iDF��log-linear��ԁj
    // �菇�F���m�s���[ + �u�V�����̍Ō�̎x����DF�����m���v���g���ANPV=0 ��񕪖@�ŉ���
    DiscountCurve bootstrap_ois_curve(
        const Date& ref,
        const Date& start,
        std::vector<OisParQuote> quotes,
        FixedLegParams fp_tmpl,
        FloatLegParams lp_tmpl,
        DayCount t_dc
    );

    // �V�K: �f�|/FRA/OIS �����݂������u�[�g�X�g���b�v
    // - ref       : �J�[�u�̎Q�Ɠ�
    // - start     : �eOIS�̊����iOIS�̂ݎg�p�j
    // - insts     : Depo/FRA/OIS �̃x�N�^�i�������łȂ��Ă�OK�j
    // - fp_tmpl   : OIS�p�Œ背�b�O���`�ifixed_rate��OIS�N�H�[�g�ŏ㏑���j
    // - lp_tmpl   : OIS�p�������b�O���`�iRFR���� in-arrears �����j
    // - t_dc      : �N��t�v�Z�̋K��i�� A/365F�j
    // �Ԃ�l: �����Ȑ��ilog-linear on DF�j
    DiscountCurve bootstrap_curve_mixed(
        const Date& ref,
        const Date& start,
        std::vector<Instrument> insts,
        FixedLegParams fp_tmpl,
        FloatLegParams lp_tmpl,
        DayCount t_dc
    );

    // �� �V�K: ����(OIS)�����m�̂��ƂŁA���e�J�[�u(IBOR)���\�z
    DiscountCurve bootstrap_projection_curve(
        const Date& ref,
        const Date& start,                     // FRA/IRS�̊����i�X�|�b�g���j
        const DiscountCurve& discount_curve,   // ���ɍ\�z�ς�OIS
        std::vector<Instrument> proj_insts,    // Depo / FRA / IborSwap �݂̂�z��
        FixedLegParams ibor_fix_tmpl,          // �Œ背�b�O���`�ifreq, dc, pay, lag�j
        FloatLegParams ibor_float_tmpl,        // �������b�O���`�istyle=Simple �𐄏��j
        DayCount t_dc                          // t�v�Z�i�� A/365F�j
    );

    // �� �֗�: OIS(����) �� IBOR(���e) ���ꊇ�\�z
    struct MultiCurve {
        DiscountCurve discount;  // OIS
        DiscountCurve projection; // IBOR
    };

    MultiCurve bootstrap_multi_curve(
        const Date& ref,
        const Date& ois_start,
        const std::vector<OisParQuote>& ois_quotes,
        FixedLegParams ois_fix_tmpl, FloatLegParams ois_float_tmpl,
        const Date& ibor_start,
        std::vector<Instrument> proj_insts,
        FixedLegParams ibor_fix_tmpl, FloatLegParams ibor_float_tmpl,
        DayCount t_dc
    );

} // namespace yc
