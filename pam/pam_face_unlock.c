#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <security/pam_ext.h>
#include <security/pam_modules.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <syslog.h>
#include <unistd.h>

static int option_debug(int argc, const char **argv) {
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "debug") == 0) {
      return 1;
    }
  }

  return 0;
}

static int option_timeout_ms(int argc, const char **argv) {
  int timeout_ms = 700;

  for (int i = 0; i < argc; ++i) {
    const char *prefix = "timeout_ms=";
    const size_t prefix_len = strlen(prefix);

    if (strncmp(argv[i], prefix, prefix_len) == 0) {
      int parsed = atoi(argv[i] + prefix_len);

      if (parsed >= 50 && parsed <= 5000) {
        timeout_ms = parsed;
      }
    }
  }

  return timeout_ms;
}

static uid_t resolve_target_uid(pam_handle_t *pamh, int debug) {
  const char *env_uid = getenv("FACE_UNLOCK_UID");

  if (env_uid != NULL && env_uid[0] != '\0') {
    uid_t uid = (uid_t)strtoul(env_uid, NULL, 10);

    if (debug) {
      pam_syslog(pamh, LOG_DEBUG, "face_unlock: FACE_UNLOCK_UID=%lu",
                 (unsigned long)uid);
    }

    return uid;
  }

  const char *pam_ruser = NULL;
  const char *pam_user = NULL;

  pam_get_item(pamh, PAM_RUSER, (const void **)&pam_ruser);
  pam_get_user(pamh, &pam_user, NULL);

  const char *candidate = NULL;

  if (pam_ruser != NULL && pam_ruser[0] != '\0') {
    candidate = pam_ruser;
  } else if (pam_user != NULL && pam_user[0] != '\0') {
    candidate = pam_user;
  }

  if (candidate != NULL) {
    struct passwd *pw = getpwnam(candidate);

    if (pw != NULL) {
      if (debug) {
        pam_syslog(pamh, LOG_DEBUG,
                   "face_unlock: resolved user=%s uid=%lu",
                   candidate, (unsigned long)pw->pw_uid);
      }

      return pw->pw_uid;
    }
  }

  return getuid();
}

static void build_socket_path(char *out, size_t out_size, uid_t uid) {
  snprintf(out, out_size, "/run/user/%lu/face-unlock.sock",
           (unsigned long)uid);
}

static int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);

  if (flags < 0) {
    return -1;
  }

  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int connect_socket_with_timeout(
  pam_handle_t *pamh,
  const char *socket_path,
  int timeout_ms
) {
  int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

  if (fd < 0) {
    pam_syslog(pamh, LOG_NOTICE, "face_unlock: socket failed: %s", strerror(errno));
    return -1;
  }

  if (set_nonblocking(fd) < 0) {
    pam_syslog(pamh, LOG_NOTICE, "face_unlock: fcntl nonblock failed: %s", strerror(errno));
    close(fd);
    return -1;
  }

  struct sockaddr_un address;
  memset(&address, 0, sizeof(address));

  address.sun_family = AF_UNIX;

  if (strlen(socket_path) >= sizeof(address.sun_path)) {
    pam_syslog(pamh, LOG_NOTICE, "face_unlock: socket path too long");
    close(fd);
    return -1;
  }

  strncpy(address.sun_path, socket_path, sizeof(address.sun_path) - 1);

  int rc = connect(fd, (struct sockaddr *)&address, sizeof(address));

  if (rc == 0) {
    return fd;
  }

  if (errno != EINPROGRESS) {
    pam_syslog(pamh, LOG_NOTICE,
               "face_unlock: connect failed for %s: %s",
               socket_path, strerror(errno));
    close(fd);
    return -1;
  }

  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));

  pfd.fd = fd;
  pfd.events = POLLOUT;

  rc = poll(&pfd, 1, timeout_ms);

  if (rc <= 0) {
    pam_syslog(pamh, LOG_NOTICE,
               "face_unlock: connect timeout for %s", socket_path);
    close(fd);
    return -1;
  }

  int socket_error = 0;
  socklen_t socket_error_len = sizeof(socket_error);

  if (getsockopt(fd, SOL_SOCKET, SO_ERROR,
                 &socket_error, &socket_error_len) < 0) {
    pam_syslog(pamh, LOG_NOTICE,
               "face_unlock: getsockopt SO_ERROR failed: %s",
               strerror(errno));
    close(fd);
    return -1;
  }

  if (socket_error != 0) {
    pam_syslog(pamh, LOG_NOTICE,
               "face_unlock: connect completed with error: %s",
               strerror(socket_error));
    close(fd);
    return -1;
  }

  return fd;
}

static int wait_writable_or_readable(int fd, short events, int timeout_ms) {
  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));

  pfd.fd = fd;
  pfd.events = events;

  int rc = poll(&pfd, 1, timeout_ms);

  if (rc <= 0) {
    return -1;
  }

  if ((pfd.revents & events) == 0) {
    return -1;
  }

  return 0;
}

static int send_auth_request(
  pam_handle_t *pamh,
  int fd,
  const char *pam_user,
  int timeout_ms
) {
  char request[512];

  snprintf(request, sizeof(request),
           "{\"op\":\"auth\",\"client\":\"pam_face_unlock\",\"pam_user\":\"%s\"}\n",
           pam_user != NULL ? pam_user : "");

  if (wait_writable_or_readable(fd, POLLOUT, timeout_ms) < 0) {
    pam_syslog(pamh, LOG_NOTICE, "face_unlock: send timeout");
    return -1;
  }

  ssize_t sent = send(fd, request, strlen(request), MSG_NOSIGNAL);

  if (sent < 0) {
    pam_syslog(pamh, LOG_NOTICE, "face_unlock: send failed: %s", strerror(errno));
    return -1;
  }

  return 0;
}

static int read_auth_response(
  pam_handle_t *pamh,
  int fd,
  int timeout_ms,
  int debug
) {
  if (wait_writable_or_readable(fd, POLLIN, timeout_ms) < 0) {
    pam_syslog(pamh, LOG_NOTICE, "face_unlock: response timeout");
    return PAM_AUTH_ERR;
  }

  char response[1024];

  ssize_t n = recv(fd, response, sizeof(response) - 1, 0);

  if (n <= 0) {
    pam_syslog(pamh, LOG_NOTICE, "face_unlock: recv failed");
    return PAM_AUTH_ERR;
  }

  response[n] = '\0';

  if (debug) {
    pam_syslog(pamh, LOG_DEBUG,
               "face_unlock: daemon response: %s", response);
  }

  if (strstr(response, "\"status\":\"ok\"") != NULL &&
      strstr(response, "\"op\":\"auth\"") != NULL) {
    return PAM_SUCCESS;
  }

  return PAM_AUTH_ERR;
}

PAM_EXTERN int pam_sm_authenticate(
  pam_handle_t *pamh,
  int flags,
  int argc,
  const char **argv
) {
  (void)flags;

  const int debug = option_debug(argc, argv);
  const int timeout_ms = option_timeout_ms(argc, argv);
  const uid_t target_uid = resolve_target_uid(pamh, debug);

  char socket_path[256];
  build_socket_path(socket_path, sizeof(socket_path), target_uid);

  if (debug) {
    pam_syslog(pamh, LOG_DEBUG,
               "face_unlock: connecting to %s timeout_ms=%d",
               socket_path, timeout_ms);
  }

  const char *pam_user = NULL;
  pam_get_user(pamh, &pam_user, NULL);

  int fd = connect_socket_with_timeout(pamh, socket_path, timeout_ms);

  if (fd < 0) {
    return PAM_AUTH_ERR;
  }

  if (send_auth_request(pamh, fd, pam_user, timeout_ms) < 0) {
    close(fd);
    return PAM_AUTH_ERR;
  }

  int result = read_auth_response(pamh, fd, timeout_ms, debug);

  close(fd);
  return result;
}

PAM_EXTERN int pam_sm_setcred(
  pam_handle_t *pamh,
  int flags,
  int argc,
  const char **argv
) {
  (void)pamh;
  (void)flags;
  (void)argc;
  (void)argv;

  return PAM_SUCCESS;
}
