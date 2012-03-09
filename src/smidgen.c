// an initial cut at a select-based inotify reader
// compile me with gcc -O2 -o smidgen smidgen.c -levent
// you will need libevent-dev installed on your system (for debian)

/*
 * references:
 * http://qaa.ath.cx/inotify.html
 * http://developerweb.net/viewtopic.php?id=2933
 * tupleserver
 * cliserver
 * https://github.com/wereHamster/transmission/blob/master/third-party/libevent/test/test-time.c
 *
 */

#include <signal.h>
#include <stdio.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <event.h>
#include <fcntl.h>

#define ARGS IN_ONESHOT | IN_ACCESS

#define EVENT_SIZE 1024

/* reasonable guess as to size of 1024 events */
#define BUF_LEN        (1024 * (EVENT_SIZE + 16))
char buf[BUF_LEN];

int g_fd;

uint32_t g_wd;

static struct event_base *server_loop;

void control_c(int ignored) {
	inotify_rm_watch(g_fd, g_wd);
  fprintf(stderr, "attempting to quit...");
	if(event_base_loopexit(server_loop, NULL)) {
		fprintf(stderr,"Error shutting down server\n");
	}
	_exit (0);
}

static const char *fn;
static struct event ev;

void control_mouse(int enable){
  const char *flag = enable ? "1" : "0";

  pid_t pID = vfork();
  if(0 == pID){
    int ret = execl("/usr/bin/xinput", "/usr/bin/xinput", "set-int-prop", "12", "Device Enabled", "8", flag, (char *) 0);
    if (-1 == ret){
      perror("error executing xinput!");
    }
  } else if(pID < 0){
    perror("error forking to xinput!!!");
  }
}

////////////
static void time_cb(int fd, short event, void *arg)
{
  control_mouse(1);

  //evtimer_del(&ev);
}

static void handle_event(int g_fd, short evtype, void *arg){
  int len = read (g_fd, buf, BUF_LEN);


  evtimer_del(&ev);

  if (len > 0) {
    control_mouse(0);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 200000;
    evtimer_add(&ev, &tv);
  }

  g_wd = inotify_add_watch(g_fd,
      fn,
      ARGS);

  if (g_wd < 0) {
    perror ("inotify_add_watch");
  }
  // fflush(0);
}

static int set_nonblock(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL);
	if(flags == -1) {
		fprintf(stderr,"Error getting flags on fd %d", fd);
		return -1;
	}
	flags |= O_NONBLOCK;
	if(fcntl(fd, F_SETFL, flags)) {
		fprintf(stderr,"Error setting non-blocking I/O on fd %d", fd);
		return -1;
	}

	return 0;
}

int main(int argc, char*argv[]){
  if(argc != 2){
    fprintf(stderr,"\nusage: %s <file>\n\n", argv[0]);
    return 1;
  }

  fn = argv[1];

	signal(SIGINT, control_c);
	signal(SIGCHLD, SIG_IGN);

	g_fd = inotify_init();
	if (g_fd < 0) {
	        perror ("inotify_init");
	}
	g_wd = inotify_add_watch(g_fd,
		fn,
		ARGS);

	if (g_wd < 0) {
    perror ("inotify_add_watch");
  }

	// Initialize libevent
	printf("libevent version: %s\n", event_get_version());
	struct event_base *evloop = event_base_new();
	if(NULL == evloop) {
		perror("Error initializing event loop.\n");
		return -1;
	}
	server_loop = evloop;
	printf("libevent is using %s for events.\n", event_base_get_method(evloop));
	// Set socket for non-blocking I/O
	if(set_nonblock(g_fd)) {
		perror("Error setting listening socket to non-blocking I/O.\n");
		return -1;
	}

	struct event connect_event;
	// Add an event to wait for connections
	event_set(&connect_event, g_fd, EV_READ | EV_PERSIST, handle_event, evloop);
	event_base_set(evloop, &connect_event);
	if(event_add(&connect_event, NULL)) {
		fprintf(stderr,"Error scheduling connection event on the event loop.\n");
	}

  evtimer_set(&ev, time_cb, NULL);
	event_base_set(evloop, &ev);

	// Start the event loop
	if(event_base_dispatch(evloop)) {
		fprintf(stderr,"Error running event loop.\n");
	}

	// Clean up libevent
	if(event_del(&connect_event)) {
		fprintf(stderr,"Error removing connection event from the event loop.\n");
	}
	event_base_free(evloop);

  return 0;
}
