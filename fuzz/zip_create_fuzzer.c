#include "zip.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static char *get_tmp_file_name() {
  char *file_name = malloc(TMP_MAX);
  tmpnam(file_name);
  return file_name;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size) {
  char *file_name = get_tmp_file_name();
  char *zip_name = get_tmp_file_name();

  FILE *file = fopen(file_name, "wb");
  fwrite(data, size, 1, file);
  fclose(file);

  const char *filenames[] = {file_name};
  zip_create(zip_name, filenames, 1);

  unlink(file_name);
  unlink(zip_name);

  free(zip_name);
  free(file_name);
  return 0;
}
