/*  A3.c  — Sleeping Teaching Assistant (POSIX threads + semaphores)
 *
 *  Compile:
 *      gcc -std=c11 -O2 -pthread A3.c -o A3
 *
 *  Run (examples):
 *      ./A3                    # defaults: 5 students, 3 chairs, 3 help-requests per student
 *      ./A3 -s 8              # 8 students
 *      ./A3 -s 6 -c 3 -r 4    # 6 students, 3 chairs, each student seeks help 4 times
 *
 *  Flags:
 *      -s <int>   number of student threads          (default 5)
 *      -c <int>   number of hallway chairs           (default 3)
 *      -r <int>   help requests per student          (default 3)
 *
 *  Notes:
 *    - This is the classic “sleeping barber” pattern adapted to the TA setting.
 *    - Semaphores:
 *        customers   : counting semaphore (# of waiting students). Wakes TA when >0.
 *        ta_ready    : signals that TA is ready to help the next student.
 *    - Mutex:
 *        mtx         : protects shared state (waiting count).
 *    - The TA “naps” by blocking on sem_wait(customers). Students “wake” the TA with sem_post(customers).
 *    - Students either take a chair (if available), or leave to program more and try later.
 *    - To exit gracefully, the TA uses sem_timedwait to periodically check if all students are done.
 */

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdatomic.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

typedef struct {
    int id;
    unsigned int rng;
    int requests_to_make;
} StudentArgs;

/* ---------- Tunables (kept simple; can be randomized) ---------- */
static int NUM_STUDENTS = 5;
static int NUM_CHAIRS  = 3;
static int REQS_PER_STUDENT = 3;

/* milliseconds helpers */
static void sleep_ms(int ms) {
    if (ms <= 0) return;
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

/* random in [lo, hi] using per-thread rng */
static int rand_range(unsigned int *rng, int lo, int hi) {
    if (hi <= lo) return lo;
    return lo + (int)(rand_r(rng) % (unsigned)(hi - lo + 1));
}

/* timed wait helper for semaphore (timeout in ms); returns true if acquired */
static bool sem_wait_ms(sem_t *sem, int timeout_ms) {
    if (timeout_ms < 0) {
        /* blocking */
        return sem_wait(sem) == 0;
    }
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec  += timeout_ms / 1000;
    ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }
    while (true) {
        if (sem_timedwait(sem, &ts) == 0) return true;
        if (errno == EINTR) continue;
        if (errno == ETIMEDOUT) return false;
        perror("sem_timedwait");
        return false;
    }
}

/* ---------- Shared state ---------- */
static sem_t customers;     /* counts waiting students; TA sleeps on this when 0 */
static sem_t ta_ready;      /* TA signals a seat/session is ready for exactly one student */
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

static int waiting = 0;                 /* # of students currently sitting in chairs */
static atomic_int students_active = 0;  /* # of student threads still running */

/* ---------- Parameters for simulated work ---------- */
enum {
    PROGRAM_MIN_MS = 200, PROGRAM_MAX_MS = 800,
    HELP_MIN_MS    = 200, HELP_MAX_MS    = 600,
    TA_POLL_MS     = 300                  /* TA checks this often if everything is done */
};

/* ---------- TA thread ---------- */
static void *ta_thread(void *arg) {
    (void)arg;
    printf("[TA  ] Office open. Napping until a student arrives...\n");

    while (1) {
        /* Nap until a waiting student appears, but wake periodically to check for shutdown. */
        if (!sem_wait_ms(&customers, TA_POLL_MS)) {
            /* timed out; check for graceful shutdown */
            pthread_mutex_lock(&mtx);
            int w = waiting;
            pthread_mutex_unlock(&mtx);
            if (atomic_load(&students_active) == 0 && w == 0) {
                printf("[TA  ] No more students and no one waiting. Closing office.\n");
                break;
            }
            /* otherwise, keep napping */
            continue;
        }

        /* A student is waiting (or just arrived). Sit them with the TA. */
        pthread_mutex_lock(&mtx);
        if (waiting > 0) waiting--;  /* one student leaves the chair to get help */
        pthread_mutex_unlock(&mtx);

        /* Signal exactly one student that the TA is ready now. */
        sem_post(&ta_ready);

        /* Provide help (simulate with sleep). */
        printf("[TA  ] Helping a student...\n");
        sleep_ms(rand_range(&(unsigned int){(unsigned int)time(NULL)}, HELP_MIN_MS, HELP_MAX_MS));
        printf("[TA  ] Finished helping.\n");
    }

    return NULL;
}

/* ---------- Student thread ---------- */
static void *student_thread(void *varg) {
    StudentArgs *args = (StudentArgs *)varg;
    int id = args->id;
    unsigned int rng = args->rng;

    for (int k = 1; k <= args->requests_to_make; ++k) {
        /* Program for a while */
        int code_ms = rand_range(&rng, PROGRAM_MIN_MS, PROGRAM_MAX_MS);
        printf("[Stu%02d] Programming (%d ms) before seeking help (%d/%d).\n",
               id, code_ms, k, args->requests_to_make);
        sleep_ms(code_ms);

        /* Try to get help */
        pthread_mutex_lock(&mtx);
        if (waiting < NUM_CHAIRS) {
            waiting++;
            int pos = waiting; /* just for logging; not a real seat index */
            printf("[Stu%02d] Found a chair (waiting=%d). Waking TA if asleep.\n", id, pos);
            /* Signal that a student is waiting / arrived. This wakes the TA if sleeping. */
            sem_post(&customers);
            pthread_mutex_unlock(&mtx);

            /* Wait until TA is ready for me */
            sem_wait(&ta_ready);

            /* I'm with the TA now */
            printf("[Stu%02d] Getting help from the TA.\n", id);
            /* actual help time is simulated by TA; student just proceeds */
        } else {
            /* No chair; leave and try later */
            printf("[Stu%02d] No chairs available. Will come back later.\n", id);
            pthread_mutex_unlock(&mtx);
            /* Back to programming loop; we'll try again in next iteration (or you could retry here). */
        }
    }

    atomic_fetch_sub(&students_active, 1);
    printf("[Stu%02d] Done for the day.\n", id);
    free(args);
    return NULL;
}

/* ---------- CLI parsing ---------- */
static void parse_args(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "s:c:r:h")) != -1) {
        switch (opt) {
            case 's': NUM_STUDENTS = atoi(optarg); break;
            case 'c': NUM_CHAIRS   = atoi(optarg); break;
            case 'r': REQS_PER_STUDENT = atoi(optarg); break;
            case 'h':
            default:
                fprintf(stderr,
                    "Usage: %s [-s students] [-c chairs] [-r requests_per_student]\n", argv[0]);
                exit(opt == 'h' ? 0 : 1);
        }
    }
    if (NUM_STUDENTS < 1) NUM_STUDENTS = 1;
    if (NUM_CHAIRS   < 0) NUM_CHAIRS   = 0;
    if (REQS_PER_STUDENT < 1) REQS_PER_STUDENT = 1;
}

/* ---------- Main ---------- */
int main(int argc, char **argv) {
    parse_args(argc, argv);

    printf("Config: students=%d, chairs=%d, requests_per_student=%d\n",
           NUM_STUDENTS, NUM_CHAIRS, REQS_PER_STUDENT);

    if (sem_init(&customers, 0, 0) != 0) { perror("sem_init(customers)"); return 1; }
    if (sem_init(&ta_ready,  0, 0) != 0) { perror("sem_init(ta_ready)");  return 1; }

    pthread_t ta;
    pthread_t *students = calloc((size_t)NUM_STUDENTS, sizeof(pthread_t));
    if (!students) { perror("calloc"); return 1; }

    atomic_store(&students_active, NUM_STUDENTS);

    /* Start TA */
    if (pthread_create(&ta, NULL, ta_thread, NULL) != 0) {
        perror("pthread_create(TA)");
        return 1;
    }

    /* Start students */
    for (int i = 0; i < NUM_STUDENTS; ++i) {
        StudentArgs *sa = malloc(sizeof(*sa));
        if (!sa) { perror("malloc"); return 1; }
        sa->id = i + 1;
        sa->rng = (unsigned int)(time(NULL) ^ (uintptr_t)sa ^ (i * 2654435761u));
        sa->requests_to_make = REQS_PER_STUDENT;

        if (pthread_create(&students[i], NULL, student_thread, sa) != 0) {
            perror("pthread_create(student)");
            return 1;
        }
    }

    /* Join students */
    for (int i = 0; i < NUM_STUDENTS; ++i) {
        pthread_join(students[i], NULL);
    }
    free(students);

    /* Allow TA to finish when queue is empty and no students remain */
    pthread_join(ta, NULL);

    sem_destroy(&customers);
    sem_destroy(&ta_ready);
    pthread_mutex_destroy(&mtx);
    return 0;
}
