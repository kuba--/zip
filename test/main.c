#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zip.h>

// callback function
int on_extract_entry(const char *filename, void *arg) {
    static int i = 0;
    int n = *(int *)arg;
    printf("Extracted: %s (%d of %d)\n", filename, ++i, n);

    return 0;
}

int main() {
    /*
       Create a new zip archive with default compression level (6)
    */
    struct zip_t *zip = zip_open("foo.zip", ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
    // we should check if zip is NULL and if any other function returned < 0
    {
        zip_entry_open(zip, "foo-1.txt");
        {
            char *buf = "Some data here...";
            zip_entry_write(zip, buf, strlen(buf));
        }
        zip_entry_close(zip);

        zip_entry_open(zip, "foo-2.txt");
        {
            // merge 3 files into one entry and compress them on-the-fly.
            zip_entry_fwrite(zip, "foo-2.1.txt");
            zip_entry_fwrite(zip, "foo-2.2.txt");
            zip_entry_fwrite(zip, "foo-2.3.txt");
        }
        zip_entry_close(zip);
    }
    // always remember to close and release resources
    zip_close(zip);

    /*
        Append to existing zip archive
    */
    zip = zip_open("foo.zip", ZIP_DEFAULT_COMPRESSION_LEVEL, 'a');
    // we should check if zip is NULL
    {
        zip_entry_open(zip, "foo-3.txt");
        {
            char *buf = "Append some data here...";
            zip_entry_write(zip, buf, strlen(buf));
        }
        zip_entry_close(zip);
    }
    // always remember to close and release resources
    zip_close(zip);

    /*
        Extract the zip archive into /tmp folder
    */
    int arg = 2;
    zip_extract("foo.zip", "/tmp", on_extract_entry, &arg);

    /*
        ...or open the zip archive with only read access
     */
    void *buf = NULL;
    size_t bufsize;

    zip = zip_open("foo.zip", 0, 'r');
    // we should check if zip is NULL and if any other function returned < 0
    {
        zip_entry_open(zip, "foo-1.txt");
        {
            // extract into memory
            zip_entry_read(zip, &buf, &bufsize);
            printf("Read(foo-1.txt): %zu bytes: %.*s\n", bufsize, (int)bufsize,
                   buf);
        }
        zip_entry_close(zip);

        zip_entry_open(zip, "foo-2.txt");
        {
            // extract into a file
            zip_entry_fread(zip, "foo-2.txt");
        }
        zip_entry_close(zip);
    }
    // always remember to close and release resources
    zip_close(zip);

    // do something with buffer... and remember to free memory
    free(buf);

    return 0;
}
