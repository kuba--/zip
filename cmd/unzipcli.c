/*
 * unzipcli - extract zip archives from the command line
 *
 * Usage:
 *   unzipcli [-d dir] [-p password] [-l] [-o] [--] archive.zip [entry ...]
 *
 * Options:
 *   -d DIR     extract into DIR (default: current directory)
 *   -p PASS    decrypt with password
 *   -l         list entries instead of extracting
 *   -o         overwrite existing files without prompting
 *   -h         show help
 *   --         stop processing options
 *
 * If entry names are given after the archive, only those entries are
 * extracted (or listed). Otherwise all entries are processed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <zip.h>

/* Prints usage information to stderr. */
static void usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s [options] archive.zip [entry ...]\n"
          "\n"
          "Extract or list a zip archive.\n"
          "\n"
          "Options:\n"
          "  -d DIR     extract into DIR (default: .)\n"
          "  -p PASS    decrypt with password\n"
          "  -l         list entries only\n"
          "  -o         overwrite existing files\n"
          "  -h         show this help\n"
          "  --         stop processing options\n",
          prog);
}

/* Formats a byte count into a human-readable string (e.g. "1.5M"). */
static const char *size_human(unsigned long long bytes, char *buf,
                              size_t bufsz) {
  if (bytes >= 1024ULL * 1024 * 1024)
    snprintf(buf, bufsz, "%.1fG", (double)bytes / (1024.0 * 1024.0 * 1024.0));
  else if (bytes >= 1024ULL * 1024)
    snprintf(buf, bufsz, "%.1fM", (double)bytes / (1024.0 * 1024.0));
  else if (bytes >= 1024)
    snprintf(buf, bufsz, "%.1fK", (double)bytes / 1024.0);
  else
    snprintf(buf, bufsz, "%llu", bytes);
  return buf;
}

/* Returns 1 if name matches the entry filter list, or if no filter is set. */
static int should_process(const char *name, char *const *entries,
                          int nentries) {
  int i;
  if (nentries == 0)
    return 1;
  for (i = 0; i < nentries; i++) {
    if (strcmp(name, entries[i]) == 0)
      return 1;
  }
  return 0;
}

/* Recursively creates directories for the given path (like mkdir -p). */
static int mkdirs(const char *path) {
  char tmp[4096];
  char *p;
  size_t len;

  snprintf(tmp, sizeof(tmp), "%s", path);
  len = strlen(tmp);
  if (len > 0 && tmp[len - 1] == '/')
    tmp[len - 1] = '\0';

  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
#ifdef _WIN32
      _mkdir(tmp);
#else
      mkdir(tmp, 0755);
#endif
      *p = '/';
    }
  }
#ifdef _WIN32
  return _mkdir(tmp);
#else
  return mkdir(tmp, 0755);
#endif
}

/* Lists archive entries in a tabular format (size, compressed, CRC, name). */
static int list_archive(struct zip_t *zip, char *const *entries, int nentries) {
  ssize_t total = zip_entries_total(zip);
  ssize_t i;
  unsigned long long total_uncomp = 0, total_comp = 0;
  int count = 0;
  char sbuf[32];

  printf(" %-11s %-11s %-5s  %s\n", "Size", "Compressed", "CRC32", "Name");
  printf(" %-11s %-11s %-5s  %s\n", "-----------", "-----------", "--------",
         "----");

  for (i = 0; i < total; i++) {
    if (zip_entry_openbyindex(zip, (size_t)i) != 0)
      continue;

    const char *name = zip_entry_name(zip);
    if (!should_process(name, entries, nentries)) {
      zip_entry_close(zip);
      continue;
    }

    unsigned long long uncomp = zip_entry_size(zip);
    unsigned long long comp = zip_entry_comp_size(zip);
    unsigned int crc = zip_entry_crc32(zip);
    int isdir = zip_entry_isdir(zip);

    size_human(uncomp, sbuf, sizeof(sbuf));
    printf(" %11s", sbuf);
    size_human(comp, sbuf, sizeof(sbuf));
    printf(" %11s", sbuf);
    printf(" %08x", crc);
    printf("  %s%s\n", name, isdir ? "/" : "");

    total_uncomp += uncomp;
    total_comp += comp;
    count++;

    zip_entry_close(zip);
  }

  {
    char cbuf[32];
    size_human(total_comp, cbuf, sizeof(cbuf));
    printf(" %-11s %-11s          %d file%s\n",
           size_human(total_uncomp, sbuf, sizeof(sbuf)), cbuf, count,
           count == 1 ? "" : "s");
  }
  return 0;
}

/* Extracts matching entries to outdir, returns the number of files extracted.
 */
static int extract_archive(struct zip_t *zip, const char *outdir, int overwrite,
                           const char *password, char *const *entries,
                           int nentries) {
  ssize_t total = zip_entries_total(zip);
  ssize_t i;
  int count = 0;

  for (i = 0; i < total; i++) {
    if (zip_entry_openbyindex(zip, (size_t)i) != 0)
      continue;

    const char *name = zip_entry_name(zip);
    if (!should_process(name, entries, nentries)) {
      zip_entry_close(zip);
      continue;
    }

    int isdir = zip_entry_isdir(zip);

    char fullpath[4096];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", outdir, name);

    if (isdir) {
      mkdirs(fullpath);
      printf("   creating: %s\n", name);
    } else {
      /* ensure parent directory exists */
      char parent[4096];
      snprintf(parent, sizeof(parent), "%s", fullpath);
      char *slash = strrchr(parent, '/');
      if (slash) {
        *slash = '\0';
        mkdirs(parent);
      }

      if (!overwrite) {
        FILE *test = fopen(fullpath, "rb");
        if (test) {
          fclose(test);
          fprintf(stderr,
                  "  skipping: %s (already exists, use -o to "
                  "overwrite)\n",
                  name);
          zip_entry_close(zip);
          continue;
        }
      }

      if (password) {
        void *buf = NULL;
        size_t bufsize = 0;
        ssize_t n = zip_entry_read(zip, &buf, &bufsize);
        if (n < 0) {
          fprintf(stderr, "  error: %s: %s\n", name, zip_strerror((int)n));
          zip_entry_close(zip);
          continue;
        }
        FILE *fp = fopen(fullpath, "wb");
        if (!fp) {
          fprintf(stderr, "  error: cannot create '%s'\n", fullpath);
          free(buf);
          zip_entry_close(zip);
          continue;
        }
        fwrite(buf, 1, (size_t)n, fp);
        fclose(fp);
        free(buf);
      } else {
        if (zip_entry_fread(zip, fullpath) != 0) {
          fprintf(stderr, "  error: cannot extract '%s'\n", name);
          zip_entry_close(zip);
          continue;
        }
      }

      unsigned long long sz = zip_entry_size(zip);
      if (sz == 0)
        printf(" extracting: %s\n", name);
      else
        printf("  inflating: %s\n", name);
      count++;
    }

    zip_entry_close(zip);
  }

  return count;
}

int main(int argc, char *argv[]) {
  const char *outdir = ".";
  const char *password = NULL;
  const char *archive = NULL;
  int list_only = 0;
  int overwrite = 0;
  int i;
  int positional_start = -1;
  struct zip_t *zip = NULL;

  for (i = 1; i < argc; i++) {
    if (positional_start >= 0)
      break;
    if (strcmp(argv[i], "--") == 0) {
      positional_start = i + 1;
      break;
    }
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);
      return 0;
    }
    if (strcmp(argv[i], "-d") == 0) {
      if (++i >= argc) {
        fprintf(stderr, "Error: -d requires an argument\n");
        return 1;
      }
      outdir = argv[i];
    } else if (strcmp(argv[i], "-p") == 0) {
      if (++i >= argc) {
        fprintf(stderr, "Error: -p requires an argument\n");
        return 1;
      }
      password = argv[i];
    } else if (strcmp(argv[i], "-l") == 0) {
      list_only = 1;
    } else if (strcmp(argv[i], "-o") == 0) {
      overwrite = 1;
    } else if (argv[i][0] == '-') {
      fprintf(stderr, "Error: unknown option '%s'\n", argv[i]);
      usage(argv[0]);
      return 1;
    } else {
      positional_start = i;
      break;
    }
  }

  if (positional_start < 0 || positional_start >= argc) {
    fprintf(stderr, "Error: no archive specified\n");
    usage(argv[0]);
    return 1;
  }

  archive = argv[positional_start];

  /* remaining args after archive name are entry filters */
  int entry_start = positional_start + 1;
  int nentries = argc - entry_start;
  char **entry_filters = (nentries > 0) ? &argv[entry_start] : NULL;

  if (password) {
    zip = zip_open_with_password(archive, 0, 'r', password);
  } else {
    zip = zip_open(archive, 0, 'r');
  }
  if (!zip) {
    fprintf(stderr, "Error: cannot open '%s'\n", archive);
    return 1;
  }

  printf("Archive:  %s\n", archive);

  int ret;
  if (list_only) {
    ret = list_archive(zip, entry_filters, nentries);
  } else {
    mkdirs(outdir);
    ret = extract_archive(zip, outdir, overwrite, password, entry_filters,
                          nentries);
  }

  zip_close(zip);
  return (ret < 0) ? 1 : 0;
}
