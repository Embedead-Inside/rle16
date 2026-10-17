# Compact Bare-Metal RLE (16-bit)

[![License: MIT-0](https://img.shields.io/badge/License-MIT--0-blue.svg)](https://spdx.org/licenses/MIT-0.html)
[![Language: C99](https://img.shields.io/badge/Language-C99-green.svg)](#)

C言語で書かれた、ベアメタル環境・マイコンデータストリーム（12ビットADCや各種センサーなど）に特化した、**極小・メモリ消費ゼロ・超高速**なランレングス圧縮（RLE）エンコーダ/デコーダです。

## 特徴

- **圧倒的なコードサイズ:** 複雑な条件分岐を排除し、ビット演算のみで構成された数行のコアロジック。
- **メモリ（RAM）消費ゼロ:** 静的・動的バッファ（`malloc`等）を一切使用せず、CPUレジスタ内だけで処理が完結。
- **データ肥大化の防止:** データが連続しない場合でも、圧縮後のサイズが元のデータサイズを決して超えません（等倍を維持）。
- **型の一貫性:** パフォーマンスと安全性の観点から、引数、カウンタ、戻り値にいたるまで全て `uint16_t` に統一。
- **コンパイル時設定:** 有効データビット数やRLEカウンタビット数を、`#define` マクロで簡単に調整可能。

## ビットパッキング構造（例：12ビット有効データの場合）

ポーリング（データ蓄積）の段階であらかじめ下位の不安定なノイズビットを切り捨てておくことで、1つの16ビットの箱の中に「RLEカウンタ」と「有効データ」をギューギューに詰め込みます。

```text

| 15          12 | 11                          0 |
+----------------+-------------------------------+

| RLEカウンタ - 1 | 有効センサーデータ            |
| (4ビット: 1-16) | (12ビット: 0-4095)           |
+----------------+-------------------------------+
```

## 使い方

### 1. 設定
ソースファイル（またはヘッダファイル）で、事前に切り捨てられた有効センサーデータのビット幅を定義します。

```c
#define VALID_BITS      12  // 例：12ビットADCデータ（下位4ビットノイズ切り捨て済み）
```

### 2. エンコード（圧縮）
データが溜まったバッファとその個数を渡します。戻り値として、生成された圧縮パケットの正確な個数が返ります。

```c
#include <stdint.h>

#define BUFFER_SIZE 1000
uint16_t raw_buffer[BUFFER_SIZE];  // 切り捨て済みの生データ (0-4095)
uint16_t comp_buffer[BUFFER_SIZE]; // 圧縮データが格納されるバッファ

void compress_sample(void) {
    // 圧縮後のパケット数が uint16_t で返ります
    uint16_t compressed_count = rle_encode_block16_optimized(raw_buffer, BUFFER_SIZE, comp_buffer);
}
```

### 3. デコード（復元）
圧縮バッファとパケット数を渡すことで、元のデータストリームに展開します。戻り値として、復元されたデータの総数が返ります。

```c
uint16_t decomp_buffer[BUFFER_SIZE]; // 復元データが格納されるバッファ

void decompress_sample(uint16_t compressed_count) {
    // 復元された元のデータ総数が uint16_t で返ります
    uint16_t restored_count = rle_decode_block16_optimized(comp_buffer, compressed_count, decomp_buffer);
}
```

## ライセンス

このプロジェクトはMIT-0ライセンスのもとで公開されています。詳細は[LICENSE](LICENSE)ファイルをご覧ください。
- 商用利用・個人利用を問わず、完全自由に使用できます。
- 著作権表示やライセンス文言の保持・記載義務すらありません。
- ソースコードの改変、流用、再配布、自社製品への組み込み等、制限なくご活用いただけます。

<p align="right"><small><a name="note" class="muted-link">※一部AIによる生成・調整コードが含まれることがあります。</a></small></p>
