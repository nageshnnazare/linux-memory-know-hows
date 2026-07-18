/*
 * 02_word_alignment.c
 * --------------------
 * Inspect sizeof / alignof / offsetof of a struct, and demonstrate that
 * the compiler inserts padding to keep each field naturally aligned.
 *
 * Layout of `struct s`:
 *
 *   offset 0    1    2    3    4 5 6 7   8    9    10 11
 *          +----+----+----+----+--------+----+----+----+
 *          | a  | pad pad pad |   b    | c  |  pad     |
 *          +----+----+----+---+--------+----+----------+
 *               ^                       ^
 *               a is char (1 byte)      c is char (1 byte)
 *
 *   The 3-byte pad after `a` is so that `b` (int) is 4-byte aligned.
 *   The 3 trailing bytes after `c` are so that an array of `struct s`
 *   keeps the same alignment guarantee for `b` in element 1.
 */

#include <stdio.h>
#include <stddef.h>
#include <stdalign.h>

struct s {
    char  a;
    int   b;
    char  c;
};

struct packed {
    char a;
    int  b;
    char c;
} __attribute__((packed));

int main(void)
{
    printf("Type sizes on this machine:\n");
    printf("  char       = %zu  align=%zu\n",  sizeof(char),       alignof(char));
    printf("  short      = %zu  align=%zu\n",  sizeof(short),      alignof(short));
    printf("  int        = %zu  align=%zu\n",  sizeof(int),        alignof(int));
    printf("  long       = %zu  align=%zu\n",  sizeof(long),       alignof(long));
    printf("  long long  = %zu  align=%zu\n",  sizeof(long long),  alignof(long long));
    printf("  void *     = %zu  align=%zu\n",  sizeof(void *),     alignof(void *));
    printf("  float      = %zu  align=%zu\n",  sizeof(float),      alignof(float));
    printf("  double     = %zu  align=%zu\n",  sizeof(double),     alignof(double));
    printf("  long double= %zu  align=%zu\n",  sizeof(long double),alignof(long double));
    printf("  max_align_t= %zu\n",             alignof(max_align_t));

    printf("\nstruct s (default layout): sizeof=%zu align=%zu\n",
           sizeof(struct s), alignof(struct s));
    printf("  offsetof(a) = %zu\n", offsetof(struct s, a));
    printf("  offsetof(b) = %zu\n", offsetof(struct s, b));
    printf("  offsetof(c) = %zu\n", offsetof(struct s, c));

    printf("\nstruct packed (no padding): sizeof=%zu align=%zu\n",
           sizeof(struct packed), alignof(struct packed));
    printf("  offsetof(a) = %zu\n", offsetof(struct packed, a));
    printf("  offsetof(b) = %zu\n", offsetof(struct packed, b));
    printf("  offsetof(c) = %zu\n", offsetof(struct packed, c));

    /* Reordering fields can save space: */
    struct good { int b; char a; char c; };
    printf("\nstruct good (reordered): sizeof=%zu\n", sizeof(struct good));
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 00_fundamentals/02_word_alignment.c
 * Command: make -C 00_fundamentals 02_word_alignment
 * Exit status: 0
 * Output:
 * Type sizes on this machine:
 *   char       = 1  align=1
 *   short      = 2  align=2
 *   int        = 4  align=4
 *   long       = 8  align=8
 *   long long  = 8  align=8
 *   void *     = 8  align=8
 *   float      = 4  align=4
 *   double     = 8  align=8
 *   long double= 16  align=16
 *   max_align_t= 16
 * 
 * struct s (default layout): sizeof=12 align=4
 *   offsetof(a) = 0
 *   offsetof(b) = 4
 *   offsetof(c) = 8
 * 
 * struct packed (no padding): sizeof=6 align=1
 *   offsetof(a) = 0
 *   offsetof(b) = 1
 *   offsetof(c) = 5
 * 
 * struct good (reordered): sizeof=8
 * AUTO-GENERATED RUN OUTPUT END
 */
