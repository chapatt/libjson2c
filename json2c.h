#include <stdbool.h>
#include <stddef.h>
#include "../jsmn/jsmn.h"

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

/* take array x and return object with members x and no. elements in x */
#define ARR_ARRAY_SIZE(x) {x, ARRAY_SIZE(x)}

enum conf_elem_type {
	LEAF_INT,
	LEAF_BOOL,
	LEAF_STRING,
	NODE_SCHEMA,
	NODE_FN
};

struct conf_element {
	char *key;
	bool required;
	bool collected;
	enum conf_elem_type val_type;
	void *val_p;
	const jsmntok_t *(*fn_p)(const char *js, const jsmntok_t *tok);
};

struct conf_schema {
	struct conf_element *conf_elems;
	size_t size;
};

/* parse_tokens: traverse schema tree in parallel with jsmn tokens */
const jsmntok_t *parse_tokens(const char *js, const jsmntok_t *t,
	const char *key, struct conf_schema *conf_schema);

/* install_val: according to conf_elem, save value from jsmn token */
const jsmntok_t *install_val(const char *js, const jsmntok_t *t,
	const struct conf_element *conf_elem);

/*
 * tokstrcmp: strcmp wrapper for jsmn tokens
 * Returns an integer less than, equal to, or greater than zero if t is found,
 * respectively, to be less than, to match, or be greater than s.
 */
int tokstrcmp(const char *js, const jsmntok_t *t, const char *s);

/*
 * toktobool: takes json string and jsmntok_t pointing to a bool and returns
 * corresponding bool value. On error, returns false and sets errno.
 */
bool toktobool(const char *js, const jsmntok_t *t);

/*
 * toktoi: takes json string and jsmntok_t pointing to a number
 * and returns corresponding int value. On error, returns 0 and sets errno.
 */
int toktoi(const char *js, const jsmntok_t *t);

/*
 * toktoa: copy token t into allocated space and null terminate it.
 * Return pointer to free()-able string.
 */
char *toktoa(const char *js, const jsmntok_t *t);
