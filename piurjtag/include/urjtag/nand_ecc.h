/*
 *  drivers/mtd/nand_ecc.h
 *
 *  Copyright (C) 2000 Steven J. Hill (sjhill@cotw.com)
 *
 * $Id: nand_ecc.h,v 1.1 2000/10/12 00:57:15 sjhill Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This file is the header for the ECC algorithm.
 */

/*
 * Calculate 3 byte ECC code for 256 byte block
 */
int nand_calculate_ecc (const unsigned char *dat, unsigned char *ecc_code);

/*
 * Detect and correct a 1 bit error for 256 byte block
 */
int nand_correct_data (unsigned char *dat, unsigned char *read_ecc, unsigned char *calc_ecc);
