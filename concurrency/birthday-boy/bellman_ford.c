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
    pthread_barrier_t* barrier;
} WorkerArgs;

static void* worker_func(void* arg) {
    WorkerArgs* w = (WorkerArgs*)arg;
    int tid = w->thread_id;
    int T = w->num_threads;
    int M = w->num_edges;
    int V = w->V;
    Edge* edges = w->edges;
    int* dist = w->dist;

    int chunk = (M + T - 1) / T;
    int start = tid * chunk;
    int end = start + chunk;
    if (end > M) end = M;

    for (int iter = 0; iter < V - 1; iter++) {
        for (int i = start; i < end; i++) {
            int u = edges[i].u;
            int v = edges[i].v;

            int du = dist[u];
            if (du != INF) {
                int new_dist = du + 1;
                if (new_dist < dist[v]) {
                    dist[v] = new_dist;
                }
            }
        }

        pthread_barrier_wait(w->barrier);
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

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, T);

    for (int t = 0; t < T; t++) {
        args[t].thread_id = t;
        args[t].num_threads = T;
        args[t].V = V;
        args[t].num_edges = M;
        args[t].edges = edges;
        args[t].dist = dist;
        args[t].barrier = &barrier;

        pthread_create(&threads[t], NULL, worker_func, &args[t]);
    }

    for (int t = 0; t < T; t++) {
        pthread_join(threads[t], NULL);
    }

    pthread_barrier_destroy(&barrier);

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

    return 0;
}

