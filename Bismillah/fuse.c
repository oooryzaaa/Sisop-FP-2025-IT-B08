#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/file.h>

#define MAX_CHAR 256

char *source_dir = NULL;
char *log_file_path = NULL;

struct CharStats {
    int total_chars;
    int letters;
    int digits;
    int spaces;
    int special_chars;
    int char_frequency[MAX_CHAR];
};

void get_full_path(char full_path[], const char *path) {
    snprintf(full_path, 4096, "%s%s", source_dir, path);
}

int is_text_file(const char *filename) {
    const char *text_exts[] = {
        ".txt", ".c", ".h", ".cpp", ".hpp", ".py", ".java", ".js", ".html",
        ".css", ".php", ".sql", ".xml", ".json", ".md", ".log", ".cfg", ".ini"
    };
    size_t num_exts = sizeof(text_exts) / sizeof(text_exts[0]);

    const char *dot = strrchr(filename, '.');
    if (!dot) return 0;

    for (size_t i = 0; i < num_exts; ++i) {
        if (strcasecmp(dot, text_exts[i]) == 0)
            return 1;
    }
    return 0;
}

void count_characters(const char *content, size_t len, struct CharStats *stats) {
    memset(stats, 0, sizeof(struct CharStats));
    for (size_t i = 0; i < len; i++) {
        unsigned char c = content[i];
        stats->total_chars++;
        stats->char_frequency[c]++;
        if (isalpha(c)) stats->letters++;
        else if (isdigit(c)) stats->digits++;
        else if (isspace(c)) stats->spaces++;
        else stats->special_chars++;
    }
}

char *get_current_timestamp() {
    static char buffer[64];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, sizeof(buffer), "[%d/%m/%Y %H:%M:%S]", t);
    return buffer;
}

int compare_freq(const void *a, const void *b) {
    int freq_a = ((int *)a)[1];
    int freq_b = ((int *)b)[1];
    return freq_b - freq_a;
}

void log_character_count(const char *filename, struct CharStats *stats) {
    FILE *log = fopen(log_file_path, "a");
    if (!log) {
        perror("fopen log");
        return;
    }
    int fd = fileno(log);
    flock(fd, LOCK_EX);

    fprintf(log, "\n=====================================\n");
    fprintf(log, " FUSE CHARACTER COUNTER LOG ENTRY 🎯\n");
    fprintf(log, "=====================================\n");
    fprintf(log, " Timestamp: %s\n", get_current_timestamp());
    fprintf(log, " File: %s\n", filename);
    fprintf(log, " CHARACTER STATISTICS:\n");
    fprintf(log, "   • Total Characters: %d\n", stats->total_chars);
    fprintf(log, "   • Letters: %d\n", stats->letters);
    fprintf(log, "   • Digits: %d\n", stats->digits);
    fprintf(log, "   • Spaces: %d\n", stats->spaces);
    fprintf(log, "   • Special Characters: %d\n", stats->special_chars);
    fprintf(log, " TOP 5 MOST FREQUENT CHARACTERS:\n");

    int freq_table[MAX_CHAR][2];
    for (int i = 0; i < MAX_CHAR; ++i) {
        freq_table[i][0] = i;
        freq_table[i][1] = stats->char_frequency[i];
    }
    qsort(freq_table, MAX_CHAR, sizeof(freq_table[0]), compare_freq);

    for (int i = 0, count = 0; i < MAX_CHAR && count < 5; ++i) {
        if (freq_table[i][1] == 0) continue;
        char display[16];
        if (freq_table[i][0] == ' ') strcpy(display, "[SPACE]");
        else if (freq_table[i][0] == '\n') strcpy(display, "[NEWLINE]");
        else if (freq_table[i][0] == '\t') strcpy(display, "[TAB]");
        else snprintf(display, sizeof(display), "%c", freq_table[i][0]);
        fprintf(log, "   %d. '%s' -> %d times\n", count + 1, display, freq_table[i][1]);
        count++;
    }

    fprintf(log, "=====================================\n");
    flock(fd, LOCK_UN);
    fclose(log);
}

void analyze_and_log_fd(int fd, const char *filename) {
    struct CharStats stats;
    memset(&stats, 0, sizeof(struct CharStats));
    const size_t bufsize = 4096;
    char buffer[bufsize];
    ssize_t nread;
    lseek(fd, 0, SEEK_SET);
    while ((nread = read(fd, buffer, bufsize)) > 0) {
        count_characters(buffer, nread, &stats);
    }
    log_character_count(filename, &stats);
    lseek(fd, 0, SEEK_SET);
}

// FUSE functions

static int char_counter_getattr(const char *path, struct stat *stbuf) {
    char full_path[4096];
    get_full_path(full_path, path);
    if (stat(full_path, stbuf) == -1) return -errno;
    return 0;
}

static int char_counter_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                                off_t offset, struct fuse_file_info *fi) {
    (void) offset; (void) fi;
    char full_path[4096];
    get_full_path(full_path, path);
    DIR *dp = opendir(full_path);
    if (!dp) return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        filler(buf, de->d_name, NULL, 0);
    }

    closedir(dp);
    return 0;
}

static int char_counter_open(const char *path, struct fuse_file_info *fi) {
    char full_path[4096];
    get_full_path(full_path, path);
    int fd = open(full_path, fi->flags);
    if (fd == -1) return -errno;

    fi->fh = fd;

    if (is_text_file(full_path)) {
        analyze_and_log_fd(fd, full_path);
    }

    return 0;
}

static int char_counter_read(const char *path, char *buf, size_t size, off_t offset,
                             struct fuse_file_info *fi) {
    (void) path;
    int res = pread(fi->fh, buf, size, offset);
    return (res == -1) ? -errno : res;
}

static int char_counter_release(const char *path, struct fuse_file_info *fi) {
    (void) path;
    if (fi->fh != -1) close(fi->fh);
    return 0;
}

static struct fuse_operations operations = {
    .getattr = char_counter_getattr,
    .readdir = char_counter_readdir,
    .open    = char_counter_open,
    .read    = char_counter_read,
    .release = char_counter_release,
};

int main(int argc, char *argv[]) {
    printf(" FUSE Character Counter \n");
    printf("===========================\n");

    if (argc < 3) {
        fprintf(stderr, " Usage: %s <source_dir> <mount_point>\n", argv[0]);
        return 1;
    }

    source_dir = realpath(argv[1], NULL);
    if (!source_dir) {
        perror("realpath");
        return 1;
    }

    size_t path_len = strlen(source_dir) + strlen("/count.log") + 1;
    log_file_path = malloc(path_len);
    if (!log_file_path) {
        perror("malloc");
        free(source_dir);
        return 1;
    }
    snprintf(log_file_path, path_len, "%s/count.log", source_dir);

    printf(" Source Directory: %s\n", source_dir);
    printf(" Mount Point     : %s\n", argv[2]);
    printf(" Log File         : %s\n", log_file_path);

    FILE *log = fopen(log_file_path, "a");
    if (log) {
        int fd = fileno(log);
        flock(fd, LOCK_EX);
        fprintf(log, "\n FUSE Character Counter Started %s\n", get_current_timestamp());
        flock(fd, LOCK_UN);
        fclose(log);
    }

    char *fuse_argv[argc - 1];
    fuse_argv[0] = argv[0];
    for (int i = 2; i < argc; i++) {
        fuse_argv[i - 1] = argv[i];
    }

    int ret = fuse_main(argc - 1, fuse_argv, &operations, NULL);

    free(source_dir);
    free(log_file_path);
    return ret;
}
