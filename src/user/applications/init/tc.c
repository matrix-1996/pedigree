#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <time.h>

int frames = 0;

void fps()
{
    static time_t start_time = 0;
    time_t now = time(NULL);
    ++frames;
    if(!start_time)
        start_time = now;
    else if ((now - start_time) >= 5)
    {
        double seconds = now - start_time;
        double fps = frames / seconds;
        printf("%d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds, fps);
        start_time = now;
        frames = 0;
    }
}

int tc()
{
    struct sockaddr_un sun;
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, "unix»/srv.sock");
    socklen_t slen = sizeof(sun);

    struct sockaddr_un sunb;
    sunb.sun_family = AF_UNIX;
    strcpy(sunb.sun_path, "unix»/client.sock");

    int s = socket(AF_UNIX, SOCK_DGRAM, 0);
    int e = bind(s, (struct sockaddr *) &sunb, slen);
    if(e != 0)
        perror("bind");
    e = connect(s, (struct sockaddr *) &sun, slen);
    if(e != 0)
        perror("connect");

    while(1)
    {
        char *buff = (char *) malloc(4096);
        send(s, buff, 4096, 0);
        // recv(s, buff, 4096, 0);
        free(buff);

        fps();
    }

    return 0;
}

#ifndef __PEDIGREE__
int main(int argc, char *argv[])
{
    return tc();
}
#endif
