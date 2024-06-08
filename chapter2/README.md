# quadmathフロントエンド開発
## チャプター2: 評価と型変換

　C/C++での設計概念とオブジェクト指向の設計概念は一致することはあまりない．そのためC ABIはオブジェクト指向用の設計となると，ライブラリ関数をそのまま持ち込むのは運用面に難儀がある．これを解決するには，ライブラリ関数のラップ(ラッパー関数とも呼ばれる)が必要である．  
　前章のABI関数であるool_quad2str()は，ずいぶんとこの問題点を捉えている．ライブラリとブリッジして，機能を十分に果たしている．  
　以下は利用例である．この利用例はStringクラスへ型変換するのに進数{base number}に応じてool_quad2str()が振る舞う．ここではオーバーヘッドの最短を目指したため，コードはCに精通していないと高度に見えるかもしれない．(ポインタはもっと難しい)  


```
switch (base_number) {
case 2:
	if (isinfq(f128->value) || isnanq(f128->value))
		goto to_s_none_finite;
	if ('0' == ool_quad2str(f128->value, 'b', &exp, &sign, &str))
		goto to_s_invalid_format;
	if (sign == -1)
		return rb_sprintf("-%se%+d", str, exp);
	else
		return rb_sprintf("%se%+d", str, exp);
	break;
case 10:
	if (isinfq(f128->value) || isnanq(f128->value))
		goto to_s_none_finite;
	if ('0' == ool_quad2str(f128->value, 'e', &exp, &sign, &str))
		goto to_s_invalid_format;
	if (sign == -1)
		return rb_sprintf("-%se%+d", str, exp);
	else
		return rb_sprintf("%se%+d", str, exp);
	break;
case 16:
	if (isinfq(f128->value) || isnanq(f128->value))
		goto to_s_none_finite;
	if ('0' == ool_quad2str(f128->value, 'a', &exp, &sign, &str))
		goto to_s_invalid_format;
	if (sign == -1)
		return rb_sprintf("-%sp%+d", str, exp);
	else
		return rb_sprintf("%sp%+d", str, exp);
	break;
default:
	break;
}
```

　このABI関数を組み込むところ，ライブラリの実体には型変換メソッドに適用されていることが分かる．ということは，組み込み段階ではまだ評価の域を超えていない．また，本格的に評価するとなると，数値入力のため別途パーサが必要になる．しかし，ではパーサも用意としようとなると他にABI関数を用意することになる．実装段階では，下手に拡張するよりも評価に留めておくのがよいだろう．  

　基本的に理解すべきところとして，オブジェクト指向では数値型を評価するとなると，「ミュータブルなオブジェクト」と「イミュータブルなオブジェクト」という2つの性質に出くわす．これはオブジェクトの実体が可変型かどうかということである．ミュータブルは実体は変更可能であり，イミュータブルは実体は変更不可能である．数値型はイミュータブル型に属する．  
　Rubyの構造体では，ミュータブルなオブジェクトが基本のストラクチャのようである．数値型はイミュータブルなため，数値計算の拡張ライブラリを作る際は，クラス定義は内部を通さず外部で取り回すように設計するのが主流のようである．  

　Rubyでクラス定義はこのように書かれる．  

```
/* クラス定義．数値オブジェクトであるためrb_cNumericから継承する */
rb_cFloat128 = rb_define_class("Float128", rb_cNumeric);

/* クラスメソッド定義．ミュータブルではないのでクラス・アロケータとインスタンスはundefする */
rb_undef_alloc_func(rb_cFloat128);
rb_undef_method(CLASS_OF(rb_cFloat128), "new");
```
