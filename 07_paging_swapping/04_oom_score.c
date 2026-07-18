/*
 * 04_oom_score.c
 * ---------------
 * Read the OOM score and adjust it.
 *
 *   /proc/self/oom_score      -- kernel's current "badness" rating
 *   /proc/self/oom_score_adj  -- user adjustment, -1000..1000
 *                                -1000  = never kill (if any other victim)
 *                                +1000  = kill me first
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

static int read_int(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return -1; }
    int v; if (fscanf(f, "%d", &v) != 1) v = -1;
    fclose(f);
    return v;
}

static int write_int(const char *path, int v)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0) return -1;
    char buf[16];
    int n = snprintf(buf, sizeof buf, "%d", v);
    int w = write(fd, buf, n);
    close(fd);
    return w == n ? 0 : -1;
}

int main(void)
{
    printf("oom_score      = %d\n", read_int("/proc/self/oom_score"));
    printf("oom_score_adj  = %d\n", read_int("/proc/self/oom_score_adj"));

    if (write_int("/proc/self/oom_score_adj", -500) == 0)
        printf("set oom_score_adj = -500\n");
    else
        perror("write oom_score_adj");

    printf("oom_score now  = %d\n", read_int("/proc/self/oom_score"));
    printf("oom_score_adj  = %d\n", read_int("/proc/self/oom_score_adj"));
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 07_paging_swapping/04_oom_score.c
 * Command: make -C 07_paging_swapping 04_oom_score
 * Exit status: 0
 * Output:
 * write oom_score_adj: Permission denied
 * oom_score      = 999
 * oom_score_adj  = 500
 * oom_score now  = 999
 * oom_score_adj  = 500
 * AUTO-GENERATED RUN OUTPUT END
 */
