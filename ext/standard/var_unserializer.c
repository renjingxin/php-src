/* Generated by re2c 0.9.4 on Fri Sep 24 23:45:23 2004 */
#line 1 "/usr/src/PHP_4_3_0/ext/standard/var_unserializer.re"
/*
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2003 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Sascha Schumann <sascha@schumann.cx>                        |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php.h"
#include "ext/standard/php_var.h"
#include "php_incomplete_class.h"

/* {{{ reference-handling for unserializer: var_* */
#define VAR_ENTRIES_MAX 1024

typedef struct {
	zval *data[VAR_ENTRIES_MAX];
	int used_slots;
	void *next;
} var_entries;

static inline void var_push(php_unserialize_data_t *var_hashx, zval **rval)
{
	var_entries *var_hash = var_hashx->first, *prev = NULL;

	while (var_hash && var_hash->used_slots == VAR_ENTRIES_MAX) {
		prev = var_hash;
		var_hash = var_hash->next;
	}

	if (!var_hash) {
		var_hash = emalloc(sizeof(var_entries));
		var_hash->used_slots = 0;
		var_hash->next = 0;

		if (!var_hashx->first)
			var_hashx->first = var_hash;
		else
			prev->next = var_hash;
	}

	var_hash->data[var_hash->used_slots++] = *rval;
}

PHPAPI void var_replace(php_unserialize_data_t *var_hashx, zval *ozval, zval **nzval)
{
	int i;
	var_entries *var_hash = var_hashx->first;
	
	while (var_hash) {
		for (i = 0; i < var_hash->used_slots; i++) {
			if (var_hash->data[i] == ozval) {
				var_hash->data[i] = *nzval;
				return;
			}
		}
		var_hash = var_hash->next;
	}
}

static int var_access(php_unserialize_data_t *var_hashx, int id, zval ***store)
{
	var_entries *var_hash = var_hashx->first;
	
	while (id >= VAR_ENTRIES_MAX && var_hash && var_hash->used_slots == VAR_ENTRIES_MAX) {
		var_hash = var_hash->next;
		id -= VAR_ENTRIES_MAX;
	}

	if (!var_hash) return !SUCCESS;

	if (id >= var_hash->used_slots) return !SUCCESS;

	*store = &var_hash->data[id];

	return SUCCESS;
}

PHPAPI void var_destroy(php_unserialize_data_t *var_hashx)
{
	void *next;
	var_entries *var_hash = var_hashx->first;
	
	while (var_hash) {
		next = var_hash->next;
		efree(var_hash);
		var_hash = next;
	}
}

/* }}} */

#define YYFILL(n) do { } while (0)
#define YYCTYPE unsigned char
#define YYCURSOR cursor
#define YYLIMIT limit
#define YYMARKER marker


#line 118 "/usr/src/PHP_4_3_0/ext/standard/var_unserializer.re"




static inline int parse_iv2(const unsigned char *p, const unsigned char **q)
{
	char cursor;
	int result = 0;
	int neg = 0;

	switch (*p) {
		case '-':
			neg++;
			/* fall-through */
		case '+':
			p++;
	}
	
	while (1) {
		cursor = (char)*p;
		if (cursor >= '0' && cursor <= '9') {
			result = result * 10 + cursor - '0';
		} else {
			break;
		}
		p++;
	}
	if (q) *q = p;
	if (neg) return -result;
	return result;
}

static inline int parse_iv(const unsigned char *p)
{
	return parse_iv2(p, NULL);
}

/* no need to check for length - re2c already did */
static inline size_t parse_uiv(const unsigned char *p)
{
	unsigned char cursor;
	size_t result = 0;

	if (*p == '+') {
		p++;
	}
	
	while (1) {
		cursor = *p;
		if (cursor >= '0' && cursor <= '9') {
			result = result * 10 + (size_t)(cursor - (unsigned char)'0');
		} else {
			break;
		}
		p++;
	}
	return result;
}

#define UNSERIALIZE_PARAMETER zval **rval, const unsigned char **p, const unsigned char *max, php_unserialize_data_t *var_hash TSRMLS_DC
#define UNSERIALIZE_PASSTHRU rval, p, max, var_hash TSRMLS_CC

static inline int process_nested_data(UNSERIALIZE_PARAMETER, HashTable *ht, int elements)
{
	while (elements-- > 0) {
		zval *key, *data;

		ALLOC_INIT_ZVAL(key);

		if (!php_var_unserialize(&key, p, max, NULL TSRMLS_CC)) {
			zval_dtor(key);
			FREE_ZVAL(key);
			return 0;
		}

		if (Z_TYPE_P(key) != IS_LONG && Z_TYPE_P(key) != IS_STRING) {
			zval_dtor(key);
			FREE_ZVAL(key);
			return 0;
		}

		ALLOC_INIT_ZVAL(data);

		if (!php_var_unserialize(&data, p, max, var_hash TSRMLS_CC)) {
			zval_dtor(key);
			FREE_ZVAL(key);
			zval_dtor(data);
			FREE_ZVAL(data);
			return 0;
		}

		switch (Z_TYPE_P(key)) {
			case IS_LONG:
				zend_hash_index_update(ht, Z_LVAL_P(key), &data, sizeof(data), NULL);
				break;
			case IS_STRING:
				zend_hash_update(ht, Z_STRVAL_P(key), Z_STRLEN_P(key) + 1, &data, sizeof(data), NULL);
				break;
		}
		
		zval_dtor(key);
		FREE_ZVAL(key);

		if (elements && *(*p-1) != ';' &&  *(*p-1) != '}') {
			(*p)--;
			return 0;
		}
	}

	return 1;
}

static inline int finish_nested_data(UNSERIALIZE_PARAMETER)
{
	if (*((*p)++) == '}') 
		return 1;

#if SOMETHING_NEW_MIGHT_LEAD_TO_CRASH_ENABLE_IF_YOU_ARE_BRAVE
	zval_ptr_dtor(rval);
#endif
	return 0;
}

static inline int object_common1(UNSERIALIZE_PARAMETER, zend_class_entry *ce)
{
	int elements;

	elements = parse_iv2((*p) + 2, p);

	(*p) += 2;
	
	object_init_ex(*rval, ce);
	return elements;
}

static inline int object_common2(UNSERIALIZE_PARAMETER, int elements)
{
	zval *retval_ptr = NULL;
	zval fname;

	if (!process_nested_data(UNSERIALIZE_PASSTHRU, Z_OBJPROP_PP(rval), elements)) {
		return 0;
	}

	INIT_PZVAL(&fname);
	ZVAL_STRINGL(&fname, "__wakeup", sizeof("__wakeup") - 1, 0);
	call_user_function_ex(CG(function_table), rval, &fname, &retval_ptr, 0, 0, 1, NULL TSRMLS_CC);

	if (retval_ptr)
		zval_ptr_dtor(&retval_ptr);

	return finish_nested_data(UNSERIALIZE_PASSTHRU);

}

static char *str_tolower_copy(char *dest, const char *source, unsigned int length)
{
	register unsigned char *str = (unsigned char*)source;
	register unsigned char *result = (unsigned char*)dest;
	register unsigned char *end = str + length;

	while (str < end) {
		*result++ = tolower((int)*str++);
	}
	*result = *end;

	return dest;
}

PHPAPI int php_var_unserialize(UNSERIALIZE_PARAMETER)
{
	const unsigned char *cursor, *limit, *marker, *start;
	zval **rval_ref;

	limit = cursor = *p;
	
	if (var_hash && cursor[0] != 'R') {
		var_push(var_hash, rval);
	}

	start = cursor;

	
	

#line 7 "<stdout>"
{
	YYCTYPE yych;
	unsigned int yyaccept;
	static unsigned char yybm[] = {
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	};
	goto yy0;
yy1:	++YYCURSOR;
yy0:
	if((YYLIMIT - YYCURSOR) < 7) YYFILL(7);
	yych = *YYCURSOR;
	if(yych <= 'd'){
		if(yych <= 'R'){
			if(yych <= 'N'){
				if(yych <= 'M')	goto yy15;
				goto yy5;
			} else {
				if(yych <= 'O')	goto yy12;
				if(yych <= 'Q')	goto yy15;
				goto yy3;
			}
		} else {
			if(yych <= 'a'){
				if(yych <= '`')	goto yy15;
				goto yy10;
			} else {
				if(yych <= 'b')	goto yy6;
				if(yych <= 'c')	goto yy15;
				goto yy8;
			}
		}
	} else {
		if(yych <= 'r'){
			if(yych <= 'i'){
				if(yych <= 'h')	goto yy15;
				goto yy7;
			} else {
				if(yych == 'o')	goto yy11;
				goto yy15;
			}
		} else {
			if(yych <= '|'){
				if(yych <= 's')	goto yy9;
				goto yy15;
			} else {
				if(yych <= '}')	goto yy13;
				if(yych <= 0xBF)	goto yy15;
				goto yy2;
			}
		}
	}
yy2:	YYCURSOR = YYMARKER;
	switch(yyaccept){
	case 0:	goto yy4;
	}
yy3:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy80;
	goto yy4;
yy4:
#line 511 "/usr/src/PHP_4_3_0/ext/standard/var_unserializer.re"
{ return 0; }
#line 101 "<stdout>"
yy5:	yych = *++YYCURSOR;
	if(yych == ';')	goto yy78;
	goto yy4;
yy6:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy74;
	goto yy4;
yy7:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy68;
	goto yy4;
yy8:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy44;
	goto yy4;
yy9:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy37;
	goto yy4;
yy10:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy30;
	goto yy4;
yy11:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy23;
	goto yy4;
yy12:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy16;
	goto yy4;
yy13:	++YYCURSOR;
	goto yy14;
yy14:
#line 505 "/usr/src/PHP_4_3_0/ext/standard/var_unserializer.re"
{
	/* this is the case where we have less data than planned */
	php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Unexpected end of serialized data");
	return 0; /* not sure if it should be 0 or 1 here? */
}
#line 142 "<stdout>"
yy15:	yych = *++YYCURSOR;
	goto yy4;
yy16:	yych = *++YYCURSOR;
	if(yybm[0+yych] & 128)	goto yy18;
	if(yych != '+')	goto yy2;
	goto yy17;
yy17:	yych = *++YYCURSOR;
	if(yybm[0+yych] & 128)	goto yy18;
	goto yy2;
yy18:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy19;
yy19:	if(yybm[0+yych] & 128)	goto yy18;
	if(yych != ':')	goto yy2;
	goto yy20;
yy20:	yych = *++YYCURSOR;
	if(yych != '"')	goto yy2;
	goto yy21;
yy21:	++YYCURSOR;
	goto yy22;
yy22:
#line 424 "/usr/src/PHP_4_3_0/ext/standard/var_unserializer.re"
{
	size_t len, len2, maxlen;
	int elements;
	char *class_name;
	zend_class_entry *ce;
	int incomplete_class = 0;
	
	zval *user_func;
	zval *retval_ptr;
	zval **args[1];
	zval *arg_func_name;
	
	INIT_PZVAL(*rval);
	len2 = len = parse_uiv(start + 2);
	maxlen = max - YYCURSOR;
	if (maxlen < len || len == 0) {
		*p = start + 2;
		return 0;
	}

	class_name = (char*)YYCURSOR;

	YYCURSOR += len;

	if (*(YYCURSOR) != '"') {
		*p = YYCURSOR;
		return 0;
	}
	if (*(YYCURSOR+1) != ':') {
		*p = YYCURSOR+1;
		return 0;
	}
	
	class_name = str_tolower_copy((char *)emalloc(len+1), class_name, len);
	class_name[len] = '\0';
	
	if (zend_hash_find(CG(class_table), class_name, len + 1, (void **) &ce) != SUCCESS) {
		if ((PG(unserialize_callback_func) == NULL) || (PG(unserialize_callback_func)[0] == '\0')) {
			incomplete_class = 1;
			ce = PHP_IC_ENTRY;
		} else {
			MAKE_STD_ZVAL(user_func);
			ZVAL_STRING(user_func, PG(unserialize_callback_func), 1);

			args[0] = &arg_func_name;
			MAKE_STD_ZVAL(arg_func_name);
			ZVAL_STRING(arg_func_name, class_name, 1);
				
			if (call_user_function_ex(CG(function_table), NULL, user_func, &retval_ptr, 1, args, 0, NULL TSRMLS_CC) != SUCCESS) {
				zend_error(E_WARNING, "'unserialize_callback_func' defined (%s) but not found", user_func->value.str.val);
				incomplete_class = 1;
				ce = PHP_IC_ENTRY;
			} else {
				if (zend_hash_find(CG(class_table), class_name, len + 1, (void **) &ce) != SUCCESS) {
					zend_error(E_WARNING, "'unserialize_callback_func' (%s) hasn't defined the class it was called for", user_func->value.str.val);
					incomplete_class = 1;
					ce = PHP_IC_ENTRY;
				} else {
#ifdef ZEND_ENGINE_2
					ce = *(zend_class_entry **)ce; /* Bad hack, TBF! */
#endif	
				}
			}
		}
	} else {
#ifdef ZEND_ENGINE_2
		ce = *(zend_class_entry **)ce; /* Bad hack, TBF! */
#endif	
	}

	*p = YYCURSOR;
	elements = object_common1(UNSERIALIZE_PASSTHRU, ce);

	if (incomplete_class) {
		php_store_class_name(*rval, class_name, len2 TSRMLS_CC);
	}
	efree(class_name);

	return object_common2(UNSERIALIZE_PASSTHRU, elements);
}
#line 247 "<stdout>"
yy23:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
		goto yy24;
	} else {
		if(yych <= '-')	goto yy24;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy25;
		goto yy2;
	}
yy24:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
	goto yy25;
yy25:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy26;
yy26:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy25;
	if(yych >= ';')	goto yy2;
	goto yy27;
yy27:	yych = *++YYCURSOR;
	if(yych != '"')	goto yy2;
	goto yy28;
yy28:	++YYCURSOR;
	goto yy29;
yy29:
#line 416 "/usr/src/PHP_4_3_0/ext/standard/var_unserializer.re"
{

	INIT_PZVAL(*rval);
	
	return object_common2(UNSERIALIZE_PASSTHRU,
			object_common1(UNSERIALIZE_PASSTHRU, ZEND_STANDARD_CLASS_DEF_PTR));
}
#line 285 "<stdout>"
yy30:	yych = *++YYCURSOR;
	if(yych == '+')	goto yy31;
	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy32;
	goto yy2;
yy31:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
	goto yy32;
yy32:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy33;
yy33:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy32;
	if(yych >= ';')	goto yy2;
	goto yy34;
yy34:	yych = *++YYCURSOR;
	if(yych != '{')	goto yy2;
	goto yy35;
yy35:	++YYCURSOR;
	goto yy36;
yy36:
#line 398 "/usr/src/PHP_4_3_0/ext/standard/var_unserializer.re"
{
	int elements = parse_iv(start + 2);

	*p = YYCURSOR;

	INIT_PZVAL(*rval);
	Z_TYPE_PP(rval) = IS_ARRAY;
	ALLOC_HASHTABLE(Z_ARRVAL_PP(rval));

	zend_hash_init(Z_ARRVAL_PP(rval), elements + 1, NULL, ZVAL_PTR_DTOR, 0);

	if (!process_nested_data(UNSERIALIZE_PASSTHRU, Z_ARRVAL_PP(rval), elements)) {
		return 0;
	}

	return finish_nested_data(UNSERIALIZE_PASSTHRU);
}
#line 328 "<stdout>"
yy37:	yych = *++YYCURSOR;
	if(yych == '+')	goto yy38;
	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy39;
	goto yy2;
yy38:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
	goto yy39;
yy39:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy40;
yy40:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy39;
	if(yych >= ';')	goto yy2;
	goto yy41;
yy41:	yych = *++YYCURSOR;
	if(yych != '"')	goto yy2;
	goto yy42;
yy42:	++YYCURSOR;
	goto yy43;
yy43:
#line 370 "/usr/src/PHP_4_3_0/ext/standard/var_unserializer.re"
{
	size_t len, maxlen;
	char *str;

	len = parse_uiv(start + 2);
	maxlen = max - YYCURSOR;
	if (maxlen < len) {
		*p = start + 2;
		return 0;
	}

	str = (char*)YYCURSOR;

	YYCURSOR += len;

	if (*(YYCURSOR) != '"') {
		*p = YYCURSOR;
		return 0;
	}

	YYCURSOR += 2;
	*p = YYCURSOR;

	INIT_PZVAL(*rval);
	ZVAL_STRINGL(*rval, str, len, 1);
	return 1;
}
#line 381 "<stdout>"
yy44:	yych = *++YYCURSOR;
	if(yych <= '/'){
		if(yych <= ','){
			if(yych == '+')	goto yy48;
			goto yy2;
		} else {
			if(yych <= '-')	goto yy46;
			if(yych <= '.')	goto yy51;
			goto yy2;
		}
	} else {
		if(yych <= 'I'){
			if(yych <= '9')	goto yy49;
			if(yych <= 'H')	goto yy2;
			goto yy47;
		} else {
			if(yych != 'N')	goto yy2;
			goto yy45;
		}
	}
yy45:	yych = *++YYCURSOR;
	if(yych == 'A')	goto yy67;
	goto yy2;
yy46:	yych = *++YYCURSOR;
	if(yych <= '/'){
		if(yych == '.')	goto yy51;
		goto yy2;
	} else {
		if(yych <= '9')	goto yy49;
		if(yych != 'I')	goto yy2;
		goto yy47;
	}
yy47:	yych = *++YYCURSOR;
	if(yych == 'N')	goto yy63;
	goto yy2;
yy48:	yych = *++YYCURSOR;
	if(yych == '.')	goto yy51;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
	goto yy49;
yy49:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy50;
yy50:	if(yych <= ':'){
		if(yych <= '.'){
			if(yych <= '-')	goto yy2;
			goto yy61;
		} else {
			if(yych <= '/')	goto yy2;
			if(yych <= '9')	goto yy49;
			goto yy2;
		}
	} else {
		if(yych <= 'E'){
			if(yych <= ';')	goto yy54;
			if(yych <= 'D')	goto yy2;
			goto yy56;
		} else {
			if(yych == 'e')	goto yy56;
			goto yy2;
		}
	}
yy51:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
	goto yy52;
yy52:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy53;
yy53:	if(yych <= ';'){
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy52;
		if(yych <= ':')	goto yy2;
		goto yy54;
	} else {
		if(yych <= 'E'){
			if(yych <= 'D')	goto yy2;
			goto yy56;
		} else {
			if(yych == 'e')	goto yy56;
			goto yy2;
		}
	}
yy54:	++YYCURSOR;
	goto yy55;
yy55:
#line 363 "/usr/src/PHP_4_3_0/ext/standard/var_unserializer.re"
{
	*p = YYCURSOR;
	INIT_PZVAL(*rval);
	ZVAL_DOUBLE(*rval, atof(start + 2));
	return 1;
}
#line 479 "<stdout>"
yy56:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
		goto yy57;
	} else {
		if(yych <= '-')	goto yy57;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy58;
		goto yy2;
	}
yy57:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych == '+')	goto yy60;
		goto yy2;
	} else {
		if(yych <= '-')	goto yy60;
		if(yych <= '/')	goto yy2;
		if(yych >= ':')	goto yy2;
		goto yy58;
	}
yy58:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy59;
yy59:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy58;
	if(yych == ';')	goto yy54;
	goto yy2;
yy60:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy58;
	goto yy2;
yy61:	++YYCURSOR;
	if((YYLIMIT - YYCURSOR) < 4) YYFILL(4);
	yych = *YYCURSOR;
	goto yy62;
yy62:	if(yych <= ';'){
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy61;
		if(yych <= ':')	goto yy2;
		goto yy54;
	} else {
		if(yych <= 'E'){
			if(yych <= 'D')	goto yy2;
			goto yy56;
		} else {
			if(yych == 'e')	goto yy56;
			goto yy2;
		}
	}
yy63:	yych = *++YYCURSOR;
	if(yych != 'F')	goto yy2;
	goto yy64;
yy64:	yych = *++YYCURSOR;
	if(yych != ';')	goto yy2;
	goto yy65;
yy65:	++YYCURSOR;
	goto yy66;
yy66:
#line 346 "/usr/src/PHP_4_3_0/ext/standard/var_unserializer.re"
{
	*p = YYCURSOR;
	INIT_PZVAL(*rval);
#if defined(HAVE_ATOF_ACCEPTS_NAN) && defined(HAVE_ATOF_ACCEPTS_INF)
	ZVAL_DOUBLE(*rval, atof(start + 2));
#else
	if (!strncmp(start + 2, "NAN", 3)) {
		ZVAL_DOUBLE(*rval, php_get_nan());
	} else if (!strncmp(start + 2, "INF", 3)) {
		ZVAL_DOUBLE(*rval, php_get_inf());
	} else if (!strncmp(start + 2, "-INF", 4)) {
		ZVAL_DOUBLE(*rval, -php_get_inf());
	}
#endif
	return 1;
}
#line 558 "<stdout>"
yy67:	yych = *++YYCURSOR;
	if(yych == 'N')	goto yy64;
	goto yy2;
yy68:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
		goto yy69;
	} else {
		if(yych <= '-')	goto yy69;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy70;
		goto yy2;
	}
yy69:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
	goto yy70;
yy70:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy71;
yy71:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy70;
	if(yych != ';')	goto yy2;
	goto yy72;
yy72:	++YYCURSOR;
	goto yy73;
yy73:
#line 339 "/usr/src/PHP_4_3_0/ext/standard/var_unserializer.re"
{
	*p = YYCURSOR;
	INIT_PZVAL(*rval);
	ZVAL_LONG(*rval, parse_iv(start + 2));
	return 1;
}
#line 595 "<stdout>"
yy74:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= '2')	goto yy2;
	goto yy75;
yy75:	yych = *++YYCURSOR;
	if(yych != ';')	goto yy2;
	goto yy76;
yy76:	++YYCURSOR;
	goto yy77;
yy77:
#line 332 "/usr/src/PHP_4_3_0/ext/standard/var_unserializer.re"
{
	*p = YYCURSOR;
	INIT_PZVAL(*rval);
	ZVAL_BOOL(*rval, parse_iv(start + 2));
	return 1;
}
#line 613 "<stdout>"
yy78:	++YYCURSOR;
	goto yy79;
yy79:
#line 325 "/usr/src/PHP_4_3_0/ext/standard/var_unserializer.re"
{
	*p = YYCURSOR;
	INIT_PZVAL(*rval);
	ZVAL_NULL(*rval);
	return 1;
}
#line 624 "<stdout>"
yy80:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
		goto yy81;
	} else {
		if(yych <= '-')	goto yy81;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy82;
		goto yy2;
	}
yy81:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
	goto yy82;
yy82:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy83;
yy83:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy82;
	if(yych != ';')	goto yy2;
	goto yy84;
yy84:	++YYCURSOR;
	goto yy85;
yy85:
#line 304 "/usr/src/PHP_4_3_0/ext/standard/var_unserializer.re"
{
	int id;

 	*p = YYCURSOR;
	if (!var_hash) return 0;

	id = parse_iv(start + 2) - 1;
	if (id == -1 || var_access(var_hash, id, &rval_ref) != SUCCESS) {
		return 0;
	}

	if (*rval != NULL) {
	zval_ptr_dtor(rval);
	}
	*rval = *rval_ref;
	(*rval)->refcount++;
	(*rval)->is_ref = 1;
	
	return 1;
}
#line 672 "<stdout>"
}
#line 513 "/usr/src/PHP_4_3_0/ext/standard/var_unserializer.re"


	return 0;
}
