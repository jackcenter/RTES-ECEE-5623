#include <utilities.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#define HRES_STR "320"
#define VRES_STR "240"

void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time)
{
  static char ppm_header[]="P6\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
  static char ppm_dumpname[]="frames/test00000000.ppm";
  
  snprintf(&ppm_dumpname[11], 9, "%08d", tag);
  strncat(&ppm_dumpname[19], ".ppm", 5);
  int dumpfd = open(ppm_dumpname, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);

  snprintf(&ppm_header[4], 11, "%010d", (int)time->tv_sec);
  strncat(&ppm_header[14], " sec ", 5);
  snprintf(&ppm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));
  strncat(&ppm_header[29], " msec \n"HRES_STR" "VRES_STR"\n255\n", 19);

  // subtract 1 because sizeof for string includes null terminator
  int written = write(dumpfd, ppm_header, sizeof(ppm_header)-1);

  int total = 0;

  do
  {
      written=write(dumpfd, p, size);
      total+=written;
  } while(total < size);

  syslog(LOG_CRIT, "wrote %d bytes\n", total);

  close(dumpfd);
}

void dump_pgm(const void *p, int size, unsigned int tag, struct timespec *time)
{
  static char pgm_header[]="P5\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
  static char pgm_dumpname[]="frames/test00000000.pgm";
  
  snprintf(&pgm_dumpname[11], 9, "%08d", tag);
  strncat(&pgm_dumpname[19], ".pgm", 5);
  int dumpfd = open(pgm_dumpname, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);

  snprintf(&pgm_header[4], 11, "%010d", (int)time->tv_sec);
  strncat(&pgm_header[14], " sec ", 5);
  snprintf(&pgm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));
  strncat(&pgm_header[29], " msec \n"HRES_STR" "VRES_STR"\n255\n", 19);

  // subtract 1 because sizeof for string includes null terminator
  int written = write(dumpfd, pgm_header, sizeof(pgm_header)-1);

  int total = 0;

  do
  {
      written=write(dumpfd, p, size);
      total+=written;
  } while(total < size);

  syslog(LOG_CRIT, "wrote %d bytes\n", total);

  close(dumpfd);
}

double getDuration(struct timespec const start, struct timespec const end) {
  return (double)(end.tv_sec - start.tv_sec) + 1e-9 * ((double)(end.tv_nsec - start.tv_nsec));
}

double getDurationInMilliseconds(struct timespec const start, struct timespec const end) {
  return 1e3 * getDuration(start, end);
}


// This is probably the most acceptable conversion from camera YUYV to RGB
//
// Wikipedia has a good discussion on the details of various conversions and cites good references:
// http://en.wikipedia.org/wiki/YUV
//
// Also http://www.fourcc.org/yuv.php
//
// What's not clear without knowing more about the camera in question is how often U & V are sampled compared
// to Y.
//
// E.g. YUV444, which is equivalent to RGB, where both require 3 bytes for each pixel
//      YUV422, which we assume here, where there are 2 bytes for each pixel, with two Y samples for one U & V,
//              or as the name implies, 4Y and 2 UV pairs
//      YUV420, where for every 4 Ys, there is a single UV pair, 1.5 bytes for each pixel or 36 bytes for 24 pixels

void yuv2rgb(int y, int u, int v, unsigned char *r, unsigned char *g, unsigned char *b)
{
   int r1, g1, b1;

   // replaces floating point coefficients
   int c = y-16, d = u - 128, e = v - 128;       

   // Conversion that avoids floating point
   r1 = (298 * c           + 409 * e + 128) >> 8;
   g1 = (298 * c - 100 * d - 208 * e + 128) >> 8;
   b1 = (298 * c + 516 * d           + 128) >> 8;

   // Computed values may need clipping.
   if (r1 > 255) r1 = 255;
   if (g1 > 255) g1 = 255;
   if (b1 > 255) b1 = 255;

   if (r1 < 0) r1 = 0;
   if (g1 < 0) g1 = 0;
   if (b1 < 0) b1 = 0;

   *r = r1 ;
   *g = g1 ;
   *b = b1 ;
}

void yuv2rgb_float(float y, float u, float v, 
                   unsigned char *r, unsigned char *g, unsigned char *b)
{
    float r_temp, g_temp, b_temp;

    // R = 1.164(Y-16) + 1.1596(V-128)
    r_temp = 1.164*(y-16.0) + 1.1596*(v-128.0);  
    *r = r_temp > 255.0 ? 255 : (r_temp < 0.0 ? 0 : (unsigned char)r_temp);

    // G = 1.164(Y-16) - 0.813*(V-128) - 0.391*(U-128)
    g_temp = 1.164*(y-16.0) - 0.813*(v-128.0) - 0.391*(u-128.0);
    *g = g_temp > 255.0 ? 255 : (g_temp < 0.0 ? 0 : (unsigned char)g_temp);

    // B = 1.164*(Y-16) + 2.018*(U-128)
    b_temp = 1.164*(y-16.0) + 2.018*(u-128.0);
    *b = b_temp > 255.0 ? 255 : (b_temp < 0.0 ? 0 : (unsigned char)b_temp);
}
