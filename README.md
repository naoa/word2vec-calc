# word2vec-calc

word2vecで学習したモデルでベクトルの四則演算(+ - / *)を行います。

## 使い方
コンパイルします。

```bash
% git clone https://github.com/naoa/word2vec-calc.git
% cd word2vec-calc
% make
```

標準入力またはファイルから単語の演算式(+ - * /)を入力し、ベクトル演算します。  
単語1個の場合はdistance相当、単語2 - 単語1 + 単語3とするとword-analogy相当になります。たぶん100個ぐらいつなげられます。

```bash
% echo "データベース + 車" | ./word2vec-calc --file_path jpa_abst5.bin --o
utput 1
>
Word: データベース  Position in vocabulary: 1228

Word: 車  Position in vocabulary: 877
0.745027        中古車データベース
0.666922        情報格納装置
0.666620        自車位置情報取得部
0.666324        運行情報データベース
0.661899        自車位置取得手段
```

* 入力形式  
UTF8の文字コードのテキストのみ対応しています。

| 引数        | 説明       |デフォルト   |
|:-----------|:------------|:------------|
|--file_path|学習済みモデルファイル|<code>/var/lib/word2vec/learn.bin</code>|
|--input|単語の演算式(例：単語1 + 単語2 - 単語3) 指定なしで標準入力 標準入力の場合EOSで終了|標準入力
|--output|出力形式 1:単語,距離 2:単語 3:カンマ区切り 4:タブ区切り|1
|--offset|結果出力のオフセット|0
|--limit|結果出力の上限件数|-1(全て)
|--threshold|結果出力の閾値、1以下の小数を指定|
|--no_normalize|NFKC正規化+アルファベットの大文字小文字変換しない|
|--term_filter|出力をさせない単語にマッチする正規表現(完全一致)|
|--output_filter|出力結果から除去したい文字列の正規表現(全置換)|
| --h |オプションの説明||

* 出力結果  
ベクトル演算結果が標準出力に出力されます。

単語の演算式を標準入力またはファイルから1行ずつ実行するので、単語リストを
読み込ませれば、検索データベース用に同義語一覧を得ることもできます。

```bash
% cat data.csv | ./word2vec-calc --file_path jpa_abst5.bin --output 4 --limit 3 --threshold 0.75
筆記具  筆記具  ボールペン      筆記    万年筆
自動車  自動車  乗用車  オートバイ      車両
スマートフォン  スマートフォン  ＰＤＡ
円滑    円滑    スムーズ        スムース
```

## 依存関係
このプログラムでは、<code>ICU</code>、<code>gflags</code>のライブラリを利用しています。

CentOSではたとえば、以下のようにしてインストールできます。

```
% yum install -y icu libicu-devel
% rpm --import http://ftp.riken.jp/Linux/fedora/epel/RPM-GPG-KEY-EPEL
% yum localinstall -y http://ftp-srv2.kddilabs.jp/Linux/distributions/fedora/epel/6/x86_64/epel-release-6-8.noarch.rpm
% yum install -y re2 re2-devel
% yum install -y gflags gflags-devel
```

## Docker
string-splitterとword2vecとword2vec-calcを含むDockerファイルです。

https://github.com/naoa/docker-word2vec

## Author

Naoya Murakami naoya@createfield.com

## License

 Apache License 2.0
