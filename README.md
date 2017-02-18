### A portable (OSX/Linux/Windows), simple zip library written in C
This is done by hacking awesome [miniz](https://code.google.com/p/miniz) library and layering functions on top of the miniz v1.15 API.

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/d70fb0b050b74ef7aed70087e377e7d7)](https://www.codacy.com/app/kuba--/zip?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=kuba--/zip&amp;utm_campaign=Badge_Grade)

[![Windows][win-badge]][win-link] [![OS X][osx-linux-badge]][osx-linux-link]

[win-badge]: https://img.shields.io/appveyor/ci/kuba--/zip/master.svg?label=windows "AppVeyor build status"
[win-link]:  https://ci.appveyor.com/project/kuba--/zip "AppVeyor build status"
[osx-linux-badge]: https://img.shields.io/travis/kuba--/zip/master.svg?label=linux/osx "Travis CI build status"
[osx-linux-link]:  https://travis-ci.org/kuba--/zip "Travis CI build status"

# The Idea
<img src="zip.png" name="zip" />
... Some day, I was looking for zip library written in C for my project, but I could not find anything simple enough and lightweight.
Everything what I tried required 'crazy mental gymnastics' to integrate or had some limitations or was too heavy.
I hate frameworks, factories and adding new dependencies. If I must to install all those dependencies and link new library, I'm getting almost sick.
I wanted something powerfull and small enough, so I could add just a few files and compile them into my project.
And finally I found miniz.
Miniz is a lossless, high performance data compression library in a single source file. I only needed simple interface to append buffers or files to the current zip-entry. Thanks to this feature I'm able to merge many files/buffers and compress them on-the-fly.

It was the reason, why I decided to write zip module on top of the miniz. It required a little bit hacking and wrapping some functions, but I kept simplicity. So, you can grab these 3 files and compile them into your project. I hope that interface is also extremely simple, so you will not have any problems to understand it.

### Example (compress)

```c
#include <stdio.h>
#include <string.h>

#include <zip.h>

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

    return 0;
}
```

### Example (decompress)

```c
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
        Extract the zip archive into /tmp folder
    */
    int arg = 2;
    zip_extract("foo.zip", "/tmp", on_extract_entry, &arg);

    /*
        ...or open the zip archive with only read access
     */
    void *buf = NULL;
    size_t bufsize;

    struct zip_t *zip = zip_open("foo.zip", 0, 'r');
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
```


