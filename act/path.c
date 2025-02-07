/*************************************************************************
 *
 *  This file is part of the ACT library
 *
 *  Copyright (c) 2011, 2019 Rajit Manohar
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *
 **************************************************************************
 */
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <ctype.h>
#include <common/misc.h>
#include "path.h"

static int first = 1;

static
struct pathlist {
  char *path;
  struct pathlist *next;
} *hd = NULL, *tl;


static
char *expand (char *s)
{
  char *path;
  char *t;
  char *ret;
  struct passwd *pwd;

  if (s[0] == '~' && *(s+1) == '/') {
    path = getenv ("HOME");
    if (!path) path = "";
    MALLOC (ret, char, strlen (s) + strlen(path)+ 5);
    strcpy (ret, path);
    strcat (ret, s+1);
  }
  else if (s[0] == '~') {
    t = s+1;
    while (*t && *t != '/') t++;
    if (!*t) fatal_error ("Invalid pathname!");
    *t = '\0';
    if (strcmp (s+1, "cad") == 0 && getenv ("CAD_HOME"))
      path = getenv ("CAD_HOME");
    else {
      pwd  = getpwnam (s+1);
      if (pwd->pw_dir == NULL) path = "";
      else path = pwd->pw_dir;
    }
    *t = '/';
    MALLOC (ret, char, strlen (t) + strlen (path) + 6);
    strcpy (ret, path);
    strcat (ret, t);
  }
  else {
    MALLOC (ret, char, strlen (s) + 6);
    strcpy (ret, s);
  }
  return ret;
}

static int addpath (char *s, int i)
{
  char *t;
  char c;

  if (s[i] == '"') i++;
  i--;
  do {
    i++;
    t = s+i;
    while (s[i] && !isspace(s[i]) && s[i] != '"' && s[i] != ':')
      i++;
    if (t != s+i) {
      c = s[i];
      s[i] = '\0';
      if (!hd) {
	MALLOC (hd, struct pathlist, 1);
	tl = hd;
	tl->next = NULL;
      }
      else {
	MALLOC (tl->next, struct pathlist, 1);
	tl = tl->next;
	tl->next = NULL;
      }
      MALLOC (tl->path, char, strlen (t)+1);
      strcpy (tl->path, t);
      s[i] = c;
    }
  } while (s[i] == ':');
  if (s[i] == '"') i++;
  return i;
}


/*
 * Initialize the search path
 */
static void _path_init (void)
{
  char buf[2];
  char *env;
  char *file;

  if (first) {
    /* first thing in the search path is the current directory */
    buf[0] = '.';
    buf[1] = '\0';
    addpath (buf, 0);

    /* next thing in the search path is $ACT_PATH */
    env = getenv ("ACT_PATH");
    if (env)
      addpath (env, 0);
    
    /* last item in the search path is $ACT_HOME */
    env = getenv ("ACT_HOME");
    if (env) {
      MALLOC (file, char, strlen (env)+8);
      strcpy (file, env);
      strcat (file, "/act");
      addpath (file, 0);
      FREE (file);
    }
  }
  first = 0;
}

/*------------------------------------------------------------------------
 *
 *  path_open --
 *
 *    Search through $ACT_PATH and $ACT_HOME/act to find a path name
 * where the file is located.
 *
 *------------------------------------------------------------------------
 */
char *path_open (char *name)
{
  struct pathlist *p;
  char *file, *try;
  FILE *fp;

  _path_init ();

  p = hd;
  try = expand(name);
  if ((fp = fopen (try, "r"))) {
    fclose (fp);
    return try;
  }
  FREE (try);
  while (p) {
    MALLOC (file, char, strlen (p->path)+strlen(name)+7);
    strcpy (file, p->path);
    strcat (file, "/");
    strcat (file, name);
    try = expand (file);
    FREE (file);
    fp = fopen (try, "r");
    if (fp) { 
      fclose (fp);
      return try;
    }
    strcat (try, ".act");
    fp = fopen (try, "r");
    if (fp) {
      fclose (fp);
      return try;
    }
    FREE (try);
    p = p->next;
  }
  return Strdup (name);
}


/*------------------------------------------------------------------------
 *
 *  path_open --
 *
 *    Search through $ACT_PATH and $ACT_HOME/act to find a path name
 * where the file is located.
 *
 *------------------------------------------------------------------------
 */
char *path_open_skipcwd (const char *name)
{
  struct pathlist *p;
  char *file, *try;
  FILE *fp;

  _path_init ();

  if (!hd) return Strdup (name);

  /* skip "." */
  p = hd->next;
  while (p) {
    MALLOC (file, char, strlen (p->path)+strlen(name)+7);
    strcpy (file, p->path);
    strcat (file, "/");
    strcat (file, name);
    try = expand (file);
    FREE (file);
    fp = fopen (try, "r");
    if (fp) { 
      fclose (fp);
      return try;
    }
    strcat (try, ".act");
    fp = fopen (try, "r");
    if (fp) {
      fclose (fp);
      return try;
    }
    FREE (try);
    p = p->next;
  }
  return NULL;
}



/*--- import list ---*/

struct import_list {
  char *file;
  struct import_list *next;
};

static struct import_list *il = NULL;
static struct import_list *pending = NULL;

void act_push_import (char *file)
{
  struct import_list *t;

  NEW (t, struct import_list);
  t->file = Strdup (file);
  t->next = pending;
  pending = t;
}

int act_isimported (const char *file)
{
  struct import_list *t;

  for (t = il; t; t = t->next) {
    if (strcmp (t->file, file) == 0) return 1;
  }
  return 0;
}

void act_pop_import (char *file)
{
  struct import_list *t;
  
  Assert (strcmp (pending->file, file) == 0, "what?");

  t = pending;
  pending = pending->next;
  t->next = il;
  il = t;
}

int act_pending_import (char *file)
{
  struct import_list *t;

  for (t = pending; t; t = t->next) {
    if (strcmp (file, t->file) == 0) return 1;
  }
  return 0;
}

void act_print_import_stack (FILE *fp)
{
  struct import_list *t;

  fprintf (fp, "Import history:\t");
  for (t = il; t != pending; t = t->next) {
    if (t != il) {
      fprintf (fp, "\n\t<- ");
    }
    fprintf (fp, "%s", t->file);
  }
  if (il == NULL) {
    fprintf (fp, "<toplevel>");
  }
  fprintf (fp, "\n");
}
  
