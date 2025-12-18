## 一、基础语法

### 1. 字面量字符（直接匹配）
```
abc123
\\
\(
\]
\{
\.
\?
\+
\*
```

### 2. 字符类（匹配一类字符）

```
[abc]        匹配 a、b、c 中的任意一个
[^abc]       否定字符类，匹配除 a、b、c 外的任意字符
[a-z]        匹配 a-z 小写字母（区间）
[A-Z]        匹配 A-Z 大写字母
[0-9]        匹配 0-9 数字（等价于 \d）
[a-zA-Z0-9]  匹配字母 + 数字（等价于 \w）
[a-dm-p]     多区间合并，匹配 a-d 或 m-p
```

### 3. 预定义字符类（快捷方式）

```
.                          匹配任意字符（除换行符 \n，DOTALL 模式下包含换行）
\d (等价于[0-9])            匹配数字（0-9）
\D (等价于[^0-9])           匹配非数字
\w (等价于[a-zA-Z0-9_])     匹配单词字符（字母、数字、下划线 _，Unicode 模式下含中文等）
\W (等价于[^a-zA-Z0-9_])    匹配非单词字符
\s (等价于[ \t\n\r\f])      匹配空白字符（空格、制表符 \t、换行 \n、回车 \r 等）
\S (等价于[^ \t\n\r\f])     匹配非空白字符
\b                         单词边界（匹配单词开头 / 结尾，如 \bcat\b 匹配 "cat" 但不匹配 "category"）
\B                         非单词边界
\n                         换行符
\t                         制表符
```

### 4. 量词（匹配次数）

```
*        匹配 0 次或多次（贪婪：尽可能多匹配
+        匹配 1 次或多次（贪婪）
?        匹配 0 次或 1 次（贪婪）
{n}      精确匹配 n 次
{n,}     匹配 n 次或多次（贪婪）
{n,m}    匹配 n~m 次（贪婪）
*?       非贪婪（尽可能少匹配）
+?       非贪婪
??       非贪婪
{n,m}?   非贪婪
```

### 5. 边界匹配（锚点）

```
^    匹配字符串开头（多行模式下匹配行首）
$    匹配字符串结尾（多行模式下匹配行尾
\b   单词边界（匹配单词开头 / 结尾，如 \bcat\b 匹配 "cat" 但不匹配 "category"）
\B   非单词边界
```

### 6. 转义字符
```
\a         alarm, that is, the BEL character (hex 07)
\cx        "control-x", where x is a non-control ASCII character
\e         escape (hex 1B)
\f         form feed (hex 0C)
\n         newline (hex 0A)
\r         carriage return (hex 0D)
\t         tab (hex 09)
\uhhhh     character with Unicode code point hh..
\xhh       character with hex code hh
```

## 二、高级语法（复杂匹配场景）

### 1. 分组与捕获
```
(pattern)           匹配 pattern 并捕获结果，后续可通过反向引用或 API 获取
(?:pattern)         仅分组，不捕获结果（节省性能，无需后续引用时使用）
```

### 2. 选择匹配（或逻辑）
```
abc|def         用 | 分隔多个可选模式，匹配任意一个即可（优先级最低，需用分组限定范围）。
```

### 3. 正向预查（零宽断言）

```
(?=pattern)     正向先行断言：匹配后面紧跟 pattern 的位置
(?!pattern)     正向负向先行断言：匹配后面不紧跟 pattern 的位置
(?<=pattern)    正向后行断言：匹配前面是 pattern 的位置
(?<!pattern)    正向负向后行断言：匹配前面不是 pattern 的位置
```

## 三、示例

覆盖常用规则：
```
^(?:\\.|[^\^$\\.|?*+()\[\]{}]+|(?:\(\?[:=!<>]?)?\(.*?\)|\[.*?\]|\{[0-9,]*\}|[|^$*+?{}\[\]().\\])+$

^(?:\\w|\\d|\\s|\\b|[^\x00-\x7F]|[\u4e00-\u9fa5]|[a-zA-Z0-9_]|[^abc]|.){1,5}?(?:(ab)+|(?:cd)*|(\d{4}))(?=ef)(?!gh)(?<=ij)(?<!kl)\b\Z$

a1\(\)\[\]\{\}\.\?\+\*\\\\
|[a1.+*_-\\][^abc][a-z0-9A-Z]
|.\d\D\w\W\s\S\n\t
|a*b+c?d{2}e{1,}f{3,4}a*?b+?c??d{2}?e{1,}?f{3,4}?
|^\Banchor\b$
|\a\c1\e\f\n\r\t\v\0
|\cA[\ca-\cZ]\u9fa5[\u4e00-\u9fa5]\xAa[\x00-\xff]
|([a-z]\d)(?:\w\s+)
|a(?=followed_by_this)b(?!not_followed_by_this)c
|d(?<=preceded_by_this)e(?<!preceded_by_this)f
```


## 参考资料

- [PCRE标准](https://www.pcre.org/current/doc/html/pcre2syntax.html)