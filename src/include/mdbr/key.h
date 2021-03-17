#ifndef KEY_H
#define KEY_H

// TODO: change this to Datum and deal
// with this somehow
// the problem is that in current implementation we store everything
// (shard info, meta, shading keys) in shared memory, which means we have to know
// how much memory we need to use to store every key, since we cannot store pointers in
// shared memory (because pointer is anonther process prvate memory and access to this memory will lead us to
// segmentation fault)

typedef long long int mdbr_key_t;

extern int key_cmp(mdbr_key_t l, mdbr_key_t r);

#endif
