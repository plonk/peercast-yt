# HTMLテンプレート

テンプレートタグは、開き波括弧 `{` で始まり、閉じ波括弧で `}` 終わりま
す。`{` に続く一文字の記号がタグの種類を表わしています。

<table>
<tr><td>^</td><td>マクロ</td></tr>
<tr><td>#</td><td>翻訳メッセージ</td></tr>
<tr><td>@</td><td>ディレクティブ、変数展開</td></tr>
<tr><td>!</td><td>変数展開(HTMLエスケープ無効)</td></tr>
<tr><td>\</td><td>変数展開(JavaScript文字列リテラル)</td></tr>
</table>

マクロ展開 `^` と翻訳メッセージ `#` の埋め込みは、`ui/macro-exapand`
スクリプトと、`ui/message-interpolate` スクリプトによってビルド時に行
われます。

その他のタグの処理は実行時に行われます。

# マクロタグの文法と意味

`macro-expand` コマンドは一度に1つの入力ファイルを処理します。この入力
ファイルはマクロタグによって、別のファイルを取り込んだり、また逆に入力
ファイルを別のファイル(レイアウトファイル)の一部として埋め込むことがで
きます。

例えば、`ui` ディレクトリで以下のコマンドラインを実行すると、
`html-master/index.html` を処理した後の結果が標準出力に印刷されます。

    $ ./macro-expand html-master/index.html

`index.html` の最初の行は `{^define LAYOUT layout.html}` となっており、
これはレイアウトファイルとして `Templates/layout.html` を利用すること
を意味しています。

`{^define 変数名 値}` の形式のタグはマクロ変数に値を設定します。
`{^define 変数名}値{^end}` のように書くこともできます。値を設定したマ
クロ変数は、`{^変数名}` の形式で展開することができます。

`{^include ファイル名}` の形式のタグは、`Templates/ファイル名` の内容
をその位置に展開します。

# 翻訳メッセージの挿入

HTML 中の自然言語のメッセージ(ラベルなど)は、`{#翻訳元メッセージ}` の
形式でタグ付けされ、`message-interpolate` スクリプトがこれをメッセージ
カタログの内容に従って、各国語に翻訳します。例えば：

    $ echo '{#_Information_navbar}' | ./message-interpolate -c catalogs/en.json
    Information
    $ echo '{#_Information_navbar}' | ./message-interpolate -c catalogs/ja.json
    情報

メッセージカタログは、`ui/catalogs/言語コード.json` にある JSON ファイ
ルです。このファイルは1つの JSON オブジェクトを含んでおり、キーが翻訳
元のメッセージ、値が翻訳後のメッセージになっています。

カタログファイルに翻訳元メッセージをキーとする項目が見付からない場合は、
翻訳元メッセージがそのまま出力されます。

     $ echo '{#nonexistent message}' | ./message-interpolate --verbose -c catalogs/ja.json
     Warning: translation for "nonexistent message" missing
     nonexistent message
