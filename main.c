#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include <errno.h>

// posix specific headers
// allows forking childs in linux
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define OLIVEC_IMPLEMENTATION
#include "olive.c"

#define READ_END 0
#define WRITE_END 1

#define VIDEO_WIDTH 800
#define VIDEO_HEIGHT 600
#define VIDEO_FPS 60
uint32_t video_pixles[VIDEO_WIDTH*VIDEO_HEIGHT];

#define STR2(x) #x
#define STR(x) STR2(x)

int main(void)
{
    // connecting two processes with a pipe ~ unidirectional pipe
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        fprintf(stderr, "ERROR: could not create a pipe: %s\n", strerror(errno));
        return 1;
    }
    // Fork the current process
    pid_t child = fork();
    // if child pid is negative it means that the child process was not created
    if (child < 0) {
        fprintf(stderr, "ERROR: could not fork a child: %s\n", strerror(errno));
        return 1;
    }
    // if you are the child process pid is equal to 0
    if (child == 0) {
        // replace the stdinput with the read end of the pipe
        if (dup2(pipefd[READ_END], STDIN_FILENO) < 0) {
            fprintf(stderr, "ERROR: could not reopen read end of the pipe as stdin: %s\n", strerror(errno));
            return 1;
        }
        close(pipefd[WRITE_END]);

        int ret = execlp("ffmpeg",
             "ffmpeg",
             "-loglevel", "verbose",
             "-y",
             "-f", "rawvideo",
             "-pix_fmt", "rgb32",
             "-s", STR(VIDEO_WIDTH) "x" STR(VIDEO_HEIGHT),
             "-r", STR(VIDEO_FPS),
             "-an",
             "-i", "-", 
             "-c:v", "libx264",
             "output.mp4",
             NULL
        );
        if (ret < 0) {
            fprintf(stderr, "ERROR: could not run ffmpeg as a child process: %s\n", strerror(errno));
            return 1;
        }
        assert(0 && "unreachable");
    }

    close(pipefd[READ_END]);

    Olivec_Canvas oc = olivec_canvas(video_pixles, VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_WIDTH);

    size_t duration = 10;
    float x =(float)VIDEO_WIDTH/2;
    float y =(float)VIDEO_HEIGHT/2;
    float r =(float)VIDEO_HEIGHT/8;
    float dx = 500;
    float dy = 500;
    float dt = 1.f/VIDEO_FPS;
    for (size_t i = 0; i < VIDEO_FPS*duration; ++i) {
        float nx = x + dx*dt;
        if (0 < nx - r&& nx + r < VIDEO_WIDTH) {
            x = nx;
        } else {
            dx = -dx;
        }
        float ny = y + dy*dt;
        if (0 < ny - r && ny + r < VIDEO_HEIGHT) {
            y = ny;
        } else {
            dy = -dy;
        }
        olivec_fill(oc, 0xFF181818);
        olivec_circle(oc, nx, ny, r, 0xFF00FF00);
        write(pipefd[WRITE_END], video_pixles, sizeof(*video_pixles)*VIDEO_WIDTH*VIDEO_HEIGHT); 
    }

    close(pipefd[WRITE_END]);

    // wait for the child to finish executing
    wait(NULL);

    printf("Done rendering the video!\n");

    return 0;
}
