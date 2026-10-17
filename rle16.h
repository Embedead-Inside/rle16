/**
 * @file rle16.h
 * @brief 超軽量・ベアメタル環境向け 16-bit RLE（ランレングス圧縮）ライブラリ
 * @details 
 * 本ライブラリは、OSの存在しないベアメタル環境やリソースが極めて制限されたマイコンシステムにおいて、
 * センサーデータ（例: 12ビットADC）などのデータストリームを、高速かつメモリ消費ゼロ（RAM割り当てなし）で
 * 圧縮・展開するために設計されています。
 *
 * @copyright Copyright (c) 2026 Embedead-Inside
 * @license SPDX-License-Identifier: MIT-0
 */

#ifndef RLE16_H
#define RLE16_H

#include <stdint.h>

/**
 * @defgroup rle16_config ライブラリ設定
 * @brief コンパイル時に決定されるビット割当の定義
 * @{
 */

/**
 * @brief 有効データ（センサー値など）のビット幅
 * @details 
 * ポーリング等でバッファに蓄積される段階で、あらかじめ下位ビット（ノイズ等）が切り捨てられた
 * データのビット幅を指定します。
 * - 12ビットADC（下位4ビット切り捨て）の場合： 12
 * - 10ビットADC（下位6ビット切り捨て）の場合： 10
 */
#define RLE16_VALID_BITS      12

/** @brief RLEカウンタに割り当てられるビット幅（自動計算） */
#define RLE16_COUNTER_BITS    (16 - RLE16_VALID_BITS)

/** @brief 1つのパケットで表現可能な最大連続回数（自動計算） */
#define RLE16_MAX_COUNT       (1 << RLE16_COUNTER_BITS)

/** @brief 有効データ部分のみを抽出するためのビットマスク（自動計算） */
#define RLE16_DATA_MASK       ((1 << RLE16_VALID_BITS) - 1)

/** @} */ // end of rle16_config


#ifdef __cplusplus
extern "C" {
#endif

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
uint16_t rle16_encode(const uint16_t *src_buf, uint16_t src_count, uint16_t *dest_buf);

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
uint16_t rle16_decode(const uint16_t *src_buf, uint16_t src_count, uint16_t *dest_buf);

#ifdef __cplusplus
}
#endif

#endif /* RLE16_H */
