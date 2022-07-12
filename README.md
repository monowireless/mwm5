# mwm5 library - TWELITE STAGE

Mono Wireless TWELITE STAGE Application and C++ library for Windows/Linux/macOS/(M5Stack)



This project implements a "TWELITE STAGE application". It is also possible to write simple applications using related libraries.

本プロジェクトでは「TWELITE STAGE アプリ」を実装しています。また、関連のライブラリを用いたシンプルなアプリケーションを記述することも可能です。



---

Building EPS32 (M5) with Version 1.3 or later is not considered in the source code.

Version 1.3 以降での EPS32 (M5) のビルドについては、ソースコード上考慮していません。

---





以下にライブラリの基本機能を挙げます。

- シリアルメッセージのパーサー
  - TWELITE PAL (MAG/AMB) のメッセージ解釈
- 固定幅文字のテキストボックス描画ライブラリ
  - パブリックドメイン扱いの日本語フォント (10/12/14/16ドット)
  - 簡易ターミナル機能
    - 行末スクロール
    - エスケープシーケンス（ANSIターミナル互換を目的としたものではありません）
- 実装例
  - examples/TWELITE_Stage : TWELITE STAGE
  - examples/AppTwelite_Recv : 標準アプリ App_Twelite の 0x81 シリアルメッセージの解釈と表示
  - examples/PAL_Recv : TWELITE PAL シリアルメッセージの解釈と表示





## License

[MW-OSSLA](license/MW-OSSLA-1E.txt) (Monowireless Open Source Software License Agreement).

[MW-OSSLA](license/MW-OSSLA-1J.txt) (モノワイヤレスオープンソースソフトウェア使用許諾契約書)を適用します。



For src/twesettings, the dual license of MW-SLA (Monowireless Software License Agreement) and MW-OSSLA (Monowireless Open Source Software License Agreement) is applied.

src/twesettings については、MW-SLA (モノワイヤレスソフトウェア使用許諾書)と MW-OSSLA (モノワイヤレスオープンソースソフトウェア使用許諾契約書)のデュアルラインセンスを適用します。



Open source deliverables are described separately in the source directory or in the source code.

オープンソース成果物については、ソースディレクトリまたはソースコード中に個別に記述があります。



## Information (資料)

* English
  * [Building notes](BUILD_en.md)
  * [MWM5 library manual (In Japanese)](https://mwm5.twelite.info)
  * [TWELITE STAGE APP MANUAL](https://stage.twelite.info/v/eng/)
* 日本語
  * [ビルド メモ](Build_jp.md)
  * [MWM5ライブラリマニュアル](https://mwm5.twelite.info)
  * [TWELITE STAGE アプリ マニュアル](https://stage.twelite.info/) 
