#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <obs.h>
#include <obs-module.h>

static int set_interface_attribs(int fd, int speed) {
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int serial_open(const char *file, int baudrate) {
    int fd = open(file, O_RDWR | O_NOCTTY | O_SYNC);

    if (fd < 0) {
        blog(LOG_ERROR, "Error opening %s: %s", file, strerror(errno));
        return -1;
    }

    set_interface_attribs(fd, baudrate);

    return fd;
}

void serial_close(int fd) {
    close(fd);
}

void serial_write(int fd, uint64_t timestamp, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    char *buffer;
    int len = asprintf(
        &buffer,
        "%d,%d/%d,%d %lld\n",
        (int)(((double)x - ((double)width / 2.0)) / ((double)width / 8.0)),
        (int)(((double)y - ((double)height / 2.0)) / ((double)height / 8.0)),
        x,
        y,
        timestamp
    );

    int wlen = write(fd, buffer, len);
    if (wlen != len) {
        blog(LOG_ERROR, "Error from write: %d, %d", wlen, errno);
    }
    tcdrain(fd);    /* delay for output */
    free(buffer);
}