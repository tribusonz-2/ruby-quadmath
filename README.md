# Quadmath for Ruby

Quadmath library for Ruby

Welcome to your new gem! In this directory, you'll find the files you need to be able to package up your 
Ruby library into a gem. Put your Ruby code in the file `lib/ruby/quadmath`. To experiment with that code, run `bin/console` for an interactive prompt.

## Installation

Install the gem and add to the application's Gemfile by executing:

```bash
bundle add quadmath
```

If bundler is not being used to manage dependencies, install the gem by executing:

```bash
gem install quadmath
```

## Usage

Require it below.  

```Ruby
require 'quadmath'
```

Along with the QuadMath module, you can use the Float128 class, which is an object of type __float128, and Complex128, which is an object of type __complex128.  
Objects are designed as Ruby primitives.  
It implements arithmetic operations and allows numerical calculations on primitive types.  

```Ruby
Float128('5') + Float128('6') # => 11.0
Complex128('1+1i') + Complex128('2+2i') # => (3.0+3.0i)
```

It also implements arithmetic with Ruby's primitive types.  

```Ruby
Float128('5') + 1 # => 6.0
Complex128('2+1i') * 2 # => (4.0+2.0i)
```

Numeric types are implemented to be mutually convertible according to Ruby specifications (e.g., `#to_r` for Rational classes). There are also `#to_f128` and `#to_c128` functions that convert Ruby's numeric primitive types to quadruple precision. These correspond to the Float128 and Complex128 types, respectively.  

```Ruby
1.to_f128 # => 1.0
1.to_c128 #=> (1.0+0.0i)
```

Rational also implements a conversion between them, which is useful because it handles them internally as quadruple precision and does not incur the loss of trailing digits that occurs with double precision.  

```Ruby
(1/3r).to_f128 # => 0.3333333333333333333333333333333333
```

Conversion from double precision to quad precision is also supported, although (as expected) there is a loss of trailing digits.  

```Ruby
(1/3r).to_f.to_f128 #=> 0.333333333333333314829616256247391
```

When calculating Complex types and Complex128 types, please note that the returned value will be a Complex type with Float128 type as a member.  

```Ruby
c = Complex128('1+2i') + Complex('2+2i') #=> (3.0+4.0i)
c.class # => Complex
```

However, Complex128 type cannot be compared.  

```Ruby
Complex128::I == Complex::I # => false
```

The QuadMath module is a combination of the Float128 and Complex128 types.  
The library function to be used varies depending on the argument type. For example, if the argument to #sqrt is Float128, sqrtq() is used, and if it is Complex128, csqrtq() is used.  

```Ruby
QuadMath.sqrt(Float128('1')) # => 1.0
QuadMath.sqrt(Complex128('1')) # => (1.0+0.0i)
```

If the function has branch cuts, it will be interpreted as a complex solution outside the real number domain.  

```Ruby
QuadMath.sqrt(-1) # => (0.0+1.0i)
```

### Specification

Library functions are wrapped to match the Ruby implementation. For example, `#inspect` converts strings using the function `ool_quad2str()`, which uses `quadmath_snprintf()`.  

```Ruby
QuadMath.sqrt(2) # => 1.4142135623730950488016887242096981
```

However, there are problems with accuracy. Can anyone tell me how to improve this?  

```Ruby
Float128('8') # => 8.0000000000000000000000000000000004
```

`#to_s` is also this implementation. It is based on Ruby notation.  

The `quadmath_snprintf()` function itself has a front end called `#quadmath_sprintf`. Note, however, that the method name is slightly different.  

```Ruby
quadmath_sprintf("%Qf", 2) # => "2.000000"
quadmath_sprintf("%Qf", 7/10r) # => "0.700000"
quadmath_sprintf("%.*Qf", Float128::DIG, 1/3r) # => "0.333333333333333333333333333333333"
quadmath_sprintf("%.*Qf", Float128::DIG, 1.0/3.0) # => "0.333333333333333314829616256247391"
quadmath_sprintf("%.*Qf", Float128::DIG, 1.to_f128 / 3) # => "0.333333333333333333333333333333333"
width = 46; prec = 20;
quadmath_sprintf("%+-#*.*Qe", width, prec, QuadMath.sqrt(2)) # => "+1.41421356237309504880e+00                   "
```

To create instances of primitive types, `Float128()` is used for Float128 types and `Complex128()` is used for Complex types, with the implementation wrapping `strtoflt128()` respectively.  

```Ruby
Float128('1.0') # => 1.0
Float128('0x2.0p+3') # => 16.0 # Hexadecimal support
Complex128('1+1i') # => (1.0+1.0i)
Complex128('2e1+3e1i') # => (20.0+30.0i)
```

A front end is also available for strtoflt128() alone.  
The second argument is treated as an option `sp`.  

```Ruby
strtoflt128('inf') # => Infinity
strtoflt128('-1') # => -1.0
strtoflt128('0xdeadbeef') # => 3735928559.0
sp = '' # => ""
strtoflt128('0+1i', sp: sp) # => 0.0
sp # => "+1i"
```

`#floor` `#ceil` `#round` `#truncate` are each library functions converted to Float128 type methods.  

```Ruby
Float128('8.5').floor # => 8
Float128('8.5').ceil # => 9
Float128('8.5').round # => 9
Float128('8.5').truncate # => 8
```

`nextafterq()` is used to implement `#next_float` and `#prev_float`.  

```Ruby
f128 = Float128(1.0) # => 1.0
f128.next_float # => 1.0000000000000000000000000000000002
f128.prev_float # => 0.9999999999999999999999999999999999
```

For other implementations, see `ext/quadmath` in the source code.  

### Lists

List of wrapped constants in the Float128 class  

| QuadMath Constants | Library           |
|:------------------:|:-----------------:|
| NAN                | nanq()            |
| INFINITY           | HUGE_VALQ         |
| MAX                | FLT128_MAX        |
| MIN                | FLT128_MIN        |
| EPSILON            | FLT128_EPSILON    |
| DENORM_MIN         | FLT128_DENORM_MIN |
| MANT_DIG           | FLT128_MANT_DIG   |
| MIN_EXP            | FLT128_MIN_EXP    |
| MAX_EXP            | FLT128_MAX_EXP    |
| DIG                | FLT128_DIG        |
| MIN_10_EXP         | FLT128_MIN_10_EXP |
| MAX_10_EXP         | FLT128_MAX_10_EXP |


List of wrapped functions in the QuadMath module  

| Method Name | Function for Real | Function for Complex |
|:-----------:|:-----------------:|:--------------------:|
| :exp        | expq()            | cexpq()              |
| :exp2       | exp2q()           | ---                  |
| :expm1      | expm1q()          | ---                  |
| :log        | logq()            | clogq()              |
| :log2       | log2q()           | ---                  |
| :log10      | log10q()          | clog10q()            |
| :log1p      | log1pq()          | ---                  |
| :sqrt       | sqrtq()           | csqrtq()             |
| :sqrt3      | cbrtq()           | ---                  |
| :cbrt       | cbrtq()           | ---                  |
| :sin        | sinq()            | csinq()              |
| :cos        | cosq()            | ccosq()              |
| :tan        | tanq()            | ctanq()              |
| :asin       | asinq()           | casinq()             |
| :acos       | acosq()           | cacosq()             |
| :atan       | atanq()           | catanq()             |
| :atan2      | atan2()           | ---                  |
| :quadrant   | atan2()           | ---                  |
| :sinh       | sinhq()           | csinhq()             |
| :cosh       | coshq()           | ccoshq()             |
| :tanh       | tanhq()           | ctanhq()             |
| :asinh      | asinhq()          | casinhq()            |
| :acosh      | acoshq()          | cacoshq()            |
| :atanh      | atanhq()          | catanhq()            |
| :hypot      | hypotq()          | ---                  |
| :erf        | erfq()            | ---                  |
| :erfc       | erfcq()           | ---                  |
| :lgamma     | lgammaq()         | ---                  |
| :lgamma_r   | lgammaq()         | ---                  |
| :signgam    | lgammaq()         | ---                  |
| :gamma      | tgammaq()         | ---                  |
| :j0         | j0q()             | ---                  |
| :j1         | j1q()             | ---                  |
| :jn         | jnq()             | ---                  |
| :y0         | y0q()             | ---                  |
| :y1         | y1q()             | ---                  |
| :yn         | ynq()             | ---                  |

List of wrapped constants in the QuadMath module  

| QuadMath Constants | Library Constants |
|:------------------:|:-----------------:|
| E                  | M_Eq              |
| LOG2E              | M_LOG2Eq          |
| LOG10E             | M_LOG10Eq         |
| LN2                | M_LN2q            |
| LN10               | M_LN10q           |
| PI                 | M_PIq             |
| PI_2               | M_PI_2q           |
| PI_4               | M_PI_4q           |
| ONE_PI             | M_1_PIq           |
| TWO_PI             | M_2_PIq           |
| TWO_SQRTPI         | M_2_SQRTPIq       |
| SQRT2              | M_SQRT2q          |
| SQRT1_2            | M_SQRT1_2q        |

## Development

After checking out the repo, run `bin/setup` to install dependencies. Then, run `rake test` to run the tests. You can also run `bin/console` for an interactive prompt that will allow you to experiment.

To install this gem onto your local machine, run `bundle exec rake install`. To release a new version, update the version number in `version.rb`, and then run `bundle exec rake release`, which will create a git tag for the version, push git commits and the created tag, and push the `.gem` file to [rubygems.org](https://rubygems.org).

## Contributing

Bug reports and pull requests are welcome on GitHub at https://github.com/tribusonz-2/ruby-quadmath. This project is intended to be a safe, welcoming space for collaboration, and contributors are expected to adhere to the [code of conduct](https://github.com/tribusonz-2/ruby-quadmath/blob/main/CODE_OF_CONDUCT.md).

## License

The gem is available as open source under the terms of the [MIT License](https://opensource.org/licenses/MIT).

## Code of Conduct

Everyone interacting in the Quadmath project's codebases, issue trackers, chat rooms and mailing lists is expected to follow the [code of conduct](https://github.com/tribusonz-2/ruby-quadmath/blob/main/CODE_OF_CONDUCT.md).
