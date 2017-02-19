#include <zip.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define ZIPNAME "test.zip"
#define TESTDATA1 "Some test data 1...\0"
#define TESTDATA2 "Some test data 2...\0"

static void test_write(void) {
    struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
    assert(zip != NULL);

    assert(0 == zip_entry_open(zip, "test-1.txt"));
    assert(0 == zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
    assert(0 == zip_entry_close(zip));

    zip_close(zip);
}

static void test_append(void) {
    struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'a');
    assert(zip != NULL);

    assert(0 == zip_entry_open(zip, "test-2.txt"));
    assert(0 == zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2)));
    assert(0 == zip_entry_close(zip));

    zip_close(zip);
}

static void test_read(void) {
    char *buf = NULL;
    size_t bufsize;
    struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
    assert(zip != NULL);

    assert(0 == zip_entry_open(zip, "test-1.txt"));
    assert(0 == zip_entry_read(zip, (void **)&buf, &bufsize));
    assert(bufsize == strlen(TESTDATA1));
    assert(0 == strncmp(buf, TESTDATA1, bufsize));
    assert(0 == zip_entry_close(zip));

    assert(0 == zip_entry_open(zip, "test-2.txt"));
    assert(0 == zip_entry_read(zip, (void **)&buf, &bufsize));
    assert(bufsize == strlen(TESTDATA2));
    assert(0 == strncmp(buf, TESTDATA2, bufsize));
    assert(0 == zip_entry_close(zip));

    zip_close(zip);
}

int main(int argc, char *argv[]) {
    test_write();
    test_append();
    test_read();

    return remove(ZIPNAME);
}
