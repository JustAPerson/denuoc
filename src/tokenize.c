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

#include <ctype.h>
#include <string.h>

#include "vec.h"
#include "dcc.h"
#include "tokenize.h"

DEFINE_VEC_NEW(token, token)
DEFINE_VEC_PUSH(token, token)
DEFINE_VEC_FREE(token, token)

static bool starts_ident(char c) {
  return isalpha(c) || c == '_';
}

static const char* word_end(const char *input) {
  for (char c = *input; c && (isalnum(c) || c == '_'); input++, c = *input) {}
  return input;
}

static token_tag match_keyword(const char *input, int len) {
  static struct {
    char *value;
    token_tag tag;
  } KEYWORDS[] = {
    {"void", TOKEN_KEYWORD_VOID},
    {0, 0}
  };

  for (int i = 0; KEYWORDS[i].value; i++) {
    if (strncmp(input, KEYWORDS[i].value,len) == 0) {
      return KEYWORDS[i].tag;
    }
  }

  return TOKEN_UNKNOWN;
}

token_vec dcc_tokenize(const char *input) {
  token_vec tokens = token_vec_new();

  for (char c = *input; c; c = *input) {
    if (isspace(c)) {
      input++;
    } else  if (starts_ident(c)) {
      const char *begin = input, *end = word_end(input + 1);
      input = end;
      token_tag tag = match_keyword(begin, end - begin);

      if (tag == TOKEN_UNKNOWN) {
        tag = TOKEN_IDENT;
      }

      token token = { begin, end, tag };
      token_vec_push(&tokens, token);
    } else {
      dcc_ice("untokenizable character `%c`\n", c);
    }
  }

  return tokens;
}

void dcc_log_tokens(const token_vec *tokens) {
  for (int i = 0; i < tokens->size; i++) {
    token token = tokens->data[i];
    dcc_log(LOG_TRACE, "%s %.*s\n", dcc_token_tag_str(token.tag), (int)(token.end - token.begin), token.begin);
  }
}

char* dcc_token_tag_str(token_tag tag) {
  static char *STRINGS_OF_TOKENS[] = {
    "TOKEN_UNKNOWN",
    "TOKEN_EOF",
    "TOKEN_IDENT",
    "TOKEN_STRING",
    "TOKEN_KEYWORD_VOID",
  };

  if (tag >= TOKEN_MAX) {
    dcc_ice("invalid tag");
  }
  return STRINGS_OF_TOKENS[tag];
}
