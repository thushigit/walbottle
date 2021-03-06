/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Walbottle
 * Copyright (C) Philip Withnall 2014, 2015, 2016 <philip@tecnocode.co.uk>
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

#ifndef WBL_SCHEMA_H
#define WBL_SCHEMA_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

/**
 * WblSchemaError:
 * @WBL_SCHEMA_ERROR_MALFORMED: Loaded JSON Schema does not conform to the JSON
 *   Schema standard.
 * @WBL_SCHEMA_ERROR_INVALID: Instance does not conform to the given
 *   JSON Schema.
 *
 * Error codes for #WblSchema operations.
 *
 * Since: 0.1.0
 */
typedef enum {
	WBL_SCHEMA_ERROR_MALFORMED = 1,
	WBL_SCHEMA_ERROR_INVALID,
} WblSchemaError;

#define WBL_SCHEMA_ERROR		wbl_schema_error_quark ()

GQuark wbl_schema_error_quark (void) G_GNUC_CONST;

/**
 * WblSchemaNode:
 *
 * A reference counted structure which represents a single schema or subschema.
 *
 * All the fields in the #WblSchemaNode structure are private and should never
 * be accessed directly.
 *
 * Since: 0.1.0
 */
typedef struct _WblSchemaNode WblSchemaNode;

GType wbl_schema_node_get_type (void) G_GNUC_CONST;

WblSchemaNode *wbl_schema_node_ref (WblSchemaNode *self);
void wbl_schema_node_unref (WblSchemaNode *self);

JsonObject *
wbl_schema_node_get_root (WblSchemaNode *self);
const gchar *
wbl_schema_node_get_title (WblSchemaNode *self);
const gchar *
wbl_schema_node_get_description (WblSchemaNode *self);
JsonNode *
wbl_schema_node_get_default (WblSchemaNode *self);

#define WBL_TYPE_SCHEMA			(wbl_schema_get_type ())
#define WBL_SCHEMA(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), WBL_TYPE_SCHEMA, WblSchema))
#define WBL_SCHEMA_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), WBL_TYPE_SCHEMA, WblSchemaClass))
#define WBL_IS_SCHEMA(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), WBL_TYPE_SCHEMA))
#define WBL_IS_SCHEMA_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), WBL_TYPE_SCHEMA))
#define WBL_SCHEMA_GET_CLASS(o)		(G_TYPE_INSTANCE_GET_CLASS ((o), WBL_TYPE_SCHEMA, WblSchemaClass))

typedef struct _WblSchemaPrivate	WblSchemaPrivate;

/**
 * WblSchema:
 *
 * All the fields in the #WblSchema structure are private and should never
 * be accessed directly.
 *
 * Since: 0.1.0
 */
typedef struct {
	/*< private >*/
	GObject parent;
} WblSchema;

/**
 * WblSchemaClass:
 * @validate_schema: Walk over a parsed schema and validate that it is a valid
 * schema. The default implementation checks against JSON Schema, but overriding
 * implementations could check extension keywords. If %NULL, no validation
 * will be performed. Information, warning and error messages are returned.
 * @apply_schema: Apply a parsed schema to a JSON instance, validating the
 *   instance against the schema. The default implementation applies standard
 *   JSON Schema keywords, but overriding implementations could implement extension
 *   keywords. If %NULL, no application will be performed.
 * @generate_instance_nodes: Generate a set of JSON instances which are valid
 *   and invalid for this JSON Schema. The default implementation generates for
 *   all standard JSON Schema keywords, but overriding implementations could
 *   generate for extension keywords. If %NULL, no instances will be generated.
 *
 * Most of the fields in the #WblSchemaClass structure are private and should
 * never be accessed directly.
 *
 * Since: 0.1.0
 */
typedef struct {
	/*< private >*/
	GObjectClass parent;

	/*< public >*/
	GPtrArray/*<owned WblValidateMessage>*/ *(*validate_schema) (WblSchema *self,
	                                                             WblSchemaNode *root,
	                                                             GError **error);
	void (*apply_schema) (WblSchema *self,
	                      WblSchemaNode *root,
	                      JsonNode *instance,
	                      GError **error);
	GHashTable/*<owned JsonNode>*/ *(*generate_instance_nodes) (WblSchema      *self,
	                                                            WblSchemaNode  *root);
} WblSchemaClass;

GType wbl_schema_get_type (void) G_GNUC_CONST;

WblSchema *wbl_schema_new (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

void wbl_schema_load_from_data (WblSchema *self,
                                const gchar *data,
                                gssize length,
                                GError **error);

void wbl_schema_load_from_file (WblSchema *self,
                                const gchar *filename,
                                GError **error);

void wbl_schema_load_from_stream (WblSchema *self,
                                  GInputStream *stream,
                                  GCancellable *cancellable,
                                  GError **error);
void wbl_schema_load_from_stream_async (WblSchema *self,
                                        GInputStream *stream,
                                        GCancellable *cancellable,
                                        GAsyncReadyCallback callback,
                                        gpointer user_data);
void wbl_schema_load_from_stream_finish (WblSchema *self,
                                         GAsyncResult *result,
                                         GError **error);

void wbl_schema_load_from_json (WblSchema     *self,
                                JsonNode      *root,
                                GCancellable  *cancellable,
                                GError       **error);

WblSchemaNode *
wbl_schema_get_root (WblSchema *self);
GPtrArray *
wbl_schema_get_validation_messages (WblSchema *self);

void
wbl_schema_apply (WblSchema *self,
                  JsonNode *instance,
                  GError **error);

/**
 * WblValidateMessageLevel:
 * @WBL_VALIDATE_MESSAGE_ERROR: Error message.
 *
 * Severity levels of messages from the validation process for a JSON Schema.
 *
 * Since: UNRELEASED
 */
typedef enum {
	WBL_VALIDATE_MESSAGE_ERROR = 0,
} WblValidateMessageLevel;

/**
 * WBL_SCHEMA_CORE:
 *
 * Canonical name for the JSON Schema Core standard.
 *
 * Since: UNRELEASED
 */
#define WBL_SCHEMA_CORE "json-schema-core"

/**
 * WBL_SCHEMA_VALIDATION:
 *
 * Canonical name for the JSON Schema Validation standard.
 *
 * Since: UNRELEASED
 */
#define WBL_SCHEMA_VALIDATION "json-schema-validation"

/**
 * WblValidateMessage:
 *
 * An allocated structure which represents a message from the validation process
 * for a JSON Schema. This may be an error message (for an invalid schema), or
 * an informational or warning message. Associated metadata, such as the
 * relevant part of the JSON Schema standard, and the location in the schema
 * input which caused the message, are included.
 *
 * All the fields in the #WblValidateMessage structure are private and should
 * never be accessed directly.
 *
 * Since: UNRELEASED
 */
typedef struct _WblValidateMessage WblValidateMessage;

GType
wbl_validate_message_get_type (void) G_GNUC_CONST;

WblValidateMessage *
wbl_validate_message_copy (WblValidateMessage *self);
void
wbl_validate_message_free (WblValidateMessage *self);

gchar *wbl_validate_message_build_specification_link (WblValidateMessage *self);
const gchar *wbl_validate_message_get_path (WblValidateMessage *self);
WblValidateMessageLevel wbl_validate_message_get_level (WblValidateMessage *self);
const gchar *wbl_validate_message_get_message (WblValidateMessage *self);
GPtrArray *wbl_validate_message_get_sub_messages (WblValidateMessage *self);
gchar *wbl_validate_message_build_json (WblValidateMessage *self);

/**
 * WblGenerateInstanceFlags:
 * @WBL_GENERATE_INSTANCE_NONE: No flags set.
 * @WBL_GENERATE_INSTANCE_IGNORE_VALID: Do not return valid instances.
 * @WBL_GENERATE_INSTANCE_IGNORE_INVALID: Do not return invalid instances.
 * @WBL_GENERATE_INSTANCE_INVALID_JSON: Generate a test vector containing
 *    invalid JSON. (Since: 0.2.0)
 *
 * Flags affecting the generation of JSON instances for schemas using
 * wbl_schema_generate_instances().
 *
 * Since: 0.1.0
 */
typedef enum {
	WBL_GENERATE_INSTANCE_NONE = 0,
	WBL_GENERATE_INSTANCE_IGNORE_VALID = (1 << 0),
	WBL_GENERATE_INSTANCE_IGNORE_INVALID = (1 << 1),
	WBL_GENERATE_INSTANCE_INVALID_JSON = (1 << 2),
} WblGenerateInstanceFlags;

/**
 * WblGeneratedInstance:
 *
 * An allocated structure which represents a generated JSON instance which may
 * be valid for a given JSON Schema. Associated metadata is stored with the
 * instance, such as whether it is expected to be valid.
 *
 * All the fields in the #WblGeneratedInstance structure are private and should
 * never be accessed directly.
 *
 * Since: 0.1.0
 */
typedef struct _WblGeneratedInstance WblGeneratedInstance;

GType
wbl_generated_instance_get_type (void) G_GNUC_CONST;

WblGeneratedInstance *
wbl_generated_instance_new_from_string (const gchar *json,
                                        gboolean valid);
WblGeneratedInstance *
wbl_generated_instance_copy (WblGeneratedInstance *self);
void
wbl_generated_instance_free (WblGeneratedInstance *self);

const gchar *
wbl_generated_instance_get_json (WblGeneratedInstance *self);

gboolean
wbl_generated_instance_is_valid (WblGeneratedInstance *self);

GPtrArray *wbl_schema_generate_instances (WblSchema *self, WblGenerateInstanceFlags flags);

/**
 * WblSchemaInfo:
 *
 * An allocated structure which stores debugging and timing information about a
 * particular schema or sub-schema, which might be useful in optimising the
 * schema file design.
 *
 * All the fields in the #WblSchemaInfo structure are private and should never
 * be accessed directly.
 *
 * Since: UNRELEASED
 */
typedef struct _WblSchemaInfo WblSchemaInfo;

GType wbl_schema_info_get_type (void) G_GNUC_CONST;

WblSchemaInfo *wbl_schema_info_copy (WblSchemaInfo *self);
void wbl_schema_info_free (WblSchemaInfo *self);

gint64 wbl_schema_info_get_generation_time (WblSchemaInfo *self);
guint wbl_schema_info_get_n_times_generated (WblSchemaInfo *self);
guint wbl_schema_info_get_id (WblSchemaInfo *self);
guint wbl_schema_info_get_n_instances_generated (WblSchemaInfo *self);
gchar *wbl_schema_info_build_json (WblSchemaInfo *self);

GPtrArray *wbl_schema_get_schema_info (WblSchema *self);

G_END_DECLS

#endif /* !WBL_SCHEMA_H */
