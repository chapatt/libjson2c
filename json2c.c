#include "json2c.h"
#include <stdio.h> /* won't need after error reporting is fixed */
#include <stdlib.h>
#include <string.h>
#include <errno.h> /* won't need after error reporting is fixed */
#include <stdbool.h>
#include <ctype.h>
#include "../jsmn/jsmn.h"

static bool check_schema_completion(const char *key,
	struct conf_schema *conf_schema);

/* parse_tokens: traverse schema tree in parallel with jsmn tokens */ 
const jsmntok_t *parse_tokens(const char *js, const jsmntok_t *t,
	const char *key, struct conf_schema *conf_schema)
{
	int cmp;
	size_t i;
	int tok_start = t->start;
	int tok_end = t->end;
	struct conf_element *conf_elem;

	++t;
	while (t->start > tok_start && t->end <= tok_end)
	{
		if (t->type != JSMN_STRING) {
			fprintf(stderr, "Badly formed configuration: "
				"%.*s should not be in %s! "
				"All JSON keys are strings.\n",
				t->end - t->start, js+t->start, key);
			return NULL;
		}
		for (i=0; i < conf_schema->size; ++i) {
			conf_elem = &conf_schema->conf_elems[i];
			errno = 0;
			if ((cmp = tokstrcmp(js, t, conf_elem->key)) == 0 && errno) {
				fprintf(stderr, "Can't allocate memory");
				return NULL;
			}
			if (cmp == 0) {
				switch (conf_elem->val_type) {
				case LEAF_INT:
				case LEAF_BOOL:
				case LEAF_STRING:
					if ((t =
						install_val(js, ++t, conf_elem))
						== NULL)
					{
						return NULL;
					}
					break;
				case NODE_SCHEMA:
					if ((t = parse_tokens(js, ++t,
						conf_elem->key,
						(struct conf_schema *)
							conf_elem->val_p))
						== NULL)
					{
						return NULL;
					}
					break;
				case NODE_FN:
					if ((t = (*conf_elem->fn_p)(js, ++t))
						== NULL)
					{
						return NULL;
					}
					break;
				}
				conf_elem->collected = true;
				break; /* don't test rest of schema */
			}
		}
		if (cmp != 0) {
			fprintf(stderr, "Badly formed configuration: "
				"%.*s should not be in %s!\n",
				t->end - t->start, js+t->start, key);
			return NULL;
		}
	}

	if (!check_schema_completion(key, conf_schema))
		return NULL;

	return t;
}

/* check_schema_completion: check that required elements have been collected */
static bool check_schema_completion(const char *key, struct conf_schema *conf_schema)
{
	size_t i;
	struct conf_element *conf_elem;

	for (i=0; i < conf_schema->size; ++i) {
		conf_elem = &conf_schema->conf_elems[i];
		if (conf_elem->required && !conf_elem->collected) {
			fprintf(stderr, "Badly formed configuration: "
				"%s is required in %s!\n", conf_elem->key, key);
			return false;
		}
	}

	return true;
}

/* install_val: according to conf_elem, save value from jsmn token */
const jsmntok_t *install_val(const char *js, const jsmntok_t *t,
	const struct conf_element *conf_elem)
{
	int tmp_i;
	bool tmp_bool;
	char *tmp_char_p;

	switch (conf_elem->val_type) {
	case LEAF_INT:
		errno = 0;
		tmp_i = toktoi(js, t);
		if (errno) {
			fprintf(stderr,
				"%s expects an integer\n", conf_elem->key);
			return NULL;
		}
		*(int *) conf_elem->val_p = tmp_i;
		break;
	case LEAF_BOOL:
		errno = 0;
		tmp_bool = toktobool(js, t);
		if (errno) {
			fprintf(stderr,
				"%s expects a boolean\n", conf_elem->key);
			return NULL;
		}
		*(bool *) conf_elem->val_p = tmp_bool;
		break;
	case LEAF_STRING:
		errno = 0;
		tmp_char_p = toktoa(js, t);
		if (errno) {
			fprintf(stderr,
				"%s expects a string\n", conf_elem->key);
			return NULL;
		}
		/* FIXME! free old pointer if malloc'd */
		*(char **) conf_elem->val_p = tmp_char_p;
		break;
	}

	return ++t;
}

/*
 * tokstrcmp: strcmp wrapper for jsmn tokens
 * returns an integer less than, equal to, or greater than zero if t is found,
 * respectively, to be less than, to match, or be greater than s. 
 */
int tokstrcmp(const char *js, const jsmntok_t *t, const char *s)
{
	char *buf;
	int r;

	errno = 0;
	if ((buf = toktoa(js, t)) == NULL) {
		errno = ENOMEM;
		return 0;
	}

	r = strcmp(buf, s);
	free(buf);

	return r;
}

/*
 * toktobool: takes json string and jsmntok_t pointing to a bool
 * and returns corresponding bool value. On error, returns false and sets errno.
 */
bool toktobool(const char *js, const jsmntok_t *t)
{
	if (t->type != JSMN_PRIMITIVE) {
		errno = EINVAL;
		return false;
	}

	if (!tokstrcmp(js, t, "true")) {
		return true;
	} else if (!tokstrcmp(js, t, "false")) {
		return false;
	} else {
		errno = EINVAL;
		return false;
	}
}

/*
 * toktoi: takes json string and jsmntok_t pointing to a number
 * and returns corresponding int value. On error, returns 0 and sets errno.
 */
int toktoi(const char *js, const jsmntok_t *t)
{
	char c;
	size_t i;

	if (t->type != JSMN_PRIMITIVE) {
		errno = EINVAL;
		return 0;
	}

	if(!isdigit(c = *(js+t->start)) &&  c != '-') {
		errno = EINVAL;
		return 0;
	}
	for (i = t->start + 1; i < t->end; ++i) {
		if(!isdigit(js[i])) {
			errno = EINVAL;
			return 0;
		}
	}

	return (int) strtol(js+t->start, NULL, 0);
}

/*
 * toktoa: allocate space for token t and copy it in with null-terminator.
 * Return pointer to free()-able string.
 */
char *toktoa(const char *js, const jsmntok_t *t)
{
	char *buf;
	size_t tok_len;

	tok_len = (t->end - t->start);
	if ((buf = malloc((tok_len + 1) * sizeof(*buf))) == NULL)
		return NULL;
	strncpy(buf, js+t->start, tok_len);
	buf[tok_len] = '\0';

	return buf;
}
