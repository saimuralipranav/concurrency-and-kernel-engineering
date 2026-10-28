#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int fd;
  struct stat st;
  char buf[512];
  int n;

  if (argc != 2) {
    fprintf(2, "Usage: readlink <path>\n");
    exit(1);
  }

  if ((fd = open(argv[1], O_RDONLY | O_NOFOLLOW)) < 0) {
    fprintf(2, "readlink: cannot open %s\n", argv[1]);
    exit(1);
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "readlink: cannot stat %s\n", argv[1]);
    close(fd);
    exit(1);
  }

  if (st.type != T_SYMLINK) {
    fprintf(2, "readlink: %s: not a symbolic link\n", argv[1]);
    close(fd);
    exit(1);
  }

  n = read(fd, buf, sizeof(buf) - 1);
  close(fd);

  if (n < 0) {
    fprintf(2, "readlink: cannot read %s\n", argv[1]);
    exit(1);
  }

  buf[n] = '\0';
  printf("%s\n", buf);
  exit(0);
}
