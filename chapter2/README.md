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
