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

#ifndef IDENTIFY_ENCODING_H
#define IDENTIFY_ENCODING_H

#define IDENTIFY_MAX_LENGTH 256

enum identify_encoding_order {
	ieo_EUCJP = 0,
	ieo_SJIS  = 1
};

typedef struct {
	int flag;
	int state;
	int c_type;
} ie_state_t;

typedef struct {
	enum identify_encoding_order order;
	ie_state_t  st_ascii;
	ie_state_t  st_jis;
	ie_state_t  st_sjis;
	ie_state_t  st_eucjp;
	ie_state_t  st_utf8;
} identify_encoding_t;

identify_encoding_t* identify_encoding_open(enum identify_encoding_order order);
void identify_encoding_close(identify_encoding_t* cd);
void identify_encoding_reset(identify_encoding_t* cd);
const char *identify_encoding(identify_encoding_t *cd, char* instr);

#endif
