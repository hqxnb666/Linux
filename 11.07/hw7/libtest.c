#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>

/* Needed for wait(...) */
#include <sys/types.h>
#include <sys/wait.h>

/* Needed for semaphores */
#include <semaphore.h>

/* Include struct definitions and other libraries that need to be
 * included by both client and server */
#include "common.h"

/* Simple main function to test key functions in the imglib and
 * md5lib */
int main (int argc, char ** argv) {

	char * buffer = malloc(818058);
	size_t toread = 818058;
	size_t got = 0;
	int ff = open("sample.bmp", O_RDONLY);
	while(got < toread) {
	    got += read(ff, buffer + got, toread - got);
	}
	struct md5digest md5dig = buf_md5sum(buffer, got);
	print_digest(md5dig);
	close(ff);

	struct image * img_test = loadBMP("sample.bmp");
	if (!img_test) {
	    perror("Unable to load image.");
	}

	int err = saveBMP("sample_out.bmp", img_test);
	if (err) {
	    perror("Unable to save image.");
	}

	struct image * img_rot = rotate90Clockwise(img_test, NULL);

	err = saveBMP("sample_rot.bmp", img_rot);
	if (err) {
	    perror("Unable to save image.");
	}

	struct image * img_sharp = sharpenImage(img_test);

	err = saveBMP("sample_sharp.bmp", img_sharp);
	if (err) {
	    perror("Unable to save image.");
	}

	struct image * img_blur = blurImage(img_test);

	err = saveBMP("sample_blur.bmp", img_blur);
	if (err) {
	    perror("Unable to save image.");
	}

	struct image * img_vert = detectVerticalEdges(img_test);

	err = saveBMP("sample_vert.bmp", img_vert);
	if (err) {
	    perror("Unable to save image.");
	}

	struct image * img_horiz = detectHorizontalEdges(img_test);

	err = saveBMP("sample_horiz.bmp", img_horiz);
	if (err) {
	    perror("Unable to save image.");
	}

	return EXIT_SUCCESS;
}
