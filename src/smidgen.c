// an initial cut at a select-based inotify reader

// thanks to http://qaa.ath.cx/inotify.html and http://developerweb.net/viewtopic.php?id=2933
// for the example code

#include <signal.h>
#include <stdio.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>

#define ARGS IN_ONESHOT | IN_ALL_EVENTS

#define EVENT_SIZE 1024

/* reasonable guess as to size of 1024 events */
#define BUF_LEN        (1024 * (EVENT_SIZE + 16))
char buf[BUF_LEN];

int g_fd;

uint32_t g_wd;

void control_c(int ignored) {
	inotify_rm_watch(g_fd, g_wd);
	_exit (0);
}

int main(int argc, char*argv[]){
  if(argc != 2){
    fprintf(stderr,"\nusage: %s <file>\n\n", argv[0]);
    return 1;
  }

	signal(SIGINT, control_c);

	g_fd = inotify_init();
	if (g_fd < 0) {
	        perror ("inotify_init");
	}
	g_wd = inotify_add_watch(g_fd,
		argv[1],
		ARGS);

	if (g_wd < 0) {
    perror ("inotify_add_watch");
  }

  while(1){
    int result;
    fd_set readset;
    do {
      FD_ZERO(&readset);
      FD_SET(g_fd, &readset);
      result = select(g_fd + 1, &readset, NULL, NULL, NULL);
    } while (result == -1 && errno == EINTR);

    if(result > 0){
      int len = read (g_fd, buf, BUF_LEN);

      if (len > 0) {
        printf("events seen: %d\n",len/sizeof(struct inotify_event));
      }

      g_wd = inotify_add_watch(g_fd,
          argv[1],
          ARGS);

      if (g_wd < 0) {
        perror ("inotify_add_watch");
      }
      // fflush(0);
    }
  }

  return 0;
}
