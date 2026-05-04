/* -------------------------------------------------------------------------
 * Run SHA-3 (NIST FIPS 202) on the given file. 
 *
 * Call as
 *
 * sha3sum 256|384|512 file_path
 *
 * See sha3.c for additional details. 
 *
 * Jun 2018. Andrey Jivsov. crypto@brainhub.org
 * ---------------------------------------------------------------------- */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "sha3.h"

static void help(const char *argv0) {
    printf("To call: %s 256|384|512 [-k] file_path.\n", argv0);
}

static void byte_to_hex(uint8_t b, char s[23]) {
    unsigned i=1;
    s[0] = s[1] = '0';
    s[2] = '\0';
    while(b) {
        unsigned t = b & 0x0f;
        if( t < 10 ) {
            s[i] = '0' + t;
        } else {
            s[i] = 'a' + t - 10;
        }
        i--;
        b >>= 4;
    }
}

int main(int argc, char *argv[])
{
    sha3_context c;
    const uint8_t *hash;
    unsigned image_size;
    const char *file_path;
    int fd;
    struct stat st;
    void *p;
    unsigned i;
    unsigned use_keccak = 0;

    if( argc != 3 && argc != 4 ) {
	    help(argv[0]);
	    return 1;
    }

    image_size = atoi(argv[1]);
    switch( image_size ) {
	case 256:
	case 384:
	case 512:
		break;
	default:
		help(argv[0]);
		return 1;
    }

    file_path = argv[2];

    if( argc == 4 && file_path[0] == '-' && file_path[1] == 'k' )  {
        use_keccak = 1;
        file_path = argv[3];
    }

    if( access(file_path, R_OK)!=0 ) {
	    printf("Cannot read file '%s'", file_path);
	    return 2;
    }

    fd = open(file_path, O_RDONLY);
    if( fd == -1 ) {
	    printf("Cannot open file '%s' for reading", file_path);
	    return 2;
    }
    i = fstat(fd, &st);
    if( i ) {
	    close(fd);
	    printf("Cannot determine the size of file '%s'", file_path);
	    return 2;
    }

    p = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if( p==NULL ) {
	    printf("Cannot memory-map file '%s'", file_path);
	    return 2;
    }

    switch(image_size) {
	case 256:
    		sha3_Init256(&c);
		break;
	case 384:
    		sha3_Init384(&c);
		break;
	case 512:
    		sha3_Init512(&c);
		break;
    }

    if( use_keccak ) {
        enum SHA3_FLAGS flags2 = sha3_SetFlags(&c, SHA3_FLAGS_KECCAK);
        if( flags2 != SHA3_FLAGS_KECCAK )  {
	    printf("Failed to set Keccak mode");
            return 2;
        }
    }
    sha3_Update(&c, p, st.st_size);
    hash = sha3_Finalize(&c);

    munmap(p, st.st_size);

    for(i=0; i<image_size/8; i++) {
	    char s[3];
	    byte_to_hex(hash[i],s);
	    printf("%s", s);
    }
    printf("  %s\n", file_path);

    return 0;
}
