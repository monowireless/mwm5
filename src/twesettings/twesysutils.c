/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/** @file
 *
 * @defgroup TWESYSUTL
 *
 */

#ifdef JENNIC_CHIP

#include "twecommon.h"
#include "ToCoNet.h"

#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppApi.h>

/**
 * @brief TWENET のシステムカウンタを用いたポーリング待ち処理。
 *   - TickTimer のカウント値を基に時間待ちを行う
 *   - ポーリング中は電池消費が高くなるため CPU のクロックを落とす
 *   - 長時間街を行えるよう WatchDog のリスタートを都度行う
 * 
 * @param u32WaitCt 待ちカウンタ
 */
void TWESYSUTL_vWaitPoll(uint32 u32WaitCt) {
    // tick counter のカウントを数えながら ms 待ちを行う
    uint32 u32tc = u32AHI_TickTimerRead();
    uint32 u32total = 0;
    uint32 u32maxtick;
    switch(sToCoNet_AppContext.u16TickHz) { // 割り算は勘弁だ！
    case 1000: u32maxtick = (16000000UL / 1000); break; // 1ms
    case 500: u32maxtick = (16000000UL / 500); break; // 2ms
    case 250: u32maxtick = (16000000UL / 250); break; // 4ms
    default: u32maxtick = 16000000UL / sToCoNet_AppContext.u16TickHz;
    }
    uint32 u32target = 0;
    uint16 u16wdtct = 0;

    if (u32WaitCt >= 3) {
        u32target = u32WaitCt * 16000 - 2000;
        bAHI_SetClockRate(6); // set 1MHz CPU
    } else {
        // 16Mhz or 32Mhz を想定
        u32target = u32WaitCt * 16000;
    }
    while(TRUE) {
        uint32 u32tc1 = u32AHI_TickTimerRead();
        if (u32tc1 >= u32tc) {
            u32total += (u32tc1 - u32tc);
        } else {
            u32total += (u32maxtick - u32tc);
            u32total += u32tc1;
        }

        // 目標のカウント値になったらブレーク
        if (u32total > u32target) break;

        // 一定カウントごとに WatchDog をリスタート
        uint16 u16w = (uint16)(u32total >> 16);
        if (u16wdtct != u16w) {
            vAHI_WatchdogRestart();
            u16wdtct = u16w;
        }

        // カウントの更新
        u32tc = u32tc1;
    }
    if (u32WaitCt >= 3) {
        bAHI_SetClockRate(sToCoNet_AppContext.u8CPUClk); // revert clock
    }
}

/**
 * @brief TWENET のシステムカウンタを用いたポーリング待ち処理(Micro sec版)
 *   - TickTimer のカウント値を基に時間待ちを行う
 * 
 * @param u32WaitCt 待ちカウンタ
 */
void TWESYSUTL_vWaitPollMicro(uint32 u32WaitCt) {
    // tick counter のカウントを数えながら ms 待ちを行う
    uint32 u32tc = u32AHI_TickTimerRead();
    uint32 u32maxtick;
    switch(sToCoNet_AppContext.u16TickHz) { // 割り算は勘弁だ！
    case 1000: u32maxtick = (16000000UL / 1000); break; // 1ms
    case 500: u32maxtick = (16000000UL / 500); break; // 2ms
    case 250: u32maxtick = (16000000UL / 250); break; // 4ms
    default: u32maxtick = 16000000UL / sToCoNet_AppContext.u16TickHz;
    }
    uint32 u32target = 0; 
    uint32 u32total = 12; // オーバーヘッド分、少しだけ短く設定

    // 16Mhz or 32Mhz を想定
    u32target = u32WaitCt * 16;
    while(TRUE) {
        uint32 u32tc1 = u32AHI_TickTimerRead();
        if (u32tc1 >= u32tc) {
            u32total += (u32tc1 - u32tc);
        } else {
            u32total += (u32maxtick - u32tc);
            u32total += u32tc1;
        }

        // 目標のカウント値になったらブレーク
        if (u32total > u32target) break;

        // カウントの更新
        u32tc = u32tc1;
    }
}

#endif // JENNIC_CHIP