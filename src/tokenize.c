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

static void free_token(token_t *token) {
  if (token->tag == TOKEN_IDENT) {
    free(token->val.string);
  }
}
DEFINE_VEC3(token_t, token_vec, free_token);

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

      token_t token = { tag, val, { begin, end } };
      token_vec_push(&tokens, token);
    } else if (issymb(c)) {
      static char *C_SYMBOL1_STRS[] = {
        "[", "]", "(", ")", "{",
        "}", ".", "&", "*", "+",
        "-", "~", "!", "/", "%",
        "<", ">", "^", "|", "?",
        ":", ";", "=", 0,
      };
      static char *C_SYMBOL2_STRS[] = {
        "->", "++", "--", "<<", ">>",
        ">=", "<=", "==", "!=", "&&",
        "||", "*=", "/=", "%=", "+=",
        "-=", "&=", "^=", "|=", 0,
      };
      static char *C_SYMBOL3_STRS[] = {
        "<<=", ">>=", "...", 0,
      };
      static token_tag_t C_SYMBOL1_TAGS[] = {
        TOKEN_LSQUARE, TOKEN_RSQUARE, TOKEN_LPAREN, TOKEN_RPAREN,
        TOKEN_LCURLY, TOKEN_RCURLY, TOKEN_DOT, TOKEN_AMP, TOKEN_STAR,
        TOKEN_PLUS, TOKEN_MINUS, TOKEN_SQUIGGLE, TOKEN_EXCLAIM, TOKEN_FORWARD,
        TOKEN_PERCENT, TOKEN_LESS, TOKEN_MORE, TOKEN_CARET, TOKEN_PIPE,
        TOKEN_QUEST, TOKEN_COLON, TOKEN_SEMI, TOKEN_EQUAL,
      };
      static token_tag_t C_SYMBOL2_TAGS[] = {
        TOKEN_ARROW, TOKEN_INCREMENT, TOKEN_DECREMENT, TOKEN_LEFT,
        TOKEN_RIGHT, TOKEN_MOREEQ, TOKEN_LESSEQ, TOKEN_EQEQ, TOKEN_NOTEQ,
        TOKEN_AMPAMP, TOKEN_PIPEPIPE, TOKEN_STAREQ, TOKEN_FORWARDEQ,
        TOKEN_PERCENTEQ, TOKEN_PLUSEQ, TOKEN_MINUSEQ, TOKEN_AMPEQ,
        TOKEN_CARETEQ, TOKEN_PIPEEQ,
      };
      static token_tag_t C_SYMBOL3_TAGS[] = {
        TOKEN_LEFTEQ, TOKEN_RIGHTEQ, TOKEN_ELLIPSE,
      };

      static char **C_SYMBOLS[] = {
        C_SYMBOL1_STRS,
        C_SYMBOL2_STRS,
        C_SYMBOL3_STRS,
      };
      static token_tag_t *C_SYMBOL_TAGS[] = {
        C_SYMBOL1_TAGS,
        C_SYMBOL2_TAGS,
        C_SYMBOL3_TAGS,
      };

      for (int l = 3; l > 0; l--) {
        char **symbols = C_SYMBOLS[l-1];
        for (int i = 0; symbols[i]; i++) {
          if (strncmp(input, symbols[i], l) == 0) {
            token_tag_t tag = C_SYMBOL_TAGS[l-1][i];
            token_t token = { tag, 0, { input, input + l}};
            token_vec_push(&tokens, token);

            input += l;
            goto matched;
          }
        }
      }
    matched:
      ;
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
        if (sscanf(input, "%lli", &integer) && (strtoul(input, &end2, 0), end2 >= end)) {
          tag = TOKEN_INTEGER;
          val.integer = integer;
        } else {
          val.real = floating;
        }

        token_t token = { tag, val, { input, end }};
        token_vec_push(&tokens, token);
      } else {
        dcc_ice("malformed number %.*s\n", malformed_token_end(input + 1) - input, input);
        // TODO is this even necessary
      }
      // TODO numeric suffixes
      input = end;
    } else {
      dcc_ice("untokenizable character `%c`\n", c);
    }
  }

  token_t token = { TOKEN_EOF, 0, { input, input }};
  token_vec_push(&tokens, token);

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
    dcc_log(LOG_TRACE, "%s \"%.*s\" extra=%s\n",
            dcc_token_tag_str(token.tag),
            (int)(token.span.end - token.span.begin),
            token.span.begin,
            extra);
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
    "TOKEN_LSQUARE",
    "TOKEN_RSQUARE",
    "TOKEN_LPAREN",
    "TOKEN_RPAREN",
    "TOKEN_LCURLY",
    "TOKEN_RCURLY",
    "TOKEN_DOT",
    "TOKEN_AMP",
    "TOKEN_STAR",
    "TOKEN_PLUS",
    "TOKEN_MINUS",
    "TOKEN_SQUIGGLE",
    "TOKEN_EXCLAIM",
    "TOKEN_FORWARD",
    "TOKEN_PERCENT",
    "TOKEN_LESS",
    "TOKEN_MORE",
    "TOKEN_CARET",
    "TOKEN_PIPE",
    "TOKEN_QUEST",
    "TOKEN_COLON",
    "TOKEN_SEMI",
    "TOKEN_EQUAL",
    "TOKEN_ARROW",
    "TOKEN_INCREMENT",
    "TOKEN_DECREMENT",
    "TOKEN_LEFT",
    "TOKEN_RIGHT",
    "TOKEN_MOREEQ",
    "TOKEN_LESSEQ",
    "TOKEN_EQEQ",
    "TOKEN_NOTEQ",
    "TOKEN_AMPAMP",
    "TOKEN_PIPEPIPE",
    "TOKEN_STAREQ",
    "TOKEN_FORWARDEQ",
    "TOKEN_PERCENTEQ",
    "TOKEN_PLUSEQ",
    "TOKEN_MINUSEQ",
    "TOKEN_AMPEQ",
    "TOKEN_CARETEQ",
    "TOKEN_PIPEEQ",
    "TOKEN_LEFTEQ",
    "TOKEN_RIGHTEQ",
    "TOKEN_ELLIPSE",
  };

  if (tag >= sizeof(STRINGS_OF_TOKENS) / sizeof(char*) ) {
    dcc_ice("invalid tag %d\n", tag);
  }
  return STRINGS_OF_TOKENS[tag];
}
