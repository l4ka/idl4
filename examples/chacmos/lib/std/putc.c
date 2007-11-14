#include <l4/l4.h>

#define DISPLAY ((char*)0xb8000 + 15*160)
#define COLOR 7
#define NUM_LINES 10
unsigned __cursor = 0;

void putc(char c)
{
    int i;

    switch(c) {
    case '\r':
        break;
    case '\n':
        __cursor += (160 - (__cursor % 160));
        break;
    case '\t':
	for ( i = 0; i < (int)(8 - (__cursor % 8)); i++ )
	{
	    DISPLAY[__cursor++] = ' ';
	    DISPLAY[__cursor++] = COLOR;
	}
        break;
    default:
        DISPLAY[__cursor++] = c;
        DISPLAY[__cursor++] = COLOR;
    }
    if ((__cursor / 160) == NUM_LINES) {
	for (i = 40; i < 40*NUM_LINES; i++)
	    ((dword_t*)DISPLAY)[i - 40] = ((dword_t*)DISPLAY)[i];
	for (i = 0; i < 40; i++)
	    ((dword_t*)DISPLAY)[40*(NUM_LINES-1) + i] = 0;
	__cursor -= 160;
    }
}

