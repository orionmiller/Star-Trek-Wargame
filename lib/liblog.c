#include "liblog.h"

void open_log(log *Log)
{
  if ((Log->fd = open(Log->filename, Log->flags, Log->mode)) < 0)
    {
      perror("Problem Opening Log File");
      printf("ERROR\n");
      Log->fd = STDOUT_FILENO;
    }
}

/*Make sure safe malloc call*/
log *gen_log(char *filename)
{
  log *Log = malloc(sizeof(log));
  Log->flags = LOG_FLAGS;
  Log->mode = LOG_MODE;
  Log->filename = (const char *)filename;
  return Log;
}

log *setup_log(char *filename)
{
  log *Log = gen_log(filename);
  open_log(Log);
  return Log;
}

void write_log(log *Log, char *msg)
{
  char *time_str;
  Log->tm = time(NULL);
  time_str = ctime(&(Log->tm));
  time_str[strlen(time_str)-1] = '\0';
  snprintf(Log->msg, MSG_SIZE, "%s: %s\n", time_str, msg);
  write(Log->fd, Log->msg, strlen(Log->msg));
}

void write_errlog(log *Log,char *msg)
{
  char *time_str;
  Log->tm = time(NULL);
  time_str = ctime(&(Log->tm));
  time_str[strlen(time_str)-1] = '\0';
  snprintf(Log->msg, MSG_SIZE, "%s: %s : %s\n", time_str, msg, strerror(errno));
  write(Log->fd, Log->msg, strlen(Log->msg));
}
