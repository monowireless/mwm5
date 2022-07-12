# mwm5 - ビルドについて

Mono Wireless TWELITE STAGE Application and C++ library for Windows/Linux/macOS/(M5Stack)



---

Version 1.3 以降での EPS32 (M5) のビルドについては、ソースコード上考慮していません。

---



## ビルドに必要な環境

環境に依存して、記載内容以外のパッケージ等の導入が必要になる場合があります。



### Windows10,11

* Visual Studio 2019 (VC++) を用いています。msc ディレクトリ以下にプロジェクトファイルを用意しています。
* Sketchフィルター以下にあるプロジェクト (TWELITE_Stageなど) をスタートアッププロジェクトにして、Release/Debug、64bit/32bit を選択してビルドします。



VC++のみ32bitのビルド定義を用意しています。



### macOS

macOS 10.14 以降でビルドできますが、原則として、直近の macOS によるビルドを対象とします。

* brew にて gcc (本バージョンの確認は gcc-11, g++-11) をインストールしておいてください。
  ※ clang によるビルド定義も用意していますが、一部 C++ ライブラリが macOS 15 (Catalina) 以降からの対応となるため、それ以前の macOS ではビルドまたは動作が行えません。
* XCode に付属する make を利用できるようにしておいてください。
* examples/???/build ディレクトリ上で make を実行します。

```
make オプション
  MACOS_TARGET=X86_64_GCC -> Homebrew gcc-11,g++-11 用のビルド
  MACOS_TARGET=ARM64	  -> Apple Silicon 用のビルド
  DEBUG_BUILD=1           -> デバッグビルド
```



[osx/readme.txt](osx/readme.txt) も参照ください。



### Linux

Ubuntu16.04 18.04 20.04 でビルドできます。

* gcc (本バージョンの確認は gcc-11, g++-11) をインストールしておいてください。
* SDL2 (libsdl2-dev パッケージ) をインストールしておいてください。
* examples/???/build ディレクトリ上で make を実行します。

```
make オプション
  DEBUG_BUILD=1           -> デバッグビルド
```



[linux/readme.txt](linux/readme.txt) も参照ください。





## 添付ライブラリ

以下は、ビルド作業の簡素化と環境の一致のために外部プロジェクトの配布ライブラリ・ソースコードを添付しています。

* SDL2 ソース、ライブラリ
* FTDI D2XX ドライバ
* sqlite3, SQLiteCpp



### macOS

* ライブラリのコンパイラ

    * Intel版は homebrew よりインストールした gcc-9,g++-9 または gcc-11, g++-11 でビルド

    * Apple Silicon 版は clang でビルド



### Linux

* sndio (SDL2に依存しているがスタティックリンクできないため)
