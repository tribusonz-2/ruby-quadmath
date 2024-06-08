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

　Rubyで数値クラス定義はこのように書かれる．  

```
/* クラス定義．数値オブジェクトであるためrb_cNumericから継承する */
rb_cFloat128 = rb_define_class("Float128", rb_cNumeric);

/* クラスメソッド定義．ミュータブルではないのでクラス・アロケータとインスタンスはundefする */
rb_undef_alloc_func(rb_cFloat128);
rb_undef_method(CLASS_OF(rb_cFloat128), "new");
```

　振る舞いとしてはアロケータによって内部か外部かが決まる．設計は計算結果をオブジェクトにするなど，兼ねて考慮するべきだろう．

```
ミュータブル -> アロケーションは内部
イミュータブル -> アロケーションは外部
```

　実装は以下のような功罪を設計概念に持つようになる．  

```
// 加算のオブジェクト設計．Float128型同士の計算を想定する．

// NG
static VALUE
float128_add_coerce_f128(VALUE self, VALUE other)
{
	struct F128 *a = rb_check_typeddata(self, &float128_data_type);
	struct F128 *b = rb_check_typeddata(other, &float128_data_type);

	a->value += b->value; // NG．左辺オペランドのオブジェクトを更新している
	return self;
}

// OK
static VALUE
float128_add_coerce_f128(VALUE self, VALUE other)
{
	struct F128 *a = rb_check_typeddata(self, &float128_data_type);
	struct F128 *b = rb_check_typeddata(other, &float128_data_type);

	return rb_float128_cf128(a->value + b->value); // OK．C=A+Bの形．
}
```

　ここでは計算の定義をシンプルに示したが，四則演算するといっても，他のプリミティブ型がオペランドだったときの振る舞いもきちんと定義しなければならない．  
　数値計算でプリミティブ型というと，整数クラス・実数クラス・複素数クラスは最低限用意するべきである．実装では実数は整数の拡張，複素数は実数の拡張という位置づけになる．この拡張は，計算結果にも影響を及ぼす．  

```
a = 1; b = 2 # オペランドは整数同士
a / b #=> 0 # 整数として除算
a = 1; b = 2.0 # オペランドは整数と実数
a / b #=> 0.5 # 実数として除算
a = 1; b = 2.0+0i # オペランドは整数と複素数
a / b #=> (0.5+0i) # 複素数として除算
```

　これは整数と実数がオペランドにあった場合，計算結果は実数に，整数または実数と複素数がオペランドにあった場合，計算結果は複素数になる，という基本的な仕組みである．実装としてはプライオリティ{優先順位}という．  
　quadmathライブラリには四倍精度の複素数型である__complex128も用意されている．プライオリティとしては__float128型と計算されれば，当然__complex128型が返却される．  
　しかしRubyに用意される複素数型は，実部・虚部は同じ数値型とは定まっていない．内部で二変数はフック変数として用いられている．  

```
Complex(0,0.0) #=> (0+0.0i) # 整数と実数の混在な複素数
```

　このあたりの設計はよく見極めべきだろう．  
　では，型の混在するフック変数を相手に，どうやって演算しているのか．これは数値型に実装が決まっている演算を「メソッド呼び出し」することで可能にしている．  

```
複素数の演算ルーチン:
コール->[実部の演算メソッド, 虚部の演算メソッド]->結果の返却
```

　多くの場合はメソッド呼び出しによって機能が実現している．このあたりの設計は一通り定石と化しており，クラス定義はデフォルトのメソッドが定義された状態で行われる．また，数値クラス型が新しく定義される場合，Rubyでは実数を仮定している．例えば実数かどうかを評価するメソッド#real?では，Numeric型より継承した場合，デフォルトでtrueが返却されるようになっている．

```
Float128('0.0').real? #=> true
```

　__complex128型はfalseが返却されるべきなので，以下のように再定義する．

```
// 実体
static VALUE
complex128_real_p(VALUE self)
{
	return Qfalse;
}

// 定義
rb_define_method(rb_cComplex128, "real?", complex128_real_p, 0);
```

　この仮定は実数を目安にしているだけに，#to_fメソッドが定義されただけでずいぶんと振る舞いが充実してくる．例えば単項マイナスは定義されていない場合#to_fメソッドをメソッド呼び出しするので，以下のように振る舞う．よく評価して都度に再定義するべきである．  

```
-Float128::INFINITY #=> -Infinity
-Float128::INFINITY.class #=> Float
```

