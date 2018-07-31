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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dcc.h"
#include "tokenize.h"
#include "parse.h"


log_level active_log_level = LOG_TRACE;

static char* read_stdin() {
  char *output = 0;
  char buffer[512];
  size_t len = 0;

  while (!feof(stdin)) {
    size_t quantity = fread(buffer, 1, sizeof buffer, stdin);

    // TODO smarter growth strategy
    // +1 for null terminator
    output = dcc_realloc(output, len + quantity + 1);
    memcpy(output + len, buffer, quantity);
    len += quantity;
  }

  output[len] = 0;
  return output;
}

int main(int argc, char *argv[]) {
  char *input = read_stdin();
  token_vec_t tokens = dcc_tokenize(input);
  dcc_log_tokens(&tokens);

  dcc_parse(&tokens);

  return 0;
}
