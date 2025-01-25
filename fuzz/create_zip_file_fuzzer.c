#include "zip.h"
#include <stdint.h>
#include <stdlib.h>

int on_extract_entry(const char *filename, void *arg) {
    static int i = 0;
    int n = *(int *)arg;
    printf("Extracted: %s (%d of %d)\n", filename, ++i, n);
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
    struct zip_t *zip = zip_open("test.zip", ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
    zip_entry_open(zip, "test-1.txt");
    zip_entry_write(zip, data, size);
    zip_entry_close(zip);
    zip_close(zip);

    int arg = 2;
    zip_extract("test.zip", "./tmp", on_extract_entry, &arg);

    return 0;
}
