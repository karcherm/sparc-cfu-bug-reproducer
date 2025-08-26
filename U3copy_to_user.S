/* SPDX-License-Identifier: GPL-2.0 */
/* U3copy_to_user.S: UltraSparc-III optimized copy to userspace.
 *
 * Copyright (C) 1999, 2000, 2004 David S. Miller (davem@redhat.com)
 */

#define EX_ST(x,y)		\
98:	x;			\
	.section __ex_table,"a";\
	.align 4;		\
	.word 98b, y;		\
	.text;			\
	.align 4;

#define EX_ST_FP(x,y)		\
98:	x;			\
	.section __ex_table,"a";\
	.align 4;		\
	.word 98b, y##_fp;	\
	.text;			\
	.align 4;

#define FUNC_NAME		U3copy_to_user
#define STORE(type,src,addr)	type src, [addr]
#define STORE_BLK(src,addr)	stda src, [addr] ASI_BLK_P
#define EX_RETVAL(x)		0

#include <arch/sparc/lib/U3memcpy.S>
