/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Walbottle
 * Copyright (C) Philip Withnall 2015 <philip@tecnocode.co.uk>
 *
 * Walbottle is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Walbottle is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Walbottle.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <locale.h>
#include <string.h>

#include "wbl-schema.h"

/* Utility method to assert that the generated instances in two arrays match,
 * ignoring order. */
static void
assert_generated_instances_match (GPtrArray/*<owned WblGeneratedInstance>*/ *actual,
                                  const gchar **expected  /* NULL terminated */)
{
	guint i;

	for (i = 0; i < actual->len; i++) {
		const gchar *actual_json;
		gboolean found = FALSE;
		guint j;

		actual_json = wbl_generated_instance_get_json (actual->pdata[i]);

		for (j = 0; expected[j] != NULL; j++) {
			if (g_strcmp0 (actual_json, expected[j]) == 0) {
				found = TRUE;
				break;
			}
		}

		/* Nice error message. */
		if (!found) {
			g_assert_cmpstr (actual_json, ==, "not found");
		}
		g_assert (found);
	}

	g_assert_cmpuint (actual->len, ==, g_strv_length ((gchar **) expected));
}

/* multipleOf. json-schema-validation§5.1.1. */
static void
assert_schema_keyword (const gchar *valid_schema,
                       const gchar **invalid_schemas,
                       const gchar **valid_instances,
                       const gchar **invalid_instances,
                       const gchar **expected_instances)
{
	WblSchema *schema = NULL;  /* owned */
	JsonParser *parser = NULL;  /* owned */
	GPtrArray/*<owned WblGeneratedInstace>*/ *instances = NULL;  /* owned */
	guint i;
	GError *error = NULL;

	schema = wbl_schema_new ();

	/* Test parsing the invalid schemas. */
	for (i = 0; invalid_schemas[i] != NULL; i++) {
		wbl_schema_load_from_data (schema, invalid_schemas[i], -1,
		                           &error);
		g_assert_error (error,
		                WBL_SCHEMA_ERROR, WBL_SCHEMA_ERROR_MALFORMED);
		g_clear_error (&error);
	}

	/* Test parsing the valid schema. */
	wbl_schema_load_from_data (schema, valid_schema, -1, &error);
	g_assert_no_error (error);

	/* Check validation of valid and invalid instances. */
	parser = json_parser_new ();

	/* Valid cases. */
	for (i = 0; valid_instances[i] != NULL; i++) {
		json_parser_load_from_data (parser, valid_instances[i], -1,
		                            &error);
		g_assert_no_error (error);

		wbl_schema_apply (schema,
		                  json_parser_get_root (parser), &error);
		g_assert_no_error (error);
	}

	/* Invalid cases. */
	for (i = 0; invalid_instances[i] != NULL; i++) {
		json_parser_load_from_data (parser,
		                            invalid_instances[i], -1, &error);
		g_assert_no_error (error);

		wbl_schema_apply (schema,
		                  json_parser_get_root (parser), &error);
		g_assert_error (error,
		                WBL_SCHEMA_ERROR, WBL_SCHEMA_ERROR_INVALID);
		g_clear_error (&error);
	}

	/* Check generated instances. */
	instances = wbl_schema_generate_instances (schema,
	                                           WBL_GENERATE_INSTANCE_NONE);
	assert_generated_instances_match (instances, expected_instances);
	g_ptr_array_unref (instances);

	g_object_unref (parser);
	g_object_unref (schema);
}

/* multipleOf using an integer. json-schema-validation§5.1.1. */
static void
test_schema_keywords_multiple_of_integer (void)
{
	const gchar *valid_schema = "{ \"multipleOf\": 5 }";
	const gchar *invalid_schemas[] = {
		"{ \"multipleOf\": null }",  /* not a number */
		"{ \"multipleOf\": 0 }",  /* not greater than zero */
		"{ \"multipleOf\": -1 }",  /* not greater than zero */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"\"no\"",  /* wrong type */
		"0",  /* multiple of anything */
		"5",  /* normal multiple */
		"10",  /* normal multiple */
		"10.0",  /* mixed type multiple */
		NULL,
	};
	const gchar *invalid_instances[] = {
		"1",  /* not a multiple */
		"6",  /* not a multiple */
		NULL,
	};
	const gchar *expected_instances[] = {
		"0",
		"0.0",
		"5",
		"6",
		"10",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* multipleOf using a double. json-schema-validation§5.1.1. */
static void
test_schema_keywords_multiple_of_double (void)
{
	const gchar *valid_schema = "{ \"multipleOf\": 1.1 }";
	const gchar *invalid_schemas[] = {
		"{ \"multipleOf\": null }",  /* not a number */
		"{ \"multipleOf\": 0.0 }",  /* not greater than zero */
		"{ \"multipleOf\": -1.7 }",  /* not greater than zero */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"\"no\"",  /* wrong type */
		"0",  /* multiple of anything */
		"0.0",  /* multiple of anything */
		"1.1",  /* normal multiple */
		"2.2",  /* normal multiple */
		"11",  /* mixed type multiple */
		NULL,
	};
	const gchar *invalid_instances[] = {
		"1.2",  /* not a multiple */
		"6.1",  /* not a multiple */
		NULL,
	};
	const gchar *expected_instances[] = {
		"0",
		"0.0",
		"1.100000",
		"2.200000",
		"1.200000",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* maximum. json-schema-validation§5.1.2. */
static void
test_schema_keywords_maximum (void)
{
	const gchar *valid_schema = "{ \"maximum\": 5 }";
	const gchar *invalid_schemas[] = {
		"{ \"maximum\": null }",  /* not a number */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"\"no\"",  /* wrong type */
		"5",  /* equal to */
		"4",  /* less than */
		"-4",  /* less than */
		"4.1",  /* less than */
		"5.0",  /* equal to */
		NULL,
	};
	gchar **invalid_instances;
	const gchar *expected_instances[] = {
		"5",
		"5.000000",
		"6",
		NULL,
	};

	invalid_instances = g_malloc0_n (5, sizeof (gchar *));
	invalid_instances[0] = g_strdup ("6");  /* greater than */
	invalid_instances[1] = g_strdup ("5.1");  /* greater than */
	invalid_instances[2] = g_strdup_printf ("%" G_GINT64_FORMAT,
	                                        G_MAXINT64);  /* greater than */
	invalid_instances[3] = g_strdup_printf ("%f", G_MAXDOUBLE);
	invalid_instances[4] = NULL;

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       (const gchar **) invalid_instances,
	                       expected_instances);

	g_strfreev (invalid_instances);
}

/* exclusiveMaximum. json-schema-validation§5.1.2. */
static void
test_schema_keywords_exclusive_maximum (void)
{
	const gchar *valid_schema = "{ \"maximum\": 5, "
	                              "\"exclusiveMaximum\": true }";
	const gchar *invalid_schemas[] = {
		"{ \"maximum\": 5, \"exclusiveMaximum\": null }",  /* type */
		"{ \"exclusiveMaximum\": true }",  /* missing maximum */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"\"no\"",  /* wrong type */
		"4",  /* less than */
		"-4",  /* less than */
		NULL,
	};
	const gchar *invalid_instances[] = {
		"5",  /* equal to */
		"6",  /* greater than */
		NULL,
	};
	const gchar *expected_instances[] = {
		"4",
		"5",
		"5.000000",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* minimum. json-schema-validation§5.1.3. */
static void
test_schema_keywords_minimum (void)
{
	const gchar *valid_schema = "{ \"minimum\": 5 }";
	const gchar *invalid_schemas[] = {
		"{ \"minimum\": null }",  /* not a number */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"\"no\"",  /* wrong type */
		"5",  /* equal to */
		"6",  /* greater than */
		"5.1",  /* greater than */
		"5.0",  /* equal to */
		NULL,
	};
	gchar **invalid_instances;
	const gchar *expected_instances[] = {
		"4",
		"5",
		"5.000000",
		NULL,
	};

	invalid_instances = g_malloc0_n (5, sizeof (gchar *));
	invalid_instances[0] = g_strdup ("4");  /* less than */
	invalid_instances[1] = g_strdup ("4.9");  /* less than */
	invalid_instances[2] = g_strdup_printf ("%" G_GINT64_FORMAT,
	                                        G_MININT64);  /* less than */
	invalid_instances[3] = g_strdup_printf ("%f", G_MINDOUBLE);
	invalid_instances[4] = NULL;

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       (const gchar **) invalid_instances,
	                       expected_instances);

	g_strfreev (invalid_instances);
}

/* exclusiveMinimum. json-schema-validation§5.1.3. */
static void
test_schema_keywords_exclusive_minimum (void)
{
	const gchar *valid_schema = "{ \"minimum\": 5, "
	                              "\"exclusiveMinimum\": true }";
	const gchar *invalid_schemas[] = {
		"{ \"minimum\": 5, \"exclusiveMinimum\": null }",  /* type */
		"{ \"exclusiveMinimum\": true }",  /* missing minimum */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"\"no\"",  /* wrong type */
		"6",  /* greater than */
		NULL,
	};
	const gchar *invalid_instances[] = {
		"5",  /* equal to */
		"4",  /* less than */
		NULL,
	};
	const gchar *expected_instances[] = {
		"5",
		"5.000000",
		"6",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* maxLength. json-schema-validation§5.2.1. */
static void
test_schema_keywords_max_length (void)
{
	const gchar *valid_schema = "{ \"maxLength\": 5 }";
	const gchar *invalid_schemas[] = {
		"{ \"maxLength\": null }",  /* incorrect type */
		"{ \"maxLength\": -1 }",  /* negative */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"0",  /* wrong type */
		"\"\"",  /* empty string */
		"\"hi\"",  /* shorter */
		"\"hello\"",  /* equal length */
		NULL,
	};
	const gchar *invalid_instances[] = {
		"\"hello there\"",  /* too long */
		NULL,
	};
	const gchar *expected_instances[] = {
		"\"00000\"",
		"\"000000\"",
		"\"🐵🐵🐵🐵🐵\"",
		"\"🐵🐵🐵🐵🐵🐵\"",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* minLength. json-schema-validation§5.2.2. */
static void
test_schema_keywords_min_length (void)
{
	const gchar *valid_schema = "{ \"minLength\": 5 }";
	const gchar *invalid_schemas[] = {
		"{ \"minLength\": null }",  /* incorrect type */
		"{ \"minLength\": -1 }",  /* negative */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"0",  /* wrong type */
		"\"hello\"",  /* equal length */
		"\"hello there\"",  /* longer */
		NULL,
	};
	const gchar *invalid_instances[] = {
		"\"\"",  /* empty string */
		"\"hi\"",  /* shorter */
		NULL,
	};
	const gchar *expected_instances[] = {
		"\"0000\"",
		"\"00000\"",
		"\"🐵🐵🐵🐵\"",
		"\"🐵🐵🐵🐵🐵\"",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* pattern. json-schema-validation§5.2.3. */
static void
test_schema_keywords_pattern (void)
{
	const gchar *valid_schema = "{ \"pattern\": \"[a-zA-Z0-9]+\" }";
	const gchar *invalid_schemas[] = {
		"{ \"pattern\": null }",  /* incorrect type */
		"{ \"pattern\": 0 }",  /* incorrect type */
		"{ \"pattern\": \"++\" }",  /* invalid regex */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"0",  /* wrong type */
		"\"hello\"",  /* matches regex */
		"\"!hello\"",  /* matches regex without anchoring */
		NULL,
	};
	const gchar *invalid_instances[] = {
		"\"\"",  /* empty string */
		"\"++\"",  /* non-matching */
		NULL,
	};
	const gchar *expected_instances[] = {
		"\"\"",
		"\"non-empty\"",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* maxProperties. json-schema-validation§5.4.1. */
static void
test_schema_keywords_max_properties (void)
{
	const gchar *valid_schema = "{ \"maxProperties\": 2 }";
	const gchar *invalid_schemas[] = {
		"{ \"maxProperties\": null }",  /* incorrect type */
		"{ \"maxProperties\": false }",  /* incorrect type */
		"{ \"maxProperties\": -1 }",  /* negative */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"0",  /* wrong type */
		"{}",  /* empty object */
		"{ \"test\": null, \"test2\": null }",  /* boundary */
		NULL,
	};
	const gchar *invalid_instances[] = {
		"{ \"a\": 1, \"b\": 2, \"c\": 3 }",  /* too many properties */
		NULL,
	};
	const gchar *expected_instances[] = {
		"{\"0\":null,\"1\":null}",
		"{\"0\":null,\"1\":null,\"2\":null}",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* minProperties. json-schema-validation§5.4.2. */
static void
test_schema_keywords_min_properties (void)
{
	const gchar *valid_schema = "{ \"minProperties\": 2 }";
	const gchar *invalid_schemas[] = {
		"{ \"minProperties\": null }",  /* incorrect type */
		"{ \"minProperties\": false }",  /* incorrect type */
		"{ \"minProperties\": -1 }",  /* negative */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"0",  /* wrong type */
		"{ \"a\": 1, \"b\": 2, \"c\": 3 }",  /* lots */
		"{ \"a\": 1, \"b\": 2 }",  /* boundary */
		NULL,
	};
	const gchar *invalid_instances[] = {
		"{}",  /* empty object */
		"{ \"a\": 1 }",  /* too few properties */
		NULL,
	};
	const gchar *expected_instances[] = {
		"{\"0\":null}",
		"{\"0\":null,\"1\":null}",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* required. json-schema-validation§5.4.3. */
static void
test_schema_keywords_required (void)
{
	const gchar *valid_schema = "{ \"required\": [ \"a\", \"b\" ] }";
	const gchar *invalid_schemas[] = {
		"{ \"required\": null }",  /* incorrect type */
		"{ \"required\": false }",  /* incorrect type */
		"{ \"required\": [] }",  /* empty array */
		"{ \"required\": [ null ] }",  /* non-string element */
		"{ \"required\": [ \"a\", \"a\" ] }",  /* duplicate elements */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"0",  /* wrong type */
		"{ \"a\": 1, \"b\": 2, \"c\": 3 }",  /* extra properties */
		"{ \"a\": 1, \"b\": 2 }",  /* exact properties */
		NULL,
	};
	const gchar *invalid_instances[] = {
		"{}",  /* empty object */
		"{ \"a\": 1 }",  /* too few properties */
		NULL,
	};
	const gchar *expected_instances[] = {
		"{\"a\":null}",
		"{\"b\":null}",
		"{\"a\":null,\"b\":null}",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* additionalProperties set to true. json-schema-validation§5.4.4. */
static void
test_schema_keywords_additional_properties_true (void)
{
	const gchar *valid_schema = "{ \"additionalProperties\": true, "
	                              "\"properties\": {} }";
	const gchar *invalid_schemas[] = {
		"{ \"additionalProperties\": null }",  /* incorrect type */
		"{ \"additionalProperties\": 0 }",  /* incorrect type */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"0",  /* wrong type */
		"{}",  /* matching properties */
		"{ \"a\": 1 }",  /* extra properties */
		NULL,
	};
	const gchar *invalid_instances[] = {
		/* Always valid if additionalProperties is true. */
		NULL,
	};
	const gchar *expected_instances[] = {
		"{}",
		"{\"additionalProperties-test-0\":null}",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* additionalProperties set to a schema. json-schema-validation§5.4.4. */
static void
test_schema_keywords_additional_properties_schema (void)
{
	const gchar *valid_schema = "{ \"additionalProperties\": "
	                                "{"
	                                     "\"required\": [ \"a\" ], "
	                                     "\"properties\": {"
	                                         "\"a\": { \"type\": \"string\" }"
	                                     "}"
	                                "},"
	                              "\"properties\": {} }";
	const gchar *invalid_schemas[] = {
		"{ \"additionalProperties\": "
		  "{ \"additionalProperties\": null} }",  /* invalid schema */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"0",  /* wrong type */
		"{}",  /* matching properties */
		"{ \"0\": 1 }",  /* extra properties (wrong type) */
		NULL,
	};
	const gchar *invalid_instances[] = {
		/* Only invalid subschema instances are invalid. */
		"{\"0\":{\"a\":null}}",  /* wrong type */
		NULL,
	};
	const gchar *expected_instances[] = {
		"{}",
		"{\"additionalProperties-test-0\":{}}",
		"{\"additionalProperties-test-0\":{\"a\":null}}",
		"{\"additionalProperties-test-0\":{\"a\":''}}",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* additionalProperties set to false. json-schema-validation§5.4.4. */
static void
test_schema_keywords_additional_properties_false (void)
{
	const gchar *valid_schema = "{ \"additionalProperties\": false, "
	                              "\"properties\": { \"a\": {} } }";
	const gchar *invalid_schemas[] = {
		/* Covered in the other additionalProperties tests. */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"0",  /* wrong type */
		"{ \"a\": null }",  /* matching properties */
		NULL,
	};
	const gchar *invalid_instances[] = {
		"{ \"a\": null, \"b\": null }",  /* extra property */
		NULL,
	};
	const gchar *expected_instances[] = {
		"{}",
		"{\"additionalProperties-test-0\":null}",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* properties. json-schema-validation§5.4.4. */
static void
test_schema_keywords_properties (void)
{
	const gchar *valid_schema = "{ \"properties\": {"
		"\"a\": {},"
		"\"b\": { \"type\": \"boolean\" },"
		"\"c\": { \"type\": \"number\" }"
	"}, \"additionalProperties\": false }";
	const gchar *invalid_schemas[] = {
		"{ \"properties\": null }",  /* incorrect type */
		"{ \"properties\": 0 }",  /* incorrect type */
		"{ \"properties\": { \"a\": false } }",  /* incorrect type */
		"{ \"properties\": { \"a\": { \"type\": false }}}",  /* invalid schema */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"0",  /* wrong type */
		"{}",  /* matching properties */
		"{ \"a\": 1 }",  /* matching properties */
		"{ \"a\": 1, \"b\": false }",  /* matching properties */
		"{ \"c\": 5.1 }",  /* matching properties */
		NULL,
	};
	const gchar *invalid_instances[] = {
		"{ \"d\": false }",  /* extra property */
		NULL,
	};
	const gchar *expected_instances[] = {
		"{}",
		"{\"b\":null}",
		"{\"b\":true}",
		"{\"c\":null}",
		"{\"c\":0.1}",
		"{\"additionalProperties-test-0\":null}",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* patternProperties. json-schema-validation§5.4.4. */
static void
test_schema_keywords_pattern_properties (void)
{
	const gchar *valid_schema = "{ \"patternProperties\": {"
		"\"[a-z]+\": { \"type\": \"boolean\" },"
		"\"[0-9]+\": { \"type\": \"number\" }"
	"}, \"additionalProperties\": false }";
	const gchar *invalid_schemas[] = {
		"{ \"patternProperties\": null }",  /* incorrect type */
		"{ \"patternProperties\": 0 }",  /* incorrect type */
		"{ \"patternProperties\": { \"a\": false } }",  /* incorrect type */
		"{ \"patternProperties\": { \"++\": {} } }",  /* invalid regex */
		"{ \"patternProperties\": { \"a\": { \"type\": false }}}",  /* invalid schema */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* wrong type */
		"0",  /* wrong type */
		"{}",  /* matching properties */
		"{ \"a\": true }",  /* matching properties */
		"{ \"beee\": false }",  /* matching properties */
		"{ \"a\": false, \"beee\": false }",  /* matching properties */
		"{ \"15\": 5.1, \"asd\": false }",  /* matching properties */
		NULL,
	};
	const gchar *invalid_instances[] = {
		"{ \"_\": false }",  /* extra property */
		NULL,
	};
	const gchar *expected_instances[] = {
		/* FIXME */
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* enum. json-schema-validation§5.5.1. */
static void
test_schema_keywords_enum (void)
{
	const gchar *valid_schema = "{ \"enum\": [ 1, \"hi\", { \"a\": 0 } ] }";
	const gchar *invalid_schemas[] = {
		"{ \"enum\": null }",  /* incorrect type */
		"{ \"enum\": 0 }",  /* incorrect type */
		"{ \"enum\": [] }",  /* empty array */
		"{ \"enum\": [ null, null ] }",  /* duplicate elements */
		NULL,
	};
	const gchar *valid_instances[] = {
		"1",  /* matching */
		"\"hi\"",  /* matching */
		"{\"a\":0}",  /* matching */
		NULL,
	};
	const gchar *invalid_instances[] = {
		"null",  /* non-matching */
		NULL,
	};
	const gchar *expected_instances[] = {
		"1",
		"\"hi\"",
		"{\"a\":0}",  /* matching */
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* type set to array. json-schema-validation§5.5.2. */
static void
test_schema_keywords_type_string_array (void)
{
	const gchar *valid_schema = "{ \"type\": \"array\" }";
	const gchar *invalid_schemas[] = {
		"{ \"type\": null }",  /* incorrect type */
		"{ \"type\": 0 }",  /* incorrect type */
		"{ \"type\": \"promise\" }",  /* invalid type name */
		NULL,
	};
	const gchar *valid_instances[] = {
		"[]",
		NULL,
	};
	const gchar *invalid_instances[] = {
		"null",  /* invalid type */
		"{}",  /* invalid type */
		NULL,
	};
	const gchar *expected_instances[] = {
		"[]",
		"null",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* type set to boolean. json-schema-validation§5.5.2. */
static void
test_schema_keywords_type_string_boolean (void)
{
	const gchar *valid_schema = "{ \"type\": \"boolean\" }";
	const gchar *invalid_schemas[] = {
		"{ \"type\": null }",  /* incorrect type */
		"{ \"type\": 0 }",  /* incorrect type */
		"{ \"type\": \"promise\" }",  /* invalid type name */
		NULL,
	};
	const gchar *valid_instances[] = {
		"true",
		NULL,
	};
	const gchar *invalid_instances[] = {
		"null",  /* invalid type */
		"{}",  /* invalid type */
		NULL,
	};
	const gchar *expected_instances[] = {
		"true",
		"null",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* type set to integer. json-schema-validation§5.5.2. */
static void
test_schema_keywords_type_string_integer (void)
{
	const gchar *valid_schema = "{ \"type\": \"integer\" }";
	const gchar *invalid_schemas[] = {
		"{ \"type\": null }",  /* incorrect type */
		"{ \"type\": 0 }",  /* incorrect type */
		"{ \"type\": \"promise\" }",  /* invalid type name */
		NULL,
	};
	const gchar *valid_instances[] = {
		"5",
		NULL,
	};
	const gchar *invalid_instances[] = {
		"null",  /* invalid type */
		"3.1",  /* invalid type */
		NULL,
	};
	const gchar *expected_instances[] = {
		"1",
		"null",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* type set to number. json-schema-validation§5.5.2. */
static void
test_schema_keywords_type_string_number (void)
{
	const gchar *valid_schema = "{ \"type\": \"number\" }";
	const gchar *invalid_schemas[] = {
		"{ \"type\": null }",  /* incorrect type */
		"{ \"type\": 0 }",  /* incorrect type */
		"{ \"type\": \"promise\" }",  /* invalid type name */
		NULL,
	};
	const gchar *valid_instances[] = {
		"1",  /* integers are valid numbers */
		"5.9",
		NULL,
	};
	const gchar *invalid_instances[] = {
		"null",  /* invalid type */
		"{}",  /* invalid type */
		NULL,
	};
	const gchar *expected_instances[] = {
		"0.1",
		"null",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* type set to null. json-schema-validation§5.5.2. */
static void
test_schema_keywords_type_string_null (void)
{
	const gchar *valid_schema = "{ \"type\": \"null\" }";
	const gchar *invalid_schemas[] = {
		"{ \"type\": null }",  /* incorrect type */
		"{ \"type\": 0 }",  /* incorrect type */
		"{ \"type\": \"promise\" }",  /* invalid type name */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",
		NULL,
	};
	const gchar *invalid_instances[] = {
		"false",  /* invalid type */
		"{}",  /* invalid type */
		NULL,
	};
	const gchar *expected_instances[] = {
		"null",
		"false",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* type set to object. json-schema-validation§5.5.2. */
static void
test_schema_keywords_type_string_object (void)
{
	const gchar *valid_schema = "{ \"type\": \"object\" }";
	const gchar *invalid_schemas[] = {
		"{ \"type\": null }",  /* incorrect type */
		"{ \"type\": 0 }",  /* incorrect type */
		"{ \"type\": \"promise\" }",  /* invalid type name */
		NULL,
	};
	const gchar *valid_instances[] = {
		"{}",
		NULL,
	};
	const gchar *invalid_instances[] = {
		"null",  /* invalid type */
		"[]",  /* invalid type */
		NULL,
	};
	const gchar *expected_instances[] = {
		"{}",
		"null",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* type set to string. json-schema-validation§5.5.2. */
static void
test_schema_keywords_type_string_string (void)
{
	const gchar *valid_schema = "{ \"type\": \"string\" }";
	const gchar *invalid_schemas[] = {
		"{ \"type\": null }",  /* incorrect type */
		"{ \"type\": 0 }",  /* incorrect type */
		"{ \"type\": \"promise\" }",  /* invalid type name */
		NULL,
	};
	const gchar *valid_instances[] = {
		"\"\"",
		NULL,
	};
	const gchar *invalid_instances[] = {
		"null",  /* invalid type */
		"{}",  /* invalid type */
		NULL,
	};
	const gchar *expected_instances[] = {
		"''",
		"null",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* type as an array. json-schema-validation§5.5.2. */
static void
test_schema_keywords_type_array (void)
{
	const gchar *valid_schema = "{ \"type\": [ \"string\", \"number\" ] }";
	const gchar *invalid_schemas[] = {
		"{ \"type\": [ \"null\", \"null\" ] }",  /* duplicates */
		"{ \"type\": [ 0 ] }",  /* non-string element */
		NULL,
	};
	const gchar *valid_instances[] = {
		"\"\"",
		"1",
		"1.5",
		NULL,
	};
	const gchar *invalid_instances[] = {
		"null",  /* invalid type */
		"{}",  /* invalid type */
		NULL,
	};
	const gchar *expected_instances[] = {
		"''",
		"null",
		"0.1",
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* title. json-schema-validation§6.1. */
static void
test_schema_keywords_title (void)
{
	const gchar *valid_schema = "{ \"title\": \"hi\" }";
	const gchar *invalid_schemas[] = {
		"{ \"title\": null }",  /* invalid type */
		"{ \"title\": 1 }",  /* invalid type */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* validation always succeeds */
		NULL,
	};
	const gchar *invalid_instances[] = {
		/* Validation always succeeds. */
		NULL,
	};
	const gchar *expected_instances[] = {
		/* Not generated. */
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* description. json-schema-validation§6.1. */
static void
test_schema_keywords_description (void)
{
	const gchar *valid_schema = "{ \"description\": \"hi\" }";
	const gchar *invalid_schemas[] = {
		"{ \"description\": null }",  /* invalid type */
		"{ \"description\": 1 }",  /* invalid type */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* validation always succeeds */
		NULL,
	};
	const gchar *invalid_instances[] = {
		/* Validation always succeeds. */
		NULL,
	};
	const gchar *expected_instances[] = {
		/* Not generated. */
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

/* default. json-schema-validation§6.2. */
static void
test_schema_keywords_default (void)
{
	const gchar *valid_schema = "{ \"default\": null }";
	const gchar *invalid_schemas[] = {
		/* Validation always succeeds. */
		NULL,
	};
	const gchar *valid_instances[] = {
		"null",  /* validation always succeeds */
		NULL,
	};
	const gchar *invalid_instances[] = {
		/* Validation always succeeds. */
		NULL,
	};
	const gchar *expected_instances[] = {
		/* Not generated. */
		NULL,
	};

	assert_schema_keyword (valid_schema, invalid_schemas, valid_instances,
	                       invalid_instances, expected_instances);
}

int
main (int argc, char *argv[])
{
#if !GLIB_CHECK_VERSION (2, 35, 0)
	g_type_init ();
#endif

	setlocale (LC_ALL, "");

	g_test_init (&argc, &argv, NULL);
	g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=");

	/* Schema tests. */
	g_test_add_func ("/schema/keywords/multiple-of/integer",
	                 test_schema_keywords_multiple_of_integer);
	g_test_add_func ("/schema/keywords/multiple-of/double",
	                 test_schema_keywords_multiple_of_double);
	g_test_add_func ("/schema/keywords/maximum",
	                 test_schema_keywords_maximum);
	g_test_add_func ("/schema/keywords/exclusive-maximum",
	                 test_schema_keywords_exclusive_maximum);
	g_test_add_func ("/schema/keywords/minimum",
	                 test_schema_keywords_minimum);
	g_test_add_func ("/schema/keywords/exclusive-minimum",
	                 test_schema_keywords_exclusive_minimum);
	g_test_add_func ("/schema/keywords/max-length",
	                 test_schema_keywords_max_length);
	g_test_add_func ("/schema/keywords/min-length",
	                 test_schema_keywords_min_length);
	g_test_add_func ("/schema/keywords/pattern",
	                 test_schema_keywords_pattern);
	g_test_add_func ("/schema/keywords/max-properties",
	                 test_schema_keywords_max_properties);
	g_test_add_func ("/schema/keywords/min-properties",
	                 test_schema_keywords_min_properties);
	g_test_add_func ("/schema/keywords/required",
	                 test_schema_keywords_required);
	g_test_add_func ("/schema/keywords/additional-properties/true",
	                 test_schema_keywords_additional_properties_true);
	g_test_add_func ("/schema/keywords/additional-properties/false",
	                 test_schema_keywords_additional_properties_false);
	g_test_add_func ("/schema/keywords/additional-properties/schema",
	                 test_schema_keywords_additional_properties_schema);
	g_test_add_func ("/schema/keywords/properties",
	                 test_schema_keywords_properties);
	g_test_add_func ("/schema/keywords/pattern-properties",
	                 test_schema_keywords_pattern_properties);
	g_test_add_func ("/schema/keywords/enum",
	                 test_schema_keywords_enum);
	g_test_add_func ("/schema/keywords/type/string/array",
	                 test_schema_keywords_type_string_array);
	g_test_add_func ("/schema/keywords/type/string/boolean",
	                 test_schema_keywords_type_string_boolean);
	g_test_add_func ("/schema/keywords/type/string/integer",
	                 test_schema_keywords_type_string_integer);
	g_test_add_func ("/schema/keywords/type/string/number",
	                 test_schema_keywords_type_string_number);
	g_test_add_func ("/schema/keywords/type/string/null",
	                 test_schema_keywords_type_string_null);
	g_test_add_func ("/schema/keywords/type/string/object",
	                 test_schema_keywords_type_string_object);
	g_test_add_func ("/schema/keywords/type/string/string",
	                 test_schema_keywords_type_string_string);
	g_test_add_func ("/schema/keywords/type/array",
	                 test_schema_keywords_type_array);
	g_test_add_func ("/schema/keywords/title",
	                 test_schema_keywords_title);
	g_test_add_func ("/schema/keywords/description",
	                 test_schema_keywords_description);
	g_test_add_func ("/schema/keywords/default",
	                 test_schema_keywords_default);

	/* TODO:
	 * • additionalItems
	 * • items
	 * • maxItems
	 * • minItems
	 * • uniqueItems
	 * • dependencies
	 * • allOf
	 * • anyOf
	 * • oneOf
	 * • not
	 * • definitions
	 * • format
	 */

	return g_test_run ();
}