# AviUtl プラグイン - お気に入りフォント選択

* version 2.0.0 by 蛇色 - 2021/12/25 簡易編集機能を追加
* version 1.1.0 by 蛇色 - 2021/12/25 爆速プラグインと共存できるように修正
* version 1.0.0 by 蛇色 - 2021/12/22 初版

テキストオブジェクトのオブジェクト編集ダイアログで、お気に入り、またはよく使うフォントからフォントを選択できるようにします。

## インストール

以下のファイルを AviUtl の Plugins フォルダに入れてください。
* SelectFavoriteFont.auf
* SelectFavoriteFont.popular.txt
* SelectFavoriteFont.favorite.txt

## 使い方

1. まずテキストオブジェクトのオブジェクト編集ダイアログで普通にフォントを選択します。
2. 段々その横のコンボボックスにフォントが溜まっていくので、ある程度溜まったら一旦 AviUtl を終了します。
3. SelectFavoriteFont.popular.txt をテキストエディタで開き、お気に入りのフォントをカットして取り除きます。
4. SelectFavoriteFont.favorite.txt をテキストエディタで開き、お気に入りのフォントをペーストして追加します。
5. 1～4 を繰り返してお気に入りのフォントを構築します。
6. フォント選択コンボボックスの左に生えているコンボボックスが、よく使うフォント、その下がお気に入りフォントになっているので、これらのコンボボックスを使ってフォントを選択します。

## コンテキストメニュー

コンボボックスを右クリックするとコンテキストメニューが表示されます。

* テキストオブジェクトで現在選択されているフォントを追加することができます。
* コンボボックスで現在選択されているフォントを削除することができます。

## 設定

現時点では設定で変更できる所はありません。

## 動作確認

* (必須) AviUtl 1.10 & 拡張編集 0.92 http://spring-fragrance.mints.ne.jp/aviutl/
* (共存確認) patch.aul r8 https://scrapbox.io/ePi5131/patch.aul
* (共存確認) eclipse_fast 1.00 https://www.nicovideo.jp/watch/sm39756003
* (共存確認) 爆速プラグイン 1.06 https://www.nicovideo.jp/watch/sm39679253
