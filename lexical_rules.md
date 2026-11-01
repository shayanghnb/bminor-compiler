# Formal Lexical Specification for the B-Minor Compiler Front-End

**Course:** Principles of Compiler Design (Fall 2026 / 1405)  
**Author:** Undergraduate Student Specification  
**Component:** Lexical Analyzer (Scanner)  
**Target Language:** B-Minor  

---

## 1. Overview & Architectural Role

The lexical analyzer (scanner) forms the initial stage of the B-Minor compiler front-end pipeline. Its primary mathematical abstraction is a Deterministic Finite Automaton (DFA) constructed from regular expressions defining the language's token vocabulary. 

The scanner ingests an unformatted stream of source characters $C = \langle c_1, c_2, \dots, c_n \rangle$ and maps valid lexemes into an ordered stream of typed token tuples:

$$\text{Token} = \langle \text{Type}, \text{Lexeme}, \text{Location} \rangle$$

where:
* **$\text{Type}$** denotes the syntactic category (e.g., `KW_IF`, `IDENT`, `INT_LITERAL`, `OP_LEQ`).
* **$\text{Lexeme}$** represents the exact matched character sequence from the source.
* **$\text{Location}$** is a coordinate tuple $\langle \text{file}, \text{line}, \text{column} \rangle$ corresponding to the first character of the lexeme, critical for downstream AST annotation, compiler diagnostics, and syntax highlighting.

Non-semantic elements (whitespace, comments) are recognized to update the source location tracker and are subsequently discarded unless requested by a documentation/formatting pipeline.

---

## 2. Token Categories & Formal Regular Expressions

### 2.1 Notation Conventions
* $\epsilon$: Empty string.
* $[a-z]$: Character class alternation from character $a$ to $z$.
* $[\text{^\dots}]$: Negated character class.
* $r*$: Kleene closure (zero or more repetitions).
* $r+$: Positive closure ($r r*$, one or more repetitions).
* $r?$: Optional occurrence ($\epsilon \mid r$).
* $r_1 \mid r_2$: Alternation.

---

### 2.2 Character Sets and Auxiliaries

```
Letter          = [a-zA-Z]
Digit           = [0-9]
HexDigit        = [0-9a-fA-F]
GraphicChar     = [ -~]  (* Printable ASCII characters, 0x20 through 0x7E *)
Newline         = \n | \r\n | \r
WhitespaceChar  = [ \t\v\f] | Newline
EscapeSeq       = \\ ( [ntvbrfa0\\'"] | 0x HexDigit HexDigit )
```

---

### 2.3 Token Category Specification

| Category | Token Name | Regular Expression / Lexical Rule | Examples | Description & Semantics |
| :--- | :--- | :--- | :--- | :--- |
| **Keywords** | `KW_ARRAY`<br>`KW_AUTO`<br>`KW_BOOLEAN`<br>`KW_CHAR`<br>`KW_ELSE`<br>`KW_FALSE`<br>`KW_FOR`<br>`KW_FUNCTION`<br>`KW_IF`<br>`KW_INTEGER`<br>`KW_PRINT`<br>`KW_RETURN`<br>`KW_STRING`<br>`KW_TRUE`<br>`KW_VOID`<br>`KW_WHILE` | `"array"`<br>`"auto"`<br>`"boolean"`<br>`"char"`<br>`"else"`<br>`"false"`<br>`"for"`<br>`"function"`<br>`"if"`<br>`"integer"`<br>`"print"`<br>`"return"`<br>`"string"`<br>`"true"`<br>`"void"`<br>`"while"` | `array`<br>`function`<br>`integer`<br>`return`<br>`while` | Reserved words in B-Minor. They cannot be used as user-defined identifiers. Checked before regular identifiers. |
| **Identifiers** | `IDENT` | `(Letter \| "_") (Letter \| Digit \| "_")*` | `factorial`<br>`_temp`<br>`buffer_size_2` | Variable, function, and parameter identifiers. Maximum supported length is 256 characters. |
| **Integer Literals** | `INT_LITERAL` | `Digit+` | `0`<br>`42`<br>`1048576` | Non-negative decimal integer constants (represented as signed 64-bit integers in B-Minor AST). Unary negative values are parsed at the expression level. |
| **Boolean Literals** | `BOOL_LITERAL` | `"true" \| "false"` | `true`<br>`false` | Boolean constant truth values. |
| **Character Literals** | `CHAR_LITERAL` | `' ( [^'\\\n\r] \| EscapeSeq ) '` | `'a'`<br>`'\n'`<br>`'\\'`<br>`'\0x1F'` | Single 8-bit ASCII character enclosed in single quotes. Must not contain raw unescaped newlines. |
| **String Literals** | `STRING_LITERAL` | `" ( [^"\\\n\r] \| EscapeSeq )* "` | `"hello\n"`<br>`"Result: "`<br>`""` | Null-terminated string constants. Max length of 1024 characters. Raw newlines inside strings are strictly prohibited. |
| **Arithmetic Operators** | `OP_PLUS`<br>`OP_MINUS`<br>`OP_MUL`<br>`OP_DIV`<br>`OP_MOD`<br>`OP_EXP`<br>`OP_INC`<br>`OP_DEC` | `"+"`<br>`"-"`<br>`"*"`<br>`"/"`<br>`"%"`<br>`"^"`<br>`"++"`<br>`"--"` | `+`<br>`-`<br>`*`<br>`/`<br>`%`<br>`^`<br>`++`<br>`--` | Addition, subtraction, multiplication, division, modulo, right-associative exponentiation, postfix increment, and postfix decrement. |
| **Relational Operators** | `OP_LT`<br>`OP_LEQ`<br>`OP_GT`<br>`OP_GEQ` | `"<"`<br>`"<="`<br>`">"`<br>`">="` | `<`<br>`<=`<br>`>`<br>`>=` | Less-than, less-than-or-equal, greater-than, greater-than-or-equal. |
| **Equality Operators** | `OP_EQ`<br>`OP_NEQ` | `"=="`<br>`"!="` | `==`<br>`!=` | Value equality and inequality comparisons. |
| **Logical Operators** | `OP_LAND`<br>`OP_LOR`<br>`OP_LNOT` | `"&&"`<br>`"\|\|"`<br>`"!"` | `&&`<br>`\|\|`<br>`!` | Short-circuit boolean AND, boolean OR, and boolean NOT. |
| **Assignment Operator** | `OP_ASSIGN` | `"="` | `=` | Variable assignment operator. |
| **Delimiters & Punctuation** | `LPAREN`<br>`RPAREN`<br>`LBRACKET`<br>`RBRACKET`<br>`LBRACE`<br>`RBRACE`<br>`COLON`<br>`SEMICOLON`<br>`COMMA` | `"("`<br>`")"`<br>`"["`<br>`"]"`<br>`"{"`<br>`"}"`<br>`":"`<br>`";"`<br>`","` | `(`<br>`)`<br>`[`<br>`]`<br>`{`<br>`}`<br>`:`<br>`;`<br>`,` | Lexical structural punctuation for parameter lists, array indexing, compound blocks, type declarations, statements, and argument separation. |
| **Comments** | `COMMENT` | `("//" [^\n\r]* )`<br>$\mid$<br>`("/*" ([^*] \| \*+ [^*/])* \*+ "/")` | `// line comment`<br>`/* block comment */` | Ignored by parser; maintains line and column tracking. Block comments do not nest. |
| **Whitespace** | `WHITESPACE` | `WhitespaceChar+` | ` ` `\t` `\n` | Skipped; line counter increments on `\n`. |
| **Error / Invalid** | `TOKEN_INVALID` | Any unmatched byte | `@`, `$`, `#` | Emitted when an input byte cannot be accepted by any valid DFA transition. |

---

## 3. Disambiguation Policies

When scanning an input string, regular expression definitions can overlap or produce conflicting matches. The B-Minor scanner adheres to two formal disambiguation rules.

### 3.1 Maximal Munch (Longest Match Rule)

#### Mathematical Formulation
Let the remaining unread input stream at position $i$ be $S_i = c_i c_{i+1} \dots c_n$. Let $L(R) = \bigcup_{k} L(R_k)$ be the language formed by the union of all token regular expressions.

The scanner selects the prefix $w = c_i \dots c_{i+m-1}$ such that:
1. $w \in L(R)$ (the prefix forms a valid token under at least one rule).
2. For all $m' > m$, the prefix $w' = c_i \dots c_{i+m'-1} \notin L(R)$ (no longer valid token prefix exists in the input).

#### DFA Implementation Mechanism
1. The lexer maintains two tracking pointers: `last_accepting_pos` and `current_pos`, alongside a state variable `last_accepting_token_type`.
2. As characters are consumed, the DFA transitions between states:
   $$\text{state}_{next} = \delta(\text{state}_{current}, c)$$
3. Whenever $\text{state}_{next} \in F$ (where $F$ is the set of accepting states), the lexer records:
   $$\text{last\_accepting\_pos} \leftarrow \text{current\_pos}$$
   $$\text{last\_accepting\_token\_type} \leftarrow \text{token}(\text{state}_{next})$$
4. When the DFA reaches a dead state ($\delta(\text{state}, c) = \emptyset$):
   * If `last_accepting_pos` is valid, the scanner rolls back the input stream to `last_accepting_pos`, outputs the token of type `last_accepting_token_type`, and resets the DFA.
   * If `last_accepting_pos` was never reached from the start position, a lexical error is triggered on $c_i$.

#### Practical Examples in B-Minor
* **Operator Ambiguity:** Given the input sequence `<=`, the scanner must emit a single `OP_LEQ` token rather than emitting `OP_LT` followed by `OP_ASSIGN`.
* **Postfix Increment vs. Addition:** Given `x++`, the scanner identifies `IDENT(x)` followed by `OP_INC (++)`, rather than two consecutive `OP_PLUS (+)` tokens.
* **Array Type vs. Identifiers:** In `array [10]`, `array` is matched as a single token rather than broken into prefixes `arr` or `a`.

---

### 3.2 Keyword-Over-Identifier Precedence

#### The Conflict
The regular expression for identifiers `[a-zA-Z_][a-zA-Z0-9_]*` matches any valid keyword in B-Minor (e.g., `return`, `function`, `integer`, `while`). Because both the keyword rule and the identifier rule match the exact same number of characters for a lexeme like `"return"` (length 6), the Maximal Munch rule alone cannot resolve this ambiguity.

#### Resolution Strategy
Keyword priority is enforced through one of two equivalent formal mechanisms:

1. **Rule Ordering in Unified Lexer Automata:**
   In Lex/Flex specifications or composite NFA-to-DFA conversions, the keyword regular expressions are listed *strictly prior* to the identifier regular expression. When an accepting state represents an equivalence class containing both a keyword token type and `IDENT`, the tie-breaker chooses the higher-priority rule:
   $$\text{Priority}(\text{KW\_RETURN}) > \text{Priority}(\text{IDENT})$$

2. **Symbol Table / Keyword Dictionary Lookup (Recommended for Hand-Written DFA):**
   * The DFA accepts all sequences conforming to `(Letter | "_")(Letter | Digit | "_")*` into a generic identifier state.
   * Before constructing the final token, the scanner queries a statically initialized hash table or prefix tree containing the 16 reserved keywords.
   * If the lexeme exists in the table:
     $$\text{Token.Type} \leftarrow \text{LookupKeywordTable}(\text{lexeme})$$
   * Otherwise:
     $$\text{Token.Type} \leftarrow \text{IDENT}$$

This eliminates state-space explosion in hand-crafted DFAs while guaranteeing $O(1)$ keyword disambiguation.

---

## 4. Lexical Error Detection & Recovery

The lexical analyzer must never terminate abnormally or enter an infinite loop when encountering ill-formed inputs. It reports diagnostics containing exact file coordinates and resumes scanning.

### 4.1 Invalid Characters
* **Condition:** A character $c$ does not participate in any outgoing DFA transition from the start state (e.g., `@`, `$`, `#`, `~`).
* **Action:** Emit a `TOKEN_INVALID` token containing lexeme $c$ and coordinate $\langle \text{line}, \text{col} \rangle$.
* **Recovery:** Advance the input pointer by one character ($i \leftarrow i + 1$) and reset the DFA to the initial state.

### 4.2 Unterminated String Literals
* **Condition:** A double quote `"` is opened, but a newline character (`\n`) or End-Of-File (`EOF`) is encountered before the closing double quote `"`.
* **Action:** Emit an error: `Lexical Error: Unterminated string literal at line L, col C`.
* **Recovery:** Consume characters up to the end of the current line, emit a `TOKEN_INVALID`, and resume scanning at the subsequent line.

### 4.3 Unterminated Block Comments
* **Condition:** A block comment delimiter `/*` is scanned, but the input stream terminates (`EOF`) without a matching `*/`.
* **Action:** Emit an error: `Lexical Error: Unterminated block comment starting at line L, col C`.
* **Recovery:** Emit `TOKEN_INVALID` or `TOKEN_EOF` and complete the lexical scan gracefully.

### 4.4 Illegal Escape Sequences
* **Condition:** Inside a character or string literal, a backslash `\` is followed by an invalid escape character (e.g., `\q` or `\0xGG`).
* **Action:** Report warning/error: `Invalid escape sequence '\q' at line L, col C`. The lexer falls back to treating the character literally or substitutes the Unicode replacement character.
