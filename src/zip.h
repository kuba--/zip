/*
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#ifndef	ZIP_H
#define ZIP_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_PATH
#define MAX_PATH    32767    /* # chars in a path name including NULL */
#endif

#define ZIP_DEFAULT_COMPRESSION_LEVEL   6

/* This data structure is used throughout the library to represent zip archive - forward declaration. */
struct zip_t;

/*
  Opens zip archive with compression level.
  If append is 0 then new archive will be created, otherwise function will try to append to the specified zip archive,
  instead of creating a new one.
  Compression levels: 0-9 are the standard zlib-style levels.
  Returns pointer to zip_t structure or NULL on error.
*/
struct zip_t *zip_open(const char *zipname, int level, int append);

/* Closes zip archive, releases resources - always finalize. */
void zip_close(struct zip_t *zip);

/*
  Opens a new entry for writing in a zip archive.
  Returns negative number (< 0) on error, 0 on success.
*/
int zip_entry_open(struct zip_t *zip, const char *entryname);

/*
  Closes zip entry, flushes buffer and releases resources.
  Returns negative number (< 0) on error, 0 on success.
*/
int zip_entry_close(struct zip_t *zip);

/*
  Compresses an input buffer for the current zip entry.
  Returns negative number (< 0) on error, 0 on success.
*/
int zip_entry_write(struct zip_t *zip, const void *buf, size_t bufsize);

/*
  Compresses a file for the current zip entry.
  Returns negative number (< 0) on error, 0 on success.
*/
int zip_entry_fwrite(struct zip_t *zip, const char *filename);

/*
  Creates a new archive and puts len files into a single zip archive
  Returns negative number (< 0) on error, 0 on success.
*/
int zip_create(const char *zipname, const char *filenames[], size_t len);

/*
  Extracts a zip archive file into dir.
  If on_extract_entry is not NULL, the callback will be called after successfully extracted each zip entry.
  Returning a negative value from the callback will cause abort the extract and return an error.

  The last argument (void *arg) is optional, which you can use to pass some data to the on_extract_entry callback.

  Returns negative number (< 0) on error, 0 on success.
*/
int zip_extract(const char *zipname, const char *dir, int (* on_extract_entry)(const char *filename, void *arg), void *arg);

#ifdef __cplusplus
}
#endif

#endif



