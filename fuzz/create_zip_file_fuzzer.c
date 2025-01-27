#include "zip.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
    char* temp_name = tmpnam(NULL);
    FILE* file = fopen(temp_name, "wb");
    fwrite(data, size, 1, file);
    fclose(file);

    char* temp_zip_name = tmpnam(NULL);
    const char *filenames[] = {temp_name};
    zip_create(temp_zip_name, filenames, 1);

    unlink(temp_name);
    unlink(temp_zip_name);
    return 0;
}
