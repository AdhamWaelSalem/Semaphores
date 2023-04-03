#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>

typedef struct Node {
    int value;
    struct Node *next;
} Node;

typedef struct {
    Node *front;
    Node *rear;
} Queue;

Queue *init();
int isEmpty(Queue *q);
int isFull(Queue *q);
int dequeue(Queue *q);
void enqueue(Queue *q, int val);
void destroy(Queue *q);
void *counterThread(void *arg);
_Noreturn void *monitorThread(void *arg);
_Noreturn void *collectorThread(void *arg);

sem_t counter_s, buffer_s,  buffer_occupied, buffer_free;
int counter, accumulated;
int counterWait, collectorWait, monitorWait;
Queue* buffer;

int main() {
    srandom(time(NULL));
    counter = 0;
    accumulated = 0;
    buffer = init();
    int *number_t;
    int n;
    int bufferSize;
    char answer;
    printf("Do you want to custom parameters? Enter(y for yes / anything else for default)\n");
    scanf("%c",&answer);
    if (answer == 'y') {
        printf("Enter Number of Counter Threads:\n");
        scanf("%d",&n);
        printf("Enter size of Buffer:\n");
        scanf("%d",&bufferSize);
        printf("Enter Max Waiting Time for Counter Threads:\n");
        scanf("%d",&counterWait);
        printf("Enter Max Waiting Time for Monitor Thread:\n");
        scanf("%d",&monitorWait);
        printf("Enter Max Waiting Time for Collector Thread:\n");
        scanf("%d",&collectorWait);
    }
    else {
        n = 9;
        bufferSize = 3;
        counterWait = 11;
        monitorWait = 11;
        collectorWait = 22;
        printf("\nDefault Parameters\nBuffer Size = 3\n");
        printf("Counter Threads = 9\n");
        printf("Counter Thread max waiting 11 seconds\n");
        printf("Monitor Thread max waiting 11 seconds\n");
        printf("Collector Thread max waiting 22 seconds\n\n");
    }

    pthread_t counter_t[n], monitor_t, collector_t;
    sem_init(&counter_s, 0, 1);
    sem_init(&buffer_s, 0, 1);
    sem_init(&buffer_occupied, 0, 0);
    sem_init(&buffer_free, 0, bufferSize);
    for (int i = 0; i < n; ++i) {
        number_t = malloc(sizeof(int));
        *number_t = i;
        if (pthread_create(counter_t + i, NULL, &counterThread, number_t) != 0)
            perror("Thread creation failed.\n");
    }
    if (pthread_create(&monitor_t, NULL, &monitorThread, NULL) != 0)
        perror("Thread creation failed.\n");
    if (pthread_create(&collector_t, NULL, &collectorThread, NULL) != 0)
        perror("Thread creation failed.\n");
    for (int i = 0; i < n; ++i) {
        if (pthread_join(counter_t[i], (void **) &number_t) != 0)
            perror("Thread joining failed.\n");
        free(number_t);
    }
    if (pthread_join(monitor_t, NULL) != 0)
        perror("Thread joining failed.\n");
    if (pthread_join(collector_t, NULL) != 0)
        perror("Thread joining failed.\n");
    sem_destroy(&counter_s);
    sem_destroy(&buffer_s);
    sem_destroy(&buffer_occupied);
    sem_destroy(&buffer_free);
    destroy(buffer);
    return 0;
}

void *counterThread(void *arg) {
    int number = *(int *) arg + 1;
    while (1) {
        printf("Counter Thread %d --> Waiting For Access...\n", number);
        sem_wait(&counter_s);
        counter++;
        printf("Counter Thread %d --> Incrementing Counter   : Counter Value = %d\n", number, counter);
        sem_post(&counter_s);
        sleep(random() % counterWait);
        printf("Counter Thread %d --> Received A Message\n", number);
    }
    return arg;
}

_Noreturn void *monitorThread(void *arg) {
    int data;
    int temp;
    while (1) {
        printf("Monitor Thread   --> Waiting For Access...\n");
        sem_wait(&counter_s);
        printf("Monitor Thread   --> Reading Counter        : Counted Messages = %d\n", counter);
        data = counter;
        counter = 0;
        sem_post(&counter_s);
        sem_getvalue(&buffer_free, &temp);
        if (temp <= 0) {
            printf("Monitor Thread   --> Buffer is Full\n");
        }
        sem_wait(&buffer_free);
        sem_wait(&buffer_s);
        printf("Monitor Thread   --> Inserting in Buffer    : Entry Value = %d\n", data);
        enqueue(buffer, data);
        sem_post(&buffer_s);
        sem_post(&buffer_occupied);
        sleep(random() % monitorWait);
        printf("Monitor Thread   --> Reactivated\n");
    }
}

_Noreturn void *collectorThread(void *arg) {
    int temp;
    int data;
    while (1) {
        sem_getvalue(&buffer_occupied, &temp);
        if (temp <= 0) {
            printf("Collector Thread --> Buffer is Empty\n");
        }
        sem_wait(&buffer_occupied);
        sem_wait(&buffer_s);
        data = dequeue(buffer);
        sem_post(&buffer_s);
        sem_post(&buffer_free);
        accumulated += data;
        printf("Collector Thread --> Retrieving from Buffer : Retrieved Value = %d, Accumulated Messages = %d\n", data, accumulated);
        sleep(random() % collectorWait);
        printf("Collector Thread --> Reactivated\n");
    }
}

Queue *init() {
    Queue *q = (Queue *) malloc(sizeof(Queue));
    q->front = NULL;
    q->rear = NULL;
    return q;
}

void enqueue(Queue *q, int val) {
    if (q->front == NULL) {
        q->front = (Node *) malloc(sizeof(Node));
        q->rear = q->front;
    } else {
        q->rear->next = (Node *) malloc(sizeof(Node));
        q->rear = q->rear->next;
    }
    q->rear->value = val;
    q->rear->next = NULL;
}

int dequeue(Queue *q) {
    int val;
    Node *temp;
    val = q->front->value;
    temp = q->front->next;
    if (q->front == q->rear) {
        q->rear = NULL;
    }
    free(q->front);
    q->front = temp;
    return val;
}

int isEmpty(Queue *q) {
    if (q->front == NULL && q->rear == NULL)
        return 1;
    else
        return 0;
}

void destroy(Queue *q) {
    while (!isEmpty(q))
        dequeue(q);
    free(q);
}