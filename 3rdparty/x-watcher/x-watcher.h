#ifndef __X_WATCHER_H
#define __X_WATCHER_H

// necessary includes for the public stuff
#include <stdbool.h>
#if defined(__linux__)
#include <pthread.h>
// noop
#elif defined(__WIN32__) || defined(_WIN32)
#include <windows.h>
#include <stdint.h>
#else
#error "Unsupported"
#endif

// PUBLIC STUFF
typedef enum event {
  XWATCHER_FILE_UNSPECIFIED,
  XWATCHER_FILE_REMOVED,
  XWATCHER_FILE_CREATED,
  XWATCHER_FILE_MODIFIED,
  XWATCHER_FILE_OPENED,
  XWATCHER_FILE_ATTRIBUTES_CHANGED,
  XWATCHER_FILE_NONE,
  XWATCHER_FILE_RENAMED,
  // probs more but i couldn't care much
} XWATCHER_FILE_EVENT;

typedef struct xWatcher_reference {
  char *path;
  void (*callback_func)(XWATCHER_FILE_EVENT event, const char *path, int context,
                        void *additional_data);
  int context;
  void *additional_data;
} xWatcher_reference;

struct file {
  // just the file name alone
  char *name;
  // used for adding (additional) context in the handler (if needed)
  int context;
  // in case you'd like to avoid global variables
  void *additional_data;

  void (*callback_func)(XWATCHER_FILE_EVENT event, const char *path, int context,
                        void *additional_data);
} file;

struct directory {
  // list of files
  struct file *files;

  char *path;
  // used for adding (additional) context in the handler (if needed)
  int context;
  // in case you'd like to avoid global variables
  void *additional_data;

  void (*callback_func)(XWATCHER_FILE_EVENT event, const char *path, int context,
                        void *additional_data);

#if defined(__linux__)
  // we need additional file descriptors (per directory basis)
  int inotify_watch_fd;
#elif defined(__WIN32__) || defined(_WIN32)
  HANDLE handle;
  OVERLAPPED overlapped;
  uint8_t *event_buffer;
#else
#error "Unsupported"
#endif
} directory;

typedef struct x_watcher {
  struct directory *directories;

  int thread_id;
  bool alive;

#if defined(__linux__)
  pthread_t thread;
  int inotify_fd; // fd == file descriptor (a common UNIX thing)
#elif defined(__WIN32__) || defined(_WIN32)
  // literal noop
#else
#error "Unsupported"
#endif
} x_watcher;

// PRIVATE STUFF
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"

#ifdef __linux__
#include <sys/types.h>
#include <sys/inotify.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>  // for POLLIN
#include <fcntl.h> // for O_NONBLOCK

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN    (1024 * (EVENT_SIZE + 16))
#define DIRBRK     '/'

static inline void *__internal_xWatcherProcess(void *argument) {
  x_watcher *watcher = (x_watcher *)argument;
  char buffer[BUF_LEN];
  ssize_t lenght;
  struct directory *directories = watcher->directories;

  while (watcher->alive) {
    // poll for events
    struct pollfd pfd = {watcher->inotify_fd, POLLIN, 0};
    int ret           = poll(&pfd, 1, 50); // timeout of 50ms

    if (ret < 0) {
      // oops
      fprintf(stderr, "poll failed: %s\n", strerror(errno));
      break;
    } else if (ret == 0) {
      // Timeout with no events, move on.
      continue;
    }

    // wait for the kernel to do it's thing
    lenght = read(watcher->inotify_fd, buffer, BUF_LEN);
    if (lenght < 0) {
      // something messed up clearly
      perror("read");
      return NULL;
    }

    // pointer to the event structure
    ssize_t i = 0;
    while (i < lenght) {
      // the event list itself
      struct inotify_event *event = (struct inotify_event *)&buffer[i];

      // find directory for which this even matches via the descriptor
      struct directory *directory = NULL;
      for (size_t j = 0; j < arr_count(directories); j++) {
        if (directories[j].inotify_watch_fd == event->wd) {
          directory = &directories[j];
        }
      }
      if (directory == NULL) {
        fprintf(stderr, "MATCHING FILE DESCRIPTOR NOT FOUND! ERROR!\n");
        // BAIL????
      }

      // find matching file (if any)
      struct file *file = NULL;
      for (size_t j = 0; j < arr_count(directory->files); j++) {
        if (strcmp(directory->files[j].name, event->name) == 0) {
          file = &directory->files[j];
        }
      }

      XWATCHER_FILE_EVENT send_event = XWATCHER_FILE_NONE;

      if (event->mask & IN_CREATE)
        send_event = XWATCHER_FILE_CREATED;
      if (event->mask & IN_MODIFY)
        send_event = XWATCHER_FILE_MODIFIED;
      if (event->mask & IN_DELETE)
        send_event = XWATCHER_FILE_REMOVED;
      if (event->mask & IN_CLOSE_WRITE || event->mask & IN_CLOSE_NOWRITE)
        send_event = XWATCHER_FILE_REMOVED;
      if (event->mask & IN_ATTRIB)
        send_event = XWATCHER_FILE_ATTRIBUTES_CHANGED;
      if (event->mask & IN_OPEN)
        send_event = XWATCHER_FILE_OPENED;

      // file found(?)
      if (file != NULL) {
        if (send_event != XWATCHER_FILE_NONE) {
          // figure out the file path size
          size_t filepath_size = strlen(directory->path);
          filepath_size += strlen(file->name);
          filepath_size += 2;

          // create file path string
          char *filepath = (char *)malloc(filepath_size);
          snprintf(filepath, filepath_size, "%s/%s", directory->path, file->name);

          // callback
          file->callback_func(send_event, filepath, file->context, file->additional_data);

          // free that garbage
          free(filepath);
        }
      } else {
        // Cannot find file, lets try directory
        if (directory->callback_func != NULL && send_event != XWATCHER_FILE_NONE) {
          directory->callback_func(send_event, directory->path, directory->context,
                                   directory->additional_data);
        }
      }

      i += EVENT_SIZE + event->len;
    }
  }

  // cleanup time
  for (size_t i = 0; i < arr_count(watcher->directories); i++) {
    struct directory *directory = &watcher->directories[i];
    for (size_t j = 0; j < arr_count(directory->files); j++) {
      struct file *file = &directory->files[j];
      free(file->name);
    }
    arr_free(directory->files);
    free(directory->path);
    inotify_rm_watch(watcher->inotify_fd, directory->inotify_watch_fd);
  }
  close(watcher->inotify_fd);
  arr_free(watcher->directories);
  // should we signify that the thread is dead?
  return NULL;
}
#elif defined(__WIN32__) || defined(_WIN32)
#include <tchar.h>

#define BUF_LEN 1024
#define DIRBRK  '\\'

static void xWatcherUpdate(x_watcher *watcher) {
  struct directory *directories = watcher->directories;

  // create an event list so we can still make use of the Windows API
  HANDLE events[arr_count(directories)];
  for (int i = 0; i < arr_count(directories); i++) {
    events[i] = directories[i].overlapped.hEvent;
  }

  // wait for any of the objects to respond
  DWORD result =
      WaitForMultipleObjects(arr_count(directories), events, FALSE, 0 /** timeout of 50ms **/);

  // test which object was it
  int object_index = -1;
  for (int i = 0; i < arr_count(directories); i++) {
    if (result == (WAIT_OBJECT_0 + i)) {
      object_index = i;
      break;
    }
  }

  if (object_index == -1) {
    if (result == WAIT_TIMEOUT) {
      // it just timed out, let's continue
      return;
    } else {
      // RUNTIME ERROR! Let's bail
      // DWORD error = GetLastError();
      // ExitProcess(error);
      return;
    }
  }

  // shorhand for convenience
  struct directory *dir = &directories[object_index];

  // retrieve event data
  DWORD bytes_transferred;
  GetOverlappedResult(dir->handle, &dir->overlapped, &bytes_transferred, FALSE);

  // assign the data's pointer to a proper format for convenience
  FILE_NOTIFY_INFORMATION *event = (FILE_NOTIFY_INFORMATION *)dir->event_buffer;

  // loop through the data
  for (;;) {
    // figure out the wchar string size and allocate as needed
    DWORD name_len = event->FileNameLength / sizeof(wchar_t);
    char *name_char = malloc(sizeof(char) * (name_len + 1));
    size_t converted_chars;

    // convert wchar* filename to char*
    wcstombs_s(&converted_chars, name_char, name_len + 1, event->FileName, name_len);

    // convert to proper event type
    XWATCHER_FILE_EVENT send_event = XWATCHER_FILE_NONE;
    switch (event->Action) {
    case FILE_ACTION_ADDED:
      send_event = XWATCHER_FILE_CREATED;
      break;
    case FILE_ACTION_REMOVED:
      send_event = XWATCHER_FILE_REMOVED;
      break;
    case FILE_ACTION_MODIFIED:
      send_event = XWATCHER_FILE_MODIFIED;
      break;
    case FILE_ACTION_RENAMED_OLD_NAME:
    case FILE_ACTION_RENAMED_NEW_NAME:
      send_event = XWATCHER_FILE_RENAMED;
      break;
    default:
      send_event = XWATCHER_FILE_UNSPECIFIED;
      break;
    }

    // find matching file (if any)
    struct file *file = NULL;
    for (size_t j = 0; j < arr_count(dir->files); j++) {
      if (strcmp(dir->files[j].name, name_char) == 0) {
        file = &dir->files[j];
      }
    }

    // file found(?)
    if (file != NULL) {
      if (send_event != XWATCHER_FILE_NONE) {
        // figure out the file path size
        size_t filepath_size = strlen(dir->path);
        filepath_size += strlen(file->name);
        filepath_size += 2;

        // create file path string
        char *filepath = (char *)malloc(filepath_size);
        snprintf(filepath, filepath_size, "%s%c%s", dir->path, DIRBRK, file->name);

        // callback
        file->callback_func(send_event, filepath, file->context, file->additional_data);

        // free that garbage
        free(filepath);
      }
    } else {
      // Cannot find file, lets try directory
      if (dir->callback_func != NULL && send_event != XWATCHER_FILE_NONE) {
        dir->callback_func(send_event, dir->path, dir->context, dir->additional_data);
      }
    }

    // free up the converted string
    free(name_char);

    // Are there more events to handle?
    if (event->NextEntryOffset) {
      *((uint8_t **)&event) += event->NextEntryOffset;
    } else {
      break;
    }
  }

  DWORD dwNotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                         FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
                         FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS |
                         FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;
  BOOL recursive = FALSE;

  // Queue the next event
  BOOL success = ReadDirectoryChangesW(dir->handle, dir->event_buffer, BUF_LEN, recursive,
                                       dwNotifyFilter, NULL, &dir->overlapped, NULL);

  if (!success) {
    // get error code
    DWORD error = GetLastError();
    char *message;

    // get error message
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&message, 0, NULL);

    fprintf(stderr, "ReadDirectoryChangesW failed: %s\n", message);

    // ExitProcess(error);
  }
}

static inline void *__internal_xWatcherProcess(void *argument) {
  x_watcher *watcher = (x_watcher *)argument;
  struct directory *directories = watcher->directories;

  // obv first check if we need to stay alive
  while (watcher->alive) {
    xWatcherUpdate(watcher);
  }

  return NULL;
}
#else
#error "Unsupported"
#endif

static inline x_watcher *xWatcher_create(void) {
  x_watcher *watcher = (x_watcher *)malloc(sizeof(x_watcher));

  arr_init(watcher->directories);

#if defined(__WIN32__) || defined(_WIN32)
  // literally noop
#elif defined(__linux__)
  watcher->inotify_fd = inotify_init1(O_NONBLOCK);
  if (watcher->inotify_fd < 0) {
    perror("inotify_init");
    return NULL;
  }
#else
#error "Unsupported"
#endif

  return watcher;
}

static inline bool xWatcher_appendFile(x_watcher *watcher, xWatcher_reference *reference) {
  char *path = _strdup(reference->path);

  // the file MUST NOT contain slashed at the end
  if (path[strlen(path) - 1] == DIRBRK)
    return false;

  char *filename = NULL;

  // we need to split the filename and path
  for (size_t i = strlen(path) - 1; i > 0; i--) {
    if (path[i] == DIRBRK) {
      path[i]  = '\0';         // break the string, so it splits into two
      filename = &path[i + 1]; // set the rest of it as the filename
      break;
    }
  }

  struct directory *dir = NULL;

  // check against the database of (pre-existing) directories
  for (size_t i = 0; i < arr_count(watcher->directories); i++) {
    // paths match
    if (strcmp(watcher->directories[i].path, path) == 0) {
      dir = &watcher->directories[i];
    }
  }

  // directory exists, check if an callback has been already added
  if (dir == NULL) {
    struct directory new_dir;

    new_dir.callback_func   = NULL; // DO NOT add callbacks if it's a file
    new_dir.context         = 0;    // context should be invalid as well
    new_dir.additional_data = NULL; // so should the data
    new_dir.path            = path; // add a path to the directory
#if defined(__linux__)
    new_dir.inotify_watch_fd = -1; // invalidate inotify
#elif defined(__WIN32__) || defined(_WIN32)
    new_dir.handle = NULL;
#else
#error "Unsupported"
#endif

    // initialize file arrays
    arr_init(new_dir.files);

    // add the directory to the masses
    arr_add(watcher->directories, new_dir);

    // move the pointer to the newly added element
    dir = &watcher->directories[arr_count(watcher->directories) - 1];
  }

  // search for the file
  struct file *file = NULL;
  for (size_t i = 0; i < arr_count(dir->files); i++) {
    if (strcmp(dir->files[i].name, filename) == 0) {
      file = &dir->files[i];
    }
  }

  if (file != NULL) {
    return false; // file already exists, that's an ERROR
  }

  struct file new_file;
  // avoid an invalid free because this shares the memory space
  // of the full path string
  new_file.name            = _strdup(filename);
  new_file.context         = reference->context;
  new_file.additional_data = reference->additional_data;
  new_file.callback_func   = reference->callback_func;

  // the the element
  arr_add(dir->files, new_file);

  // and move the pointer to the newly added element
  file = &dir->files[arr_count(watcher->directories) - 1];

// add the file watcher
#if defined(__linux__)
  if (dir->inotify_watch_fd == -1) {
    dir->inotify_watch_fd = inotify_add_watch(watcher->inotify_fd, path, IN_ALL_EVENTS);
    if (dir->inotify_watch_fd == -1) {
      perror("inotify_watch_fd");
      return false;
    }
  }
#elif defined(__WIN32__) || defined(_WIN32)
  // add directory path
  dir->handle = CreateFile(dir->path, FILE_LIST_DIRECTORY,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                           OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
  if (dir->handle == INVALID_HANDLE_VALUE) {
    // get error code
    DWORD error = GetLastError();
    char *message;

    // get error message
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&message, 0, NULL);

    fprintf(stderr, "CreateFile failed: %s\n", message);

    return false;
  }

  // create event structure
  dir->overlapped.hEvent = CreateEvent(NULL, FALSE, 0, NULL);
  if (dir->overlapped.hEvent == NULL) {
    // get error code
    DWORD error = GetLastError();
    char *message;

    // get error message
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&message, 0, NULL);

    fprintf(stderr, "CreateEvent failed: %s\n", message);

    return false;
  }

  // allocate the event buffer
  dir->event_buffer = malloc(BUF_LEN);
  if (dir->event_buffer == NULL) {
    fprintf(stderr, "malloc failed at __FILE__:__LINE__!\n");
    return false;
  }

  // set reading params
  BOOL success = ReadDirectoryChangesW(dir->handle, dir->event_buffer, BUF_LEN, TRUE,
                                       FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                           FILE_NOTIFY_CHANGE_LAST_WRITE,
                                       NULL, &dir->overlapped, NULL);
  if (!success) {
    // get error code
    DWORD error = GetLastError();
    char *message;

    // get error message
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&message, 0, NULL);

    fprintf(stderr, "ReadDirectoryChangesW failed: %s\n", message);

    return false;
  }
#else
#error "Unsupported"
#endif
  return true;
}

static inline bool xWatcher_appendDir(x_watcher *watcher, xWatcher_reference *reference) {
  char *path = _strdup(reference->path);

  // inotify only works with directories that do NOT have a front-slash
  // at the end, so we have to make sure to cut that out
  if (path[strlen(path) - 1] == DIRBRK)
    path[strlen(path) - 1] = '\0';

  struct directory *dir = NULL;

  // check against the database of (pre-existing) directories
  for (size_t i = 0; i < arr_count(watcher->directories); i++) {
    // paths match
    if (strcmp(watcher->directories[i].path, path) == 0) {
      dir = &watcher->directories[i];
    }
  }

  // directory exists, check if an callback has been already added
  if (dir) {
    // ERROR, CALLBACK EXISTS
    if (dir->callback_func) {
      return false;
    }

    dir->callback_func   = reference->callback_func;
    dir->context         = reference->context;
    dir->additional_data = reference->additional_data;
  } else {
    // keep an eye for this one as it's on the stack
    struct directory dir;

    dir.path            = path;
    dir.callback_func   = reference->callback_func;
    dir.context         = reference->context;
    dir.additional_data = reference->additional_data;

    // initialize file arrays
    arr_init(dir.files);

#if defined(__linux__)
    dir.inotify_watch_fd = inotify_add_watch(watcher->inotify_fd, dir.path, IN_ALL_EVENTS);
    if (dir.inotify_watch_fd == -1) {
      perror("inotify_watch_fd");
      return false;
    }
#elif defined(__WIN32__) || defined(_WIN32)
    // add directory path
    dir.handle = CreateFile(dir.path, FILE_LIST_DIRECTORY,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
    if (dir.handle == INVALID_HANDLE_VALUE) {
      // get error code
      DWORD error = GetLastError();
      char *message;

      // get error message
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&message, 0,
                    NULL);

      fprintf(stderr, "CreateFile failed: %s\n", message);

      return false;
    }

    // create event structure
    dir.overlapped.hEvent = CreateEvent(NULL, FALSE, 0, NULL);
    if (dir.overlapped.hEvent == NULL) {
      // get error code
      DWORD error = GetLastError();
      char *message;

      // get error message
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&message, 0,
                    NULL);

      fprintf(stderr, "CreateEvent failed: %s\n", message);

      return false;
    }

    // allocate the event buffer
    dir.event_buffer = malloc(BUF_LEN);
    if (dir.event_buffer == NULL) {
      fprintf(stderr, "malloc failed at __FILE__:__LINE__!\n");
      return false;
    }

    // set reading params
    BOOL success = ReadDirectoryChangesW(
        dir.handle, dir.event_buffer, BUF_LEN, TRUE,
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
        NULL, &dir.overlapped, NULL);
    if (!success) {
      // get error code
      DWORD error = GetLastError();
      char *message;

      // get error message
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&message, 0,
                    NULL);

      fprintf(stderr, "ReadDirectoryChangesW failed: %s\n", message);

      return false;
    }
#else
#error "Unsupported"
#endif

    arr_add(watcher->directories, dir);
  }

  return true;
}

static inline bool xWatcher_start(x_watcher *watcher) {
  watcher->alive = true;

#if defined(__linux__)
  // create watcher thread
  watcher->thread_id = pthread_create(&watcher->thread, NULL, __internal_xWatcherProcess, watcher);

  if (watcher->thread_id != 0) {
    perror("pthread_create");
    watcher->alive = false;
    return false;
  }
#endif

  return true;
}

static inline void xWatcher_destroy(x_watcher *watcher) {
  void *ret;
  watcher->alive = false;

  // cleanup time
  for (size_t i = 0; i < arr_count(watcher->directories); i++) {
    struct directory *directory = &watcher->directories[i];
    for (size_t j = 0; j < arr_count(directory->files); j++) {
      struct file *file = &directory->files[j];
      free(file->name);
    }
    arr_free(directory->files);
    free(directory->path);
    free(directory->event_buffer);
    CloseHandle(directory->handle);
  }
  arr_free(watcher->directories);

#if defined(__linux__)
  pthread_join(watcher->thread, &ret);
#endif

  free(watcher);
}

#endif
