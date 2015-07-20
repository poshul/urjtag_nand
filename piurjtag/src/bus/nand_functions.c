/*
 * nand_functions.c
 *
 *  Created on: Feb 12, 2015
 *      Author: samuel
 */

#include <urjtag/writenand.h>



uint32_t do_erasenand(urj_bus_t *bus,uint32_t addr){
	uint32_t data;
	int retries=0;
	do{
	retries++;
	uint32_t actaddr=addr/BSIZE;
	urj_log (URJ_LOG_LEVEL_DEBUG, _("\nacctaddr %x.\n"),(actaddr));
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+3, ERASES);
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+5, (actaddr)&255);
	urj_log (URJ_LOG_LEVEL_DEBUG, _("\n%x\n"),(actaddr)&255);
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+5, (actaddr>>8));
	urj_log (URJ_LOG_LEVEL_DEBUG, _("\n%x\n"),(actaddr>>8));
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+3, ERASEE);
	do{
		usleep(ERASEDELAY); //we generally need to do this loop twice before the chip is ready.
		URJ_BUS_WRITE (bus, PSEUDOADDRESS+3, GETSTATUS);
		URJ_BUS_READ_START (bus, PSEUDOADDRESS);
		data =  URJ_BUS_READ_END (bus);
		urj_log (URJ_LOG_LEVEL_DEBUG, _("\nchip status after erase returned status %x.\n"),(data));
	}while((data&64)==0 &&(data&32)==0); //loop while the chip is not ready
	}while ((data&32)==1&& retries<3);
	data=data&1;
	return data; //return status of erase
}

uint32_t do_writenand(urj_bus_t *bus,uint32_t addr,uint8_t *b ){
	uint32_t actaddr=addr/BSIZE;
	for (int i=0; i<BSIZE;i++)
		urj_log (URJ_LOG_LEVEL_ALL, _("%x"),(b[0])); //If you want to watch the data go by
	urj_log (URJ_LOG_LEVEL_ALL, _("\n"));
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+3, 00);
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+3, PAGEPROGS);
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+5, 00);
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+5, (actaddr)&255);
	urj_log (URJ_LOG_LEVEL_DEBUG, _("Writing to address:\n%x\n"),(actaddr)&255);
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+5, (actaddr>>8));
	urj_log (URJ_LOG_LEVEL_DEBUG, _("%x\n"),(actaddr>>8));


	for(int i=0;i<BSIZE;i++){
		URJ_BUS_WRITE (bus, PSEUDOADDRESS, b[i]);
	}
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+3, PAGEPROGE);
	usleep(WRITEDELAY); //sleep for WRITEDELAY uS
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+3, GETSTATUS);
	URJ_BUS_READ_START (bus, PSEUDOADDRESS);
	uint32_t data =  URJ_BUS_READ_END (bus);
	data=data&1; //just look at the last bit
	urj_log (URJ_LOG_LEVEL_NORMAL, _("\nwriteblock returned status %d.\n"),(data));
	return data; //return status of write
}

uint32_t do_read1(urj_bus_t *bus,uint32_t addr,uint8_t *b ){
	uint32_t actaddr=addr/BSIZE;
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+3, READ1S);
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+5, 00);
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+5, (actaddr)&255);
	urj_log (URJ_LOG_LEVEL_DEBUG, _("\nReading Address:%x"),(actaddr)&255);
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+5, (actaddr>>8));
	urj_log (URJ_LOG_LEVEL_DEBUG, _("%x\n"),(actaddr>>8));
	URJ_BUS_READ_START (bus, PSEUDOADDRESS);
	for(int i=1;i<BSIZE;i++){
		b[i-1] = URJ_BUS_READ_NEXT (bus, i);
	}
	b[BSIZE-1] = URJ_BUS_READ_END (bus);
	return 0;
}


uint32_t do_read2(urj_bus_t *bus,uint32_t addr,uint8_t *b ){
	uint32_t actaddr=addr/BSIZE;
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+3, READ2S);
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+5, 00);
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+5, (actaddr)&255);
	urj_log (URJ_LOG_LEVEL_DEBUG, _("\nReading Address:%x"),(actaddr)&255);
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+5, (actaddr>>8));
	urj_log (URJ_LOG_LEVEL_DEBUG, _("%x\n"),(actaddr>>8));
	URJ_BUS_READ_START (bus, PSEUDOADDRESS);
	for(int i=1;i<ECCSIZE;i++){
		b[i-1] = URJ_BUS_READ_NEXT (bus, i);
	}
	b[ECCSIZE-1] = URJ_BUS_READ_END (bus);
	//urj_log (URJ_LOG_LEVEL_NORMAL, _("\n %x.\n"),b[0]);
	return 0;
}

uint32_t do_seqread(urj_bus_t *bus,uint32_t addr,uint8_t *b ){
	//TODO detect b too small
	uint32_t actaddr=addr/BSIZE;
	urj_log (URJ_LOG_LEVEL_DEBUG, _("\nACTADDR:%x"),(actaddr));
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+3, READ1S);
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+5, 00);
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+5, (actaddr));
	urj_log (URJ_LOG_LEVEL_NORMAL, _("\nReading Address:%x"),(actaddr));
	URJ_BUS_WRITE (bus, PSEUDOADDRESS+5, (actaddr)/256);
	urj_log (URJ_LOG_LEVEL_NORMAL, _("\nReading add2:%x\n"),(actaddr)/256);
	usleep(10);//unhardcode this
	if (URJ_BUS_READ_START (bus, PSEUDOADDRESS) !=URJ_STATUS_OK)
		return URJ_STATUS_FAIL;
	for (int j=0;j<PAGEINBLOCK;j++){ //with the chip we can sequentially read a whole block
		for(int i=1;i<BSIZE;i++){
			b[(i-1)+j*BSIZE] = URJ_BUS_READ_NEXT (bus, PSEUDOADDRESS);
		}
		usleep(READPAGESLEEP);
	}
	b[PAGEINBLOCK*BSIZE-1] = URJ_BUS_READ_END (bus);
	return 0;
}
