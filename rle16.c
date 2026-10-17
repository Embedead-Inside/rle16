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
 * @brief RLEエンコード（ブロック一括圧縮）
 * @details 
 * 入力バッファ内の連続する同一データをスキャンし、[連続回数] と [データ値] を 
 * 1つの16ビットパケットに詰め込んで圧縮バッファへ書き出します。
 * データが一切連続しない最悪のケースでも、圧縮パケット数は入力データ数（等倍）に収まり、
 * 元の容量より肥大化することが絶対にありません。
 * 
 * @param[in]  src_buf   入力データ（すでに下位ビットが切り捨て済みの生データ配列）へのポインタ
 * @param[in]  src_count 入力データの要素数（最大 65535 個）
 * @param[out] dest_buf  圧縮された 16-bit パケットを書き込む出力バッファへのポインタ
 * 
 * @return uint16_t 生成された圧縮パケットの総数（出力バッファに書き込まれた要素数）
 * 
 * @note 
 * - @b 時間計算量: O(N) - 入力データ数に比例した一本道のループ（条件分岐が極めてシンプル）
 * - @b 空間計算量: O(1) - 静的・動的メモリ（RAM）を一切消費せず、CPUレジスタのみで動作
 * - @b 警告: `src_buf` 内の各データは、あらかじめ `RLE16_DATA_MASK`（例: 12ビットなら 0〜4095）の
 *            範囲内に収まっている必要があります。範囲外のビットは圧縮時にカウンタ領域を破壊します。
 */
uint16_t rle16_encode(const uint16_t *src_buf, uint16_t src_count, uint16_t *dest_buf)
{
    /* 入力データが空の場合は即座に終了 */
    if (src_count == 0) return 0;

    uint16_t dest_idx = 0;              /**< 出力バッファのインデックス書き込み位置 */
    uint16_t current_val = src_buf[0];  /**< 現在連続性をチェックしている対象のデータ値 */
    uint16_t count = 1;                 /**< 現在のデータの連続回数カウンター */

    /* 2番目のデータから順にスキャン */
    for (uint16_t i = 1; i < src_count; i++) {
        uint16_t next_val = src_buf[i];

        /* 値が同じ、かつカウンタがビット幅の最大値未満であればカウントアップ */
        if (next_val == current_val && count < RLE16_MAX_COUNT) {
            count++;
        } else {
            /* 
             * カウンタとデータを合体してパケットを生成 
             * カウンタは 1〜MAX を 0〜(MAX-1) として格納するため (count - 1) を行う
             */
            dest_buf[dest_idx++] = ((count - 1) << RLE16_VALID_BITS) | current_val;

            /* 次の新しい値で状態をリセット */
            current_val = next_val;
            count = 1;
        }
    }
    
    /* ループ終了後、最後に残った未出力のデータをパケット化して書き出し */
    dest_buf[dest_idx++] = ((count - 1) << RLE16_VALID_BITS) | current_val;

    /* 生成された総パケット数を返す */
    return dest_idx;
}

/**
 * @brief RLEデコード（ブロック一括復元）
 * @details 
 * 圧縮された16ビットパケットの配列を読み込み、ビットパッキングを解除して
 * 元の連続するデータストリームを出力バッファへ展開（復元）します。
 * 
 * @param[in]  src_buf   圧縮された 16-bit パケット配列へのポインタ
 * @param[in]  src_count 圧縮パケットの個数（`rle16_encode` の戻り値）
 * @param[out] dest_buf  復元されたデータを書き込む出力バッファへのポインタ
 * 
 * @return uint16_t 復元（展開）された元のデータの総数
 * 
 * @note 
 * - @b 時間計算量: O(N) - 復元される総データ数に依存
 * - @b 空間計算量: O(1) - CPUレジスタのみで動作。内部でのバッファ確保は一切ありません。
 * - @b 注意: 出力バッファ（`dest_buf`）は、復元後の総データ数を格納するのに十分な容量を
 *            あらかじめ確保しておく必要があります（最大で圧縮前の元サイズと同等）。
 */
uint16_t rle16_decode(const uint16_t *src_buf, uint16_t src_count, uint16_t *dest_buf)
{
    uint16_t dest_idx = 0; /**< 復元先出力バッファのインデックス書き込み位置 */

    /* すべての圧縮パケットを順番に処理 */
    for (uint16_t i = 0; i < src_count; i++) {
        uint16_t packet = src_buf[i];

        /* 1. 上位ビットからカウンタを取り出し、元の連続回数に戻す（+1） */
        uint16_t count = (packet >> RLE16_VALID_BITS) + 1;

        /* 2. マスクを適用し、下位ビットから有効データのみを抽出 */
        uint16_t restored_data = packet & RLE16_DATA_MASK;

        /* 3. 取り出したデータを、連続回数分だけ出力バッファへ連続して書き込み（展開） */
        for (uint16_t j = 0; j < count; j++) {
            dest_buf[dest_idx++] = restored_data;
        }
    }

    /* 最終的に復元された元の全要素数を返す */
    return dest_idx;
}
