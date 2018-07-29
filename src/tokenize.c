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
DEFINE_VEC_PUSH(token, token_t)
DEFINE_VEC_FREE(token, token_t)

static bool starts_ident(char c) {
  return isalpha(c) || c == '_';
}

static const char* word_end(const char *input) {
  for (char c = *input; c && (isalnum(c) || c == '_'); input++, c = *input) {}
  return input;
}

static const char* malformed_token_end(const char* input ) {
  for (char c = *input; c && !isspace(c); input++, c = *input) {}
  return input;
}

static token_tag_t match_keyword(const char *input, int len) {
  static struct {
    char *value;
    token_tag_t tag;
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

static bool issymb(char c) {
  static char C_SYMBOLS[] = {
    '!', '%', '&', '(', ')',
    '*', '+', ',', '-', '.',
    '/', ':', ';', '<', '=',
    '>', '?', '[', ']', '^',
    '{', '|', '}', '~',  0,
  };

  for (int i = 0; C_SYMBOLS[i]; i++) {
    if (C_SYMBOLS[i] == c) {
      return true;
    }
  }
  return false;
}

token_vec_t dcc_tokenize(const char *input) {
  token_vec_t tokens = token_vec_new();

  for (char c = *input; c; c = *input) {
    if (isspace(c)) {
      input++;
    } else  if (starts_ident(c)) {
      const char *begin = input, *end = word_end(input + 1);
      input = end;

      token_tag_t tag = match_keyword(begin, end - begin);
      token_val_t val = { 0 };

      if (tag == TOKEN_UNKNOWN) {
        tag = TOKEN_IDENT;
        val.string = dcc_malloc(end - begin + 1);
        strncpy(val.string, begin, end-begin);
        val.string[end-begin] = 0;
      }

      token_t token = { begin, end, tag, val };
      token_vec_push(&tokens, token);
    } else if (issymb(c)) {
      //  3 three len symbols
      // 19 two len symbols
      // 23 one len symbols
      /* static char *C_SYMBOLS3[] = {"<<=", ">>=", "...", }; */
      /* static char *C_SYMBOLS2[] = {"->", "++", "--", "<<", ">>", */
      /*                              ">=", "<=", "==", "!=", "&&", */
      /*                              "||", "*=", "/=", "%=", "+=", */
      /*                              "-=", "&=", "^=", "|=", }; */
      /* static char  C_SYMBOLS1[] = {'[', ']', '(', ')', '{', */
      /*                              '}', '.', '&', '*', '+', */
      /*                              '-', '~', '!', '/', '%', */
      /*                              '<', '>', '^', '|', '?', */
      /*                              ':', ';', '=', }; */
      dcc_nyi("symbol lexing");
    } else if (isdigit(c)) {
      uint64_t integer;
      double floating;

      char *end;
      if (sscanf(input, "%lf", &floating)) {
        double confirm = strtod(input, &end);
        dcc_assert(floating == confirm);

        // float may actually be decimal integer
        char *end2;
        token_tag_t tag = TOKEN_REAL;
        token_val_t val = { 0 };
        if (sscanf(input, "%llu", &integer) && (strtoul(input, &end2, 10), end2 == end)) {
          tag = TOKEN_INTEGER;
          val.integer = integer;
        } else {
          val.real = floating;
        }

        token_t token = {input, end, tag, val };
        token_vec_push(&tokens, token);
      } else if (sscanf(input, "%lli", &integer)) {
        uint64_t confirm = strtoull(input, &end, 0);
        dcc_assert(integer == confirm);

        token_t token = {input, end, TOKEN_INTEGER, { .integer = integer } };
        token_vec_push(&tokens, token);
      } else {
        dcc_ice("malformed number %.*s\n", malformed_token_end(input + 1) - input, input);
      }
      input = end;
    } else {
      dcc_ice("untokenizable character `%c`\n", c);
    }
  }

  return tokens;
}

void dcc_log_tokens(const token_vec_t *tokens) {
  VEC_FOREACH(token_t, token, tokens) {
    char buffer[32];
    char *extra = "<null>";

    if (token.tag == TOKEN_IDENT || token.tag == TOKEN_STRING) {
      extra = token.val.string;
    } else if (token.tag == TOKEN_INTEGER) {
      snprintf(buffer, sizeof buffer, "%llu", token.val.integer);
      extra = buffer;
    } else if (token.tag == TOKEN_REAL) {
      snprintf(buffer, sizeof buffer, "%f", token.val.real);
      extra = buffer;
    }
    dcc_log(LOG_TRACE, "%s \"%.*s\" extra=%s\n", dcc_token_tag_str(token.tag), (int)(token.end - token.begin), token.begin, extra);
  }
}

char* dcc_token_tag_str(token_tag_t tag) {
  static char *STRINGS_OF_TOKENS[] = {
    "TOKEN_UNKNOWN",
    "TOKEN_EOF",
    "TOKEN_IDENT",
    "TOKEN_STRING",
    "TOKEN_INTEGER",
    "TOKEN_REAL",
    "TOKEN_KEYWORD_VOID",
  };

  if (tag >= sizeof(STRINGS_OF_TOKENS) / sizeof(char*) ) {
    dcc_ice("invalid tag");
  }
  return STRINGS_OF_TOKENS[tag];
}
