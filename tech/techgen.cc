/*************************************************************************
 *
 *  Copyright (c) 2019 Rajit Manohar
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
#include <unistd.h>
#include <math.h>
#include <map>

#include <act/act.h>
#include <act/tech.h>
#include <pp.h>

#define pp_nl pp_forced (pp, 0)
#define pp_nltab pp_forced (pp, 3)
#define pp_TAB pp_nltab; pp_setb (pp)
#define pp_UNTAB pp_endb (pp); pp_nl
#define pp_SPACE pp_nl; pp_nl
 
static void emit_header (pp_t *pp)
{
  pp_printf (pp, "tech"); pp_TAB;
  pp_printf (pp, "format 33"); pp_nl;
  pp_printf (pp, "%s", Technology::T->name); pp_UNTAB;
  pp_printf (pp, "end"); pp_SPACE;

  pp_printf (pp, "version"); pp_TAB;
  pp_printf (pp, "version 0"); pp_nl;
  pp_printf (pp, "description \"%s\"", Technology::T->date); pp_UNTAB;
  pp_printf (pp, "end"); pp_SPACE;
}

static void emit_planes (pp_t *pp)
{
  pp_printf (pp, "#--- Planes ---"); pp_nl;
  pp_printf (pp, "planes"); pp_TAB;
  pp_printf (pp, "well,w"); pp_nl;
  pp_printf (pp, "active,a"); pp_nl;
  for (int i=0; i < Technology::T->nmetals; i++) {
    pp_printf (pp, "metal%d,m%d", i+1, i+1);
    if (i != Technology::T->nmetals-1) {
      pp_nl;
    }
    else {
      pp_UNTAB;
    }
  }
  pp_printf (pp, "end"); pp_SPACE;
}

static void emit_tiletypes (pp_t *pp)
{
  int empty = 1;
  
  pp_printf (pp, "#--- Tile types ---"); pp_nl;
  pp_printf (pp, "types"); pp_TAB;
  for (int i=0; i < Technology::T->num_devs; i++) {
    for (int j=0; j < 2; j++) {
      if (Technology::T->well[j][i]) {
	pp_printf (pp, "well   %s", Technology::T->well[j][i]->getName());
	pp_nl;
	empty = 0;
      }
    }
  }
  if (!empty) {
    pp_nl;
  }
  empty = 1;
  for (int i=0; i < Technology::T->num_devs; i++) {
    for (int j=0; j < 2; j++) {
      if (Technology::T->diff[j][i]) {
	pp_printf (pp, "active   %s", Technology::T->diff[j][i]->getName());
	pp_nl;
	pp_printf (pp, "active   %sc", Technology::T->diff[j][i]->getName());
	pp_nl;
	empty = 0;
      }
      if (Technology::T->well[j][i]) {
	pp_printf (pp, "active   %sc", Technology::T->well[j][i]->getName());
	pp_nl;
	empty = 0;
      }
      if (Technology::T->fet[j][i]) {
	pp_printf (pp, "active   %s", Technology::T->fet[j][i]->getName());
	pp_nl;
	empty = 0;
      }
    }
  }
  pp_printf (pp, "active   %s", Technology::T->poly->getName()); pp_nl;
  pp_printf (pp, "active   %sc", Technology::T->poly->getName()); pp_nl;
  pp_nl;

  for (int i=0; i < Technology::T->nmetals; i++) {
    pp_printf (pp, "metal%d m%d", i+1, i+1); pp_nl;
    pp_printf (pp, "metal%d m%dpin", i+1, i+1); pp_nl;
    if (i != Technology::T->nmetals-1) {
      pp_printf (pp, "metal%d m%dc", i+1, i+2); pp_nl;
    }
    else {
      pp_printf (pp, "metal%d pad", i+1);
    }
  }
  pp_UNTAB;
  pp_printf (pp, "end"); pp_SPACE;
}


static void emit_contacts (pp_t *pp)
{
  pp_printf (pp, "#--- contacts ---"); pp_nl;
  pp_printf (pp, "contact"); pp_TAB;

  pp_printf (pp, "%sc %s metal1", Technology::T->poly->getName(),
	     Technology::T->poly->getName());
  pp_nl;
  for (int i=0; i < Technology::T->num_devs; i++) {
    for (int j=0; j < 2; j++) {
      if (Technology::T->diff[j][i]) {
	pp_printf (pp, "%sc %s metal1", Technology::T->diff[j][i]->getName(),
		   Technology::T->diff[j][i]->getName());
	pp_nl;
      }
      if (Technology::T->well[j][i]) {
	pp_printf (pp, "%sc %s metal1", Technology::T->well[j][i]->getName(),
		   Technology::T->well[j][i]->getName());
	pp_nl;
      }
    }
  }

  for (int i=1; i < Technology::T->nmetals; i++) {
    pp_printf (pp, "m%dc metal%d metal%d", i+1, i, i+1);
    pp_nl;
  }
  pp_printf (pp, "stackable"); pp_UNTAB;
  pp_printf (pp, "end"); pp_SPACE;
}


static void emit_styles (pp_t *pp)
{
  pp_printf (pp, "styles"); pp_TAB;
  pp_printf (pp, "styletype mos"); pp_TAB;

  

  pp_endb (pp); pp_UNTAB;
  pp_printf (pp, "end"); pp_SPACE;
}

int main (int argc, char **argv)
{
  pp_t *pp;
  
  Act::Init (&argc, &argv);
  Technology::Init("layout.conf");
  
  if (Technology::T->nmetals < 2) {
    fatal_error ("Can't handle a process with fewer than two metal layers!");
  }

  pp = pp_init (stdout, 72);

  emit_header (pp);
  emit_planes (pp);
  emit_tiletypes (pp);
  emit_contacts (pp);
  emit_styles (pp);

  pp_close (pp);
  
  return 0;
}
