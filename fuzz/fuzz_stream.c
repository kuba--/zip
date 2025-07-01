#include "zip.h"
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size) {
    /* Discard inputs larger than 1MB. */
    static const size_t MaxSize = 1024 * 1024;
    if (size < 1 || size > MaxSize) {
      return 0;
    }

    char *outbuf = NULL;
    size_t outbufsize = 0;
    {

        struct zip_t *zip = zip_stream_open(NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');

        zip_entry_open(zip, "test");
        zip_entry_write(zip, data, size);
        zip_entry_close(zip);
        zip_stream_copy(zip, (void **)&outbuf, &outbufsize);
        zip_stream_close(zip);
    }
    {
        void *inbuf = NULL;
        size_t inbufsize = 0;

        struct zip_t *zip = zip_stream_open(outbuf, outbufsize, 0, 'r');
        zip_entry_open(zip, "test");
        zip_entry_read(zip, &inbuf, &inbufsize);
        zip_entry_close(zip);
        free(inbuf);

        assert(inbufsize == size);
    }
    free(outbuf);

    return 0;
}
