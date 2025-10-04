#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LINE     1024      // maximum length of command line
#define MAX_ARGS     64        // maximum number of arguments
#define HISTORY_SIZE 5         // keep last 5 commands

// --------- History (circular buffer) ----------
typedef struct {
    char *items[HISTORY_SIZE];
    int count;         // number of valid entries (<= HISTORY_SIZE)
    long long int total;   // total number of commands ever stored (monotonic)
    int next;          // index where next entry will be written (0..HISTORY_SIZE-1)
} History;

void history_init(History *h) {
    memset(h, 0, sizeof(*h));
}

void history_free(History *h) {
    for (int i = 0; i < HISTORY_SIZE; ++i) {
        free(h->items[i]);
        h->items[i] = NULL;
    }

    h->count = 0;
    h->total = 0;
    h->next  = 0;
}

// Add a command string (ownership is copied).
// If buffer full, evict oldest (at next position) automatically.
void history_add(History *h, const char *cmd) {
    if (!cmd || *cmd == '\0') {
        return;
    }

    // Replace existing at next
    free(h->items[h->next]);
    h->items[h->next] = strdup(cmd);

    if (!h->items[h->next]) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    h->next = (h->next + 1) % HISTORY_SIZE;

    if (h->count < HISTORY_SIZE) {
        h->count++;
    }
    
    h->total++;
}

// Get most recent command (NULL if none). Does not transfer ownership.
const char* history_most_recent(const History *h) {
    if (h->count == 0) {
        return NULL;
    }

    int idx = (h->next - 1 + HISTORY_SIZE) % HISTORY_SIZE;
    return h->items[idx];
}

// Print last up-to-5 commands in reverse chronological order with numbering.
void history_print(const History *h) {
    if (h->count == 0) {
        return;
    }


    // k=0 is most recent; k=count-1 is oldest
    for (int k = 0; k < h->count; ++k) {
        int idx = (h->next - 1 - k + HISTORY_SIZE) % HISTORY_SIZE;
        long long int number = h->total - k; // continues beyond 5
        printf("%lld %s\n", number, h->items[idx]);
    }
}

// --------- String utilities ----------

// Trim leading/trailing whitespace in place. Returns pointer to first non-space char.
char* str_trim(char *s) {
    if (!s) {
        return s;
    }

    // leading
    while (isspace((unsigned char)*s)) {
        s++;
    }

    if (*s == '\0') {
        return s;
    }

    // trailing
    char *end = s + strlen(s) - 1;

    while (end >= s && isspace((unsigned char)*end)) {
        *end = '\0';
        --end;
    }

    return s;
}

// If line ends with '&' (optionally after spaces), remove it and set *is_bg = 1.
// Also trims trailing whitespace after removal. Operates in place.
void strip_background_amp(char *line, int *is_bg) {
    *is_bg = 0;

    if (!line) {
        return;
    }

    size_t n = strlen(line);
    // Walk back over trailing whitespace
    ssize_t i = (ssize_t)n - 1;

    while (i >= 0 && isspace((unsigned char)line[i])) {
        line[i] = '\0';
        --i;
    }

    if (i >= 0 && line[i] == '&') {
        line[i] = '\0';
        *is_bg = 1;
        // Strip any remaining whitespace before '&'
        while (i - 1 >= 0 && isspace((unsigned char)line[i - 1])) {
            line[i - 1] = '\0';
            --i;
        }
    }
}

// Tokenize line into argv[], returns argc. Modifies line in-place (inserts NULs).
int parse_args(char *line, char *argv[MAX_ARGS]) {
    int argc = 0;
    char *tok = strtok(line, " \t");
    
    while (tok && argc < MAX_ARGS - 1) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t");
    }

    argv[argc] = NULL;
    return argc;
}

// Join argv back to a single space-separated string (for echo/history), up to out_cap.
void join_args(char *out, size_t out_cap, char *const argv[MAX_ARGS]) {
    out[0] = '\0';
    size_t used = 0;

    for (int i = 0; argv[i]; ++i) {
        size_t need = strlen(argv[i]) + (i > 0 ? 1 : 0);
        
        if (used + need + 1 >= out_cap) {
            break;
        }

        if (i > 0) {
            out[used++] = ' ';
            out[used] = '\0';
        }

        strncat(out, argv[i], out_cap - used - 1);
        used = strlen(out);
    }
}

// Execute one parsed command (argv). If bg==0, waits; else returns immediately in parent.
void execute_command(char *const argv[MAX_ARGS], int bg) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        // Child: replace image
        execvp(argv[0], argv);
        // If execvp returns, it's an error
        perror("execvp");
        _exit(127);
    } else {
        // Parent
        if (!bg) {
            int status = 0;
            if (waitpid(pid, &status, 0) < 0) {
                perror("waitpid");
            }
        } else {
            // Background: do not wait
            printf("[bg pid %d]\n", pid);
        }
    }
}

int main(void) {
    History hist;
    history_init(&hist);

    char line_buf[MAX_LINE];
    int should_run = 1;

    while (should_run) {
        // Prompt
        printf("osh> ");
        fflush(stdout);

        // Read line
        if (!fgets(line_buf, sizeof(line_buf), stdin)) {
            // EOF (Ctrl-D) or error: exit gracefully
            putchar('\n');
            should_run = 0;
            continue;
        }

        // Normalize whitespace & handle empty
        char *line = str_trim(line_buf);
        if (*line == '\0') {
            continue; // ignore empty lines
        }

        // Built-in: history (must NOT be stored)
        if (strcmp(line, "history") == 0) {
            history_print(&hist);
            continue;
        }

        // Built-in: exit (must NOT be stored)
        if (strcmp(line, "exit") == 0) {
            should_run = 0;
            continue;
        }

        int is_bg = 0;
        // Check/strip trailing '&' (not stored in history)
        strip_background_amp(line, &is_bg);

        // Special: "!!" — repeat most recent
        if (strcmp(line, "!!") == 0) {
            const char *recent = history_most_recent(&hist);

            if (!recent) {
                printf("No commands in history.\n");
                continue;
            }

            // Echo the command to the user (as specified)
            printf("%s\n", recent);

            // Parse and execute the recent command
            char tmp[MAX_LINE];
            strncpy(tmp, recent, sizeof(tmp));
            tmp[sizeof(tmp)-1] = '\0';

            // Prepare argv
            char *argv[MAX_ARGS];
            int argc = parse_args(tmp, argv);
            if (argc == 0) {
                continue;
            }

            // Add to history as the "next" command
            history_add(&hist, recent);

            // Execute (foreground by default for repeated commands)
            execute_command(argv, 0);
            continue;
        }

        // For a "normal" command: parse args
        // Make a safe copy for history before parse (without '&')
        char line_copy_for_history[MAX_LINE];
        strncpy(line_copy_for_history, line, sizeof(line_copy_for_history));
        line_copy_for_history[sizeof(line_copy_for_history)-1] = '\0';

        // Build argv from the working buffer (in-place tokenization)
        char work[MAX_LINE];
        strncpy(work, line, sizeof(work));
        work[sizeof(work)-1] = '\0';

        char *argv[MAX_ARGS];
        int argc = parse_args(work, argv);
        if (argc == 0) continue; // should not happen after trimming

        // Add to history (exclude 'history'/'exit' handled above, and we already stripped '&')
        history_add(&hist, str_trim(line_copy_for_history));

        // Execute
        execute_command(argv, is_bg);
    }

    history_free(&hist);
    return 0;
}