#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>

#define INF (1000000000)

typedef struct {
    int u;
    int v;
} Edge;

typedef struct {
    int thread_id;
    int num_threads;
    int V;
    int num_edges;
    Edge* edges;
    int* dist;
    bool* thread_changed;
    pthread_barrier_t* barrier1;
    pthread_barrier_t* barrier2;
    bool* terminated;
} WorkerArgs;

static void* worker_func(void* arg) {
    WorkerArgs* w = (WorkerArgs*)arg;
    int tid = w->thread_id;
    int T = w->num_threads;
    int M = w->num_edges;
    int V = w->V;
    Edge* edges = w->edges;
    int* dist = w->dist;

    int start = (int)(((long long)tid * M) / T);
    int end = (int)(((long long)(tid + 1) * M) / T);

    for (int iter = 0; iter < V - 1; iter++) {
        bool local_changed = false;

        for (int i = start; i < end; i++) {
            int u = edges[i].u;
            int v = edges[i].v;

            int du = dist[u];
            if (du != INF) {
                int new_dist = du + 1;
                int old_dist = dist[v];
                while (new_dist < old_dist) {
                    if (__atomic_compare_exchange_n(&dist[v], &old_dist, new_dist,
                                                   false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
                        local_changed = true;
                        break;
                    }
                }
            }
        }

        w->thread_changed[tid] = local_changed;

        // Synchronize after relaxation
        pthread_barrier_wait(w->barrier1);

        // Thread 0 checks if any thread made changes
        if (tid == 0) {
            bool any_changed = false;
            for (int t = 0; t < T; t++) {
                if (w->thread_changed[t]) {
                    any_changed = true;
                    break;
                }
            }
            if (!any_changed) {
                *(w->terminated) = true;
            }
        }

        // Synchronize on termination flag
        pthread_barrier_wait(w->barrier2);

        if (*(w->terminated)) {
            break;
        }
    }

    return NULL;
}

int main(void) {
    int V, E, N, T;
    if (scanf("%d %d %d %d", &V, &E, &N, &T) != 4) {
        return 1;
    }

    int M = 2 * E;
    Edge* edges = (Edge*)malloc(sizeof(Edge) * (M > 0 ? M : 1));
    if (!edges) {
        return 1;
    }

    int edge_idx = 0;
    for (int i = 0; i < E; i++) {
        int u, v;
        if (scanf("%d %d", &u, &v) != 2) break;
        edges[edge_idx].u = u;
        edges[edge_idx].v = v;
        edge_idx++;
        edges[edge_idx].u = v;
        edges[edge_idx].v = u;
        edge_idx++;
    }
    M = edge_idx;

    int* dist = (int*)malloc(sizeof(int) * V);
    for (int i = 0; i < V; i++) {
        dist[i] = INF;
    }
    dist[N] = 0;

    if (T < 1) T = 1;

    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * T);
    WorkerArgs* args = (WorkerArgs*)malloc(sizeof(WorkerArgs) * T);
    bool* thread_changed = (bool*)malloc(sizeof(bool) * T);
    bool terminated = false;

    pthread_barrier_t barrier1, barrier2;
    pthread_barrier_init(&barrier1, NULL, T);
    pthread_barrier_init(&barrier2, NULL, T);

    for (int t = 0; t < T; t++) {
        args[t].thread_id = t;
        args[t].num_threads = T;
        args[t].V = V;
        args[t].num_edges = M;
        args[t].edges = edges;
        args[t].dist = dist;
        args[t].thread_changed = thread_changed;
        args[t].barrier1 = &barrier1;
        args[t].barrier2 = &barrier2;
        args[t].terminated = &terminated;

        pthread_create(&threads[t], NULL, worker_func, &args[t]);
    }

    for (int t = 0; t < T; t++) {
        pthread_join(threads[t], NULL);
    }

    pthread_barrier_destroy(&barrier1);
    pthread_barrier_destroy(&barrier2);

    // Print shortest paths
    for (int i = 0; i < V; i++) {
        if (i > 0) printf(" ");
        if (dist[i] == INF) {
            printf("-1");
        } else {
            printf("%d", dist[i]);
        }
    }
    printf("\n");

    free(edges);
    free(dist);
    free(threads);
    free(args);
    free(thread_changed);

    return 0;
}
