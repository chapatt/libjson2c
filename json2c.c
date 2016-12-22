/*
 * Copyright (c) 2016 Chase Patterson
 */

#include "json2c.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>

static bool iscomplete(struct conf_schema *conf_schema);
static struct conf_element *search_schema (const char *js, const jsmntok_t *t,
	struct conf_schema *conf_schema);
static const jsmntok_t *install_val(const char *js, const jsmntok_t *t,
	struct conf_element *conf_elem);

/*
 * parse_tokens: traverse schema tree in parallel with jsmn tokens
 * installing values or dispatching them to user functions along the way.
 * Returns next jsmn token or NULL on error and sets json2cerrno appropriately.
 */ 
const jsmntok_t *parse_tokens(const char *js, const jsmntok_t *t,
	struct conf_schema *conf_schema)
{
	size_t children = t->size;

	++t;
	while (children > 0) {
		if (t->type != JSMN_STRING) {
			json2cerrno = JSON2C_EWRONGTYPE;
			return NULL;
		}
		struct conf_element *conf_elem =
			search_schema(js, t, conf_schema);
		if (conf_elem == NULL) {
			json2cerrno = JSON2C_ESTRANGEKEY;
			return NULL;
		}
		if ((t = install_val(js, ++t, conf_elem)) == NULL) {
			return NULL;
		}
		--children;
	}
	if (!iscomplete(conf_schema)) {
		json2cerrno = JSON2C_EINCOMPSCH;
		return NULL;
	}

	return t;
}

/*
 * search_schema: searches schema for key matching token
 * Returns pointer to conf_element if found, NULL if not
 */
static struct conf_element *search_schema(const char *js, const jsmntok_t *t,
	struct conf_schema *conf_schema)
{
	size_t i;
	int cmp;
	struct conf_element *conf_elem;

	for (i=0; i < conf_schema->size; ++i) {
		conf_elem = &conf_schema->conf_elems[i];
		json2cerrno = 0;
		if ((cmp = tokstrcmp(js, t, conf_elem->key)) == 0
			&& json2cerrno)
		{
			return NULL;
		}
		if (cmp == 0) {
			return conf_elem;
			break; /* we found it; don't search rest of schema */
		}
	} 
	if (cmp != 0 ) {
		return NULL;
	}
}
		

/*
 * iscomplete: check that required elements have been collected.
 * Returns true if they have been, false if not.
 */
static bool iscomplete(struct conf_schema *conf_schema)
{
	for (size_t i=0; i < conf_schema->size; ++i) {
		struct conf_element *conf_elem = &conf_schema->conf_elems[i];
		if (conf_elem->flags & REQUIRED
			&& !(conf_elem->flags & COLLECTED))
		{
			return false;
		}
	}

	return true;
}

/*
 * install_val: according to conf_elem, save value from jsmn token,
 * call parse_tokens, or call user function.
 * Returns next jsmn token or NULL on error and sets json2cerrno appropriately
 */
static const jsmntok_t *install_val(const char *js, const jsmntok_t *t,
	struct conf_element *conf_elem)
{
	int tmp_i;
	bool tmp_bool;
	char *tmp_char_p;

	if (conf_elem->flags & COLLECTED) {
		if (conf_elem->flags & NO_REPEAT) {
			json2cerrno = JSON2C_EREPEAT;
			return NULL;
		}
		if (!(conf_elem->flags & USE_LAST)) {
			return ++t;;
		}
	}

	if (conf_elem->val_p == NULL) {
		json2cerrno = JSON2C_ENULLELEM;
		return NULL;
	}

	switch (conf_elem->val_type) {
	case LEAF_INT:
		json2cerrno = 0;
		tmp_i = toktoi(js, t++);
		if (json2cerrno) {
			return NULL;
		}

		*(int *) conf_elem->val_p = tmp_i;
		break;
	case LEAF_BOOL:
		json2cerrno = 0;
		tmp_bool = toktobool(js, t++);
		if (json2cerrno) {
			return NULL;
		}

		*(bool *) conf_elem->val_p = tmp_bool;
		break;
	case LEAF_STRING:
		json2cerrno = 0;
		tmp_char_p = toktoa(js, t++);
		if (json2cerrno) {
			return NULL;
		}

		/* FIXME! free old pointer if malloc'd */
		*(char **) conf_elem->val_p = tmp_char_p;
		break;
	case NODE_SCHEMA:
		if ((t = parse_tokens(js, t,
			(struct conf_schema *) conf_elem->val_p))
			== NULL)
		{
			return NULL;
		}
		break;
	case NODE_FN:
		if ((t = (*conf_elem->fn_p)(js, t)) == NULL) {
			return NULL;
		}
		break;
	}
	conf_elem->flags |= COLLECTED;

	return t;
}

/*
 * tokstrcmp: strcmp wrapper for jsmn tokens
 * returns an integer less than, equal to, or greater than zero if t is found,
 * respectively, to be less than, to match, or be greater than s 
 * or 0 on error, setting json2cerrno appropriately
 */
int tokstrcmp(const char *js, const jsmntok_t *t, const char *s)
{
	char *buf;
	json2cerrno = 0;
	if ((buf = toktoa(js, t)) == NULL) {
		return 0;
	}

	int cmp = strcmp(buf, s);
	free(buf);

	return cmp;
}

/*
 * toktobool: takes json string and jsmntok_t pointing to a bool and
 * returns corresponding bool value. On error, returns false and sets json2cerrno.
 */
bool toktobool(const char *js, const jsmntok_t *t)
{
	if (t->type != JSMN_PRIMITIVE) {
		json2cerrno = JSON2C_EWRONGTYPE;
		return false;
	}

	if (tokstrcmp(js, t, "true") == 0) {
		return true;
	} else if (tokstrcmp(js, t, "false") == 0) {
		return false;
	} else {
		json2cerrno = JSON2C_EINVALDATA;
		return false;
	}
}

/*
 * toktoi: takes json string and jsmntok_t pointing to a number and
 * returns corresponding int value. On error, returns 0 and sets json2cerrno.
 */
int toktoi(const char *js, const jsmntok_t *t)
{
	if (t->type != JSMN_PRIMITIVE) {
		json2cerrno = JSON2C_EWRONGTYPE;
		return 0;
	}

	char c;
	if(!isdigit(c = *(js+t->start)) &&  c != '-') {
		json2cerrno = JSON2C_EINVALDATA;
		return 0;
	}
	for (size_t i = t->start + 1; i < t->end; ++i) {
		if(!isdigit(js[i])) {
			json2cerrno = JSON2C_EINVALDATA;
			return 0;
		}
	}

	errno = 0;
	long l = strtol(js+t->start, NULL, 0);
	if (errno) {
		json2cerrno = JSON2C_ERANGE;
		return 0;
	}
	if (l > INT_MAX) {
		json2cerrno = JSON2C_ERANGE;
		return 0;
	}

	return (int) l;
}

/*
 * toktoa: allocate space for token t and copy it in with null-terminator.
 * Return pointer to free()-able string or NULL on error, setting json2cerrno.
 */
char *toktoa(const char *js, const jsmntok_t *t)
{
	size_t tok_len = (t->end - t->start);

	char *buf;
	if ((buf = malloc((tok_len + 1) * sizeof(*buf))) == NULL) {
		json2cerrno = JSON2C_ENOMEM;
		return NULL;
	}

	strncpy(buf, js+t->start, tok_len);
	buf[tok_len] = '\0';

	return buf;
}
