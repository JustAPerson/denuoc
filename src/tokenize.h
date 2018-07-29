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

typedef enum {
  TOKEN_UNKNOWN,
  TOKEN_EOF,
  TOKEN_IDENT,
  TOKEN_STRING,
  TOKEN_INTEGER,
  TOKEN_REAL,
  TOKEN_KEYWORD_VOID,
  TOKEN_MAX,
} token_tag_t;

typedef union {
  uint64_t integer;
  double real;
  char *string;
} token_val_t;

typedef struct {
  const char *begin, *end;
  token_tag_t tag;
  token_val_t val;
} token_t;
DECLARE_VEC(token_t, token_vec)

token_vec_t dcc_tokenize(const char *input);
void dcc_log_tokens(const token_vec_t *tokens);
char* dcc_token_tag_str(token_tag_t tag);
