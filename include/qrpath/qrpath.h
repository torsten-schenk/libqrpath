#pragma once

#include <stdlib.h>

typedef struct qrpath qrpath_t;

enum {
	QRPATH_SUCCESS = 0,
	QRPATH_UNSET = 0,
	QRPATH_SET
};

enum {
	QRPATH_OUT_OF_BOUNDS = -71000
};

qrpath_t *qrpath_new(
		int w,
		int h);

void qrpath_destroy(
		qrpath_t *self);

int qrpath_set(
		qrpath_t *self,
		int x,
		int y);

int qrpath_unset(
		qrpath_t *self,
		int x,
		int y);

int qrpath_isset(
		qrpath_t *self,
		int x,
		int y);

int qrpath_run_even_odd(
		qrpath_t *self,
		void (*begin)(void *user, int x, int y, int area),
		void (*end)(void *user),
		void (*lineto)(void *user, int x, int y),
		void *user);

const int *qrpath_areas(
		qrpath_t *self);

void qrpath_dump(
		qrpath_t *self);

