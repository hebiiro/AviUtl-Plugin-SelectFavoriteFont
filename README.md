![SelectFavoriteFont](https://user-images.githubusercontent.com/96464759/147817330-a17f2c76-f2eb-4a90-b3c4-d781f6de91e6.png)

# AviUtl プラグイン - お気に入りフォント選択

* version 5.1.0 by 蛇色 - 2022/01/23 コモンコントロール V6 に変更
* version 5.0.0 by 蛇色 - 2021/12/31 フォントプレビュー機能を追加
* version 4.0.0 by 蛇色 - 2021/12/31 ウィンドウの位置とサイズを固定しないように変更
* version 3.0.0 by 蛇色 - 2021/12/27 お気に入りコンボボックスをツリービューに変更
* version 2.0.0 by 蛇色 - 2021/12/25 簡易編集機能を追加
* version 1.1.0 by 蛇色 - 2021/12/25 爆速プラグインと共存できるように修正
* version 1.0.0 by 蛇色 - 2021/12/22 初版

テキストオブジェクトのオブジェクト編集ダイアログで、お気に入り、または最近使ったフォントからフォントを選択できるようにします。

## インストール

以下のファイルを AviUtl の Plugins フォルダに入れてください。
* SelectFavoriteFont.auf
* SelectFavoriteFont.xml

## 使い方

1. まずテキストオブジェクトのオブジェクト編集ダイアログで普通にフォントを選択します。
2. SelectFavoriteFont ウィンドウのコンボボックスにフォントが溜まっていくので、ある程度溜まったら一旦 AviUtl を終了します。
3. SelectFavoriteFont.xml をテキストエディタで編集します。
4. 1～3 を繰り返してお気に入りのフォントを構築していきます。

* 基本的に ```<recent>``` 内の ```<font>``` をカットして取り除き、```<favorite>``` 内にコピーして増やしていきます。

### 「最近使ったフォント」コンボボックスのコンテキストメニュー

コンボボックスを右クリックするとコンテキストメニューが表示されます。

* テキストオブジェクトで現在選択されているフォントを追加することができます。
* コンボボックスで現在選択されているフォントを削除することができます。

### 「お気に入りフォント」ツリービューのコンテキストメニュー

ツリービューを右クリックするとコンテキストメニューが表示されます。

* 「選択中の～の子要素として～を追加」選択中のアイテムの子要素としてフォントを追加することができます。
* 「選択中の～を削除」選択中のアイテムを削除することができます。
* 「選択中の～のフォント名を～で置き換える」で選択中のアイテムのフォント名をテキストオブジェクトで現在選択されているフォント名に置き換えることができます。
* 「選択中の～のフォント名を消去する」で選択中のアイテムに間違えて付与してしまったフォント名を消去できます。

## 設定

* ```<setting>```
	* ```x``` ```y``` ```w``` ```h``` コンテナウィンドウの位置とサイズを指定します。
	* ```labelFormat``` ラベルの文字列フォーマットを指定します。
	* ```separatorFormat``` セパレータの文字列フォーマットを指定します。
* ```<preview>```
	* ```enable``` "YES" を指定するとプレビュー機能が有効になります。
	* ```itemWidth``` プレビューウィンドウの幅を指定します。
	* ```itemHeight``` プレビューフォントの大きさを指定します。
	* ```textFormat``` プレビューの文字列フォーマットを指定します。
* ```<favorite>```
	* ```<font>``` 入れ子にできます。alias も name も指定されていない場合はセパレータになります。
		* ```alias``` 別名を指定します。表示用なので自由な文字列を指定できます。
		* ```name``` フォント名を指定します。フォント名が指定されているアイテムを選択すると、テキストオブジェクトのフォントが切り替わります。
		* ```expand``` "YES" を指定すると、初期状態で展開した状態になります。
* ```<recent>``` 最近使ったフォントが溜まっていきます。

## 動作確認

* (必須) AviUtl 1.10 & 拡張編集 0.92 http://spring-fragrance.mints.ne.jp/aviutl/
* (共存確認) patch.aul r8 https://www.nicovideo.jp/watch/sm39491708
* (共存確認) eclipse_fast 1.00 https://www.nicovideo.jp/watch/sm39756003
* (共存確認) 爆速プラグイン 1.06 https://www.nicovideo.jp/watch/sm39679253
