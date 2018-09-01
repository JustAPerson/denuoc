/*
  Copyright (C) 2018  Jason Priest

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
