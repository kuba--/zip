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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_PATH
#define MAX_PATH    32767    /* # chars in a path name including NULL */
#endif

#define ZIP_DEFAULT_COMPRESSION_LEVEL   6

// This data structure is used throughout the library to represent zip archive - forward declaration.
typedef struct zip_t zip_t;

// Opens zip archive with compression level.
// If add is 0 then new archive will be created,
// otherwise function will try to open existing archive to add data.
// Compression levels: 0-9 are the standard zlib-style levels.
// Returns pointer to zip_t structure or NULL on error.
zip_t *zip_open(const char *zipname, int level, int add);

// Closes zip archive, releases resources - always finalize.
void zip_close(zip_t *zip);

// Opens a new entry for writing in a zip archive.
// Returns 0 or -1 on error.
int zip_entry_open(zip_t *zip, const char *entryname);

// Closes zip entry, flushes buffer and releases resources.
// Returns 0 or -1 on error.
int zip_entry_close(zip_t *zip);

// Compresses an input buffer for the current zip entry.
// Returns 0 or -1 on error.
int zip_entry_write(zip_t *zip, const void *buf, size_t bufsize);

// Compresses a file for the current zip entry.
// Returns 0 or -1 on error.
int zip_entry_fwrite(zip_t *zip, const char *filename);

// Puts len files into a single zip archive
// Returns 0 or -1 on error.
int zip_create(zip_t *zip, const char *filenames[], size_t len);

// Extracts a zip archive file into dir.
// Returns 0 or -1 on error.
int zip_extract(const char *zipname, const char *dir, int (* on_extract)(const char *filename, void *arg), void *arg);

#ifdef __cplusplus
}
#endif

#endif



