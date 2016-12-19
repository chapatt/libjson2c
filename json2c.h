#ifndef LIBJSON2C_H
#define LIBJSON2C_H

#include <stdbool.h>
#include <stddef.h>
#include "../jsmn/jsmn.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))
#endif

#ifndef ARR_ARRAY_SIZE
/* take array x and return object with members x and no. elements in x */
#define ARR_ARRAY_SIZE(x) {x, ARRAY_SIZE(x)}
#endif

enum json2cerrnos {
	JSON2C_EWRONGTYPE,	/* token jsmntype_t doesn't match schema */
	JSON2C_EINVALDATA,	/* value not valid type specified in schema */
	JSON2C_ESTRANGEKEY,	/* got key not found in schema */
	JSON2C_EINCOMPSCH,	/* required keys not collected */
	JSON2C_EREPEAT,		/* key flagged NO_REPEAT repeated */
	JSON2C_ERANGE,		/* numeric value can't be represented */
	JSON2C_ENOMEM		/* couldn't allocate memory */
} json2cerrno;

enum conf_elem_type {
	LEAF_INT,
	LEAF_BOOL,
	LEAF_STRING,
	NODE_SCHEMA,
	NODE_FN
};

enum conf_elem_flag {
	REQUIRED =	001,	/* key is required */
	COLLECTED =	002,	/* key has been found */
	USE_LAST =	010,	/* later occurences of key replace prev. val */
	NO_REPEAT =	020	/* return error on repeated key */
};

struct conf_element {
	char *key;
	enum conf_elem_flag flags;
	enum conf_elem_type val_type;
	void *val_p;
	const jsmntok_t *(*fn_p)(const char *js, const jsmntok_t *tok);
};

struct conf_schema {
	struct conf_element *conf_elems;
	size_t size;
};

const jsmntok_t *parse_tokens(const char *js, const jsmntok_t *t,
	struct conf_schema *conf_schema);

int tokstrcmp(const char *js, const jsmntok_t *t, const char *s);

bool toktobool(const char *js, const jsmntok_t *t);

int toktoi(const char *js, const jsmntok_t *t);

char *toktoa(const char *js, const jsmntok_t *t);

#endif
