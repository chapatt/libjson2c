/*
 * Copyright (c) 2016 Chase Patterson
 */

#include "json2c.c"
#include <assert.h>
#include <string.h>
#include <stdio.h>

bool test_toktoi(void);
bool test_toktoa(void);
bool test_tokstrcmp(void);
bool test_toktobool(void);
bool test_install_val(void);
bool test_iscomplete(void);
bool test_search_schema(void);
bool test_parse_tokens(void);

int main(void)
{
	struct test {
		char *name;
		bool (*fn)(void);
	} tests[] = {
		{ "toktoi", test_toktoi },
		{ "toktoa", test_toktoa },
		{ "tokstrcmp", test_tokstrcmp },
		{ "toktobool", test_toktobool },
		{ "install_val", test_install_val },
		{ "iscomplete", test_iscomplete },
		{ "search_schema", test_search_schema },
		{ "parse_tokens", test_parse_tokens }
	};

	size_t fail_count = 0;
	for (size_t i=0; i < ARRAY_SIZE(tests); ++i) {
		printf("Testing %s", tests[i].name);
		for (size_t j = 80 - (14 + strlen(tests[i].name)); j > 0; --j)
			putchar('.');
		bool test_result = tests[i].fn();
		if (!test_result)
			++fail_count;
		printf("%s\n", test_result ? "PASSED" : "FAILED");
	}

	if (fail_count)
		printf("%d test%s failed!\n",
			fail_count, (fail_count == 1) ? "" : "s");
	else
		printf("All tests passed!\n");
		

	return fail_count;
}

bool test_toktoi(void)
{
	int i = 123;
	char *istr = "123";
	jsmntok_t itok = { JSMN_PRIMITIVE, 0, strlen(istr), 0 };
	return (toktoi(istr, &itok) == i);
}

bool test_toktoa(void)
{
	char *s = "foobar";
	jsmntok_t stok = { JSMN_STRING, 0, strlen(s), 0 };
	return (strcmp(toktoa(s, &stok), s) == 0);
}

bool test_tokstrcmp(void)
{
	bool test_result = 1;

	char *s = "foo";
	jsmntok_t stok = { JSMN_STRING, 0, strlen(s), 0 };

	test_result = test_result && (tokstrcmp(s, &stok, s) == 0);
	test_result = test_result && (tokstrcmp(s, &stok, "bar") != 0);

	return test_result;
}

bool test_toktobool(void)
{
	bool test_result = 1;

	bool a = true;
	char *astr = "true";
	jsmntok_t atok = { JSMN_PRIMITIVE, 0, strlen(astr), 0 };
	test_result = test_result && (toktobool(astr, &atok) == a);

	bool b = false;
	char *bstr = "false";
	jsmntok_t btok = { JSMN_PRIMITIVE, 0, strlen(bstr), 0 };
	test_result = test_result && (toktobool(bstr, &btok) == b);

	return test_result;
}

bool test_install_val(void)
{
	bool test_result = 1;

	int i = 123;
	int j = 0;
	char *istr = "123";
	jsmntok_t itok = { JSMN_PRIMITIVE, 0, strlen(istr), 0 };
	struct conf_element iconf_elem =
		{ "i", 0, LEAF_INT, &j, NULL };
	const jsmntok_t *t = install_val(istr, &itok, &iconf_elem);
	test_result = test_result && (j == i);
	test_result = test_result && (t = &itok + sizeof(jsmntok_t));

	bool a = true;
	bool b = false;
	char *astr = "true";
	jsmntok_t atok = { JSMN_PRIMITIVE, 0, strlen(astr), 0 };
	struct conf_element aconf_elem =
		{ "a", 0, LEAF_BOOL, &b, NULL };
	const jsmntok_t *t2 = install_val(astr, &atok, &aconf_elem);
	test_result = test_result && (b == a);
	test_result = test_result && (t2 = &atok + sizeof(jsmntok_t));

	int k = 123;
	char *kstr = "123";
	jsmntok_t ktok = { JSMN_PRIMITIVE, 0, strlen(kstr), 0 };
	struct conf_element kconf_elem =
		{ "k", 0, LEAF_INT, NULL, NULL };
	test_result = test_result
		&& install_val(kstr, &ktok, &kconf_elem) == NULL;
	test_result = test_result && (json2cerrno == JSON2C_ENULLELEM);

	return test_result;
}

bool test_iscomplete(void)
{
	bool test_result = 1;

	struct conf_schema schema1 = ARR_ARRAY_SIZE((
		(struct conf_element []) {
			{ "a", REQUIRED, LEAF_INT, NULL, NULL },
			{ "b", REQUIRED, LEAF_INT, NULL, NULL }
		}
	));
	test_result = test_result && (!iscomplete(&schema1));

	struct conf_schema schema2 = ARR_ARRAY_SIZE((
		(struct conf_element []) {
			{ "a", REQUIRED | COLLECTED, LEAF_INT, NULL, NULL },
			{ "b", REQUIRED | COLLECTED, LEAF_INT, NULL, NULL }
		}
	));
	test_result = test_result && (iscomplete(&schema2));

	struct conf_schema schema3 = ARR_ARRAY_SIZE((
		(struct conf_element []) {
			{ "a", REQUIRED | COLLECTED, LEAF_INT, NULL, NULL },
			{ "b", 0, LEAF_INT, NULL, NULL }
		}
	));
	test_result = test_result && (iscomplete(&schema3));

	struct conf_schema schema4 = ARR_ARRAY_SIZE((
		(struct conf_element []) {
			{ "a", REQUIRED | COLLECTED, LEAF_INT, NULL, NULL },
			{ "b", COLLECTED, LEAF_INT, NULL, NULL }
		}
	));
	test_result = test_result && (iscomplete(&schema4));

	return test_result;
}

bool test_search_schema(void)
{
	bool test_result = 1;

	char *s = "b";
	jsmntok_t stok = { JSMN_STRING, 0, strlen(s), 0 };
	char *t = "c";
	jsmntok_t ttok = { JSMN_STRING, 0, strlen(t), 0 };
	struct conf_schema schema = ARR_ARRAY_SIZE((
		(struct conf_element []) {
			{ "a", 0, LEAF_INT, NULL, NULL },
			{ "b", 0, LEAF_INT, NULL, NULL }
		}
	));
	struct conf_element *ptrtob = &schema.conf_elems[1];

	test_result = test_result
		&& (search_schema(s, &stok, &schema) == ptrtob);

	test_result = test_result
		&& (search_schema(t, &ttok, &schema) == NULL);

	return test_result;
}

bool test_parse_tokens(void)
{
	bool test_result = 1;

	const jsmntok_t *t = NULL;
	int a = 0;
	int ba = 0;
	int bb = 0;
	char *js = "{\"a\": 123, \"b\": {\"ba\": 456, \"bb\": 789}}";
	jsmntok_t toks[] = {
		{ JSMN_OBJECT, 0, 38, 2 },
		{ JSMN_STRING, 2, 3, 0 },
		{ JSMN_PRIMITIVE, 6, 9, 0 },
		{ JSMN_STRING, 12, 13, 0 },
		{ JSMN_OBJECT, 16, 38, 2 },
		{ JSMN_STRING, 18, 20, 0 },
		{ JSMN_PRIMITIVE, 23, 26, 0 },
		{ JSMN_STRING, 29, 31, 0 },
		{ JSMN_PRIMITIVE, 34, 37, 0 }
	};
	struct conf_schema subschema = ARR_ARRAY_SIZE((
		(struct conf_element []) {
			{ "ba", 0, LEAF_INT, &ba, NULL },
			{ "bb", 0, LEAF_INT, &bb, NULL }
		}
	));
	struct conf_schema schema = ARR_ARRAY_SIZE((
		(struct conf_element []) {
			{ "a", 0, LEAF_INT, &a, NULL },
			{ "b", 0, NODE_SCHEMA, &subschema, NULL }
		}
	));
	t = parse_tokens(js, toks, &schema);

	test_result = test_result && (t == toks + ARRAY_SIZE(toks));
	test_result = test_result && (a == 123);
	test_result = test_result && (ba == 456);
	test_result = test_result && (bb == 789);
	test_result = test_result && (schema.conf_elems[0].flags & COLLECTED);

	return test_result;
}
