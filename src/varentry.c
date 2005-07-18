/* Hey EMACS -*- linux-c -*- */
/* $Id: files9x.c 1343 2005-07-06 15:26:11Z roms $ */

/*  libtifiles - Ti File Format library, a part of the TiLP project
 *  Copyright (C) 1999-2005  Romain Lievin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
	VarEntry structure management routines
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tifiles.h"

/**
 * tifiles_ve_create:
 *
 * Allocate a new VarEntry structure.
 *
 * Return value: the entry or NULL if error.
 **/
TIEXPORT VarEntry*	TICALL tifiles_ve_create(void)
{
	return calloc(1, sizeof(VarEntry));
}

/**
 * tifiles_ve_create_with_data:
 * @size: length of data.
 *
 * Allocate a new VarEntry structure and space for data.
 *
 * Return value: the entry or NULL if error.
 **/
TIEXPORT VarEntry*	TICALL tifiles_ve_create_with_data(uint32_t size)
{
	VarEntry* ve = tifiles_ve_create();
	ve->data = (uint8_t *) calloc(size, 1);

	return ve;
}

/**
 * tifiles_ve_create_array:
 * @nelts: size of NULL-terminated array (number of VarEntry structures).
 *
 * Allocate a NULL-terminated array of VarEntry structures. You have to allocate
 * each elements of the array by yourself.
 *
 * Return value: the array or NULL if error.
 **/
TIEXPORT VarEntry*	TICALL tifiles_ve_create_array(int nelts)
{
	return calloc(nelts + 1, sizeof(VarEntry));
}

/**
 * tifiles_ve_delete:
 * @ve: var entry.
 *
 * Free data buffer and the structure itself.
 *
 * Return value: none.
 **/
TIEXPORT void			TICALL tifiles_ve_delete(VarEntry* ve)
{
	assert(ve != NULL);

	free(ve->data);
	ve->data = NULL;
	free(ve);
}

/**
 * tifiles_ve_delete_array:
 * @array: an NULL-terminated array of VarEntry structures.
 *
 * Free the whole array (data buffer, VarEntry structure and array itself).
 *
 * Return value: none.
 **/
TIEXPORT void			TICALL tifiles_ve_delete_array(VarEntry* array)
{
	VarEntry* ptr;

	assert(array != NULL);

	for(ptr = array; ptr; ptr++)
		tifiles_ve_delete(ptr);
	free(array);
}

/**
 * tifiles_ve_copy:
 * @dst: destination entry.
 * @src: source entry.
 *
 * Copy VarEntry and its content from src to dst.
 * If data is NULL, a new buffer is allocated before copying.
 *
 * Return value: the dst pointer or NULL if malloc error.
 **/
TIEXPORT VarEntry*	TICALL tifiles_ve_copy(VarEntry* dst, VarEntry* src)
{
	int alloc = dst->data == NULL;

	assert(src != NULL);
	assert(dst != NULL);
		
	memcpy(dst, src, sizeof(VarEntry));
	if(alloc)
	{
		dst->data = (uint8_t *) calloc(dst->size, 1);
		if (dst->data == NULL)
			return NULL;
	}
	memcpy(dst->data, src->data, src->size);

	return dst;
}

/**
 * tifiles_ve_dup:
 * @src: source entry.
 *
 * Duplicate VarEntry and its content from src to dst.
 *
 * Return value: a newly allocated entry (must be freed with #tifiles_ve_delete when no longer needed).
 **/
TIEXPORT VarEntry*	TICALL tifiles_ve_dup(VarEntry* src)
{
	VarEntry* dst = calloc(1, sizeof(VarEntry));

	assert(src != NULL);

	dst->data = (uint8_t *) calloc(dst->size, 1);
	if (dst->data == NULL)
		return NULL;
	memcpy(dst->data, src->data, dst->size);

	return dst;
}