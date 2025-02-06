#include "zip.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size) {
  char *temp_name = tmpnam(NULL);
  FILE *file = fopen(temp_name, "wb");
  fwrite(data, size, 1, file);
  fclose(file);

  int n = strlen(temp_name);
  char *temp_zip_name = malloc(n + 5);
  memcpy(temp_zip_name, temp_name, n);
  temp_zip_name[n] = '.';
  temp_zip_name[n + 1] = 'z';
  temp_zip_name[n + 2] = 'i';
  temp_zip_name[n + 3] = 'p';
  temp_zip_name[n + 4] = '\0';
  const char *filenames[] = {temp_name};
  zip_create(temp_zip_name, filenames, 1);
  free(temp_zip_name);

  unlink(temp_name);
  unlink(temp_zip_name);
  return 0;
}
