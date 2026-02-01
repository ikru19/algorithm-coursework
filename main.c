#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_VERTICES 26
#define MAX_DELIVERIES 50
#define INF 999999

typedef enum {
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3
} Priority;
typedef struct Edge {
    char destination;
    int weight;
    struct Edge* next;
} Edge;
typedef struct Graph {
    Edge* adjList[MAX_VERTICES];
    int vertices;
    char vertexNames[MAX_VERTICES];
    int vertexCount;
} Graph;

typedef struct Delivery {
    char location;
    Priority priority;
    int timeWindowStart;
    int timeWindowEnd;
    int completed;
} Delivery;

typedef struct DeliveryList {
    Delivery deliveries[MAX_DELIVERIES];
    int count;
} DeliveryList;

typedef struct PathInfo {
    int distance;
    char previous;
} PathInfo;
typedef struct DijkstraResult {
    PathInfo paths[MAX_VERTICES];
    char source;
    int vertexCount;
} DijkstraResult;

typedef struct DeliveryStep {
    char location;
    Priority priority;
    char path[100];
    int cost;
    int stepNumber;
    struct DeliveryStep* next;
} DeliveryStep;

typedef struct DeliverySequence {
    DeliveryStep* head;
    int totalSteps;
    int totalCost;
    char warehouse;
} DeliverySequence;

Graph* createGraph();
void freeGraph(Graph* graph);
int getVertexIndex(Graph* graph, char vertex);
int addVertex(Graph* graph, char vertex);
int addEdge(Graph* graph, char src, char dest, int weight);
void displayGraph(Graph* graph);
int loadMapFromFile(Graph* graph, const char* filename);

int loadDeliveriesFromFile(DeliveryList* deliveries, const char* filename);
Priority getPriorityFromString(const char* priorityStr);
const char* getPriorityString(Priority priority);
void displayDeliveries(DeliveryList* deliveries);

DijkstraResult* dijkstra(Graph* graph, char source);
void freeDijkstraResult(DijkstraResult* result);
int getMinDistanceVertex(int distances[], int visited[], int vertexCount);
int getShortestDistance(DijkstraResult* result, Graph* graph, char destination);
void reconstructPath(DijkstraResult* result, Graph* graph, char destination, char* pathStr);

DeliverySequence* optimizeDeliveryRoute(Graph* graph, DeliveryList* deliveries, char warehouse);
void freeDeliverySequence(DeliverySequence* sequence);
void displayDeliverySequence(DeliverySequence* sequence);
int selectNextDelivery(Graph* graph, DeliveryList* deliveries, char currentLocation, DijkstraResult* dijkstraResult);
double calculateDeliveryScore(Delivery* delivery, int distance);


Graph* createGraph() {
    Graph* graph = (Graph*)malloc(sizeof(Graph));
    if (!graph) return NULL;

    graph->vertices = MAX_VERTICES;
    graph->vertexCount = 0;

    for (int i = 0; i < MAX_VERTICES; i++) {
        graph->adjList[i] = NULL;
        graph->vertexNames[i] = '\0';
    }

    return graph;
}

void freeGraph(Graph* graph) {
    if (!graph) return;

    for (int i = 0; i < graph->vertexCount; i++) {
        Edge* current = graph->adjList[i];
        while (current) {
            Edge* temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(graph);
}

int getVertexIndex(Graph* graph, char vertex) {
    for (int i = 0; i < graph->vertexCount; i++) {
        if (graph->vertexNames[i] == vertex) {
            return i;
        }
    }
    return -1;
}

int addVertex(Graph* graph, char vertex) {
    if (getVertexIndex(graph, vertex) != -1) {
        return getVertexIndex(graph, vertex);
    }

    if (graph->vertexCount >= MAX_VERTICES) {
        return -1;
    }

    graph->vertexNames[graph->vertexCount] = vertex;
    return graph->vertexCount++;
}

int addEdge(Graph* graph, char src, char dest, int weight) {
    int srcIndex = addVertex(graph, src);
    int destIndex = addVertex(graph, dest);

    if (srcIndex == -1 || destIndex == -1) {
        return -1;
    }

    Edge* newEdge1 = (Edge*)malloc(sizeof(Edge));
    if (!newEdge1) return -1;
    newEdge1->destination = dest;
    newEdge1->weight = weight;
    newEdge1->next = graph->adjList[srcIndex];
    graph->adjList[srcIndex] = newEdge1;

    Edge* newEdge2 = (Edge*)malloc(sizeof(Edge));
    if (!newEdge2) {
        free(newEdge1);
        return -1;
    }
    newEdge2->destination = src;
    newEdge2->weight = weight;
    newEdge2->next = graph->adjList[destIndex];
    graph->adjList[destIndex] = newEdge2;

    return 0;
}

void displayGraph(Graph* graph) {
    printf("Graph Structure (Vertices: %d):\n", graph->vertexCount);
    for (int i = 0; i < graph->vertexCount; i++) {
        printf("%c: ", graph->vertexNames[i]);
        Edge* current = graph->adjList[i];
        while (current) {
            printf("-> %c(%d) ", current->destination, current->weight);
            current = current->next;
        }
        printf("\n");
    }
}

int loadMapFromFile(Graph* graph, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return -1;
    }

    char src, dest;
    int weight;
    int edgesLoaded = 0;

    while (fscanf(file, " %c %c %d", &src, &dest, &weight) == 3) {
        if (addEdge(graph, src, dest, weight) == 0) {
            edgesLoaded++;
        }
    }

    fclose(file);
    printf("Loaded %d edges successfully.\n", edgesLoaded);
    return 0;
}
int loadDeliveriesFromFile(DeliveryList* deliveries, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return -1;
    }

    deliveries->count = 0;
    char location;
    char priorityStr[20];
    int timeStart, timeEnd;

    while (fscanf(file, " %c %s %d %d", &location, priorityStr, &timeStart, &timeEnd) == 4) {
        if (deliveries->count >= MAX_DELIVERIES) break;

        deliveries->deliveries[deliveries->count].location = location;
        deliveries->deliveries[deliveries->count].priority = getPriorityFromString(priorityStr);
        deliveries->deliveries[deliveries->count].timeWindowStart = timeStart;
        deliveries->deliveries[deliveries->count].timeWindowEnd = timeEnd;
        deliveries->deliveries[deliveries->count].completed = 0;
        deliveries->count++;
    }

    fclose(file);
    return 0;
}

Priority getPriorityFromString(const char* priorityStr) {
    if (strcmp(priorityStr, "High") == 0) return HIGH;
    if (strcmp(priorityStr, "Medium") == 0) return MEDIUM;
    return LOW;
}

const char* getPriorityString(Priority priority) {
    switch (priority) {
        case HIGH: return "High";
        case MEDIUM: return "Medium";
        case LOW: return "Low";
        default: return "Unknown";
    }
}

void displayDeliveries(DeliveryList* deliveries) {
    printf("Delivery Requests:\n");
    printf("Location\tPriority\tTime Window\n");
    printf("------------------------------------\n");
    for (int i = 0; i < deliveries->count; i++) {
        printf("%c\t\t%s\t\t%d-%d\n",
               deliveries->deliveries[i].location,
               getPriorityString(deliveries->deliveries[i].priority),
               deliveries->deliveries[i].timeWindowStart,
               deliveries->deliveries[i].timeWindowEnd);
    }
}

DijkstraResult* dijkstra(Graph* graph, char source) {
    int sourceIndex = getVertexIndex(graph, source);
    if (sourceIndex == -1) return NULL;

    DijkstraResult* result = (DijkstraResult*)malloc(sizeof(DijkstraResult));
    if (!result) return NULL;

    result->source = source;
    result->vertexCount = graph->vertexCount;

    int distances[MAX_VERTICES];
    int visited[MAX_VERTICES];
    char previous[MAX_VERTICES];

    for (int i = 0; i < graph->vertexCount; i++) {
        distances[i] = INF;
        visited[i] = 0;
        previous[i] = '\0';
        result->paths[i].distance = INF;
        result->paths[i].previous = '\0';
    }

    distances[sourceIndex] = 0;
    result->paths[sourceIndex].distance = 0;

    for (int count = 0; count < graph->vertexCount; count++) {
        int u = getMinDistanceVertex(distances, visited, graph->vertexCount);
        if (u == -1 || distances[u] == INF) break;

        visited[u] = 1;

        Edge* current = graph->adjList[u];
        while (current) {
            int v = getVertexIndex(graph, current->destination);
            if (v != -1 && !visited[v]) {
                int newDistance = distances[u] + current->weight;
                if (newDistance < distances[v]) {
                    distances[v] = newDistance;
                    previous[v] = graph->vertexNames[u];
                    result->paths[v].distance = newDistance;
                    result->paths[v].previous = graph->vertexNames[u];
                }
            }
            current = current->next;
        }
    }

    return result;
}

int getMinDistanceVertex(int distances[], int visited[], int vertexCount) {
    int minDistance = INF;
    int minIndex = -1;

    for (int v = 0; v < vertexCount; v++) {
        if (!visited[v] && distances[v] < minDistance) {
            minDistance = distances[v];
            minIndex = v;
        }
    }
    return minIndex;
}

int getShortestDistance(DijkstraResult* result, Graph* graph, char destination) {
    int destIndex = getVertexIndex(graph, destination);
    if (destIndex == -1 || !result) return INF;
    return result->paths[destIndex].distance;
}

void reconstructPath(DijkstraResult* result, Graph* graph, char destination, char* pathStr) {
    char tempPath[MAX_VERTICES];
    int pathLength = 0;
    char current = destination;
    int destIndex = getVertexIndex(graph, destination);

    if (destIndex == -1 || result->paths[destIndex].distance == INF) {
        strcpy(pathStr, "No path");
        return;
    }

    while (current != result->source && current != '\0') {
        tempPath[pathLength++] = current;
        int currentIndex = getVertexIndex(graph, current);
        if (currentIndex == -1) break;
        current = result->paths[currentIndex].previous;
    }

    if (current == result->source) {
        tempPath[pathLength++] = result->source;
    }

    pathStr[0] = '\0';
    for (int i = pathLength - 1; i >= 0; i--) {
        if (strlen(pathStr) > 0) {
            strcat(pathStr, " -> ");
        }
        char vertex[2] = {tempPath[i], '\0'};
        strcat(pathStr, vertex);
    }
}

void freeDijkstraResult(DijkstraResult* result) {
    if (result) free(result);
}
double calculateDeliveryScore(Delivery* delivery, int distance) {
    double priorityWeight = (double)delivery->priority * 10.0;
    double distanceWeight = distance > 0 ? 50.0 / (double)distance : 50.0;
    return priorityWeight + distanceWeight;
}

int selectNextDelivery(Graph* graph, DeliveryList* deliveries, char currentLocation, DijkstraResult* dijkstraResult) {
    int bestIndex = -1;
    double bestScore = -1.0;

    for (int i = 0; i < deliveries->count; i++) {
        if (deliveries->deliveries[i].completed) continue;

        int distance = getShortestDistance(dijkstraResult, graph, deliveries->deliveries[i].location);
        if (distance == INF) continue;

        double score = calculateDeliveryScore(&deliveries->deliveries[i], distance);

        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }
    return bestIndex;
}
DeliverySequence* optimizeDeliveryRoute(Graph* graph, DeliveryList* deliveries, char warehouse) {
    DeliverySequence* sequence = (DeliverySequence*)malloc(sizeof(DeliverySequence));
    if (!sequence) return NULL;

    sequence->head = NULL;
    sequence->totalSteps = 0;
    sequence->totalCost = 0;
    sequence->warehouse = warehouse;

    char currentLocation = warehouse;
    DeliveryStep* lastStep = NULL;

    for (int completed = 0; completed < deliveries->count; completed++) {
        DijkstraResult* dijkstraResult = dijkstra(graph, currentLocation);
        if (!dijkstraResult) break;

        int nextDeliveryIndex = selectNextDelivery(graph, deliveries, currentLocation, dijkstraResult);
        if (nextDeliveryIndex == -1) {
            freeDijkstraResult(dijkstraResult);
            break;
        }

        deliveries->deliveries[nextDeliveryIndex].completed = 1;
        char nextLocation = deliveries->deliveries[nextDeliveryIndex].location;
        int distance = getShortestDistance(dijkstraResult, graph, nextLocation);

        DeliveryStep* step = (DeliveryStep*)malloc(sizeof(DeliveryStep));
        if (!step) {
            freeDijkstraResult(dijkstraResult);
            break;
        }

        step->location = nextLocation;
        step->priority = deliveries->deliveries[nextDeliveryIndex].priority;
        step->cost = distance;
        step->stepNumber = sequence->totalSteps + 1;
        step->next = NULL;

        reconstructPath(dijkstraResult, graph, nextLocation, step->path);

        if (!sequence->head) {
            sequence->head = step;
        } else {
            lastStep->next = step;
        }
        lastStep = step;

        sequence->totalSteps++;
        sequence->totalCost += distance;
        currentLocation = nextLocation;

        freeDijkstraResult(dijkstraResult);
    }

    return sequence;
}

void displayDeliverySequence(DeliverySequence* sequence) {
    if (!sequence || !sequence->head) {
        printf("No delivery sequence available.\n");
        return;
    }

    printf("Delivery Sequence:\n");
    DeliveryStep* current = sequence->head;

    while (current) {
        printf("%d. %c (%s Priority) via path %s [Cost: %d]\n",
               current->stepNumber,
               current->location,
               getPriorityString(current->priority),
               current->path,
               current->cost);
        current = current->next;
    }
}

void freeDeliverySequence(DeliverySequence* sequence) {
    if (!sequence) return;

    DeliveryStep* current = sequence->head;
    while (current) {
        DeliveryStep* temp = current;
        current = current->next;
        free(temp);
    }
    free(sequence);
}

int main() {
    printf("=== Delivery Route Optimizer for Urban Logistics ===\n\n");

    Graph* graph = createGraph();
    if (!graph) {
        printf("Error: Failed to create graph\n");
        return 1;
    }

    printf("Loading map data from map.txt...\n");
    if (loadMapFromFile(graph, "map.txt") != 0) {
        printf("Error: Could not load map data\n");
        freeGraph(graph);
        return 1;
    }

    printf("Loading delivery requests from deliveries.txt...\n");
    DeliveryList deliveries;
    if (loadDeliveriesFromFile(&deliveries, "deliveries.txt") != 0) {
        printf("Error: Could not load delivery data\n");
        freeGraph(graph);
        return 1;
    }

    printf("Data loaded successfully!\n\n");

    printf("=== MAP STRUCTURE ===\n");
    displayGraph(graph);

    printf("\n=== DELIVERY REQUESTS ===\n");
    displayDeliveries(&deliveries);

    char warehouse = 'A';
    printf("\n=== ROUTE OPTIMIZATION ===\n");
    printf("Starting from Warehouse: %c\n\n", warehouse);

    DeliverySequence* sequence = optimizeDeliveryRoute(graph, &deliveries, warehouse);
    if (!sequence) {
        printf("Error: Route optimization failed\n");
        freeGraph(graph);
        return 1;
    }

    printf("=== OPTIMIZED DELIVERY SEQUENCE ===\n");
    displayDeliverySequence(sequence);
    printf("\nTotal Delivery Cost: %d\n", sequence->totalCost);

    freeDeliverySequence(sequence);
    freeGraph(graph);

    printf("\n=== PROGRAM COMPLETED SUCCESSFULLY ===\n");
    return 0;
}
