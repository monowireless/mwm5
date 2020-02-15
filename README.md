# mwm5 library

Mono Wireless C++ library for ESP32 (M5stack).

M5stack 用に記述されたTWELITE無線モジュール向け、シリアルメッセージパーサー、液晶画面用描画ライブラリなどが含まれます。

M5stackとTWELITEはシリアルポート(UART)により接続します。代表的な利用では、TWELITEからのシリアルメッセージを読み込み、電文を解釈し、LCD画面上に表示を行います。



以下にライブラリの基本機能を挙げます。

- シリアルメッセージのパーサー
  - TWELITE PAL (MAG/AMB) のメッセージ解釈
- 固定幅文字のテキストボックス描画ライブラリ
  - パブリックドメイン扱いの日本語フォント (10/12/14/16ドット)
  - 簡易ターミナル機能
    - 行末スクロール
    - エスケープシーケンス（ANSIターミナル互換を目的としたものではありません）
- サンプルスケッチ
  - examples/AppTwelite_Recv : 標準アプリ App_Twelite の 0x81 シリアルメッセージの解釈と表示
  - examples/PAL_Recv : TWELITE PAL シリアルメッセージの解釈と表示



## License (ライセンス)

[MW-OSSLA](license/MW-OSSLA-1J.txt) (モノワイヤレスオープンソースソフトウェア使用許諾契約書)を適用します。



## ビルドに必要な環境

https://github.com/m5stack/M5Stack を参考にしてください。



## 資料

https://mwm5.twelite.info を参照ください。

* ライセンス
* 使用方法
* APIリファレンス

