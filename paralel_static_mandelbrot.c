#include <stdio.h>
#include <mpi.h>

#define WIDTH 1200
#define HEIGHT 1200
#define MAX_ITER 255

struct complex {
    double real;
    double imag;
};

int cal_pixel(struct complex c) {
    double z_real = 0;
    double z_imag = 0;
    double z_real2, z_imag2, lengthsq;

    int iter = 0;
    do {
        z_real2 = z_real * z_real;
        z_imag2 = z_imag * z_imag;

        z_imag = 2 * z_real * z_imag + c.imag;
        z_real = z_real2 - z_imag2 + c.real;
        lengthsq = z_real2 + z_imag2;
        iter++;
    } while ((iter < MAX_ITER) && (lengthsq < 4.0));

    return iter;
}

void save_pgm(const char *filename, int image[HEIGHT][WIDTH]) {
    FILE *pgmimg;
    int temp;
    pgmimg = fopen(filename, "wb");
    fprintf(pgmimg, "P2\n");
    fprintf(pgmimg, "%d %d\n", WIDTH, HEIGHT);
    fprintf(pgmimg, "255\n");

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            temp = image[i][j];
            fprintf(pgmimg, "%d ", temp);
        }
        fprintf(pgmimg, "\n");
    }
    fclose(pgmimg);
}

int main() {
    int image[HEIGHT][WIDTH];

    MPI_Init(NULL, NULL);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int rps = HEIGHT / size;

    struct complex c;
// accoding to the rank each processor has its sub region assigned
    int pointer = rank * rps;

    double start_time = MPI_Wtime();

    for (int i = pointer; i < pointer + rps; i++) {
        for (int j = 0; j < WIDTH; j++) {
            c.real = (j - WIDTH / 2.0) * 4.0 / WIDTH;
            c.imag = (i - HEIGHT / 2.0) * 4.0 / HEIGHT;
            image[i][j] = cal_pixel(c);
        }
    }

    if (rank != 0) {
        //slave proceess sending its computations
        MPI_Send(&image[pointer][0], rps * WIDTH, MPI_INT, 0, 1, MPI_COMM_WORLD);
    } else {
     // master recieving and saving them in primary array
        for (int i = 1; i < size; i++) {
         
            MPI_Recv(&image[i * rps][0], rps * WIDTH, MPI_INT, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
//ending time after recivng all computations and saving them
        double end_time = MPI_Wtime();
        double ttime = end_time - start_time;

        save_pgm("Mandelbrot_static.pgm", image);
        printf("The execution time is: %f s\n", ttime);
    }

    MPI_Finalize();

    return 0;
}
