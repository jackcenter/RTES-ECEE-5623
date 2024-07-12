/*
 *
 *  Adapted by Sam Siewert for use with UVC web cameras and Bt878 frame
 *  grabber NTSC cameras to acquire digital video from a source,
 *  time-stamp each frame acquired, save to a PGM or PPM file.
 *
 *  The original code adapted was open source from V4L2 API and had the
 *  following use and incorporation policy:
 *
 *  This program can be used and distributed without restrictions.
 *
 *      This program is provided with the V4L2 API
 * see http://linuxtv.org/docs.php for more information
 */

// This is necessary for CPU affinity macros in Linux
#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <fcntl.h>  /* low-level i/o */
#include <getopt.h> /* getopt_long() */
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/videodev2.h>

#include <utilities.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define COLOR_CONVERT
#define HRES 320
#define VRES 240
#define HRES_STR "320"
#define VRES_STR "240"
#define FRAMES 10

typedef void (*TimerCallback)(int, siginfo_t *, void *);

sem_t sem_0;
bool abort_service_0 = false;
static timer_t timers[1];

void sigint_handler(int sig);
void interval_expired(int signum, siginfo_t *info, void *context);

void *service_0(void *threadp);

static void setup_main_thread(void);
static void initialize_timer(const struct timespec period, const size_t id,
                             TimerCallback callback, timer_t *const timer);

// Format is used by a number of functions, so made as a file global
static struct v4l2_format fmt;

enum io_method {
  IO_METHOD_READ,
  IO_METHOD_MMAP,
  IO_METHOD_USERPTR,
};

struct buffer {
  void *start;
  size_t length;
};

static char *dev_name;
// static enum io_method   io = IO_METHOD_USERPTR;
// static enum io_method   io = IO_METHOD_READ;
static enum io_method io = IO_METHOD_MMAP;
static int fd = -1;
struct buffer *buffers;
static unsigned int n_buffers;
static int out_buf;
static int force_format = 1;
static int frame_count = FRAMES;
static double frame_times_ms[FRAMES];
static double acquisition_times_ms[FRAMES];
static double transformation_times_ms[FRAMES];
static double write_back_times_ms[FRAMES];
static double service_request_times_ms[FRAMES];

static void errno_exit(const char *s) {
  fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
  exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg) {
  int r;

  do {
    r = ioctl(fh, request, arg);

  } while (-1 == r && EINTR == errno);

  return r;
}

unsigned int framecnt = 0;
unsigned char bigbuffer[(1280 * 960)];
unsigned char transform_buffer[(1280 * 960)];

static void process_image(const void *p, int size) {
  int i, newi, newsize = 0;
  struct timespec frame_time;
  int y_temp, y2_temp, u_temp, v_temp;
  unsigned char *pptr = (unsigned char *)p;

  // record when process was called
  clock_gettime(CLOCK_REALTIME, &frame_time);
  syslog(LOG_CRIT, "frame %d: ", framecnt);

  // This just dumps the frame to a file now, but you could replace with
  // whatever image processing you wish.
  struct timespec write_back_start_time_ms;
  if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_GREY) {
    syslog(LOG_CRIT, "Dump graymap as-is size %d\n", size);
    dump_pgm(p, size, framecnt, &frame_time);
  } else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {

#if defined(COLOR_CONVERT)
    syslog(LOG_CRIT, "Dump YUYV converted to RGB size %d\n", size);

    // Pixels are YU and YV alternating, so YUYV which is 4 bytes
    // We want RGB, so RGBRGB which is 6 bytes

    // Transformation
    struct timespec transformation_start_time_ms;
    clock_gettime(CLOCK_REALTIME, &transformation_start_time_ms);
    for (i = 0, newi = 0; i < size; i = i + 4, newi = newi + 6) {
      y_temp = (int)pptr[i];
      u_temp = (int)pptr[i + 1];
      y2_temp = (int)pptr[i + 2];
      v_temp = (int)pptr[i + 3];
      yuv2rgb(y_temp, u_temp, v_temp, &bigbuffer[newi], &bigbuffer[newi + 1],
              &bigbuffer[newi + 2]);
      yuv2rgb(y2_temp, u_temp, v_temp, &bigbuffer[newi + 3],
              &bigbuffer[newi + 4], &bigbuffer[newi + 5]);
    }

    // So, we should have rgb in the big buffer in increments of 3.
    const size_t rgb_size =
        (6 * size) / 4; // Number of data points in the RGB conversion

    // Balanced grayscale converions
    for (size_t j = 0; j < rgb_size; ++j) {
      const float red = (float)bigbuffer[(3 * j)];
      const float green = (float)bigbuffer[(3 * j) + 1];
      const float blue = (float)bigbuffer[(3 * j) + 2];

      const float balance = 0.3f * red + 0.59f * green + 0.11f * blue;
      transform_buffer[j] = balance;
    }

    // R, G, or B grayscale conversion
    // const int offset = 2;                       // Distance to offset for
    // grayscale conversion (0 = R, 1 = G, 2 = B) for (size_t j = 0; j <
    // rgb_size; ++j)
    // {
    //     transform_buffer[j] = bigbuffer[(3 * j) + offset];
    // }
    struct timespec transformation_stop_time_ms;
    clock_gettime(CLOCK_REALTIME, &transformation_stop_time_ms);
    transformation_times_ms[framecnt] = getDurationInMilliseconds(
        transformation_start_time_ms, transformation_stop_time_ms);
    // Transformation Stop

    // Write-back
    clock_gettime(CLOCK_REALTIME, &write_back_start_time_ms);

    const size_t gray_size = rgb_size / 3;
    dump_pgm(transform_buffer, gray_size, framecnt, &frame_time);
    // dump_ppm(bigbuffer, rgb_size, framecnt, &frame_time);

#else
    syslog(LOG_CRIT, "Dump YUYV converted to YY size %d\n", size);

    // Pixels are YU and YV alternating, so YUYV which is 4 bytes
    // We want Y, so YY which is 2 bytes
    //
    for (i = 0, newi = 0; i < size; i = i + 4, newi = newi + 2) {
      // Y1=first byte and Y2=third byte
      bigbuffer[newi] = pptr[i];
      bigbuffer[newi + 1] = pptr[i + 2];
    }

    dump_pgm(bigbuffer, (size / 2), framecnt, &frame_time);
#endif

  } else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24) {
    syslog(LOG_CRIT, "Dump RGB as-is size %d\n", size);
    dump_ppm(p, size, framecnt, &frame_time);
  } else {
    syslog(LOG_CRIT, "ERROR - unknown dump format\n");
  }

  fflush(stderr);
  fflush(stdout);

  struct timespec write_back_stop_time_ms;
  clock_gettime(CLOCK_REALTIME, &write_back_stop_time_ms);
  write_back_times_ms[framecnt] = getDurationInMilliseconds(
      write_back_start_time_ms, write_back_stop_time_ms);
  // Write-back Stop
}

static int read_frame(void) {
  struct v4l2_buffer buf;
  unsigned int i;

  switch (io) {

  case IO_METHOD_READ:
    if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
      switch (errno) {

      case EAGAIN:
        return 0;

      case EIO:
        /* Could ignore EIO, see spec. */

        /* fall through */

      default:
        errno_exit("read");
      }
    }

    process_image(buffers[0].start, buffers[0].length);
    break;

  case IO_METHOD_MMAP:
    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    // Acquisition
    struct timespec acquisition_start_time_ms;
    clock_gettime(CLOCK_REALTIME, &acquisition_start_time_ms);
    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
      switch (errno) {
      case EAGAIN:
        return 0;

      case EIO:
        /* Could ignore EIO, but drivers should only set for serious errors,
           although some set for non-fatal errors too.
         */
        return 0;

      default:
        syslog(LOG_CRIT, "mmap failure\n");
        errno_exit("VIDIOC_DQBUF");
      }
    }
    struct timespec acquisition_stop_time_ms;
    clock_gettime(CLOCK_REALTIME, &acquisition_stop_time_ms);
    acquisition_times_ms[framecnt] = getDurationInMilliseconds(
        acquisition_start_time_ms, acquisition_stop_time_ms);
    // Acquisition Stop

    assert(buf.index < n_buffers);

    process_image(buffers[buf.index].start, buf.bytesused);
    struct timespec process_end_time_ms;
    clock_gettime(CLOCK_REALTIME, &process_end_time_ms);
    frame_times_ms[framecnt] = getDurationInMilliseconds(
        acquisition_start_time_ms, process_end_time_ms);

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
      errno_exit("VIDIOC_QBUF");
    break;

  case IO_METHOD_USERPTR:
    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
      switch (errno) {
      case EAGAIN:
        return 0;

      case EIO:
        /* Could ignore EIO, see spec. */

        /* fall through */

      default:
        errno_exit("VIDIOC_DQBUF");
      }
    }

    for (i = 0; i < n_buffers; ++i)
      if (buf.m.userptr == (unsigned long)buffers[i].start &&
          buf.length == buffers[i].length)
        break;

    assert(i < n_buffers);

    process_image((void *)buf.m.userptr, buf.bytesused);

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
      errno_exit("VIDIOC_QBUF");
    break;
  }

  return 1;
}

int compare_doubles(const void *a, const void *b) {
  double arg1 = *(const double *)a;
  double arg2 = *(const double *)b;

  if (arg1 < arg2)
    return -1;
  if (arg1 > arg2)
    return 1;
  return 0;
}

static void compute_metrics(double const times[], size_t const size) {
  double times_copy[size];
  for (size_t i = 0; i < size; ++i) {
    times_copy[i] = times[i];
  }

  double sum_ms = 0.0;
  double wcet_ms = 0.0;
  double bcet_ms = 1000000.0;

  for (size_t i = 0; i < size; ++i) {
    double const frame_time_ms = times_copy[i];
    sum_ms += frame_time_ms;

    if (wcet_ms < frame_time_ms) {
      wcet_ms = frame_time_ms;
    }

    if (frame_time_ms < bcet_ms) {
      bcet_ms = frame_time_ms;
    }
  }

  const double mean_execution_time_ms = sum_ms / (double)size;
  syslog(LOG_CRIT, "Mean Exectution Time: %.3lf ms\n", mean_execution_time_ms);

  qsort(times_copy, size, sizeof(double), compare_doubles);
  size_t const median_idx = size / 2;
  syslog(LOG_CRIT, "Median Exectution Time: %.3lf ms\n",
         times_copy[median_idx]);
  syslog(LOG_CRIT, "Worse Case Execution Time: %.3lf ms\n", wcet_ms);
  syslog(LOG_CRIT, "Best Case Execution Time: %.3lf ms\n", bcet_ms);
}

static void mainloop(void) {
  struct timespec read_delay;
  struct timespec time_error;

  read_delay.tv_sec = 0;
  read_delay.tv_nsec = 30000;

  size_t count = 0;
  while (count < frame_count) {
    for (;;) {
      fd_set fds;
      struct timeval tv;
      int r;

      FD_ZERO(&fds);
      FD_SET(fd, &fds);

      /* Timeout. */
      tv.tv_sec = 2;
      tv.tv_usec = 0;

      r = select(fd + 1, &fds, NULL, NULL, &tv);

      if (-1 == r) {
        if (EINTR == errno)
          continue;
        errno_exit("select");
      }

      if (0 == r) {
        fprintf(stderr, "select timeout\n");
        exit(EXIT_FAILURE);
      }

      if (read_frame()) {
        if (nanosleep(&read_delay, &time_error) != 0)
          perror("nanosleep");
        else
          syslog(LOG_CRIT, "time_error.tv_sec=%ld, time_error.tv_nsec=%ld\n",
                 time_error.tv_sec, time_error.tv_nsec);

        ++count;
        break;
      }

      /* EAGAIN - continue select loop unless count done. */
      // if(count <= 0) break;
    }

    // if(count <= 0) break;
  }
}

static void stop_capturing(void) {
  enum v4l2_buf_type type;

  switch (io) {
  case IO_METHOD_READ:
    /* Nothing to do. */
    break;

  case IO_METHOD_MMAP:
  case IO_METHOD_USERPTR:
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
      errno_exit("VIDIOC_STREAMOFF");
    break;
  }
}

static void start_capturing(void) {
  unsigned int i;
  enum v4l2_buf_type type;

  switch (io) {

  case IO_METHOD_READ:
    /* Nothing to do. */
    break;

  case IO_METHOD_MMAP:
    for (i = 0; i < n_buffers; ++i) {
      syslog(LOG_CRIT, "allocated buffer %d\n", i);
      struct v4l2_buffer buf;

      CLEAR(buf);
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      buf.index = i;

      if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        errno_exit("VIDIOC_QBUF");
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
      errno_exit("VIDIOC_STREAMON");
    break;

  case IO_METHOD_USERPTR:
    for (i = 0; i < n_buffers; ++i) {
      struct v4l2_buffer buf;

      CLEAR(buf);
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_USERPTR;
      buf.index = i;
      buf.m.userptr = (unsigned long)buffers[i].start;
      buf.length = buffers[i].length;

      if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        errno_exit("VIDIOC_QBUF");
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
      errno_exit("VIDIOC_STREAMON");
    break;
  }
}

static void uninit_device(void) {
  unsigned int i;

  switch (io) {
  case IO_METHOD_READ:
    free(buffers[0].start);
    break;

  case IO_METHOD_MMAP:
    for (i = 0; i < n_buffers; ++i)
      if (-1 == munmap(buffers[i].start, buffers[i].length))
        errno_exit("munmap");
    break;

  case IO_METHOD_USERPTR:
    for (i = 0; i < n_buffers; ++i)
      free(buffers[i].start);
    break;
  }

  free(buffers);
}

static void init_read(unsigned int buffer_size) {
  buffers = calloc(1, sizeof(*buffers));

  if (!buffers) {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }

  buffers[0].length = buffer_size;
  buffers[0].start = malloc(buffer_size);

  if (!buffers[0].start) {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }
}

static void init_mmap(void) {
  struct v4l2_requestbuffers req;

  CLEAR(req);

  req.count = 6;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
    if (EINVAL == errno) {
      fprintf(stderr,
              "%s does not support "
              "memory mapping\n",
              dev_name);
      exit(EXIT_FAILURE);
    } else {
      errno_exit("VIDIOC_REQBUFS");
    }
  }

  if (req.count < 2) {
    fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
    exit(EXIT_FAILURE);
  }

  buffers = calloc(req.count, sizeof(*buffers));

  if (!buffers) {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }

  for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
    struct v4l2_buffer buf;

    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = n_buffers;

    if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
      errno_exit("VIDIOC_QUERYBUF");

    buffers[n_buffers].length = buf.length;
    buffers[n_buffers].start =
        mmap(NULL /* start anywhere */, buf.length,
             PROT_READ | PROT_WRITE /* required */,
             MAP_SHARED /* recommended */, fd, buf.m.offset);

    if (MAP_FAILED == buffers[n_buffers].start)
      errno_exit("mmap");
  }
}

static void init_userp(unsigned int buffer_size) {
  struct v4l2_requestbuffers req;

  CLEAR(req);

  req.count = 4;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_USERPTR;

  if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
    if (EINVAL == errno) {
      fprintf(stderr,
              "%s does not support "
              "user pointer i/o\n",
              dev_name);
      exit(EXIT_FAILURE);
    } else {
      errno_exit("VIDIOC_REQBUFS");
    }
  }

  buffers = calloc(4, sizeof(*buffers));

  if (!buffers) {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }

  for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
    buffers[n_buffers].length = buffer_size;
    buffers[n_buffers].start = malloc(buffer_size);

    if (!buffers[n_buffers].start) {
      fprintf(stderr, "Out of memory\n");
      exit(EXIT_FAILURE);
    }
  }
}

static void init_device(void) {
  struct v4l2_capability cap;
  struct v4l2_cropcap cropcap;
  struct v4l2_crop crop;
  unsigned int min;

  if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
    if (EINVAL == errno) {
      fprintf(stderr, "%s is no V4L2 device\n", dev_name);
      exit(EXIT_FAILURE);
    } else {
      errno_exit("VIDIOC_QUERYCAP");
    }
  }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    fprintf(stderr, "%s is no video capture device\n", dev_name);
    exit(EXIT_FAILURE);
  }

  switch (io) {
  case IO_METHOD_READ:
    if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
      fprintf(stderr, "%s does not support read i/o\n", dev_name);
      exit(EXIT_FAILURE);
    }
    break;

  case IO_METHOD_MMAP:
  case IO_METHOD_USERPTR:
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
      fprintf(stderr, "%s does not support streaming i/o\n", dev_name);
      exit(EXIT_FAILURE);
    }
    break;
  }

  /* Select video input, video standard and tune here. */

  CLEAR(cropcap);

  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect; /* reset to default */

    if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
      switch (errno) {
      case EINVAL:
        /* Cropping not supported. */
        break;
      default:
        /* Errors ignored. */
        break;
      }
    }

  } else {
    /* Errors ignored. */
  }

  CLEAR(fmt);

  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (force_format) {
    syslog(LOG_CRIT, "FORCING FORMAT\n");
    fmt.fmt.pix.width = HRES;
    fmt.fmt.pix.height = VRES;

    // Specify the Pixel Coding Formate here

    // This one work for Logitech C200
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

    // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
    // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_VYUY;

    // Would be nice if camera supported
    // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
    // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;

    // fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
      errno_exit("VIDIOC_S_FMT");

    /* Note VIDIOC_S_FMT may change width and height. */
  } else {
    syslog(LOG_CRIT, "ASSUMING FORMAT\n");
    /* Preserve original settings as set by v4l2-ctl for example */
    if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
      errno_exit("VIDIOC_G_FMT");
  }

  /* Buggy driver paranoia. */
  min = fmt.fmt.pix.width * 2;
  if (fmt.fmt.pix.bytesperline < min)
    fmt.fmt.pix.bytesperline = min;
  min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
  if (fmt.fmt.pix.sizeimage < min)
    fmt.fmt.pix.sizeimage = min;

  switch (io) {
  case IO_METHOD_READ:
    init_read(fmt.fmt.pix.sizeimage);
    break;

  case IO_METHOD_MMAP:
    init_mmap();
    break;

  case IO_METHOD_USERPTR:
    init_userp(fmt.fmt.pix.sizeimage);
    break;
  }
}

static void close_device(void) {
  if (-1 == close(fd))
    errno_exit("close");

  fd = -1;
}

static void open_device(void) {
  struct stat st;

  if (-1 == stat(dev_name, &st)) {
    fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno,
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (!S_ISCHR(st.st_mode)) {
    fprintf(stderr, "%s is no device\n", dev_name);
    exit(EXIT_FAILURE);
  }

  fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

  if (-1 == fd) {
    fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno,
            strerror(errno));
    exit(EXIT_FAILURE);
  }
}

static void usage(FILE *fp, int argc, char **argv) {
  fprintf(fp,
          "Usage: %s [options]\n\n"
          "Version 1.3\n"
          "Options:\n"
          "-d | --device name   Video device name [%s]\n"
          "-h | --help          Print this message\n"
          "-m | --mmap          Use memory mapped buffers [default]\n"
          "-r | --read          Use read() calls\n"
          "-u | --userp         Use application allocated buffers\n"
          "-o | --output        Outputs stream to stdout\n"
          "-f | --format        Force format to 640x480 GREY\n"
          "-c | --count         Number of frames to grab [%i]\n"
          "",
          argv[0], dev_name, frame_count);
}

static const char short_options[] = "d:hmruofc:";

static const struct option long_options[] = {
    {"device", required_argument, NULL, 'd'},
    {"help", no_argument, NULL, 'h'},
    {"mmap", no_argument, NULL, 'm'},
    {"read", no_argument, NULL, 'r'},
    {"userp", no_argument, NULL, 'u'},
    {"output", no_argument, NULL, 'o'},
    {"format", no_argument, NULL, 'f'},
    {"count", required_argument, NULL, 'c'},
    {0, 0, 0, 0}};

int main(int argc, char **argv) {
  if (argc > 1)
    dev_name = argv[1];
  else
    dev_name = "/dev/video0";

  for (;;) {
    int idx;
    int c;

    c = getopt_long(argc, argv, short_options, long_options, &idx);

    if (-1 == c)
      break;

    switch (c) {
    case 0: /* getopt_long() flag */
      break;

    case 'd':
      dev_name = optarg;
      break;

    case 'h':
      usage(stdout, argc, argv);
      exit(EXIT_SUCCESS);

    case 'm':
      io = IO_METHOD_MMAP;
      break;

    case 'r':
      io = IO_METHOD_READ;
      break;

    case 'u':
      io = IO_METHOD_USERPTR;
      break;

    case 'o':
      out_buf++;
      break;

    case 'f':
      force_format++;
      break;

    case 'c':
      errno = 0;
      frame_count = strtol(optarg, NULL, 0);
      if (errno)
        errno_exit(optarg);
      break;

    default:
      usage(stderr, argc, argv);
      exit(EXIT_FAILURE);
    }
  }
  open_device();
  init_device();
  start_capturing();

  signal(SIGINT, sigint_handler); // handles exiting by ctrl-c
  setup_main_thread();

  const int rt_max_prio = sched_get_priority_max(SCHED_FIFO);

  cpu_set_t thread_cpu_set;
  CPU_ZERO(&thread_cpu_set);
  CPU_SET(3, &thread_cpu_set);

  const int number_of_threads = 1;
  pthread_attr_t pthread_attributes[number_of_threads];
  struct sched_param schedule_params[number_of_threads];
  pthread_t pthreads[number_of_threads];

  void *(*service_functions[number_of_threads])(void *);
  service_functions[0] = service_0;

  for (int i = 0; i < number_of_threads; ++i) {
    if (pthread_attr_init(&pthread_attributes[i])) {
      printf("Failed to initialize `pthread_attrubutes` for thread: %d.\n", i);
      exit(-1);
    }

    if (pthread_attr_setinheritsched(&pthread_attributes[i],
                                     PTHREAD_EXPLICIT_SCHED)) {
      printf("Failed to set inherit schedule for `pthread_attrubutes` for "
             "thread: %d.\n",
             i);
      exit(-1);
    }

    if (pthread_attr_setschedpolicy(&pthread_attributes[i], SCHED_FIFO)) {
      printf("Failed to set sched policy for `pthread_attrubutes` for thread: "
             "%d.\n",
             i);
      exit(-1);
    }

    if (pthread_attr_setaffinity_np(&pthread_attributes[i], sizeof(cpu_set_t),
                                    &thread_cpu_set)) {
      printf(
          "Failed to set affinity for `pthread_attrubutes` for thread: %d.\n",
          i);
      exit(-1);
    }

    schedule_params[i].sched_priority = rt_max_prio - i;
    if (pthread_attr_setschedparam(&pthread_attributes[i],
                                   &schedule_params[i])) {
      printf("Failed to set sched param for `pthread_attrubutes` for thread: "
             "%d.\n",
             i);
      exit(-1);
    }

    if (pthread_create(&pthreads[i], &pthread_attributes[i],
                       service_functions[i], NULL)) {
      printf("Failed to create `pthreads` for thread: %d.\n", i);
      exit(-1);
    }
  }

  struct timespec timer_0_period = {0, 35e6};
  initialize_timer(timer_0_period, 0, interval_expired, &timers[0]);

  while (framecnt + 1 < FRAMES) {
    pause();
  }
  abort_service_0 = true;

  for (size_t i = 0; i < number_of_threads; ++i) {
    pthread_join(pthreads[i], 0);
  }

  stop_capturing();
  uninit_device();
  close_device();

  syslog(LOG_CRIT, "Total: ");
  compute_metrics(frame_times_ms, FRAMES);

  syslog(LOG_CRIT, "Acquisition: ");
  compute_metrics(acquisition_times_ms, FRAMES);

  syslog(LOG_CRIT, "Transformation: ");
  compute_metrics(transformation_times_ms, FRAMES);

  syslog(LOG_CRIT, "Write-back: ");
  compute_metrics(write_back_times_ms, FRAMES);

  syslog(LOG_CRIT, "Service: ");
  compute_metrics(service_request_times_ms, FRAMES);

//   for (size_t i = 0; i < FRAMES; ++i) {
//     syslog(LOG_CRIT, "Value: %.3lf", service_request_times_ms[i]);
//   }

  fprintf(stderr, "\n");
  return 0;
}

void sigint_handler(int sig) {
  abort_service_0 = true;
  exit(0);
}

void interval_expired(int signum, siginfo_t *info, void *context) {
  static size_t count = 0;
  static bool keep_running = true;

  if (keep_running) {
    ++count;
    sem_post(&sem_0);
  }

  if (count % FRAMES == 0) {
    keep_running = false;
  }
}

void *service_0(void *threadp) {
  struct timespec start_timespec = {0, 0};
  clock_gettime(CLOCK_REALTIME, &start_timespec);
  double start_time_s = (double)start_timespec.tv_sec +
                        (double)start_timespec.tv_nsec / 1000000000.0f;

  size_t count = 0;
  while (!abort_service_0) {
    sem_wait(&sem_0);
    struct timespec service_start_time;
    clock_gettime(CLOCK_REALTIME, &service_start_time);

    struct timespec current_timespec = {0, 0};
    clock_gettime(CLOCK_REALTIME, &current_timespec);

    const double time_s = (double)current_timespec.tv_sec +
                          (double)current_timespec.tv_nsec / 1000000000.0f;

    struct timespec time_error;

    for (;;) {
      fd_set fds;
      struct timeval tv;
      int r;

      FD_ZERO(&fds);
      FD_SET(fd, &fds);

      /* Timeout. */
      tv.tv_sec = 2;
      tv.tv_usec = 0;

      r = select(fd + 1, &fds, NULL, NULL, &tv);

      if (-1 == r) {
        if (EINTR == errno)
          continue;
        errno_exit("select");
      }

      if (0 == r) {
        fprintf(stderr, "select timeout\n");
        exit(EXIT_FAILURE);
      }

      if (read_frame()) {
        syslog(LOG_CRIT, "time_error.tv_sec=%ld, time_error.tv_nsec=%ld\n",
               time_error.tv_sec, time_error.tv_nsec);

        ++count;
        break;
      }
    }

    struct timespec service_stop_time;
    clock_gettime(CLOCK_REALTIME, &service_stop_time);
    service_request_times_ms[framecnt] =
        getDurationInMilliseconds(service_start_time, service_stop_time);

    ++framecnt;
  }

  pthread_exit((void *)0);
}

void setup_main_thread(void) {
  if (sem_init(&sem_0, 0, 0)) {
    printf("Failed to initialize S0 semaphore\n");
    exit(-1);
  }

  const pid_t main_pid = getpid();

  struct sched_param main_param;
  if (sched_getparam(main_pid, &main_param)) {
    printf("Failed to get scheduling parameters for `main_param`.\n");
    exit(-1);
  }

  main_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
  if (sched_setscheduler(main_pid, SCHED_FIFO, &main_param)) {
    printf("Failed to set scheduling parameters for `main_param`. Do you use "
           "sudo?\n");
    exit(-1);
  }
}

void initialize_timer(const struct timespec period, const size_t id,
                      TimerCallback callback, timer_t *const timer) {
  struct sigaction sa;
  sa.sa_flags = SA_SIGINFO;       // expect additional info
  sa.sa_sigaction = callback;     // signal callback function
  sigemptyset(&sa.sa_mask);       // clear the signal mask
  sigaction(SIGRTMIN, &sa, NULL); // action to take on SIGALRM

  struct sigevent sev;
  sev.sigev_notify = SIGEV_SIGNAL + id; // send a signal
  sev.sigev_signo = SIGRTMIN;           // signal number
  sev.sigev_value.sival_ptr = timer;
  timer_create(CLOCK_REALTIME, &sev, timer);

  struct itimerspec its;
  its.it_value.tv_sec = period.tv_sec;
  its.it_value.tv_nsec = period.tv_nsec;
  its.it_interval.tv_sec = period.tv_sec;
  its.it_interval.tv_nsec = period.tv_nsec;
  timer_settime(*timer, 0, &its, NULL);
}
