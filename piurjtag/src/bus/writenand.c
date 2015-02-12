/*
 * $Id$
 *
 * Written by Sam Wein (sam@samwein.com 2015).
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
 * There is a bunch of error reclaimation code here. I haven't had a chance to test it
 * so it's commented out. If you need it for your application please test it before use
 *
 */

#include <sysdep.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "nand_ecc.h" //TODO this should be moved to includes
#include <urjtag/log.h>
#include <urjtag/error.h>
#include <urjtag/bus.h>
#include <urjtag/flash.h>
#include <urjtag/jtag.h>
#include "writenand.h"


//Erase a block, returns 0 if okay and 1 if erase failed.
//Write a block containing metadata
/*int mark_bad_and_advance(urj_bus_t *bus, uint32_t *addr){
	return URJ_STATUS_FAIL;// Temporary cause I don't want to deal with reclaiming errors, now we just die.
	//FIXME RESCUE DATA FROM PREVIOUS PAGES
	*addr = *addr & (~(BSIZE - 1));// go to the first page of the block
	uint8_t bad[ECCSIZE];
	for (int i=0;i<ECCSIZE;i++){
		bad[i]=0x00;
	}
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+3, READ2S); //Point to area C
	do_writenand(bus,*addr,bad);
	*addr = *addr + BSIZE ; // go to the beginning of the next block
	return URJ_STATUS_OK;
}*/

int
urj_bus_writenand (urj_bus_t *bus, FILE *f, uint32_t addr, uint32_t len, uint32_t isecc)
{
    uint32_t step;
    uint32_t a;
    size_t bc = 0;
    //int bidx = 0;
    uint8_t b[BSIZE];
    //uint8_t c[BSIZE];
    urj_bus_area_t area;
    //uint32_t end;

    if (!bus)
    {
        urj_error_set (URJ_ERROR_NO_BUS_DRIVER, _("Missing bus driver"));
        return URJ_STATUS_FAIL;
    }

    URJ_BUS_PREPARE (bus);

    if (URJ_BUS_AREA (bus, addr, &area) != URJ_STATUS_OK)
        return URJ_STATUS_FAIL;

    step = area.width / 8;

    if (step == 0)
    {
        urj_error_set (URJ_ERROR_INVALID, _("Unknown bus width"));
        return URJ_STATUS_FAIL;
    }
    if (BSIZE % step != 0)
    {
        urj_error_set (URJ_ERROR_INVALID, "step %lu must divide BSIZE %d",
                       (long unsigned) step, BSIZE);
        return URJ_STATUS_FAIL;
    }

    //addr = addr & (~(step - 1)); //round to nearest start of address

    int inputblock=BSIZE; //adjust block size if our input is padded with ecc data
    if (isecc==0)
    	inputblock=BSIZE-ECCSIZE;

	/*if (len%inputblock!=0) //golly rounding up in C is annoying
		len=(len/inputblock)+1; //pad out file to fill block
    else
    	len=(len/inputblock);*/

    len = (len + inputblock - 1) & (~(inputblock - 1));



    urj_log (URJ_LOG_LEVEL_NORMAL, _("address: 0x%08lX\n"),
             (long unsigned) addr);
    urj_log (URJ_LOG_LEVEL_NORMAL, _("length:  0x%08lX\n"),
             (long unsigned) len);

    if (len == 0)
    {
        urj_error_set (URJ_ERROR_INVALID, _("length is 0"));
        return URJ_STATUS_FAIL;
    }

    a = addr;
    int pages= len/BSIZE;
    //end = a + len;
    urj_log (URJ_LOG_LEVEL_NORMAL, _("writing:\n"));

    for (int q=0; q < pages; q++) //start at start address and write for each page
    {

        //uint32_t data;
        //int j;

        /* Read one block of data */
        if (bc == 0)
        {
            urj_log (URJ_LOG_LEVEL_NORMAL, _("addr: 0x%08llX\r"),
                     (long long unsigned) a);
            bc = fread (b, 1, inputblock, f);
            if (bc != inputblock)
            {
                urj_log (URJ_LOG_LEVEL_NORMAL, _("Short read: bc=0x%zX\n"), bc);
                if (bc < step|| isecc!=0)//ecc'ed files need to be even length
                {
                    // Not even enough for one step. Something is wrong. Check
                    // the file state and bail out.
                    if (feof (f))
                        urj_error_set (URJ_ERROR_FILEIO,
                            _("Unexpected end of file; Addr: 0x%08llX\n"),
                            (long long unsigned) a);
                    else
                    {
                        urj_error_set (URJ_ERROR_FILEIO, "fread fails");
                        urj_error_state.sys_errno = ferror(f);
                        clearerr(f);
                    }

                    return URJ_STATUS_FAIL;
                }
                else{ //we need to pad out last page
                	for (int j=bc;j<inputblock;j++)
                	{
                		b[j]=0;
                	}
                }
            }
        }

        //TODO this is 528 byte specific
        if (!isecc){
        	uint8_t codes[6];
        	nand_calculate_ecc(&(b[0]),&(codes[0]));
        	nand_calculate_ecc(&(b[256]),&(codes[3]));
        	b[512]=codes[0];
        	b[513]=codes[1];
        	b[514]=codes[2];
        	b[515]=codes[3];
        	b[516]=0xff;
        	b[517]=0xff;
        	b[518]=codes[4];
        	b[519]=codes[5];
        	for(int j=520;j<=528;j++){
        		b[j]=0xff;
        	}
        }

        if (b[516]!=0xff||b[517]!=0xff){//if we have a bad block in our input
            urj_error_set (URJ_ERROR_FLASH_PROGRAM, "bad block marker in input at location %x\n",q*BSIZE);
            urj_error_state.sys_errno = ferror(f); //This is super bad, and will result in the block being marked bad in the chip
            clearerr(f);
            return URJ_STATUS_FAIL;
        }
        uint32_t failed, baderr=0; //baderr is for errors in the mark bad function
        //do{
        	if (baderr){
        		return URJ_STATUS_FAIL;
        	}
        	failed=0; //reset the failed marker
        	//do_read2(bus,a,c);
        	//if (c[6]!=0xff)
        	//{
        	//	urj_log (URJ_LOG_LEVEL_NORMAL, _("\n bad block detected at %x.\n"),(unsigned int)a);
        	//	a = (a + BSIZE - 1) & (~(BSIZE - 1)); // go to the beginning of the next block
        	//	failed=1;
        	//}

        	//int retries=0;
        	if (q%32==0)
        		failed=do_erasenand(bus,a);
        	if (failed){
        		urj_log (URJ_LOG_LEVEL_ERROR, _("\n erase failed.\n"));
        		return URJ_STATUS_FAIL;
        		//baderr=mark_bad_and_advance(bus,&a);
        		//continue;
        	}
        	failed=do_writenand(bus,a,b);
        	if (failed){
        		urj_log (URJ_LOG_LEVEL_ERROR, _("\n write failed.\n"));
         		//baderr=mark_bad_and_advance(bus,&a);
        		return URJ_STATUS_FAIL;
				//continue;
        	}
			/*do_read1(bus,a,c);
			if (memcmp(b,c,528)!=0){
        		urj_log (URJ_LOG_LEVEL_NORMAL, _("\n read data didn't match page:%x.\n"),q);
        		return URJ_STATUS_FAIL;
				if (retries<3){//only retry three times
					retries++;
					//continue;
				}
                else
                {
                    urj_error_set (URJ_ERROR_FLASH_PROGRAM, "retried a write too many times addr %x\n",a);
                    urj_error_state.sys_errno = ferror(f);
                    clearerr(f);
                    return URJ_STATUS_FAIL;
                }
			}
			else
			{*/
				bc=0;
				//retries=0;// we reset the retries count on a successful write
			//}
        //}while (failed==1);
        a += BSIZE; //We advance one block 'cause we wrote one block
        urj_log (URJ_LOG_LEVEL_DEBUG, _("\nwrote page %d.\n"),(q));
    }

    urj_log (URJ_LOG_LEVEL_NORMAL, _("\nDone.\n"));

    return URJ_STATUS_OK;
}
