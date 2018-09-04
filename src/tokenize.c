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
    {"auto", TOKEN_KEYWORD_AUTO},
    {"break", TOKEN_KEYWORD_BREAK},
    {"case", TOKEN_KEYWORD_CASE},
    {"char", TOKEN_KEYWORD_CHAR},
    {"const", TOKEN_KEYWORD_CONST},
    {"continue", TOKEN_KEYWORD_CONTINUE},
    {"default", TOKEN_KEYWORD_DEFAULT},
    {"do", TOKEN_KEYWORD_DO},
    {"double", TOKEN_KEYWORD_DOUBLE},
    {"else", TOKEN_KEYWORD_ELSE},
    {"enum", TOKEN_KEYWORD_ENUM},
    {"extern", TOKEN_KEYWORD_EXTERN},
    {"float", TOKEN_KEYWORD_FLOAT},
    {"for", TOKEN_KEYWORD_FOR},
    {"goto", TOKEN_KEYWORD_GOTO},
    {"if", TOKEN_KEYWORD_IF},
    {"inline", TOKEN_KEYWORD_INLINE},
    {"int", TOKEN_KEYWORD_INT},
    {"long", TOKEN_KEYWORD_LONG},
    {"register", TOKEN_KEYWORD_REGISTER},
    {"restrict", TOKEN_KEYWORD_RESTRICT},
    {"return", TOKEN_KEYWORD_RETURN},
    {"short", TOKEN_KEYWORD_SHORT},
    {"signed", TOKEN_KEYWORD_SIGNED},
    {"sizeof", TOKEN_KEYWORD_SIZEOF},
    {"static", TOKEN_KEYWORD_STATIC},
    {"struct", TOKEN_KEYWORD_STRUCT},
    {"switch", TOKEN_KEYWORD_SWITCH},
    {"typedef", TOKEN_KEYWORD_TYPEDEF},
    {"union", TOKEN_KEYWORD_UNION},
    {"unsigned", TOKEN_KEYWORD_UNSIGNED},
    {"void", TOKEN_KEYWORD_VOID},
    {"volatile", TOKEN_KEYWORD_VOLATILE},
    {"while", TOKEN_KEYWORD_WHILE},
    {"_Bool", TOKEN_KEYWORD__BOOL},
    {"_Complex", TOKEN_KEYWORD__COMPLEX},
    {"_Imaginary", TOKEN_KEYWORD__IMAGINARY},
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
        ":", ";", "=", ",", 0,
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
        TOKEN_QUEST, TOKEN_COLON, TOKEN_SEMI, TOKEN_EQUAL, TOKEN_COMMA,
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
            token_t token = { tag, { 0 }, { input, input + l}};
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

      char *end = 0;
      if (sscanf(input, "%lf", &floating)) {
        double confirm = strtod(input, &end);
        dcc_assert(floating == confirm);

        // float may actually be decimal integer
        char *end2;
        token_tag_t tag = TOKEN_FLOATING;
        token_val_t val = { 0 };
        if (sscanf(input, "%lli", &integer) && (strtoul(input, &end2, 0), end2 >= end)) {
          tag = TOKEN_INTEGER;
          val.integer = integer;
        } else {
          val.floating = floating;
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

  token_t token = { TOKEN_EOF, { 0 }, { input, input }};
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
    } else if (token.tag == TOKEN_FLOATING) {
      snprintf(buffer, sizeof buffer, "%f", token.val.floating);
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
    "TOKEN_KEYWORD_AUTO",
    "TOKEN_KEYWORD_BREAK",
    "TOKEN_KEYWORD_CASE",
    "TOKEN_KEYWORD_CHAR",
    "TOKEN_KEYWORD_CONST",
    "TOKEN_KEYWORD_CONTINUE",
    "TOKEN_KEYWORD_DEFAULT",
    "TOKEN_KEYWORD_DO",
    "TOKEN_KEYWORD_DOUBLE",
    "TOKEN_KEYWORD_ELSE",
    "TOKEN_KEYWORD_ENUM",
    "TOKEN_KEYWORD_EXTERN",
    "TOKEN_KEYWORD_FLOAT",
    "TOKEN_KEYWORD_FOR",
    "TOKEN_KEYWORD_GOTO",
    "TOKEN_KEYWORD_IF",
    "TOKEN_KEYWORD_INLINE",
    "TOKEN_KEYWORD_INT",
    "TOKEN_KEYWORD_LONG",
    "TOKEN_KEYWORD_REGISTER",
    "TOKEN_KEYWORD_RESTRICT",
    "TOKEN_KEYWORD_RETURN",
    "TOKEN_KEYWORD_SHORT",
    "TOKEN_KEYWORD_SIGNED",
    "TOKEN_KEYWORD_SIZEOF",
    "TOKEN_KEYWORD_STATIC",
    "TOKEN_KEYWORD_STRUCT",
    "TOKEN_KEYWORD_SWITCH",
    "TOKEN_KEYWORD_TYPEDEF",
    "TOKEN_KEYWORD_UNION",
    "TOKEN_KEYWORD_UNSIGNED",
    "TOKEN_KEYWORD_VOID",
    "TOKEN_KEYWORD_VOLATILE",
    "TOKEN_KEYWORD_WHILE",
    "TOKEN_KEYWORD__BOOL",
    "TOKEN_KEYWORD__COMPLEX",
    "TOKEN_KEYWORD__IMAGINARY",
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
    "TOKEN_COMMA",
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
