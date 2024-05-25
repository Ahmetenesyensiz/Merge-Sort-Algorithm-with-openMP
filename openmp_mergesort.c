#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <omp.h>
#include <sys/time.h>

#define SMALL 32

double get_time(void) {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec / 1e6;
}

int read_numbers_from_csv(const char* filename, int** numbers) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    char line[256];
    int capacity = 10;
    int size = 0;
    *numbers = malloc(sizeof(int) * capacity);
    if (*numbers == NULL) {
        perror("Error allocating memory");
        fclose(file);
        return -1;
    }

    // İlk satırı (başlık) atla
    if (fgets(line, sizeof(line), file) == NULL) {
        perror("Error reading header line");
        fclose(file);
        free(*numbers);
        return -1;
    }

    // Sayıları oku
    while (fgets(line, sizeof(line), file) != NULL) {
        int value;
        if (sscanf(line, "%*d,%d", &value) != 1) {
            perror("Error parsing line");
            fclose(file);
            free(*numbers);
            return -1;
        }
        (*numbers)[size++] = value;
        if (size >= capacity) {
            capacity *= 2;
            int* temp = realloc(*numbers, sizeof(int) * capacity);
            if (temp == NULL) {
                perror("Error reallocating memory");
                free(*numbers);
                fclose(file);
                return -1;
            }
            *numbers = temp;
        }
    }

    fclose(file);
    return size;
}

void insertion_sort(int a[], int size) {
    for (int i = 0; i < size; i++) {
        int j;
        int v = a[i];
        for (j = i - 1; j >= 0; j--) {
            if (a[j] <= v)
                break;
            a[j + 1] = a[j];
        }
        a[j + 1] = v;
    }
}

void merge(int a[], int size, int temp[]) {
    int i1 = 0;
    int i2 = size / 2;
    int tempi = 0;
    while (i1 < size / 2 && i2 < size) {
        if (a[i1] < a[i2]) {
            temp[tempi] = a[i1];
            i1++;
        } else {
            temp[tempi] = a[i2];
            i2++;
        }
        tempi++;
    }
    while (i1 < size / 2) {
        temp[tempi] = a[i1];
        i1++;
        tempi++;
    }
    while (i2 < size) {
        temp[tempi] = a[i2];
        i2++;
        tempi++;
    }
    memcpy(a, temp, size * sizeof(int));
}

void mergesort_serial(int a[], int size, int temp[]) {
    if (size <= SMALL) {
        insertion_sort(a, size);
        return;
    }
    mergesort_serial(a, size / 2, temp);
    mergesort_serial(a + size / 2, size - size / 2, temp);
    merge(a, size, temp);
}

void mergesort_parallel_omp(int a[], int size, int temp[], int threads) {
    if (threads == 1) {
        mergesort_serial(a, size, temp);
    } else if (threads > 1) {
        #pragma omp parallel sections
        {
            #pragma omp section
            {
                mergesort_parallel_omp(a, size / 2, temp, threads / 2);
            }
            #pragma omp section
            {
                mergesort_parallel_omp(a + size / 2, size - size / 2, temp + size / 2, threads - threads / 2);
            }
        }
        merge(a, size, temp);
    } else {
        printf("Error: %d threads\n", threads);
        return;
    }
}

void run_omp(int a[], int size, int temp[], int threads) {
    omp_set_nested(1);
    mergesort_parallel_omp(a, size, temp, threads);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s filename\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    int size = 0;
    int* a = NULL;
    size = read_numbers_from_csv(filename, &a);
    if (size <= 0) {
        printf("Error: Could not read numbers from file %s\n", filename);
        return 1;
    }

    int *temp = malloc(sizeof(int) * size);
    if (temp == NULL) {
        printf("Error: Could not allocate temp array of size %d\n", size);
        free(a);
        return 1;
    }

    printf("Array size = %d\n", size);

    int threads = omp_get_max_threads();
    printf("Using %d threads\n", threads);

    double start = get_time();
    run_omp(a, size, temp, threads);
    double end = get_time();
    printf("Start = %.2f\nEnd = %.2f\nElapsed = %.2f\n", start, end, end - start);

    for (int i = 1; i < size; i++) {
        if (!(a[i - 1] <= a[i])) {
            printf("Implementation error: a[%d]=%d > a[%d]=%d\n", i - 1, a[i - 1], i, a[i]);
            free(a);
            free(temp);
            return 1;
        }
    }
    puts("-Success-");

    free(a);
    free(temp);
    return 0;
}