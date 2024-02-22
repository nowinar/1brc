#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_DISTINCT_GROUPS 10000
#define MAX_GROUPBY_KEY_LENGTH 100
#define HCAP (1 << 14)

#ifndef NTHREADS
#define NTHREADS 16
#endif

struct Group {
  unsigned int count;
  int min;
  int max;
  long sum;
  char key[MAX_GROUPBY_KEY_LENGTH];
};

struct Result {
  unsigned int n;
  unsigned int map[HCAP];
  unsigned long hashes[HCAP];
  struct Group groups[MAX_DISTINCT_GROUPS];
};

struct Chunk {
  size_t start;
  size_t end;
  char *data;
};

// parses a floating point number as an integer
// this is only possible because we know our data file has only a single decimal
__attribute((always_inline))
static inline char *parse_number(int *dest, char *s) {
  // parse sign
  int mod;
  if (*s == '-') {
    mod = -1;
    s++;
  } else {
    mod = 1;
  }

  if (s[1] == '.') {
    *dest = ((s[0] * 10) + s[2] - ('0' * 11)) * mod;
    return s + 4;
  }

  *dest = (s[0] * 100 + s[1] * 10 + s[3] - ('0' * 111)) * mod;
  return s + 5;
}

__attribute((always_inline))
static inline int min(int a, int b) { return a < b ? a : b; }

__attribute((always_inline))
static inline int max(int a, int b) { return a > b ? a : b; }

// qsort callback
int cmp(const void *ptr_a, const void *ptr_b) {
  return strcmp(((struct Group *)ptr_a)->key, ((struct Group *)ptr_b)->key);
}

// finds hash slot in map of key
static inline unsigned long hash_probe(struct Result *result, const char *key) {
  // hash key
  unsigned long h = (unsigned char)key[0];
  unsigned int len = 1;
  for (; key[len] != 0x0; len++) {
    h = (h * 31) + (unsigned char)key[len];
  }

  // linearly probe hashmap until match OR free spot
  while (result->hashes[h & (HCAP - 1)] != 0 &&
         h != result->hashes[h & (HCAP - 1)]) {
    h++;
  }

  return h;
}

static void *process_chunk(void *ptr) {
  struct Chunk *ch = (struct Chunk *)ptr;

  // skip start forward until SOF or after next newline
  if (ch->start > 0) {
    while (ch->data[ch->start - 1] != '\n') {
      ch->start++;
    }
  }

  while (ch->data[ch->end] != 0x0 && ch->data[ch->end - 1] != '\n') {
    ch->end++;
  }

  // initialize result
  struct Result *result = malloc(sizeof(*result));
  if (!result) {
    perror("malloc error");
    exit(EXIT_FAILURE);
  }
  result->n = 0;
  memset(result->hashes, 0, HCAP * sizeof(*result->hashes));
  memset(result->map, 0, HCAP * sizeof(*result->map));

  char *s = &ch->data[ch->start];
  char *end = &ch->data[ch->end];

  // flaming hot loop
  while (s != end) {
    char *linestart = s;

    // hash everything up to ';'
    // assumption: key is at least 1 char
    unsigned int len = 1;
    unsigned long h = (unsigned char)s[0];
    while (s[len] != ';') {
      h = (h * 31) + (unsigned char)s[len++];
    }

    // parse decimal number as int
    int temperature;
    s = parse_number(&temperature, s + len + 1);

    // probe map until free spot or match
    while (result->hashes[h & (HCAP - 1)] != 0 &&
           h != result->hashes[h & (HCAP - 1)]) {
      h++;
    }
    unsigned int c = result->map[h & (HCAP - 1)];

    if (c == 0) {
      /* new entry */
      unsigned int n = result->n;
      memcpy(result->groups[result->n].key, linestart, len);
      result->groups[n].key[len] = 0x0;
      result->groups[n].count = 1;
      result->groups[n].min = temperature;
      result->groups[n].max = temperature;
      result->groups[n].sum = temperature;
      result->hashes[h & (HCAP - 1)] = h;
      result->map[h & (HCAP - 1)] = n;
      result->n++;
    } else {
      /* existing entry */
      result->groups[c].count += 1;
      result->groups[c].min = min(result->groups[c].min, temperature);
      result->groups[c].max = max(result->groups[c].max, temperature);
      result->groups[c].sum += temperature;
    }
  }

  return (void *)result;
}

void result_to_str(char *dest, struct Result *result) {
  char buf[128];
  *dest++ = '{';
  for (unsigned int i = 0; i < result->n; i++) {
    size_t n = (size_t)snprintf(
        buf, 128, "%s=%.1f/%.1f/%.1f", result->groups[i].key,
        (float)result->groups[i].min / 10.0,
        ((float)result->groups[i].sum / (float)result->groups[i].count) / 10.0,
        (float)result->groups[i].max / 10.0);

    memcpy(dest, buf, n);
    if (i < result->n - 1) {
      memcpy(dest + n, ", ", 2);
      n += 2;
    }

    dest += n;
  }
  *dest++ = '}';
  *dest = 0x0;
}

int main(int argc, char **argv) {
  char *file = "measurements.txt";
  if (argc > 1) {
    file = argv[1];
  }

  int fd = open(file, O_RDONLY);
  if (!fd) {
    perror("error opening file");
    exit(EXIT_FAILURE);
  }

  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    perror("error getting file size");
    exit(EXIT_FAILURE);
  }

  // mmap entire file into memory
  size_t sz = (size_t)sb.st_size;
  char *data = mmap(NULL, sz, PROT_READ, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    perror("error mmapping file");
    exit(EXIT_FAILURE);
  }

  // distribute work among N worker threads
  pthread_t workers[NTHREADS];
  struct Chunk chunks[NTHREADS];
  size_t chunk_size = sz / (size_t)NTHREADS;
  for (unsigned int i = 0; i < NTHREADS; i++) {
    chunks[i].data = data;
    chunks[i].start = chunk_size * (size_t)i;
    chunks[i].end = chunk_size * ((size_t)i + 1);
    pthread_create(&workers[i], NULL, process_chunk, &chunks[i]);
  }

  // wait for all threads to finish
  struct Result *results[NTHREADS];
  for (unsigned int i = 0; i < NTHREADS; i++) {
    pthread_join(workers[i], (void *)&results[i]);
  }

  // merge results
  struct Group *b;
  struct Result *result = results[0];
  for (unsigned int i = 1; i < NTHREADS; i++) {
    for (unsigned int j = 0; j < results[i]->n; j++) {
      b = &results[i]->groups[j];
      unsigned long h = hash_probe(result, b->key);
      unsigned int c = result->map[h & (HCAP - 1)];
      if (c > 0) {
        result->groups[c].count += b->count;
        result->groups[c].sum += b->sum;
        result->groups[c].min = min(result->groups[c].min, b->min);
        result->groups[c].max = max(result->groups[c].max, b->max);
      } else {
        strcpy(result->groups[result->n].key, b->key);
        result->groups[result->n].count = b->count;
        result->groups[result->n].sum = b->sum;
        result->groups[result->n].min = b->min;
        result->groups[result->n].max = b->max;
        result->map[h & (HCAP - 1)] = result->n++;
        result->hashes[h & (HCAP - 1)] = h;
      }
    }
  }

  // sort results alphabetically
  qsort(result->groups, (size_t)result->n, sizeof(struct Group), cmp);

  // prepare output string
  char buf[(1 << 10) * 16];
  result_to_str(buf, result);
  puts(buf);

  // clean-up
  munmap((void *)data, sz);
  close(fd);
  for (unsigned int i = 0; i < NTHREADS; i++) {
    free(results[i]);
  }
  exit(EXIT_SUCCESS);
}
