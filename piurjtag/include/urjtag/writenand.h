/*
 * $Id$
 *
 * Written by Sam Wein (sam@samwein.com, 2015)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 *
 */

#include <sysdep.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <urjtag/nand_ecc.h> //TODO THIS IS SLOPPY
#include <urjtag/log.h>
#include <urjtag/error.h>
#include <urjtag/bus.h>
#include <urjtag/flash.h>
#include <urjtag/jtag.h>



//All of this should be set based on chip type. If you are using this code in this state
//fill the info in from the datasheet for the target sheet.
#define BSIZE 528
#define PAGEINBLOCK 32
#define ECCSIZE (uint32_t)16
#define PSEUDOADDRESS (uint32_t)134217728
#define READ1S (uint32_t)0
#define READ2S (uint32_t)80
#define PAGEPROGS (uint32_t)128
#define	PAGEPROGE (uint32_t)16
#define ERASES (uint32_t)96
#define ERASEE (uint32_t)208
#define GETSTATUS (uint32_t)112
#define ERASEDELAY (uint32_t) 1000000 //this is way longer than it should be for the test chip, TODO tweak it
#define WRITEDELAY (uint32_t) 500
#define READPAGESLEEP 100

//Erase a block, returns 0 if okay and 1 if erase failed.
uint32_t do_erasenand(urj_bus_t *bus,uint32_t addr); //erase a block

//Write a block containing ecc metadata
uint32_t do_writenand(urj_bus_t *bus,uint32_t addr,uint8_t *b );//write a page

uint32_t do_read1(urj_bus_t *bus,uint32_t addr,uint8_t *b ); //read a page

uint32_t do_read2(urj_bus_t *bus,uint32_t addr,uint8_t *b ); //Read area 3 (metadata)

uint32_t do_seqread(urj_bus_t *bus, uint32_t addr, uint8_t *b);//read a whole block!

int mark_bad_and_advance(urj_bus_t *bus,uint32_t *addr);// TODO not actually implemented

int
urj_bus_writenand (urj_bus_t *bus, FILE *f, uint32_t addr, uint32_t len, uint32_t isecc);
