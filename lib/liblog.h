#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#ifndef LIB_LOG_H_
#define LIB_LOG_H_

/* Default Flags & Mode */
#define LOG_FLAGS (O_WRONLY | O_CREAT | O_APPEND)
#define LOG_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

#define MSG_SIZE (256)

typedef struct {
  const char *filename;
  int flags;
  mode_t mode;
  int fd;
  time_t tm;
  char msg[MSG_SIZE];
}log;

void open_log(log *Log);
log *gen_log(char *filename);
log *setup_log(char *filename);
void write_log(log *Log, char *msg);

#endif
