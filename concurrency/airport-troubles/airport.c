#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_STUDENTS 500
#define MAX_OFFICER_VISITS 100
#define MAX_SCANNERS 3
#define BELT_CAPACITY 2
#define NUM_HANDLERS 2

typedef struct {
    int id;
    int index;
    int arrival_time;
    int entered_at;
    int scan_start;
    int scan_finish;
    bool in_hall;
    bool scanned;
    bool approved;
    int stamped_at;
    int left_hall_at;
    bool bag_queued;
    int bag_loaded_at;
    bool bag_on_belt;
    int collected_at;
    bool slot_freed;
    bool finished;
    pthread_t thread;
} Student;

typedef struct {
    int visit_time;
    bool entered;
    int entered_at;
    bool approved;
    int approved_at;
    bool left;
    int left_at;
} OfficerVisit;

typedef struct {
    int id;
    int index;
    int current_student_id;
    int busy_until;
    pthread_t thread;
} Handler;

static Student students[MAX_STUDENTS];
static int num_students = 0;

static OfficerVisit officer_visits[MAX_OFFICER_VISITS];
static int num_officer_visits = 0;
static int current_visit_idx = 0;
static bool officer_in_hall = false;
static pthread_t officer_thread;

static Handler handlers[NUM_HANDLERS];

static int current_time = 0;
static bool sim_done = false;

static int active_scanners = 0;
static int belt_slots_free = BELT_CAPACITY;

static pthread_barrier_t tick_start_barrier;
static pthread_barrier_t tick_end_barrier;
static pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t turn_cond = PTHREAD_COND_INITIALIZER;
static int current_turn = 0;

static void wait_for_turn(int my_turn) {
    pthread_mutex_lock(&state_lock);
    while (current_turn != my_turn && !sim_done) {
        pthread_cond_broadcast(&turn_cond);
        pthread_cond_wait(&turn_cond, &state_lock);
    }
}

static void advance_turn(void) {
    current_turn++;
    pthread_cond_broadcast(&turn_cond);
    pthread_mutex_unlock(&state_lock);
}

// Student Thread
static void* student_thread_func(void* arg) {
    Student* s = (Student*)arg;

    while (true) {
        pthread_barrier_wait(&tick_start_barrier);
        if (sim_done) break;

        wait_for_turn(s->index);

        // Core student workflow will be populated in next commits

        advance_turn();
        pthread_barrier_wait(&tick_end_barrier);
    }
    return NULL;
}

// Officer Thread
static void* officer_thread_func(void* arg) {
    int my_turn = (int)(intptr_t)arg;

    while (true) {
        pthread_barrier_wait(&tick_start_barrier);
        if (sim_done) break;

        wait_for_turn(my_turn);

        // Officer workflow will be populated in next commits

        advance_turn();
        pthread_barrier_wait(&tick_end_barrier);
    }
    return NULL;
}

// Handler Thread
static void* handler_thread_func(void* arg) {
    Handler* h = (Handler*)arg;

    while (true) {
        pthread_barrier_wait(&tick_start_barrier);
        if (sim_done) break;

        wait_for_turn(h->index);

        // Baggage handler workflow will be populated in next commits

        advance_turn();
        pthread_barrier_wait(&tick_end_barrier);
    }
    return NULL;
}

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) {
        return 1;
    }

    for (int i = 0; i < n; i++) {
        int t, id;
        char actor[64];
        if (scanf("%d %s", &t, actor) != 2) break;

        if (strcmp(actor, "Student") == 0) {
            if (scanf("%d", &id) != 1) break;
            Student* s = &students[num_students];
            s->id = id;
            s->index = num_students;
            s->arrival_time = t;
            s->entered_at = -1;
            s->scan_start = -1;
            s->scan_finish = -1;
            s->in_hall = false;
            s->scanned = false;
            s->approved = false;
            s->stamped_at = -1;
            s->left_hall_at = -1;
            s->bag_queued = false;
            s->bag_loaded_at = -1;
            s->bag_on_belt = false;
            s->collected_at = -1;
            s->slot_freed = false;
            s->finished = false;
            num_students++;
        } else if (strcmp(actor, "Immigration") == 0) {
            char officer_word[64];
            if (scanf("%s %d", officer_word, &id) != 2) break;
            OfficerVisit* v = &officer_visits[num_officer_visits++];
            v->visit_time = t;
            v->entered = false;
            v->entered_at = -1;
            v->approved = false;
            v->approved_at = -1;
            v->left = false;
            v->left_at = -1;
        }
    }

    for (int i = 0; i < NUM_HANDLERS; i++) {
        handlers[i].id = i + 1;
        handlers[i].index = num_students + 1 + i;
        handlers[i].current_student_id = -1;
        handlers[i].busy_until = 0;
    }

    int total_threads = num_students + 1 + NUM_HANDLERS;
    int barrier_count = total_threads + 1; // + 1 for main coordinator thread

    pthread_barrier_init(&tick_start_barrier, NULL, barrier_count);
    pthread_barrier_init(&tick_end_barrier, NULL, barrier_count);

    // Create worker threads
    for (int i = 0; i < num_students; i++) {
        pthread_create(&students[i].thread, NULL, student_thread_func, &students[i]);
    }
    pthread_create(&officer_thread, NULL, officer_thread_func, (void*)(intptr_t)num_students);
    for (int i = 0; i < NUM_HANDLERS; i++) {
        pthread_create(&handlers[i].thread, NULL, handler_thread_func, &handlers[i]);
    }

    // Main coordinator clock loop
    while (true) {
        pthread_mutex_lock(&state_lock);
        current_turn = 0;

        // Pre-tick phase
        for (int i = 0; i < num_students; i++) {
            if (students[i].scan_start != -1 && !students[i].scanned && current_time >= students[i].scan_finish) {
                active_scanners--;
                students[i].scanned = true;
            }
        }

        for (int i = 0; i < NUM_HANDLERS; i++) {
            if (handlers[i].current_student_id != -1 && current_time >= handlers[i].busy_until) {
                int sid = handlers[i].current_student_id;
                for (int s = 0; s < num_students; s++) {
                    if (students[s].id == sid) {
                        students[s].bag_on_belt = true;
                        break;
                    }
                }
                handlers[i].current_student_id = -1;
            }
        }

        if (current_visit_idx < num_officer_visits) {
            OfficerVisit* v = &officer_visits[current_visit_idx];
            if (v->left && current_time >= v->left_at + 1) {
                officer_in_hall = false;
                current_visit_idx++;
            }
        }

        for (int i = 0; i < num_students; i++) {
            if (students[i].collected_at != -1 && current_time >= students[i].collected_at + 1 && !students[i].slot_freed) {
                belt_slots_free++;
                students[i].slot_freed = true;
                students[i].finished = true;
            }
        }

        pthread_mutex_unlock(&state_lock);

        pthread_barrier_wait(&tick_start_barrier);
        pthread_barrier_wait(&tick_end_barrier);

        pthread_mutex_lock(&state_lock);
        bool all_students_done = true;
        for (int i = 0; i < num_students; i++) {
            if (!students[i].finished) {
                all_students_done = false;
                break;
            }
        }
        bool officer_done = (current_visit_idx >= num_officer_visits);

        if ((all_students_done && officer_done) || current_time > 1000) {
            sim_done = true;
            pthread_cond_broadcast(&turn_cond);
            pthread_mutex_unlock(&state_lock);
            pthread_barrier_wait(&tick_start_barrier);
            break;
        }

        current_time++;
        pthread_mutex_unlock(&state_lock);
    }

    // Join all threads
    for (int i = 0; i < num_students; i++) {
        pthread_join(students[i].thread, NULL);
    }
    pthread_join(officer_thread, NULL);
    for (int i = 0; i < NUM_HANDLERS; i++) {
        pthread_join(handlers[i].thread, NULL);
    }

    pthread_barrier_destroy(&tick_start_barrier);
    pthread_barrier_destroy(&tick_end_barrier);

    return 0;
}

