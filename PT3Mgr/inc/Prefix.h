#ifndef _Prefix_H
#define _Prefix_H

// PUBLIC_LEVEL	�p�r
// 0			��ʌ��J�p
// 1			�g���H�ꌟ���p
// 2			FPGA ��H�X�V�p
// 3			�A�[�X�\�t�g�Г��p

#include <conio.h>
#include <crtdbg.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Prefix_Integer.h"

#define delete_(p)	{ delete p; p = NULL; }

#define BIT_SHIFT_MASK(value, shift, mask) (((value) >> (shift)) & ((1<<(mask))-1))

#endif
