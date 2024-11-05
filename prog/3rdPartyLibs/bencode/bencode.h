/*
 * C implementation of a bencode decoder.
 * This is the format defined by BitTorrent:
 *  http://wiki.theory.org/BitTorrentSpecification#bencoding
 *
 * The only external requirements are a few [standard] function calls and
 * the long long type.  Any sane system should provide all of these things.
 *
 * This is released into the public domain.
 * Written by Mike Frysinger <vapier@gmail.com>.
 */

/* USAGE:
 *  - pass the string full of the bencoded data to be_decode()
 *  - parse the resulting tree however you like
 *  - call be_free() on the tree to release resources
 */

#ifndef _BENCODE_H
#define _BENCODE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  BE_STR,
  BE_INT,
  BE_LIST,
  BE_DICT,
} be_type;

struct be_dict;
struct be_node;

/*
 * XXX: the "val" field of be_dict and be_node can be confusing ...
 */

typedef struct be_dict {
  char *key;
  struct be_node *val;
} be_dict;

typedef struct be_node {
  be_type type;
  union {
    char *s;
    long long i;
    struct be_node **l;
    struct be_dict *d;
  } val;
} be_node;

extern long long be_str_len(be_node *node);
extern be_node *be_decode(const char *bencode);
extern be_node *be_decoden(const char *bencode, long long bencode_len);
extern void be_free(be_node *node);
extern void be_dump(be_node *node);

#ifdef __cplusplus
}
#endif

#endif
