# mwm5 library - TWELITE STAGE

Mono Wireless C++ library for ESP32(M5stack)/Windows/Linux/macOS
TWELITE STAGE

TWELITE無線モジュール向け、シリアルメッセージパーサー、液晶画面用描画ライブラリなどが含まれます。

M5stackとTWELITEはシリアルポート(UART)により接続します。代表的な利用では、TWELITEからのシリアルメッセージを読み込み、電文を解釈し、LCD画面上に表示を行います。

また本パッケージには mwm5 ライブラリを用いて実装された TWELITE STAGE が含まれます。



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





## License (ライセンス)

[MW-OSSLA](license/MW-OSSLA-1J.txt) (モノワイヤレスオープンソースソフトウェア使用許諾契約書)を適用します。

src/twesettings については、MW-SLA (モノワイヤレスソフトウェア使用許諾書)と MW-OSSLA (モノワイヤレスオープンソースソフトウェア使用許諾契約書)のデュアルラインセンスを適用します。

オープンソース成果物については、ソースディレクトリまたはソースコード中に個別に記述があります。





## ビルドに必要な環境

環境に依存して、記載内容以外のパッケージ等の導入が必要になる場合があります。



### ESP32(M5stack)

https://github.com/m5stack/M5Stack を参考にしてください。

* TWELITE STAGE のビルドには http://www.pjrc.com/teensy/td_libs_PS2Keyboard.html プロジェクトの成果物が必要になります。esp32/PS2Keyboard.7z のファイルをライブラリに格納してからビルドしてください（日本語キーレイアウトやカーソルキーの取り扱い等で、TWELITE STAGE に合わせた修正を行っています）。



### Windows10

* Visual Studio 2019 (VC++) を用いています。msc ディレクトリ以下にプロジェクトファイルを用意しています。

* MingW64 gcc環境 (gcc-9 g++-9) によるビルド定義も含めています。
  * examples/???/build ディレクトリ上で make を実行します。

VC++ でビルドした場合と MingW64 でビルドした場合で、DLL ファイル (SDL2.dll) の格納場所に注意が必要です。VC++ では .exe (実行形式) のあるディレクトリ直下の dll ディレクトリに格納することが可能ですが、MingW では 実行形式のあるディレクトリに置く必要があります。

VC++のみ32bitのビルド定義を用意しています。



### macOS

macOS 10.14, 10.15 でビルドできます。

* brew にて gcc (gcc-9, g++-9) をインストールしておいてください。
  ※ clang によるビルド定義も用意していますが、一部 C++17 機能を利用しているため macOS 15 (Catalina) 以降が要件になります。
* XCode に付属する make を利用できるようにしておいてください。
* examples/???/build ディレクトリ上で make を実行します。

[osx/readme.txt](osx/readme.txt) も参照ください。



### Linux

Ubuntu16.04 18.04 20.04 でビルドできます。

* gcc (gcc-9 g++-9) をインストールしておいてください。
* SDL2 (libsdl2-dev パッケージ) をインストールしておいてください。
* examples/???/build ディレクトリ上で make を実行します。

[linux/readme.txt](linux/readme.txt) も参照ください。





## 添付ライブラリ

以下は、ビルド作業の簡素化と環境の一致のために外部プロジェクトの配布ライブラリ・ソースコードを添付しています。

* SDL2 ソース、ライブラリ
* FTDI D2XX ドライバ
* sndio (SDL2に依存しているがスタティックリンクできないため)





## 資料

https://mwm5.twelite.info を参照ください。

* ライセンス
* 使用方法
* APIリファレンス

