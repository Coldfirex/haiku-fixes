/*
 * Copyright 2026, Alan Shearer.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alan Shearer
 *
 * Exercises virtio_gpu CLONE_ACCELERANT / UNINIT_ACCELERANT from a
 * second team (the same path BDirectWindow and BWindowScreen use).
 *
 * This proves the clone *success* path still works after the overlay.
 * It does not hit the clone_area failure path that closed the fd twice.
 * That path needs a one-off fail-injection build (see README).
 *
 * Needs a running VirtioGpu desktop (app_server already called INIT).
 * Do not call B_INIT_ACCELERANT from this program.
 *
 * Build on Haiku (gcc 2.95 or gcc 13):
 *		make
 * Run:
 *		./virtio_gpu_clone
 *		./virtio_gpu_clone 200
 */

#include <Accelerant.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __HAIKU__
#	include <SupportDefs.h>
#else
#	include <stdint.h>
	typedef int32_t status_t;
	typedef int32_t image_id;
#	define B_OK 0
#	define B_ERROR -1
#endif


static int
count_open_fds(void)
{
	int n;
	int fd;
	int lim;

	n = 0;
	lim = sysconf(_SC_OPEN_MAX);
	if (lim < 0 || lim > 4096)
		lim = 1024;

	for (fd = 0; fd < lim; fd++) {
		if (fcntl(fd, F_GETFD) != -1)
			n++;
	}
	return n;
}


static int
find_virtio_dev_in(const char *dirPath, char *cloneInfo, size_t cloneInfoSize)
{
	DIR *dir;
	struct dirent *ent;
	char child[256];
	struct stat st;

	dir = opendir(dirPath);
	if (dir == NULL)
		return -1;

	while ((ent = readdir(dir)) != NULL) {
		if (strcmp(ent->d_name, ".") == 0
			|| strcmp(ent->d_name, "..") == 0)
			continue;

		if (snprintf(child, sizeof(child), "%s/%s", dirPath,
			ent->d_name) >= (int)sizeof(child))
			continue;

		if (stat(child, &st) != 0)
			continue;

		if (S_ISDIR(st.st_mode)) {
			if (find_virtio_dev_in(child, cloneInfo,
				cloneInfoSize) == 0) {
				closedir(dir);
				return 0;
			}
			continue;
		}

		if (strstr(child, "virtio") == NULL)
			continue;
		if (strncmp(child, "/dev/", 5) != 0)
			continue;

		/* CLONE_ACCELERANT prepends "/dev/" itself. */
		if (snprintf(cloneInfo, cloneInfoSize, "%s", child + 5)
			>= (int)cloneInfoSize) {
			closedir(dir);
			fprintf(stderr, "device name too long: %s\n", child);
			return -1;
		}

		closedir(dir);
		return 0;
	}

	closedir(dir);
	return -1;
}


static int
find_virtio_dev(char *cloneInfo, size_t cloneInfoSize)
{
	if (find_virtio_dev_in("/dev/graphics", cloneInfo, cloneInfoSize) == 0)
		return 0;

	fprintf(stderr, "no virtio device node under /dev/graphics\n");
	fprintf(stderr, "ls -lR /dev/graphics\n");
	return -1;
}


static image_id
load_accelerant(char *loadedFrom, size_t loadedFromSize)
{
	static const char *kPaths[] = {
		"/boot/home/config/non-packaged/add-ons/accelerants/virtio_gpu.accelerant",
		"/boot/system/non-packaged/add-ons/accelerants/virtio_gpu.accelerant",
		"/boot/system/add-ons/accelerants/virtio_gpu.accelerant",
		NULL
	};
	image_id image;
	int i;

	for (i = 0; kPaths[i] != NULL; i++) {
		image = load_add_on(kPaths[i]);
		if (image >= 0) {
			snprintf(loadedFrom, loadedFromSize, "%s", kPaths[i]);
			return image;
		}
	}

	fprintf(stderr, "load_add_on virtio_gpu.accelerant failed\n");
	return -1;
}


int
main(int argc, char **argv)
{
	char cloneInfo[256];
	char loadedFrom[256];
	image_id image;
	GetAccelerantHook getHook;
	clone_accelerant cloneFn;
	uninit_accelerant uninitFn;
	status_t err;
	int loops;
	int i;
	int fdsBefore;
	int fdsAfter;
	int fdsMid;

	loops = 50;
	if (argc > 1) {
		loops = atoi(argv[1]);
		if (loops < 1)
			loops = 1;
		if (loops > 10000)
			loops = 10000;
	}

#ifndef __HAIKU__
	fprintf(stderr, "this program only runs on Haiku\n");
	return 1;
#endif

	if (find_virtio_dev(cloneInfo, sizeof(cloneInfo)) != 0)
		return 1;

	image = load_accelerant(loadedFrom, sizeof(loadedFrom));
	if (image < 0)
		return 1;

	err = get_image_symbol(image, B_ACCELERANT_ENTRY_POINT, B_SYMBOL_TYPE_TEXT,
		(void **)&getHook);
	if (err != B_OK || getHook == NULL) {
		fprintf(stderr, "get_accelerant_hook symbol missing (%ld)\n",
			(long)err);
		unload_add_on(image);
		return 1;
	}

	cloneFn = (clone_accelerant)getHook(B_CLONE_ACCELERANT, NULL);
	uninitFn = (uninit_accelerant)getHook(B_UNINIT_ACCELERANT, NULL);
	if (cloneFn == NULL || uninitFn == NULL) {
		fprintf(stderr, "CLONE or UNINIT hook missing\n");
		unload_add_on(image);
		return 1;
	}

	printf("accelerant: %s\n", loadedFrom);
	printf("clone info: %s\n", cloneInfo);
	printf("loops:      %d\n", loops);

	fdsBefore = count_open_fds();

	err = cloneFn(cloneInfo);
	if (err != B_OK) {
		fprintf(stderr, "first CLONE_ACCELERANT failed: %ld (%s)\n",
			(long)err, strerror(err < 0 ? err : 0));
		unload_add_on(image);
		return 1;
	}
	fdsMid = count_open_fds();
	uninitFn();

	for (i = 1; i < loops; i++) {
		err = cloneFn(cloneInfo);
		if (err != B_OK) {
			fprintf(stderr, "CLONE_ACCELERANT loop %d failed: %ld\n",
				i, (long)err);
			unload_add_on(image);
			return 1;
		}
		uninitFn();
	}

	fdsAfter = count_open_fds();
	unload_add_on(image);

	printf("fds before clone: %d\n", fdsBefore);
	printf("fds during clone: %d\n", fdsMid);
	printf("fds after loops:  %d\n", fdsAfter);
	printf("ok clone success path (%d rounds)\n", loops);

	if (fdsAfter > fdsBefore + 2) {
		fprintf(stderr, "fd count climbed; possible leak on success path\n");
		return 1;
	}

	return 0;
}
