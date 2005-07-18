/* Hey EMACS -*- linux-c -*- */
/* $Id$ */

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
  Grouping/Ungrouping routines
  Calcs: 73/82/83/83+/84+/85/86 & 89/89tm/92/92+/V200
*/

#include <stdlib.h>
#include <string.h>

#include "tifiles.h"
#include "error.h"
#include "macros.h"
#include "files8x.h"
#include "files9x.h"
#include "logging.h"

/***********/
/* Freeing */
/***********/

/**
 * tifiles_content_create_group:
 * @n: number of variables to allocate
 *
 * Convenient function which create a NULL-terminated array of #FileContent 
 * structures (typically used to store a group file).
 *
 * Return value: the array or NULL if failed.
 **/
TIEXPORT FileContent** TICALL tifiles_content_create_group(int n_entries)
{
	return (FileContent **)calloc(n_entries + 1, sizeof(FileContent *));
}

/**
 * tifiles_content_free_group:
 *
 * Convenient function which free a NULL-terminated array of #FileContent 
 * structures (typically used to store a group file) and the array itself.
 *
 * Return value: always 0.
 **/
TIEXPORT int TICALL tifiles_content_free_group(FileContent **array)
{
	int i, n;
	
	// counter number of files to group
	for (n = 0; array[n] != NULL; n++);

	// release allocated memory in structures
	for (i = 0; i < n; i++) 
	{
#ifndef __WIN32__
	    TRYC(tifiles_content_free_regular(array[i]));
		free(array[i]);
#endif
	}
	free(array);

  return 0;
}

/************************/
/* (Un)grouping content */
/************************/

int ti8x_dup_VarEntry(VarEntry *dst, VarEntry *src);
int ti9x_dup_VarEntry(VarEntry *dst, VarEntry *src);

static int tixx_dup_VarEntry(VarEntry *dst, VarEntry *src)
{
#if !defined(DISABLE_TI8X)
	return ti8x_dup_VarEntry(dst, src);
#elif !defined(DISABLE_TI9X)
    return ti9x_dup_VarEntry(dst, src);
#else
#error "You can't disable TI8x & TI9x support both".
#endif
}

/**
 * tifiles_group_contents:
 * @src_contents: a pointer on an array of #FileContent structures. The array must be terminated by NULL.
 * @dst_content: the address of a pointer. This pointer will contain the allocated group file.
 *
 * Must be freed when no longer needed as well as the content of each #FileContent structure
 * (use #tifiles_content_free_regular as usual).
 *
 * Group several #FileContent structures into a single one.
 *
 * Return value: an error code if unsuccessful, 0 otherwise.
 **/
TIEXPORT int TICALL tifiles_group_contents(FileContent **src_contents, FileContent **dst_content)
{
  FileContent *dst;
  int i, n;

  for (n = 0; src_contents[n] != NULL; n++);

  dst = (FileContent *) calloc(1, sizeof(FileContent));
  if (dst == NULL)
    return ERR_MALLOC;

  memcpy(dst, src_contents[0], sizeof(FileContent));

  dst->num_entries = n;
  dst->entries = (VarEntry *) calloc(n + 1, sizeof(VarEntry));
  if (dst->entries == NULL)
    return ERR_MALLOC;

  for (i = 0; i < n; i++) 
  {
    FileContent *src = src_contents[i];

    TRYC(tixx_dup_VarEntry(&(dst->entries[i]), &(src->entries[0])));
  }

  *dst_content = dst;

  return 0;
}

/**
 * tifiles_ungroup_content:
 * @src_content: a pointer on the structure to unpack.
 * @dst_contents: the address of your pointer. This pointers will point on a 
 * dynamically allocated array of structures. The array is terminated by NULL.
 *
 * Ungroup a TI file by exploding the structure into an array of structures.
 *
 * Array must be freed when no longer needed as well as the content of each #FileContent 
 * structure (use #tifiles_content_free_regular as usual).
 *
 * Return value: an error code if unsuccessful, 0 otherwise.
 **/
TIEXPORT int TICALL tifiles_ungroup_content(FileContent *src, FileContent ***dest)
{
  int i;
  FileContent **dst;

  // allocate an array of FileContent structures (NULL terminated)
  dst = *dest = (FileContent **) calloc(src->num_entries + 1,
				      sizeof(FileContent *));
  if (dst == NULL)
    return ERR_MALLOC;

  // parse each entry and duplicate it into a single content  
  for (i = 0; i < src->num_entries; i++) 
  {
    VarEntry *src_entry = &(src->entries[i]);
    VarEntry *dst_entry = NULL;

    // allocate and duplicate content
    dst[i] = (FileContent *) calloc(1, sizeof(FileContent));
    if (dst[i] == NULL)
      return ERR_MALLOC;
    memcpy(dst[i], src, sizeof(FileContent));

    // allocate and duplicate entry
    dst[i]->entries = (VarEntry *) calloc(1+1, sizeof(VarEntry));
    dst_entry = &(dst[i]->entries[0]);
    TRYC(tixx_dup_VarEntry(dst_entry, src_entry));

    // update some fields
    dst[i]->num_entries = 1;
    dst[i]->checksum +=
	tifiles_checksum((uint8_t *) dst_entry, 15);
    dst[i]->checksum +=
	tifiles_checksum(dst_entry->data, dst_entry->size);
  }
  dst[i] = NULL;

  return 0;
}

/*************************/
/* (Un)grouping of files */
/*************************/

/**
 * tifiles_group_files:
 * @src_filenames: a NULL-terminated array of strings (list of files to group).
 * @dst_filename: the filename where to store the group.
 *
 * Group several TI files into a single one (group file).
 *
 * Return value: an error code if unsuccessful, 0 otherwise.
 **/
TIEXPORT int TICALL tifiles_group_files(char **src_filenames, const char *dst_filename)
{
  int i, n;
  FileContent **src = NULL;
  FileContent *dst = NULL;
  char *unused;

  // counter number of files to group
  for (n = 0; src_filenames[n] != NULL; n++);

  // allocate space for that
  src = (FileContent **) calloc(n + 1, sizeof(FileContent *));
  if (src == NULL)
    return ERR_MALLOC;

  // allocate each structure and load file content
  for (i = 0; i < n; i++) 
  {
    src[i] = (FileContent *) calloc(1, sizeof(FileContent));
    if (src[i] == NULL)
      return ERR_MALLOC;

    TRYC(tifiles_file_read_regular(src_filenames[i], src[i]));
  }
  src[i] = NULL;

  // group the array of structures
  TRYC(tifiles_group_contents(src, &dst));

  // release allocated memory
  tifiles_content_free_group(src);

  // write grouped file
  TRYC(tifiles_file_write_regular(dst_filename, dst, &unused));

  // release allocated memory
  tifiles_content_free_regular(dst);
  free(dst);

  return 0;
}

/**
 * tifiles_ungroup_file:
 * @src_filename: full path of file to ungroup.
 * @dst_filenames: NULL or the address of a pointer where to store a NULL-terminated 
 * array of strings which contain the list of ungrouped files.
 *
 * Ungroup a TI 'group' file into several files. Resulting files have the
 * same name as the variable stored within group file.
 * Beware: there is no existence check; files may be overwritten !
 *
 * %dst_filenames must be freed when no longer used.
 *
 * Return value: an error code if unsuccessful, 0 otherwise.
 **/
TIEXPORT int TICALL tifiles_ungroup_file(const char *src_filename, char ***dst_filenames)
{
  FileContent src;
  FileContent **dst, **ptr;
  char *real_name;
  
  int i, n;

  // read group file
  TRYC(tifiles_file_read_regular(src_filename, &src));

  // ungroup structure
  TRYC(tifiles_ungroup_content(&src, &dst));

  // count number of structures and allocates array of strings
  for(ptr = dst, n = 0; *ptr != NULL; ptr++, n++);
  if(dst_filenames != NULL)
	  *dst_filenames = (char **)malloc((n + 1) * sizeof(char *));

  // store each structure content to file
  for (ptr = dst, i = 0; *ptr != NULL; ptr++, i++)
  {
    TRYC(tifiles_file_write_regular(NULL, *ptr, &real_name));

	if(dst_filenames != NULL)
		*dst_filenames[i] = real_name;
	else
		free(real_name);
  }

  // release allocated memory
  tifiles_content_free_regular(&src);
  tifiles_content_free_group(dst);

  return 0;
}
