/*
 * Copyright (c) 1998 Robert Nordier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "mkfs_msdos.h"

#define argto1(arg, lo, msg) argtou(arg, lo, 0xff, msg)
#define argto2(arg, lo, msg) argtou(arg, lo, 0xffff, msg)
#define argto4(arg, lo, msg) argtou(arg, lo, 0xffffffff, msg)
#define argtox(arg, lo, msg) argtou(arg, lo, UINT_MAX, msg)

static unsigned int argtou(const char*, unsigned int, unsigned int, const char*);
static off_t argtooff(const char*, const char*);
static void usage(void);

/*
 * Construct a FAT12, FAT16, or FAT32 file system.
 */
int main(int argc, char* argv[]) {
    static const char opts[] = "@:NB:C:F:I:L:O:S:a:b:c:e:f:h:i:k:m:n:o:r:s:u:";
    struct msdos_options o;
    const char* fname;
    char buf[MAXPATHLEN];
    int ch;

    memset(&o, 0, sizeof(o));

    while ((ch = getopt(argc, argv, opts)) != -1)
        switch (ch) {
        case '@':
            o.offset = argtooff(optarg, "offset");
            break;
        case 'N':
            o.no_create = 1;
            break;
        case 'B':
            o.bootstrap = optarg;
            break;
        case 'C':
            o.create_size = argtooff(optarg, "create size");
            break;
        case 'F':
            if (strcmp(optarg, "12") && strcmp(optarg, "16") &&
                strcmp(optarg, "32"))
                errx(1, "%s: bad FAT type", optarg);
            o.fat_type = atoi(optarg);
            break;
        case 'I':
            o.volume_id = argto4(optarg, 0, "volume ID");
            o.volume_id_set = 1;
            break;
        case 'L':
            o.volume_label = optarg;
            break;
        case 'O':
            o.OEM_string = optarg;
            break;
        case 'a':
            o.sectors_per_fat = argto4(optarg, 1, "sectors/FAT");
            break;
        case 'b':
            o.block_size = argtox(optarg, 1, "block size");
            o.sectors_per_cluster = 0;
            break;
        case 'c':
            o.sectors_per_cluster = argto1(optarg, 1, "sectors/cluster");
            o.block_size = 0;
            break;
        case 'e':
            o.directory_entries = argto2(optarg, 1, "directory entries");
            break;
        case 'i':
            o.info_sector = argto2(optarg, 1, "info sector");
            break;
        case 'k':
            o.backup_sector = argto2(optarg, 1, "backup sector");
            break;
        case 'm':
            o.media_descriptor = argto1(optarg, 0, "media descriptor");
            o.media_descriptor_set = 1;
            break;
        case 'n':
            o.num_FAT = argto1(optarg, 1, "number of FATs");
            break;
        case 'r':
            o.reserved_sectors = argto2(optarg, 1, "reserved sectors");
            break;
        default:
            usage();
        }
    argc -= optind;
    argv += optind;
    if (argc != 1)
        usage();
    fname = *argv++;
    if (!o.create_size && !strchr(fname, '/')) {
        snprintf(buf, sizeof(buf), "%s%s", _PATH_DEV, fname);
        if (!(fname = strdup(buf)))
            err(1, NULL);
    }
    return !!mkfs_msdos(fname, &o);
}

/*
 * Convert and check a numeric option argument.
 */
static unsigned int argtou(const char* arg, unsigned int lo, unsigned int hi, const char* msg) {
    char* s;
    u_long x;

    errno = 0;
    x = strtoul(arg, &s, 0);
    if (errno || !*arg || *s || x < lo || x > hi)
        errx(1, "%s: bad %s", arg, msg);
    return x;
}

/*
 * Same for off_t, with optional skmgpP suffix
 */
static off_t argtooff(const char* arg, const char* msg) {
    char* s;
    off_t x;

    errno = 0;
    x = strtoll(arg, &s, 0);
    /* allow at most one extra char */
    if (errno || x < 0 || (s[0] && s[1]))
        errx(1, "%s: bad %s", arg, msg);
    if (*s) { /* the extra char is the multiplier */
        switch (*s) {
        default:
            errx(1, "%s: bad %s", arg, msg);
        /* notreached */

        case 's': /* sector */
        case 'S':
            x <<= 9; /* times 512 */
            break;

        case 'k': /* kilobyte */
        case 'K':
            x <<= 10; /* times 1024 */
            break;

        case 'm': /* megabyte */
        case 'M':
            x <<= 20; /* times 1024*1024 */
            break;

        case 'g': /* gigabyte */
        case 'G':
            x <<= 30; /* times 1024*1024*1024 */
            break;

        case 'p': /* partition start */
        case 'P':
        case 'l': /* partition length */
        case 'L':
            errx(1, "%s: not supported yet %s", arg, msg);
            /* notreached */
        }
    }
    return x;
}

/*
 * Print usage message.
 */
static void usage(void) {
    fprintf(stderr, "usage: mkfs [ -options ] special [disktype]\n");
    fprintf(stderr, "where the options are:\n");
    static struct {
        char o;
        const char* h;
    } opts[] = {
#define AOPT(_opt, _type, _name, _min, _desc) {_opt, _desc},
        ALLOPTS
#undef AOPT
    };
    for (size_t i = 0; i < sizeof(opts) / sizeof(opts[0]); i++)
        fprintf(stderr, "\t-%c %s\n", opts[i].o, opts[i].h);
    exit(1);
}
