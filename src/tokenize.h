/*
  Copyright (c) 2018 Jason Priest

  Permission is hereby  granted, free of charge, to any  person obtaining a copy
  of this software and associated  documentation files (the "Software"), to deal
  in the Software  without restriction, including without  limitation the rights
  to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
  copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
  IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
  FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
  AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
  LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/*
  Tokenizing. Turn input string into an array of tokens, which consist of
  pointers enclosing the text represented by the token and a tag.
  See Annex A of stdspec.
*/

#pragma once

#include <stdlib.h>
#include "vec.h"

typedef enum token_tag {
  TOKEN_UNKNOWN,
  TOKEN_EOF,
  TOKEN_IDENT,
  TOKEN_STRING,
  TOKEN_INTEGER,
  TOKEN_FLOATING,
  TOKEN_KEYWORD_AUTO,
  TOKEN_KEYWORD_BREAK,
  TOKEN_KEYWORD_CASE,
  TOKEN_KEYWORD_CHAR,
  TOKEN_KEYWORD_CONST,
  TOKEN_KEYWORD_CONTINUE,
  TOKEN_KEYWORD_DEFAULT,
  TOKEN_KEYWORD_DO,
  TOKEN_KEYWORD_DOUBLE,
  TOKEN_KEYWORD_ELSE,
  TOKEN_KEYWORD_ENUM,
  TOKEN_KEYWORD_EXTERN,
  TOKEN_KEYWORD_FLOAT,
  TOKEN_KEYWORD_FOR,
  TOKEN_KEYWORD_GOTO,
  TOKEN_KEYWORD_IF,
  TOKEN_KEYWORD_INLINE,
  TOKEN_KEYWORD_INT,
  TOKEN_KEYWORD_LONG,
  TOKEN_KEYWORD_REGISTER,
  TOKEN_KEYWORD_RESTRICT,
  TOKEN_KEYWORD_RETURN,
  TOKEN_KEYWORD_SHORT,
  TOKEN_KEYWORD_SIGNED,
  TOKEN_KEYWORD_SIZEOF,
  TOKEN_KEYWORD_STATIC,
  TOKEN_KEYWORD_STRUCT,
  TOKEN_KEYWORD_SWITCH,
  TOKEN_KEYWORD_TYPEDEF,
  TOKEN_KEYWORD_UNION,
  TOKEN_KEYWORD_UNSIGNED,
  TOKEN_KEYWORD_VOID,
  TOKEN_KEYWORD_VOLATILE,
  TOKEN_KEYWORD_WHILE,
  TOKEN_KEYWORD__BOOL,
  TOKEN_KEYWORD__COMPLEX,
  TOKEN_KEYWORD__IMAGINARY,
  TOKEN_LSQUARE,
  TOKEN_RSQUARE,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LCURLY,
  TOKEN_RCURLY,
  TOKEN_DOT,
  TOKEN_AMP,
  TOKEN_STAR,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_SQUIGGLE,
  TOKEN_EXCLAIM,
  TOKEN_FORWARD,
  TOKEN_PERCENT,
  TOKEN_LESS,
  TOKEN_MORE,
  TOKEN_CARET,
  TOKEN_PIPE,
  TOKEN_QUEST,
  TOKEN_COLON,
  TOKEN_SEMI,
  TOKEN_EQUAL,
  TOKEN_COMMA,
  TOKEN_ARROW,
  TOKEN_INCREMENT,
  TOKEN_DECREMENT,
  TOKEN_LEFT,
  TOKEN_RIGHT,
  TOKEN_MOREEQ,
  TOKEN_LESSEQ,
  TOKEN_EQEQ,
  TOKEN_NOTEQ,
  TOKEN_AMPAMP,
  TOKEN_PIPEPIPE,
  TOKEN_STAREQ,
  TOKEN_FORWARDEQ,
  TOKEN_PERCENTEQ,
  TOKEN_PLUSEQ,
  TOKEN_MINUSEQ,
  TOKEN_AMPEQ,
  TOKEN_CARETEQ,
  TOKEN_PIPEEQ,
  TOKEN_LEFTEQ,
  TOKEN_RIGHTEQ,
  TOKEN_ELLIPSE,

  TOKEN_MAX,
} token_tag_t;

typedef union {
  uint64_t integer;
  double floating;
  char *string;
} token_val_t;

typedef struct {
  const char *begin, *end;
} token_span_t;

typedef struct {
  token_tag_t tag;
  token_val_t val;
  token_span_t span;
} token_t;
DECLARE_VEC(token_t, token_vec)

token_vec_t dcc_tokenize(const char *input);
void dcc_log_tokens(const token_vec_t *tokens);
char* dcc_token_tag_str(token_tag_t tag);
