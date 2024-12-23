#include "zip.h"
#include <stdint.h>
#include <stdlib.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
    void *buf = NULL;
    size_t bufsize = 0;

    struct zip_t *zip = zip_stream_open((const char *)data, size, 0, 'r');
    if (NULL == zip)
    {
        goto end;
    }

    const ssize_t zip_entries_count = zip_entries_total(zip);

    if (zip_entries_count <= 0)
    {
        goto end;
    }

    if (0 != zip_entry_openbyindex(zip, 0))
    {
        goto end;
    }

    zip_entry_read(zip, &buf, &bufsize);

end:
    zip_entry_close(zip);
    if (NULL != zip)
    {
        zip_close(zip);
    }
    free(buf);
    return 0;
}
