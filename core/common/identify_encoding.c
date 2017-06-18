/*
 *
 * 漢字コードの判別し、iconv 用の文字エンコーディング文字列を返す
 *
 * 2001/10/24  Remove static variables
 *             Kazuhiko Iwama <iwama@ymc.ne.jp>
 * 2001/10/14  First version
 *             Kazuhiko Iwama <iwama@ymc.ne.jp>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "identify_encoding.h"

enum encoding_id
{
	eid_KNOWN = 0,
	eid_ASCII = 1,
	eid_JIS   = 2,
	eid_SJIS  = 3,
	eid_EUCJP = 4,
	eid_UTF8  = 5
};

const char *encoding_str[] =
{
	"KNOWN", "ASCII", "ISO-2022-JP", "SJIS", "EUC-JP", "UTF-8"
};

static int
is_ascii(ie_state_t *st, const unsigned char *p)
{
	if (!(isprint(*p) || isspace(*p)) || *p >= 0x80)
		st->flag = 0;
	return st->flag;
}

static int
is_jis(ie_state_t *st, const unsigned char *p)
{
	if (*p >= 0x80)
		st->flag = 0;
    return st->flag;
}

static int
is_sjis(ie_state_t *st, const unsigned char *p)
{
	switch (st->state)
	{
		case 0:
			st->c_type = 0;
			if (*p >= 0x80)
			{
				st->state = 1;
				if		(*p >= 0x81 && *p <= 0x9f)	st->c_type = 1;
				else if (*p >= 0xe0 && *p <= 0xef)	st->c_type = 1;
				else if (*p >= 0xa1 && *p <= 0xdf)	st->state = 0;
				else								st->flag = 1;
			}
			break;
		case 1:
			if		(*p >= 0x40 && *p <= 0x7e)	st->state = 0;
			else if (*p >= 0x80 && *p <= 0xfc)	st->state = 0;
			else								st->flag = 0;
			break;
		default:
			st->flag = 0;
			break;
	}

	return st->flag;
}

static int
is_eucjp(ie_state_t *st, const unsigned char *p)
{
	switch (st->state)
	{
		case 0:
			st->c_type = 0;
			if (*p >= 0x80)
			{
				st->state = 1;
				if		(*p >= 0xa1 && *p <= 0xfe)  st->c_type = 1;
				else if (*p == 0x8e              )  st->c_type = 2;
				else if (*p == 0x8f              )  st->c_type = 3;
			    else								st->flag = 0;
			}
			break;
		case 1:
			switch (st->c_type)
			{
				case 1:
					if (*p >= 0xa1 && *p <= 0xfe)  st->state = 0;
					else                           st->flag = 0;
					break;
				case 2:
					if (*p >= 0x81 && *p <= 0xff)  st->state = 0;
					else                           st->flag = 0;
					break;
				case 3:
					if (*p >= 0x81 && *p <= 0xff)  st->state = 2;
					else                           st->flag = 0;
					break;
				default:
					st->flag = 0;
					break;
			}
			break;
		case 2:
			if (*p >= 0x81 && *p <= 0xff)  st->state = 0;
			else                           st->flag = 0;
		default:
			st->flag = 0;
			break;
	}

	return st->flag;
}

static int
is_utf8(ie_state_t *st, const unsigned char *p)
{
    switch (st->state)
	{
		case 0:
			st->c_type = 0;
			if (*p >= 0x80)
			{
				st->state = 1;
				if      (*p >= 0xc2 && *p <= 0xdf)  st->c_type = 1;
				else if (*p == 0xe0              )  st->c_type = 2;
				else if (*p >= 0xe1 && *p <= 0xef)  st->c_type = 3;
				else if (*p == 0xf0              )  st->c_type = 4;
				else if (*p >= 0xf1 && *p <= 0xf3)  st->c_type = 5;
				else if (*p == 0xf4              )  st->c_type = 6;
				else								st->flag = 0;
			}
			break;
		case 1:
			switch (st->c_type)
			{
				case 1:
					if (*p >= 0x80 && *p <= 0xbf)  st->state = 0;
					else                           st->flag = 0;
					break;
				case 2:
					if (*p >= 0xa0 && *p <= 0xbf)  st->state = 2;
					else                           st->flag = 0;
					break;
				case 3:
					if (*p >= 0x80 && *p <= 0xbf)  st->state = 2;
					else                           st->flag = 0;
					break;
				case 4:
					if (*p >= 0x90 && *p <= 0xbf)  st->state = 2;
					else                           st->flag = 0;
					break;
				case 5:
					if (*p >= 0x80 && *p <= 0xbf)  st->state = 2;
					else                           st->flag = 0;
					break;
				case 6:
					if (*p >= 0x80 && *p <= 0x8f)  st->state = 2;
					else                           st->flag = 0;
					break;
				default:
					st->flag = 0;
					break;
			}
			break;
		case 2:
			if (st->c_type >= 2 && st->c_type <= 3)
			{
				if (*p >= 0x80 && *p <= 0xbf)  st->state = 0;
				else                           st->flag = 0;
			}else if (st->c_type >= 4 && st->c_type <= 6)
			{
				if (*p >= 0x80 && *p <= 0xbf)  st->state = 3;
				else                           st->flag = 0;
			}else{
				st->flag = 0;
			}
			break;
		case 3:
			if (st->c_type >= 4 && st->c_type <= 6)
			{
				if (*p >= 0x80 && *p <= 0xbf)  st->state = 0;
				else                           st->flag = 0;
			}else{
				st->flag = 0;
			}
			break;
		default:
			st->flag = 0;
			break;
	}

	return st->flag;
}

identify_encoding_t*
identify_encoding_open(enum identify_encoding_order order)
{
	identify_encoding_t* cd;
	cd = (identify_encoding_t*)malloc(sizeof(identify_encoding_t));

	if (cd == NULL)
	{
		cd = (identify_encoding_t*)(-1);
    }else{
		cd->order = order;
		identify_encoding_reset(cd);
	}
	return cd;
}

void
identify_encoding_close(identify_encoding_t* cd)
{
	if (cd != (identify_encoding_t*)(-1) || cd != NULL)
	{
		free(cd);
    }
}

static void
identify_encoding_reset_state(ie_state_t* st)
{
	st->flag = 1;
	st->state = 0;
	st->c_type = 0;
}

void
identify_encoding_reset(identify_encoding_t* cd)
{
	identify_encoding_reset_state(&(cd->st_ascii));
	identify_encoding_reset_state(&(cd->st_jis));
	identify_encoding_reset_state(&(cd->st_sjis));
	identify_encoding_reset_state(&(cd->st_eucjp));
	identify_encoding_reset_state(&(cd->st_utf8));
}

const char*
identify_encoding(identify_encoding_t *cd, char* instr)
{
	int n;
	unsigned char *p;
	enum encoding_id eid = eid_KNOWN;

	identify_encoding_reset(cd);

	for (n = 0, p = (unsigned char *)instr; *p != '\0' && n < IDENTIFY_MAX_LENGTH; p++, n++)
	{
		if (cd->st_ascii.flag == 1)  is_ascii(&(cd->st_ascii), p);
		if (cd->st_jis.flag   == 1)  is_jis(&(cd->st_jis), p);
		if (cd->st_sjis.flag  == 1)  is_sjis(&(cd->st_sjis), p);
		if (cd->st_eucjp.flag == 1)  is_eucjp(&(cd->st_eucjp), p);
		if (cd->st_utf8.flag  == 1)  is_utf8(&(cd->st_utf8), p);
    }

	if      (cd->st_ascii.flag == 1)  eid = eid_ASCII;
	else if (cd->st_jis.flag   == 1)  eid = eid_JIS;
	else if (cd->st_utf8.flag  == 1)  eid = eid_UTF8;
	else if (cd->order == ieo_EUCJP)
	{
		if      (cd->st_eucjp.flag == 1)  eid = eid_EUCJP;
		else if (cd->st_sjis.flag  == 1)  eid = eid_SJIS;
    }else{
		if      (cd->st_sjis.flag  == 1)  eid = eid_SJIS;
		else if (cd->st_eucjp.flag == 1)  eid = eid_EUCJP;
	}

	return encoding_str[ eid ];
}
