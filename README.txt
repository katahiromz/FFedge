(Japanese, UTF-8)

# FFedge

## これは何？

これは、Microsoft Edgeブラウザを指定された位置とサイズで表示するためのWindows用のコマンドラインツールです。

アドレスバー、ツールバーのないEdgeブラウザを起動します。「-noborder」オプションを指定することにより、タイトルバーを消すことも可能です。

## 使い方

使用方法: FFedge [オプション] URL

オプション:
  -i URL                 インターネット上の位置またはHTMLファイルを指定します。
  -x 幅                  表示される幅を設定します。
  -y 高さ                表示される高さを設定します。
  -left 左               ウィンドウの左端のx位置を指定します（デフォルトは中央です）。
  -top 上                ウィンドウの上端のy位置を指定します（デフォルトは中央です）。
  -fs                    フルスクリーンモードで開始します。
  -noborder              枠なしウィンドウを作成します。
  -window_title タイトル ウィンドウタイトルを設定します（デフォルトはFFedgeです）。
  -help                  このヘルプメッセージを表示します。
  -version               バージョン情報を表示します。

## 対応環境

- (WebView2ランタイムのある) Windows 10 および Windows 11

## 注意

- タイトルバーがないときにウィンドウを閉じたい場合は、ウィンドウをクリックして Alt+F4を押してください。

## ライセンス

- MIT

## 連絡先

- 片山博文 <katayama.hirofumi.mz@gmail.com>

---
(English)

# FFedge

## What's this?

This is a command line tool for Windows to display the Microsoft Edge browser in a specified position and size.

This launches the Edge browser without the address bar and toolbar. You can also hide the title bar by specifying the "-noborder" option.

## Usage

Usage: FFedge [Options] URL

Options:
  -i URL                Specify an internet location or an HTML file.
  -x WIDTH              Set the displayed width.
  -y HEIGHT             Set the displayed height.
  -left LEFT            Specify the x position of the window's left edge
                        (default is centered).
  -top TOP              Specify the y position of the window's top edge
                        (default is centered).
  -fs                   Start in fullscreen mode.
  -noborder             Create a borderless window.
  -window_title TITLE   Set the window title (default is FFedge).
  -help                 Display this help message.
  -version              Display version information.

## Supported Environments

- Windows 10 and Windows 11 (with WebView2 runtime)

## Note

- If you want to close a window when it doesn't have a title bar, click on the window and press Alt+F4.

## License

- MIT

## Contact

- Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
