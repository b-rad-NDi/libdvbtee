/*****************************************************************************
 * Copyright (C) 2011-2014 Michael Ira Krufky
 *
 * Author: Michael Ira Krufky <mkrufky@linuxtv.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/
#define DBG 0

#include <string.h>
#include "rbuf.h"
#include "log.h"
#define CLASS_MODULE "rbuf"

#define dPrintf(fmt, arg...) __dPrintf(DBG_OUTPUT, fmt, ##arg)

rbuf::rbuf()
  : capacity(0)
  , p_data(NULL)
  , idx_read(0)
  , idx_write(0)
{
	dPrintf("()");
	pthread_mutex_init(&mutex, 0);
}

rbuf::~rbuf()
{
	dPrintf("()");

	dealloc();

	pthread_mutex_destroy(&mutex);
}

rbuf::rbuf(const rbuf&)
{
	dPrintf("(copy)");
	p_data    = NULL;
	capacity  = 0;
	idx_read  = 0;
	idx_write = 0;
}

rbuf& rbuf::operator= (const rbuf& cSource)
{
	dPrintf("(operator=)");

	if (this == &cSource)
		return *this;

	p_data    = NULL;
	capacity  = 0;
	idx_read  = 0;
	idx_write = 0;

	return *this;
}

void rbuf::dealloc()
{
	dPrintf("()");
	pthread_mutex_lock(&mutex);

	if (p_data)
		delete[] p_data;
	p_data = NULL;

	capacity = 0;
	__reset();

	pthread_mutex_unlock(&mutex);
}

void rbuf::set_capacity(int cap)
{
	dPrintf("(%d)", cap);
	pthread_mutex_lock(&mutex);

	if (p_data)
		delete[] p_data;

	p_data = new char[(capacity = cap)];
	__reset();

	pthread_mutex_unlock(&mutex);
}

int rbuf::get_capacity()
{
#if DBG
	dPrintf("(%d)", capacity);
#endif
	return capacity;
}

int rbuf::get_size()
{
#if DBG
	dPrintf("()");
#endif
	pthread_mutex_lock(&mutex);

	int ret = __get_size();

	pthread_mutex_unlock(&mutex);

	return ret;
}

void rbuf::reset()
{
	dPrintf("()");
	pthread_mutex_lock(&mutex);

	__reset();

	pthread_mutex_unlock(&mutex);
}

bool rbuf::check()
{
	if (!capacity) {
		dPrintf("capacity not set!!!");
		return false;
	}
	int size = get_size();

	dPrintf("%d.%02d%% usage (%d / %d)",
		100 * size / capacity,
		100 * (100 * size % capacity) / capacity,
		size, capacity);

	return true;
}

int rbuf::get_write_ptr(void** p)
{
	pthread_mutex_lock(&mutex);

	return __get_write_ptr(p);
}

void rbuf::put_write_ptr(int size)
{
	__put_write_ptr(size);

	pthread_mutex_unlock(&mutex);
}

#if 0
bool rbuf::write(const void* p, int size)
{
	pthread_mutex_lock(&mutex);

	if (__get_size() + size > capacity) {
		pthread_mutex_unlock(&mutex);
		return false;
	}

	if (idx_write + size > capacity) {
		int split = capacity - idx_write;
		memcpy(p_data + idx_write, p, split);
		idx_write = size - split;
		memcpy(p_data, (const char*) p + split, idx_write);
	} else {
		memcpy(p_data + idx_write, p, size);
		idx_write += size;
		if (idx_write == capacity)
			idx_write = 0;
	}
	pthread_mutex_unlock(&mutex);
	return true;
}
#else
bool rbuf::write(const void* p, int size)
{
	pthread_mutex_lock(&mutex);
	if (__get_size() + size > capacity) {
		pthread_mutex_unlock(&mutex);
		return false;
	}

	void *q = NULL;
	char *r = (char*)p;
	int available;

	while (size) {
		available = __get_write_ptr(&q);
		if (available >= size) {
			memcpy(q, r, size);
			__put_write_ptr(size);
			size = 0;
		} else if (available > 0) {
			memcpy(q, r, available);
			__put_write_ptr(available);
			size -= available;
			r += available;
		}
#if 0
		else {
//			pthread_mutex_unlock(&mutex);
			return false;
		}
#endif
	}
	pthread_mutex_unlock(&mutex);
	return true;
}
#endif


int rbuf::get_read_ptr(void**p, int size)
{
	pthread_mutex_lock(&mutex);

	return __get_read_ptr(p, size);
}

void rbuf::put_read_ptr(int size)
{
	__put_read_ptr(size);

	pthread_mutex_unlock(&mutex);
}

int rbuf::read(void* p, int size)
{
	void *q = NULL;
	int newsize = get_read_ptr(&q, size);

	/*  newsize will never be < 0, but this should satisfy the coverity checker */
	if (newsize > 0)
		memcpy(p, q, newsize);

	put_read_ptr(newsize);
	return newsize;
}


int rbuf::__get_size()
{
	int ret = idx_write - idx_read;

	if (ret < 0)
		ret += capacity;

	return ret;
}

void rbuf::__reset()
{
	idx_read = idx_write = 0;
}

int rbuf::__get_write_ptr(void** p)
{
	int available = (idx_read <= idx_write) ? capacity - idx_write : idx_read - idx_write;

	if (available <= 0) {
		pthread_mutex_unlock(&mutex);
		return available;
	}
	*p = &p_data[idx_write];

	return available;
}

void rbuf::__put_write_ptr(int size)
{
	if (idx_write + size >= capacity) { // == should be enough FIXME!
		idx_write = 0;
	} else {
		idx_write += size;
	}
}

int rbuf::__get_read_ptr(void**p, int size)
{
	int max_size = __get_size();

	/*  max_size will never be < 0, but this should satisfy the coverity checker */
	if (max_size <= 0)
		return 0;

	if (size > max_size)
		size = max_size;

	if (idx_read + size >= capacity) {
		size = capacity - idx_read;
		*p = p_data + idx_read;
	} else {
		*p = p_data + idx_read;
	}
	return size;
}

void rbuf::__put_read_ptr(int size)
{
	if (idx_read + size >= capacity) {
		idx_read = 0;
	} else {
		idx_read += size;
	}
}
