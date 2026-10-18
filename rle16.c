/**
 * @file rle16.c
 * @brief 超軽量・ベアメタル環境向け 16-bit RLE（ランレングス圧縮）ライブラリ
 * @details 
 * 本ライブラリは、OSの存在しないベアメタル環境やリソースが極めて制限されたマイコンシステムにおいて、
 * センサーデータ（例: 12ビットADC）などのデータストリームを、高速かつメモリ消費ゼロ（RAM割り当てなし）で
 * 圧縮・展開するために設計されています。
 *
 * @copyright Copyright (c) 2026 Embedead-Inside
 * @license SPDX-License-Identifier: MIT-0
 */

#include "rle16.h"

/**
 * @brief RLEエンコード（ストリーム一括圧縮）
 */
uint32_t rle16_encode(const uint16_t *src_buf, uint32_t src_count, uint16_t *dest_buf)
{
    /* 入力データまたはバッファが空の場合は即座に終了 */
    if (src_buf == NULL || dest_buf == NULL || src_count == 0) {
        return 0;
    }

    const uint16_t *src = src_buf;
    uint16_t *dest = dest_buf;
    const uint16_t *src_end = src_buf + src_count;

    // 最初のデータで初期化
    uint16_t current_val = *src++;
    uint32_t count = 1; // 内部カウンタを32ビットにしてレジスタ処理を最適化

    /* 2番目のデータから順にスキャン */
    while (src < src_end) {
        uint16_t next_val = *src++;

        /* 値が同じ、かつカウンタがビット幅の最大値未満であればカウントアップ */
        if (next_val == current_val && count < RLE16_MAX_COUNT) {
            count++;
        } else {
            /* 
             * カウンタとデータを合体してパケットを生成 
             * カウンタは 1〜MAX を 0〜(MAX-1) として格納するため (count - 1) を行う
             */
            *dest++ = ((uint16_t)(count - 1) << RLE16_VALID_BITS) | current_val;

            /* 次の新しい値で状態をリセット */
            current_val = next_val;
            count = 1;
        }
    }
    
    /* ループ終了後、最後に残った未出力のデータをパケット化して書き出し */
    *dest++ = ((uint16_t)(count - 1) << RLE16_VALID_BITS) | current_val;

    /* 生成された総パケット数をポインタの差分から計算して返す */
    return (uint32_t)(dest - dest_buf);
}

/**
 * @brief RLEデコード（ストリーム一括復元）
 */
uint32_t rle16_decode(const uint16_t *src_buf, uint32_t src_count, uint16_t *dest_buf)
{
    /* 入力データまたはバッファが空の場合は即座に終了 */
    if (src_buf == NULL || dest_buf == NULL || src_count == 0) {
        return 0;
    }

    const uint16_t *src = src_buf;
    uint16_t *dest = dest_buf;
    const uint16_t *src_end = src_buf + src_count;

    /* すべての圧縮パケットを順番に処理 */
    while (src < src_end) {
        uint16_t packet = *src++;

        /* 上位ビットからカウンタを取り出し、元の連続回数に戻す（+1） */
        uint32_t count = (uint32_t)(packet >> RLE16_VALID_BITS) + 1;

        /* マスクを適用し、下位ビットから有効データのみを抽出 */
        uint16_t restored_data = packet & RLE16_DATA_MASK;

        /* 取り出したデータを、連続回数分だけ出力バッファへ連続して書き込み（展開） */
        do {
            *dest++ = restored_data;
            count--;
        } while (count > 0);
    }

    /* 最終的に復元された元の全要素数をポインタの差分から計算して返す */
    return (uint32_t)(dest - dest_buf);
}
