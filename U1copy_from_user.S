/* SPDX-License-Identifier: GPL-2.0 */
/* U1copy_from_user.S: UltraSparc-I/II/IIi/IIe optimized copy from userspace.
 *
 * Copyright (C) 1999, 2000, 2004 David S. Miller (davem@redhat.com)
 */

#define EX_LD(x,y)		\
98:	x;			\
	.section __ex_table,"a";\
	.align 4;		\
	.word 98b, y;		\
	.text;			\
	.align 4;

#define EX_LD_FP(x,y)		\
98:	x;			\
	.section __ex_table,"a";\
	.align 4;		\
	.word 98b, y;		\
	.text;			\
	.align 4;

#define FUNC_NAME		U1copy_from_user
#define LOAD(type,addr,dest)	type## [addr], dest
#define LOAD_BLK(addr,dest)	ldda [addr] ASI_BLK_P, dest
#define EX_RETVAL(x)		0

#define PREAMBLE

#include "arch/sparc/lib/U1memcpy.S"
