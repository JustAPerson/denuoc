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
