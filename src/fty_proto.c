/*  =========================================================================
    fty_proto - Core FTY protocols

    Codec class for fty_proto.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: fty_proto.xml, or
     * The code generation script that built this file: zproto_codec_c_v1
    ************************************************************************
    Copyright (C) 2014 - 2020 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

/*
@header
    fty_proto - Core FTY protocols
@discuss
@end
*/

#include "../include/fty_proto.h"

//  Structure of our class

struct _fty_proto_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  fty_proto message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
    zhash_t *aux;                       //  aux
    size_t aux_bytes;                   //  aux
    uint64_t time;                      //  Metric timestamp -
    uint32_t ttl;                       //  Time to live in seconds.
    char *type;                         //  Metric name, e.g.: "temperature", "humidity", "power.load", ...
    char *name;                         //  Name of asset where metric is produced.
    char *value;                        //  Metric value, e.g.: "25.323" or "900".
    char *unit;                         //  Metric unit, e.g.: "C" or "F" for temperature.
    char *rule;                         //  Name of the rule which triggers this alert.
    char *state;                        //  Alert state.
    char *severity;                     //  Alert severity.
    char *description;                  //  Alert description.
    char *metadata;                     //  Alert metadata (opaque string payload, optional).
    zlist_t *action;                    //  List of actions, e.g.: "EMAIL", "SMS".
    char *operation;                    //  Asset operation.
    zhash_t *ext;                       //  Additional extended information.
    size_t ext_bytes;                   //  Additional extended information.
};

//  --------------------------------------------------------------------------
//  Network data encoding macros

//  Put a block of octets to the frame
#define PUT_OCTETS(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
}

//  Get a block of octets from the frame
#define GET_OCTETS(host,size) { \
    if (self->needle + size > self->ceiling) \
        goto malformed; \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (byte) (host); \
    self->needle++; \
}

//  Put a 2-byte number to the frame
#define PUT_NUMBER2(host) { \
    self->needle [0] = (byte) (((host) >> 8)  & 255); \
    self->needle [1] = (byte) (((host))       & 255); \
    self->needle += 2; \
}

//  Put a 4-byte number to the frame
#define PUT_NUMBER4(host) { \
    self->needle [0] = (byte) (((host) >> 24) & 255); \
    self->needle [1] = (byte) (((host) >> 16) & 255); \
    self->needle [2] = (byte) (((host) >> 8)  & 255); \
    self->needle [3] = (byte) (((host))       & 255); \
    self->needle += 4; \
}

//  Put a 8-byte number to the frame
#define PUT_NUMBER8(host) { \
    self->needle [0] = (byte) (((host) >> 56) & 255); \
    self->needle [1] = (byte) (((host) >> 48) & 255); \
    self->needle [2] = (byte) (((host) >> 40) & 255); \
    self->needle [3] = (byte) (((host) >> 32) & 255); \
    self->needle [4] = (byte) (((host) >> 24) & 255); \
    self->needle [5] = (byte) (((host) >> 16) & 255); \
    self->needle [6] = (byte) (((host) >> 8)  & 255); \
    self->needle [7] = (byte) (((host))       & 255); \
    self->needle += 8; \
}

//  Get a 1-byte number from the frame
#define GET_NUMBER1(host) { \
    if (self->needle + 1 > self->ceiling) \
        goto malformed; \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) \
        goto malformed; \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) \
        goto malformed; \
    (host) = ((uint32_t) (self->needle [0]) << 24) \
           + ((uint32_t) (self->needle [1]) << 16) \
           + ((uint32_t) (self->needle [2]) << 8) \
           +  (uint32_t) (self->needle [3]); \
    self->needle += 4; \
}

//  Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) \
        goto malformed; \
    (host) = ((uint64_t) (self->needle [0]) << 56) \
           + ((uint64_t) (self->needle [1]) << 48) \
           + ((uint64_t) (self->needle [2]) << 40) \
           + ((uint64_t) (self->needle [3]) << 32) \
           + ((uint64_t) (self->needle [4]) << 24) \
           + ((uint64_t) (self->needle [5]) << 16) \
           + ((uint64_t) (self->needle [6]) << 8) \
           +  (uint64_t) (self->needle [7]); \
    self->needle += 8; \
}

//  Put a string to the frame
#define PUT_STRING(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER1 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a string from the frame
#define GET_STRING(host) { \
    size_t string_size; \
    GET_NUMBER1 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

//  Put a long string to the frame
#define PUT_LONGSTR(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER4 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a long string from the frame
#define GET_LONGSTR(host) { \
    size_t string_size; \
    GET_NUMBER4 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

//  --------------------------------------------------------------------------
//  bytes cstring conversion macros

// create new array of unsigned char from properly encoded string
// len of resulting array is strlen (str) / 2
// caller is responsibe for freeing up the memory
#define BYTES_FROM_STR(dst, _str) { \
    char *str = (char*) (_str); \
    if (!str || (strlen (str) % 2) != 0) \
        return NULL; \
\
    size_t strlen_2 = strlen (str) / 2; \
    byte *mem = (byte*) zmalloc (strlen_2); \
    size_t i; \
\
    for (i = 0; i != strlen_2; i++) \
    { \
        char buff[3] = {0x0, 0x0, 0x0}; \
        strncpy (buff, str, 2); \
        unsigned int uint; \
        sscanf (buff, "%x", &uint); \
        assert (uint <= 0xff); \
        mem [i] = (byte) (0xff & uint); \
        str += 2; \
    } \
    dst = mem; \
}

// convert len bytes to hex string
// caller is responsibe for freeing up the memory
#define STR_FROM_BYTES(dst, _mem, _len) { \
    byte *mem = (byte*) (_mem); \
    size_t len = (size_t) (_len); \
    char* ret = (char*) zmalloc (2*len + 1); \
    char* aux = ret; \
    size_t i; \
    for (i = 0; i != len; i++) \
    { \
        sprintf (aux, "%02x", mem [i]); \
        aux+=2; \
    } \
    dst = ret; \
}



//  --------------------------------------------------------------------------
//  Create a new fty_proto

fty_proto_t *
fty_proto_new (int id)
{
    fty_proto_t *self = (fty_proto_t *) zmalloc (sizeof (fty_proto_t));
    self->id = id;
    return self;
}

//  --------------------------------------------------------------------------
//  Create a new fty_proto from zpl/zconfig_t *

fty_proto_t *
    fty_proto_new_zpl (zconfig_t *config)
{
    assert (config);
    fty_proto_t *self = NULL;
    char *message = zconfig_get (config, "message", NULL);
    if (!message) {
        zsys_error ("Can't find 'message' section");
        return NULL;
    }

    if (streq ("FTY_PROTO_METRIC", message))
        self = fty_proto_new (FTY_PROTO_METRIC);
    else
    if (streq ("FTY_PROTO_ALERT", message))
        self = fty_proto_new (FTY_PROTO_ALERT);
    else
    if (streq ("FTY_PROTO_ASSET", message))
        self = fty_proto_new (FTY_PROTO_ASSET);
    else
       {
        zsys_error ("message=%s is not known", message);
        return NULL;
       }

    char *s = zconfig_get (config, "routing_id", NULL);
    if (s) {
        byte *bvalue;
        BYTES_FROM_STR (bvalue, s);
        if (!bvalue) {
            fty_proto_destroy (&self);
            return NULL;
        }
        zframe_t *frame = zframe_new (bvalue, strlen (s) / 2);
        free (bvalue);
        self->routing_id = frame;
    }

    zconfig_t *content = zconfig_locate (config, "content");
    if (!content) {
        zsys_error ("Can't find 'content' section");
        return NULL;
    }
    switch (self->id) {
        case FTY_PROTO_METRIC:
            {
            zconfig_t *zhash = zconfig_locate (content, "aux");
            if (zhash) {
                zhash_t *hash = zhash_new ();
                zhash_autofree (hash);
                for (zconfig_t *child = zconfig_child (zhash);
                                child != NULL;
                                child = zconfig_next (child))
                {
                    zhash_update (hash, zconfig_name (child), zconfig_value (child));
                }
                self->aux = hash;
            }
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "time", NULL);
            if (!s) {
                zsys_error ("content/time not found");
                fty_proto_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/time: %s is not a number", s);
                fty_proto_destroy (&self);
                return NULL;
            }
            self->time = uvalue;
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "ttl", NULL);
            if (!s) {
                zsys_error ("content/ttl not found");
                fty_proto_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/ttl: %s is not a number", s);
                fty_proto_destroy (&self);
                return NULL;
            }
            self->ttl = uvalue;
            }
            {
            char *s = zconfig_get (content, "type", NULL);
            if (!s) {
                fty_proto_destroy (&self);
                return NULL;
            }
            self->type = strdup (s);
            }
            {
            char *s = zconfig_get (content, "name", NULL);
            if (!s) {
                fty_proto_destroy (&self);
                return NULL;
            }
            self->name = strdup (s);
            }
            {
            char *s = zconfig_get (content, "value", NULL);
            if (!s) {
                fty_proto_destroy (&self);
                return NULL;
            }
            self->value = strdup (s);
            }
            {
            char *s = zconfig_get (content, "unit", NULL);
            if (!s) {
                fty_proto_destroy (&self);
                return NULL;
            }
            self->unit = strdup (s);
            }
            break;
        case FTY_PROTO_ALERT:
            {
            zconfig_t *zhash = zconfig_locate (content, "aux");
            if (zhash) {
                zhash_t *hash = zhash_new ();
                zhash_autofree (hash);
                for (zconfig_t *child = zconfig_child (zhash);
                                child != NULL;
                                child = zconfig_next (child))
                {
                    zhash_update (hash, zconfig_name (child), zconfig_value (child));
                }
                self->aux = hash;
            }
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "time", NULL);
            if (!s) {
                zsys_error ("content/time not found");
                fty_proto_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/time: %s is not a number", s);
                fty_proto_destroy (&self);
                return NULL;
            }
            self->time = uvalue;
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "ttl", NULL);
            if (!s) {
                zsys_error ("content/ttl not found");
                fty_proto_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/ttl: %s is not a number", s);
                fty_proto_destroy (&self);
                return NULL;
            }
            self->ttl = uvalue;
            }
            {
            char *s = zconfig_get (content, "rule", NULL);
            if (!s) {
                fty_proto_destroy (&self);
                return NULL;
            }
            self->rule = strdup (s);
            }
            {
            char *s = zconfig_get (content, "name", NULL);
            if (!s) {
                fty_proto_destroy (&self);
                return NULL;
            }
            self->name = strdup (s);
            }
            {
            char *s = zconfig_get (content, "state", NULL);
            if (!s) {
                fty_proto_destroy (&self);
                return NULL;
            }
            self->state = strdup (s);
            }
            {
            char *s = zconfig_get (content, "severity", NULL);
            if (!s) {
                fty_proto_destroy (&self);
                return NULL;
            }
            self->severity = strdup (s);
            }
            {
            char *s = zconfig_get (content, "description", NULL);
            if (!s) {
                fty_proto_destroy (&self);
                return NULL;
            }
            self->description = strdup (s);
            }
            {
            char *s = zconfig_get (content, "metadata", NULL);
            self->metadata = strdup (s ? s : ""); // optional (backward compatibility)
            }
            {
            zconfig_t *zstrings = zconfig_locate (content, "action");
            if (zstrings) {
                zlist_t *strings = zlist_new ();
                zlist_autofree (strings);
                for (zconfig_t *child = zconfig_child (zstrings);
                                child != NULL;
                                child = zconfig_next (child))
                {
                    zlist_append (strings, zconfig_value (child));
                }
                self->action = strings;
            }
            }
            break;
        case FTY_PROTO_ASSET:
            {
            zconfig_t *zhash = zconfig_locate (content, "aux");
            if (zhash) {
                zhash_t *hash = zhash_new ();
                zhash_autofree (hash);
                for (zconfig_t *child = zconfig_child (zhash);
                                child != NULL;
                                child = zconfig_next (child))
                {
                    zhash_update (hash, zconfig_name (child), zconfig_value (child));
                }
                self->aux = hash;
            }
            }
            {
            char *s = zconfig_get (content, "name", NULL);
            if (!s) {
                fty_proto_destroy (&self);
                return NULL;
            }
            self->name = strdup (s);
            }
            {
            char *s = zconfig_get (content, "operation", NULL);
            if (!s) {
                fty_proto_destroy (&self);
                return NULL;
            }
            self->operation = strdup (s);
            }
            {
            zconfig_t *zhash = zconfig_locate (content, "ext");
            if (zhash) {
                zhash_t *hash = zhash_new ();
                zhash_autofree (hash);
                for (zconfig_t *child = zconfig_child (zhash);
                                child != NULL;
                                child = zconfig_next (child))
                {
                    zhash_update (hash, zconfig_name (child), zconfig_value (child));
                }
                self->ext = hash;
            }
            }
            break;
    }
    return self;
}

//  --------------------------------------------------------------------------
//  Destroy the fty_proto

void
fty_proto_destroy (fty_proto_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        fty_proto_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        zhash_destroy (&self->aux);
        free (self->type);
        free (self->name);
        free (self->value);
        free (self->unit);
        free (self->rule);
        free (self->state);
        free (self->severity);
        free (self->description);
        zstr_free (&self->metadata);
        if (self->action)
            zlist_destroy (&self->action);
        free (self->operation);
        zhash_destroy (&self->ext);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

//  Parses a zmsg_t and decides whether it carries a fty_proto.
//  Returns true if it does, false otherwise.
//  Doesn't destroy or modify the original message.
bool
fty_proto_is (zmsg_t *msg)
{
    if (msg == NULL)
        return false;
    assert (zmsg_is (msg));

    zframe_t *frame = zmsg_first (msg);
    if (frame == NULL)
        return false;

    //  Get and check protocol signature
    fty_proto_t *self = fty_proto_new (0);
    self->needle = zframe_data (frame);
    self->ceiling = self->needle + zframe_size (frame);
    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 1))
        goto fail;             //  Invalid signature

    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case FTY_PROTO_METRIC:
        case FTY_PROTO_ALERT:
        case FTY_PROTO_ASSET:
            fty_proto_destroy (&self);
            return true;
        default:
            goto fail;
    }
    fail:
    malformed:
        fty_proto_destroy (&self);
        return false;
}

//  --------------------------------------------------------------------------
//  Parse a fty_proto from zmsg_t. Returns a new object, or NULL if
//  the message could not be parsed, or was NULL. Destroys msg and
//  nullifies the msg reference.

fty_proto_t *
fty_proto_decode (zmsg_t **msg_p)
{
    assert (msg_p);
    zmsg_t *msg = *msg_p;
    if (msg == NULL)
        return NULL;

    fty_proto_t *self = fty_proto_new (0);
    //  Read and parse command in frame
    zframe_t *frame = zmsg_pop (msg);
    if (!frame)
        goto empty;             //  Malformed or empty

    //  Get and check protocol signature
    self->needle = zframe_data (frame);
    self->ceiling = self->needle + zframe_size (frame);
    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 1))
        goto empty;             //  Invalid signature

    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case FTY_PROTO_METRIC:
            {
                size_t hash_size;
                GET_NUMBER4 (hash_size);
                self->aux = zhash_new ();
                zhash_autofree (self->aux);
                while (hash_size--) {
                    char *key, *value;
                    GET_STRING (key);
                    GET_LONGSTR (value);
                    zhash_insert (self->aux, key, value);
                    free (key);
                    free (value);
                }
            }
            GET_NUMBER8 (self->time);
            GET_NUMBER4 (self->ttl);
            GET_STRING (self->type);
            GET_STRING (self->name);
            GET_STRING (self->value);
            GET_STRING (self->unit);
            break;

        case FTY_PROTO_ALERT:
            {
                size_t hash_size;
                GET_NUMBER4 (hash_size);
                self->aux = zhash_new ();
                zhash_autofree (self->aux);
                while (hash_size--) {
                    char *key, *value;
                    GET_STRING (key);
                    GET_LONGSTR (value);
                    zhash_insert (self->aux, key, value);
                    free (key);
                    free (value);
                }
            }
            GET_NUMBER8 (self->time);
            GET_NUMBER4 (self->ttl);
            GET_STRING (self->rule);
            GET_STRING (self->name);
            GET_STRING (self->state);
            GET_STRING (self->severity);
            GET_STRING (self->description);
            {
                size_t list_size;
                GET_NUMBER4 (list_size);
                self->action = zlist_new ();
                zlist_autofree (self->action);
                while (list_size--) {
                    char *string;
                    GET_LONGSTR (string);
                    zlist_append (self->action, string);
                    free (string);
                }
            }

            // handle backward compatibility
            // **shall** be removed when the lib will be fully integrated
            if ((self->needle + 4) <= self->ceiling) {
                GET_LONGSTR (self->metadata); // << keep that
            }
            else {
                self->metadata = strdup(""); // empty
            }
            break;

        case FTY_PROTO_ASSET:
            {
                size_t hash_size;
                GET_NUMBER4 (hash_size);
                self->aux = zhash_new ();
                zhash_autofree (self->aux);
                while (hash_size--) {
                    char *key, *value;
                    GET_STRING (key);
                    GET_LONGSTR (value);
                    zhash_insert (self->aux, key, value);
                    free (key);
                    free (value);
                }
            }
            GET_STRING (self->name);
            GET_STRING (self->operation);
            {
                size_t hash_size;
                GET_NUMBER4 (hash_size);
                self->ext = zhash_new ();
                zhash_autofree (self->ext);
                while (hash_size--) {
                    char *key, *value;
                    GET_STRING (key);
                    GET_LONGSTR (value);
                    zhash_insert (self->ext, key, value);
                    free (key);
                    free (value);
                }
            }
            break;

        default:
            goto malformed;
    }
    //  Successful return
    zframe_destroy (&frame);
    zmsg_destroy (msg_p);
    return self;

    //  Error returns
    malformed:
        zsys_error ("malformed message '%d'\n", self->id);
    empty:
        zframe_destroy (&frame);
        zmsg_destroy (msg_p);
        fty_proto_destroy (&self);
        return (NULL);
}


//  --------------------------------------------------------------------------
//  Encode fty_proto into zmsg and destroy it. Returns a newly created
//  object or NULL if error. Use when not in control of sending the message.

zmsg_t *
fty_proto_encode (fty_proto_t **self_p)
{
    assert (self_p);
    assert (*self_p);

    fty_proto_t *self = *self_p;
    zmsg_t *msg = zmsg_new ();

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case FTY_PROTO_METRIC:
            //  aux is an array of key=value strings
            frame_size += 4;    //  Size is 4 octets
            if (self->aux) {
                self->aux_bytes = 0;
                //  Add up size of dictionary contents
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    self->aux_bytes += 1 + strlen ((const char *) zhash_cursor (self->aux));
                    self->aux_bytes += 4 + strlen (item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            frame_size += self->aux_bytes;
            //  time is a 8-byte integer
            frame_size += 8;
            //  ttl is a 4-byte integer
            frame_size += 4;
            //  type is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->type)
                frame_size += strlen (self->type);
            //  name is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->name)
                frame_size += strlen (self->name);
            //  value is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->value)
                frame_size += strlen (self->value);
            //  unit is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->unit)
                frame_size += strlen (self->unit);
            break;

        case FTY_PROTO_ALERT:
            //  aux is an array of key=value strings
            frame_size += 4;    //  Size is 4 octets
            if (self->aux) {
                self->aux_bytes = 0;
                //  Add up size of dictionary contents
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    self->aux_bytes += 1 + strlen ((const char *) zhash_cursor (self->aux));
                    self->aux_bytes += 4 + strlen (item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            frame_size += self->aux_bytes;
            //  time is a 8-byte integer
            frame_size += 8;
            //  ttl is a 4-byte integer
            frame_size += 4;
            //  rule is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->rule)
                frame_size += strlen (self->rule);
            //  name is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->name)
                frame_size += strlen (self->name);
            //  state is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->state)
                frame_size += strlen (self->state);
            //  severity is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->severity)
                frame_size += strlen (self->severity);
            //  description is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->description)
                frame_size += strlen (self->description);
            //  action is an array of strings
            frame_size += 4;    //  Size is 4 octets
            if (self->action) {
                //  Add up size of list contents
                char *action = (char *) zlist_first (self->action);
                while (action) {
                    frame_size += 4 + strlen (action);
                    action = (char *) zlist_next (self->action);
                }
            }
            //  metadata is a long string with 4-byte length
            frame_size += 4;       //  Size is 4 octet
            if (self->metadata)
                frame_size += strlen (self->metadata);
            break;

        case FTY_PROTO_ASSET:
            //  aux is an array of key=value strings
            frame_size += 4;    //  Size is 4 octets
            if (self->aux) {
                self->aux_bytes = 0;
                //  Add up size of dictionary contents
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    self->aux_bytes += 1 + strlen ((const char *) zhash_cursor (self->aux));
                    self->aux_bytes += 4 + strlen (item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            frame_size += self->aux_bytes;
            //  name is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->name)
                frame_size += strlen (self->name);
            //  operation is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->operation)
                frame_size += strlen (self->operation);
            //  ext is an array of key=value strings
            frame_size += 4;    //  Size is 4 octets
            if (self->ext) {
                self->ext_bytes = 0;
                //  Add up size of dictionary contents
                char *item = (char *) zhash_first (self->ext);
                while (item) {
                    self->ext_bytes += 1 + strlen ((const char *) zhash_cursor (self->ext));
                    self->ext_bytes += 4 + strlen (item);
                    item = (char *) zhash_next (self->ext);
                }
            }
            frame_size += self->ext_bytes;
            break;

        default:
            zsys_error ("bad message type '%d', not sent\n", self->id);
            //  No recovery, this is a fatal application error
            assert (false);
    }
    //  Now serialize message into the frame
    zframe_t *frame = zframe_new (NULL, frame_size);
    self->needle = zframe_data (frame);
    PUT_NUMBER2 (0xAAA0 | 1);
    PUT_NUMBER1 (self->id);

    switch (self->id) {
        case FTY_PROTO_METRIC:
            if (self->aux) {
                PUT_NUMBER4 (zhash_size (self->aux));
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    PUT_STRING ((const char *) zhash_cursor ((zhash_t *) self->aux));
                    PUT_LONGSTR (item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty dictionary
            PUT_NUMBER8 (self->time);
            PUT_NUMBER4 (self->ttl);
            if (self->type) {
                PUT_STRING (self->type);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->name) {
                PUT_STRING (self->name);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->value) {
                PUT_STRING (self->value);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->unit) {
                PUT_STRING (self->unit);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case FTY_PROTO_ALERT:
            if (self->aux) {
                PUT_NUMBER4 (zhash_size (self->aux));
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    PUT_STRING ((const char *) zhash_cursor ((zhash_t *) self->aux));
                    PUT_LONGSTR (item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty dictionary
            PUT_NUMBER8 (self->time);
            PUT_NUMBER4 (self->ttl);
            if (self->rule) {
                PUT_STRING (self->rule);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->name) {
                PUT_STRING (self->name);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->state) {
                PUT_STRING (self->state);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->severity) {
                PUT_STRING (self->severity);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->description) {
                PUT_STRING (self->description);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->action) {
                PUT_NUMBER4 (zlist_size (self->action));
                char *action = (char *) zlist_first (self->action);
                while (action) {
                    PUT_LONGSTR (action);
                    action = (char *) zlist_next (self->action);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty string array
            if (self->metadata) {
                PUT_LONGSTR (self->metadata);
            }
            else {
                PUT_NUMBER4 (0);    //  Empty string
            }
            break;

        case FTY_PROTO_ASSET:
            if (self->aux) {
                PUT_NUMBER4 (zhash_size (self->aux));
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    PUT_STRING ((const char *) zhash_cursor ((zhash_t *) self->aux));
                    PUT_LONGSTR (item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty dictionary
            if (self->name) {
                PUT_STRING (self->name);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->operation) {
                PUT_STRING (self->operation);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->ext) {
                PUT_NUMBER4 (zhash_size (self->ext));
                char *item = (char *) zhash_first (self->ext);
                while (item) {
                    PUT_STRING ((const char *) zhash_cursor ((zhash_t *) self->ext));
                    PUT_LONGSTR (item);
                    item = (char *) zhash_next (self->ext);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty dictionary
            break;

    }
    //  Now send the data frame
    if (zmsg_append (msg, &frame)) {
        zmsg_destroy (&msg);
        fty_proto_destroy (self_p);
        return NULL;
    }
    //  Destroy fty_proto object
    fty_proto_destroy (self_p);
    return msg;
}


//  --------------------------------------------------------------------------
//  Receive and parse a fty_proto from the socket. Returns new object or
//  NULL if error. Will block if there's no message waiting.

fty_proto_t *
fty_proto_recv (void *input)
{
    assert (input);
    zmsg_t *msg = zmsg_recv (input);
    if (!msg)
        return NULL;            //  Interrupted

    //  If message came from a router socket, first frame is routing_id
    zframe_t *routing_id = NULL;
    if (zsock_type (zsock_resolve (input)) == ZMQ_ROUTER) {
        routing_id = zmsg_pop (msg);
        //  If message was not valid, forget about it
        if (!routing_id || !zmsg_next (msg))
            return NULL;        //  Malformed or empty
    }
    fty_proto_t *fty_proto = fty_proto_decode (&msg);
    if (fty_proto && zsock_type (zsock_resolve (input)) == ZMQ_ROUTER)
        fty_proto->routing_id = routing_id;

    return fty_proto;
}


//  --------------------------------------------------------------------------
//  Receive and parse a fty_proto from the socket. Returns new object,
//  or NULL either if there was no input waiting, or the recv was interrupted.

fty_proto_t *
fty_proto_recv_nowait (void *input)
{
    assert (input);
    zmsg_t *msg = zmsg_recv_nowait (input);
    if (!msg)
        return NULL;            //  Interrupted
    //  If message came from a router socket, first frame is routing_id
    zframe_t *routing_id = NULL;
    if (zsock_type (zsock_resolve (input)) == ZMQ_ROUTER) {
        routing_id = zmsg_pop (msg);
        //  If message was not valid, forget about it
        if (!routing_id || !zmsg_next (msg))
            return NULL;        //  Malformed or empty
    }
    fty_proto_t *fty_proto = fty_proto_decode (&msg);
    if (fty_proto && zsock_type (zsock_resolve (input)) == ZMQ_ROUTER)
        fty_proto->routing_id = routing_id;

    return fty_proto;
}


//  --------------------------------------------------------------------------
//  Send the fty_proto to the socket, and destroy it
//  Returns 0 if OK, else -1

int
fty_proto_send (fty_proto_t **self_p, void *output)
{
    assert (self_p);
    assert (*self_p);
    assert (output);

    //  Save routing_id if any, as encode will destroy it
    fty_proto_t *self = *self_p;
    zframe_t *routing_id = self->routing_id;
    self->routing_id = NULL;

    //  Encode fty_proto message to a single zmsg
    zmsg_t *msg = fty_proto_encode (self_p);

    //  If we're sending to a ROUTER, send the routing_id first
    if (zsock_type (zsock_resolve (output)) == ZMQ_ROUTER) {
        assert (routing_id);
        zmsg_prepend (msg, &routing_id);
    }
    else
        zframe_destroy (&routing_id);

    if (msg && zmsg_send (&msg, output) == 0)
        return 0;
    else
        return -1;              //  Failed to encode, or send
}


//  --------------------------------------------------------------------------
//  Send the fty_proto to the output, and do not destroy it

int
fty_proto_send_again (fty_proto_t *self, void *output)
{
    assert (self);
    assert (output);
    self = fty_proto_dup (self);
    return fty_proto_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Encode METRIC message

zmsg_t *
fty_proto_encode_metric (
    zhash_t *aux,
    uint64_t time,
    uint32_t ttl,
    const char *type,
    const char *name,
    const char *value,
    const char *unit)
{
    fty_proto_t *self = fty_proto_new (FTY_PROTO_METRIC);
    zhash_t *aux_copy = zhash_dup (aux);
    fty_proto_set_aux (self, &aux_copy);
    fty_proto_set_time (self, time);
    fty_proto_set_ttl (self, ttl);
    fty_proto_set_type (self, "%s", type);
    fty_proto_set_name (self, "%s", name);
    fty_proto_set_value (self, "%s", value);
    fty_proto_set_unit (self, "%s", unit);
    return fty_proto_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode ALERT message

zmsg_t *
fty_proto_encode_alert (
    zhash_t *aux,
    uint64_t time,
    uint32_t ttl,
    const char *rule,
    const char *name,
    const char *state,
    const char *severity,
    const char *description,
    zlist_t *action)
{
    fty_proto_t *self = fty_proto_new (FTY_PROTO_ALERT);
    zhash_t *aux_copy = zhash_dup (aux);
    fty_proto_set_aux (self, &aux_copy);
    fty_proto_set_time (self, time);
    fty_proto_set_ttl (self, ttl);
    fty_proto_set_rule (self, "%s", rule);
    fty_proto_set_name (self, "%s", name);
    fty_proto_set_state (self, "%s", state);
    fty_proto_set_severity (self, "%s", severity);
    fty_proto_set_description (self, "%s", description);
    zlist_t *action_copy = zlist_dup (action);
    fty_proto_set_action (self, &action_copy);
    fty_proto_set_metadata(self, "%s", "");
    return fty_proto_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode ASSET message

zmsg_t *
fty_proto_encode_asset (
    zhash_t *aux,
    const char *name,
    const char *operation,
    zhash_t *ext)
{
    fty_proto_t *self = fty_proto_new (FTY_PROTO_ASSET);
    zhash_t *aux_copy = zhash_dup (aux);
    fty_proto_set_aux (self, &aux_copy);
    fty_proto_set_name (self, "%s", name);
    fty_proto_set_operation (self, "%s", operation);
    zhash_t *ext_copy = zhash_dup (ext);
    fty_proto_set_ext (self, &ext_copy);
    return fty_proto_encode (&self);
}


//  --------------------------------------------------------------------------
//  Send the METRIC to the socket in one step

int
fty_proto_send_metric (
    void *output,
    zhash_t *aux,
    uint64_t time,
    uint32_t ttl,
    const char *type,
    const char *name,
    const char *value,
    const char *unit)
{
    fty_proto_t *self = fty_proto_new (FTY_PROTO_METRIC);
    zhash_t *aux_copy = zhash_dup (aux);
    fty_proto_set_aux (self, &aux_copy);
    fty_proto_set_time (self, time);
    fty_proto_set_ttl (self, ttl);
    fty_proto_set_type (self, "%s", type);
    fty_proto_set_name (self, "%s", name);
    fty_proto_set_value (self, "%s", value);
    fty_proto_set_unit (self, "%s", unit);
    return fty_proto_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the ALERT to the socket in one step

int
fty_proto_send_alert (
    void *output,
    zhash_t *aux,
    uint64_t time,
    uint32_t ttl,
    const char *rule,
    const char *name,
    const char *state,
    const char *severity,
    const char *description,
    zlist_t *action)
{
    fty_proto_t *self = fty_proto_new (FTY_PROTO_ALERT);
    zhash_t *aux_copy = zhash_dup (aux);
    fty_proto_set_aux (self, &aux_copy);
    fty_proto_set_time (self, time);
    fty_proto_set_ttl (self, ttl);
    fty_proto_set_rule (self, "%s", rule);
    fty_proto_set_name (self, "%s", name);
    fty_proto_set_state (self, "%s", state);
    fty_proto_set_severity (self, "%s", severity);
    fty_proto_set_description (self, "%s", description);
    zlist_t *action_copy = zlist_dup (action);
    fty_proto_set_action (self, &action_copy);
    return fty_proto_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the ASSET to the socket in one step

int
fty_proto_send_asset (
    void *output,
    zhash_t *aux,
    const char *name,
    const char *operation,
    zhash_t *ext)
{
    fty_proto_t *self = fty_proto_new (FTY_PROTO_ASSET);
    zhash_t *aux_copy = zhash_dup (aux);
    fty_proto_set_aux (self, &aux_copy);
    fty_proto_set_name (self, "%s", name);
    fty_proto_set_operation (self, "%s", operation);
    zhash_t *ext_copy = zhash_dup (ext);
    fty_proto_set_ext (self, &ext_copy);
    return fty_proto_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Duplicate the fty_proto message

fty_proto_t *
fty_proto_dup (fty_proto_t *self)
{
    if (!self)
        return NULL;

    fty_proto_t *copy = fty_proto_new (self->id);
    if (self->routing_id)
        copy->routing_id = zframe_dup (self->routing_id);
    switch (self->id) {
        case FTY_PROTO_METRIC:
            copy->aux = self->aux? zhash_dup (self->aux): NULL;
            copy->time = self->time;
            copy->ttl = self->ttl;
            copy->type = self->type? strdup (self->type): NULL;
            copy->name = self->name? strdup (self->name): NULL;
            copy->value = self->value? strdup (self->value): NULL;
            copy->unit = self->unit? strdup (self->unit): NULL;
            break;

        case FTY_PROTO_ALERT:
            copy->aux = self->aux? zhash_dup (self->aux): NULL;
            copy->time = self->time;
            copy->ttl = self->ttl;
            copy->rule = self->rule? strdup (self->rule): NULL;
            copy->name = self->name? strdup (self->name): NULL;
            copy->state = self->state? strdup (self->state): NULL;
            copy->severity = self->severity? strdup (self->severity): NULL;
            copy->description = self->description? strdup (self->description): NULL;
            copy->action = self->action? zlist_dup (self->action): NULL;
            copy->metadata = self->metadata? strdup (self->metadata): NULL;
            break;

        case FTY_PROTO_ASSET:
            copy->aux = self->aux? zhash_dup (self->aux): NULL;
            copy->name = self->name? strdup (self->name): NULL;
            copy->operation = self->operation? strdup (self->operation): NULL;
            copy->ext = self->ext? zhash_dup (self->ext): NULL;
            break;

    }
    return copy;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
fty_proto_print (fty_proto_t *self)
{
    assert (self);
    switch (self->id) {
        case FTY_PROTO_METRIC:
            zsys_debug ("FTY_PROTO_METRIC:");
            zsys_debug ("    aux=");
            if (self->aux) {
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    zsys_debug ("        %s=%s", zhash_cursor (self->aux), item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            else
                zsys_debug ("(NULL)");
            zsys_debug ("    time=%ld", (long) self->time);
            zsys_debug ("    ttl=%ld", (long) self->ttl);
            if (self->type)
                zsys_debug ("    type='%s'", self->type);
            else
                zsys_debug ("    type=");
            if (self->name)
                zsys_debug ("    name='%s'", self->name);
            else
                zsys_debug ("    name=");
            if (self->value)
                zsys_debug ("    value='%s'", self->value);
            else
                zsys_debug ("    value=");
            if (self->unit)
                zsys_debug ("    unit='%s'", self->unit);
            else
                zsys_debug ("    unit=");
            break;

        case FTY_PROTO_ALERT:
            zsys_debug ("FTY_PROTO_ALERT:");
            zsys_debug ("    aux=");
            if (self->aux) {
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    zsys_debug ("        %s=%s", zhash_cursor (self->aux), item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            else
                zsys_debug ("(NULL)");
            zsys_debug ("    time=%ld", (long) self->time);
            zsys_debug ("    ttl=%ld", (long) self->ttl);
            if (self->rule)
                zsys_debug ("    rule='%s'", self->rule);
            else
                zsys_debug ("    rule=");
            if (self->name)
                zsys_debug ("    name='%s'", self->name);
            else
                zsys_debug ("    name=");
            if (self->state)
                zsys_debug ("    state='%s'", self->state);
            else
                zsys_debug ("    state=");
            if (self->severity)
                zsys_debug ("    severity='%s'", self->severity);
            else
                zsys_debug ("    severity=");
            if (self->description)
                zsys_debug ("    description='%s'", self->description);
            else
                zsys_debug ("    description=");
            if (self->metadata)
                zsys_debug ("    metadata='%s'", self->metadata);
            else
                zsys_debug ("    metadata=");
            zsys_debug ("    action=");
            if (self->action) {
                char *action = (char *) zlist_first (self->action);
                while (action) {
                    zsys_debug ("        '%s'", action);
                    action = (char *) zlist_next (self->action);
                }
            }
            break;

        case FTY_PROTO_ASSET:
            zsys_debug ("FTY_PROTO_ASSET:");
            zsys_debug ("    aux=");
            if (self->aux) {
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    zsys_debug ("        %s=%s", zhash_cursor (self->aux), item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            else
                zsys_debug ("(NULL)");
            if (self->name)
                zsys_debug ("    name='%s'", self->name);
            else
                zsys_debug ("    name=");
            if (self->operation)
                zsys_debug ("    operation='%s'", self->operation);
            else
                zsys_debug ("    operation=");
            zsys_debug ("    ext=");
            if (self->ext) {
                char *item = (char *) zhash_first (self->ext);
                while (item) {
                    zsys_debug ("        %s=%s", zhash_cursor (self->ext), item);
                    item = (char *) zhash_next (self->ext);
                }
            }
            else
                zsys_debug ("(NULL)");
            break;

    }
}

//  --------------------------------------------------------------------------
//  Export class as zconfig_t*. Caller is responsibe for destroying the instance

zconfig_t *
fty_proto_zpl (fty_proto_t *self, zconfig_t *parent)
{
    assert (self);

    zconfig_t *root = zconfig_new ("fty_proto", parent);

    switch (self->id) {
        case FTY_PROTO_METRIC:
        {
            zconfig_put (root, "message", "FTY_PROTO_METRIC");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            if (self->aux) {
                zconfig_t *hash = zconfig_new ("aux", config);
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    zconfig_putf (hash, zhash_cursor (self->aux), "%s", item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            zconfig_putf (config, "time", "%ld", (long) self->time);
            zconfig_putf (config, "ttl", "%ld", (long) self->ttl);
            if (self->type)
                zconfig_putf (config, "type", "%s", self->type);
            if (self->name)
                zconfig_putf (config, "name", "%s", self->name);
            if (self->value)
                zconfig_putf (config, "value", "%s", self->value);
            if (self->unit)
                zconfig_putf (config, "unit", "%s", self->unit);
            break;
            }
        case FTY_PROTO_ALERT:
        {
            zconfig_put (root, "message", "FTY_PROTO_ALERT");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            if (self->aux) {
                zconfig_t *hash = zconfig_new ("aux", config);
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    zconfig_putf (hash, zhash_cursor (self->aux), "%s", item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            zconfig_putf (config, "time", "%ld", (long) self->time);
            zconfig_putf (config, "ttl", "%ld", (long) self->ttl);
            if (self->rule)
                zconfig_putf (config, "rule", "%s", self->rule);
            if (self->name)
                zconfig_putf (config, "name", "%s", self->name);
            if (self->state)
                zconfig_putf (config, "state", "%s", self->state);
            if (self->severity)
                zconfig_putf (config, "severity", "%s", self->severity);
            if (self->description)
                zconfig_putf (config, "description", "%s", self->description);
            if (self->metadata)
                zconfig_putf (config, "metadata", "%s", self->metadata);
            if (self->action) {
                zconfig_t *strings = zconfig_new ("action", config);
                size_t i = 0;
                char *action = (char *) zlist_first (self->action);
                while (action) {
                    char *key = zsys_sprintf ("%zu", i);
                    zconfig_putf (strings, key, "%s", action);
                    zstr_free (&key);
                    i++;
                    action = (char *) zlist_next (self->action);
                }
            }
            break;
            }
        case FTY_PROTO_ASSET:
        {
            zconfig_put (root, "message", "FTY_PROTO_ASSET");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            if (self->aux) {
                zconfig_t *hash = zconfig_new ("aux", config);
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    zconfig_putf (hash, zhash_cursor (self->aux), "%s", item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            if (self->name)
                zconfig_putf (config, "name", "%s", self->name);
            if (self->operation)
                zconfig_putf (config, "operation", "%s", self->operation);
            if (self->ext) {
                zconfig_t *hash = zconfig_new ("ext", config);
                char *item = (char *) zhash_first (self->ext);
                while (item) {
                    zconfig_putf (hash, zhash_cursor (self->ext), "%s", item);
                    item = (char *) zhash_next (self->ext);
                }
            }
            break;
            }
    }
    return root;
}


//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
fty_proto_routing_id (fty_proto_t *self)
{
    assert (self);
    return self->routing_id;
}

void
fty_proto_set_routing_id (fty_proto_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the fty_proto id

int
fty_proto_id (fty_proto_t *self)
{
    assert (self);
    return self->id;
}

void
fty_proto_set_id (fty_proto_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
fty_proto_command (fty_proto_t *self)
{
    assert (self);
    switch (self->id) {
        case FTY_PROTO_METRIC:
            return ("METRIC");
            break;
        case FTY_PROTO_ALERT:
            return ("ALERT");
            break;
        case FTY_PROTO_ASSET:
            return ("ASSET");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get the aux field without transferring ownership

zhash_t *
fty_proto_aux (fty_proto_t *self)
{
    assert (self);
    return self->aux;
}

//  Get the aux field and transfer ownership to caller

zhash_t *
fty_proto_get_aux (fty_proto_t *self)
{
    zhash_t *aux = self->aux;
    self->aux = NULL;
    return aux;
}

//  Set the aux field, transferring ownership from caller

void
fty_proto_set_aux (fty_proto_t *self, zhash_t **aux_p)
{
    assert (self);
    assert (aux_p);
    zhash_destroy (&self->aux);
    self->aux = *aux_p;
    *aux_p = NULL;
}

//  --------------------------------------------------------------------------
//  Get/set a value in the aux dictionary

const char *
fty_proto_aux_string (fty_proto_t *self, const char *key, const char *default_value)
{
    assert (self);
    const char *value = NULL;
    if (self->aux)
        value = (const char *) (zhash_lookup (self->aux, key));
    if (!value)
        value = default_value;

    return value;
}

uint64_t
fty_proto_aux_number (fty_proto_t *self, const char *key, uint64_t default_value)
{
    assert (self);
    uint64_t value = default_value;
    char *string = NULL;
    if (self->aux)
        string = (char *) (zhash_lookup (self->aux, key));
    if (string)
        value = atol (string);

    return value;
}

void
fty_proto_aux_insert (fty_proto_t *self, const char *key, const char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    //  Store string in hash table
    if (!self->aux) {
        self->aux = zhash_new ();
        zhash_autofree (self->aux);
    }
    zhash_update (self->aux, key, string);
    free (string);
}

size_t
fty_proto_aux_size (fty_proto_t *self)
{
    return zhash_size (self->aux);
}


//  --------------------------------------------------------------------------
//  Get/set the time field

uint64_t
fty_proto_time (fty_proto_t *self)
{
    assert (self);
    return self->time;
}

void
fty_proto_set_time (fty_proto_t *self, uint64_t time)
{
    assert (self);
    self->time = time;
}


//  --------------------------------------------------------------------------
//  Get/set the ttl field

uint32_t
fty_proto_ttl (fty_proto_t *self)
{
    assert (self);
    return self->ttl;
}

void
fty_proto_set_ttl (fty_proto_t *self, uint32_t ttl)
{
    assert (self);
    self->ttl = ttl;
}


//  --------------------------------------------------------------------------
//  Get/set the type field

const char *
fty_proto_type (fty_proto_t *self)
{
    assert (self);
    return self->type;
}

void
fty_proto_set_type (fty_proto_t *self, const char *format, ...)
{
    //  Format type from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->type);
    self->type = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the name field

const char *
fty_proto_name (fty_proto_t *self)
{
    assert (self);
    return self->name;
}

void
fty_proto_set_name (fty_proto_t *self, const char *format, ...)
{
    //  Format name from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->name);
    self->name = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the value field

const char *
fty_proto_value (fty_proto_t *self)
{
    assert (self);
    return self->value;
}

void
fty_proto_set_value (fty_proto_t *self, const char *format, ...)
{
    //  Format value from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->value);
    self->value = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the unit field

const char *
fty_proto_unit (fty_proto_t *self)
{
    assert (self);
    return self->unit;
}

void
fty_proto_set_unit (fty_proto_t *self, const char *format, ...)
{
    //  Format unit from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->unit);
    self->unit = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the rule field

const char *
fty_proto_rule (fty_proto_t *self)
{
    assert (self);
    return self->rule;
}

void
fty_proto_set_rule (fty_proto_t *self, const char *format, ...)
{
    //  Format rule from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->rule);
    self->rule = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the state field

const char *
fty_proto_state (fty_proto_t *self)
{
    assert (self);
    return self->state;
}

void
fty_proto_set_state (fty_proto_t *self, const char *format, ...)
{
    //  Format state from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->state);
    self->state = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the severity field

const char *
fty_proto_severity (fty_proto_t *self)
{
    assert (self);
    return self->severity;
}

void
fty_proto_set_severity (fty_proto_t *self, const char *format, ...)
{
    //  Format severity from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->severity);
    self->severity = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the description field

const char *
fty_proto_description (fty_proto_t *self)
{
    assert (self);
    return self->description;
}

void
fty_proto_set_description (fty_proto_t *self, const char *format, ...)
{
    //  Format description from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->description);
    self->description = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the metadata field

const char *
fty_proto_metadata (fty_proto_t *self)
{
    assert (self);
    return self->metadata;
}

void
fty_proto_set_metadata (fty_proto_t *self, const char *format, ...)
{
    //  Format description from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    zstr_free (&self->metadata);
    self->metadata = zsys_vprintf (format, argptr);
    va_end (argptr);
}

//  --------------------------------------------------------------------------
//  Get the action field, without transferring ownership

zlist_t *
fty_proto_action (fty_proto_t *self)
{
    assert (self);
    return self->action;
}

//  Get the action field and transfer ownership to caller

zlist_t *
fty_proto_get_action (fty_proto_t *self)
{
    assert (self);
    zlist_t *action = self->action;
    self->action = NULL;
    return action;
}

//  Set the action field, transferring ownership from caller

void
fty_proto_set_action (fty_proto_t *self, zlist_t **action_p)
{
    assert (self);
    assert (action_p);
    zlist_destroy (&self->action);
    self->action = *action_p;
    *action_p = NULL;
}

//  --------------------------------------------------------------------------
//  Iterate through the action field, and append a action value

const char *
fty_proto_action_first (fty_proto_t *self)
{
    assert (self);
    if (self->action)
        return (char *) (zlist_first (self->action));
    else
        return NULL;
}

const char *
fty_proto_action_next (fty_proto_t *self)
{
    assert (self);
    if (self->action)
        return (char *) (zlist_next (self->action));
    else
        return NULL;
}

void
fty_proto_action_append (fty_proto_t *self, const char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    //  Attach string to list
    if (!self->action) {
        self->action = zlist_new ();
        zlist_autofree (self->action);
    }
    zlist_append (self->action, string);
    free (string);
}

size_t
fty_proto_action_size (fty_proto_t *self)
{
    return zlist_size (self->action);
}


//  --------------------------------------------------------------------------
//  Get/set the operation field

const char *
fty_proto_operation (fty_proto_t *self)
{
    assert (self);
    return self->operation;
}

void
fty_proto_set_operation (fty_proto_t *self, const char *format, ...)
{
    //  Format operation from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->operation);
    self->operation = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get the ext field without transferring ownership

zhash_t *
fty_proto_ext (fty_proto_t *self)
{
    assert (self);
    return self->ext;
}

//  Get the ext field and transfer ownership to caller

zhash_t *
fty_proto_get_ext (fty_proto_t *self)
{
    zhash_t *ext = self->ext;
    self->ext = NULL;
    return ext;
}

//  Set the ext field, transferring ownership from caller

void
fty_proto_set_ext (fty_proto_t *self, zhash_t **ext_p)
{
    assert (self);
    assert (ext_p);
    zhash_destroy (&self->ext);
    self->ext = *ext_p;
    *ext_p = NULL;
}

//  --------------------------------------------------------------------------
//  Get/set a value in the ext dictionary

const char *
fty_proto_ext_string (fty_proto_t *self, const char *key, const char *default_value)
{
    assert (self);
    const char *value = NULL;
    if (self->ext)
        value = (const char *) (zhash_lookup (self->ext, key));
    if (!value)
        value = default_value;

    return value;
}

uint64_t
fty_proto_ext_number (fty_proto_t *self, const char *key, uint64_t default_value)
{
    assert (self);
    uint64_t value = default_value;
    char *string = NULL;
    if (self->ext)
        string = (char *) (zhash_lookup (self->ext, key));
    if (string)
        value = atol (string);

    return value;
}

void
fty_proto_ext_insert (fty_proto_t *self, const char *key, const char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    //  Store string in hash table
    if (!self->ext) {
        self->ext = zhash_new ();
        zhash_autofree (self->ext);
    }
    zhash_update (self->ext, key, string);
    free (string);
}

size_t
fty_proto_ext_size (fty_proto_t *self)
{
    return zhash_size (self->ext);
}



//  --------------------------------------------------------------------------
//  Selftest

void
fty_proto_test (bool verbose)
{
    printf (" * fty_proto: ");

    //  Silence an "unused" warning by "using" the verbose variable
    if (verbose) {;}

    //  @selftest
    //  Simple create/destroy test
    fty_proto_t *self = fty_proto_new (0);
    assert (self);
    fty_proto_destroy (&self);

    //  Create pair of sockets we can send through
    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    zsock_connect (input, "inproc://selftest-fty_proto");

    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    zsock_bind (output, "inproc://selftest-fty_proto");

    //  Encode/send/decode and verify each message type
    int instance;
    fty_proto_t *copy;
    zconfig_t *config;


    self = fty_proto_new (FTY_PROTO_METRIC);

    //  Check that _dup works on empty message
    copy = fty_proto_dup (self);
    assert (copy);
    fty_proto_destroy (&copy);

    fty_proto_aux_insert (self, "Name", "Brutus");
    fty_proto_aux_insert (self, "Age", "%d", 43);
    fty_proto_set_time (self, 123);
    fty_proto_set_ttl (self, 123);
    fty_proto_set_type (self, "Life is short but Now lasts for ever");
    fty_proto_set_name (self, "Life is short but Now lasts for ever");
    fty_proto_set_value (self, "Life is short but Now lasts for ever");
    fty_proto_set_unit (self, "Life is short but Now lasts for ever");
    // convert to zpl
    config = fty_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice from same object
    fty_proto_send_again (self, output);
    fty_proto_send (&self, output);

    for (instance = 0; instance < 3; instance++) {
        if (instance < 2)
            self = fty_proto_recv (input);
        else {
            self = fty_proto_new_zpl (config);
            zconfig_destroy (&config);
        }
        assert (self);
        if (instance < 2)
            assert (fty_proto_routing_id (self));

        assert (fty_proto_aux_size (self) == 2);
        assert (streq (fty_proto_aux_string (self, "Name", "?"), "Brutus"));
        assert (fty_proto_aux_number (self, "Age", 0) == 43);
        assert (fty_proto_time (self) == 123);
        assert (fty_proto_ttl (self) == 123);
        assert (streq (fty_proto_type (self), "Life is short but Now lasts for ever"));
        assert (streq (fty_proto_name (self), "Life is short but Now lasts for ever"));
        assert (streq (fty_proto_value (self), "Life is short but Now lasts for ever"));
        assert (streq (fty_proto_unit (self), "Life is short but Now lasts for ever"));
        assert (fty_proto_metadata (self) == NULL); // out of scope
        fty_proto_destroy (&self);
    }


    self = fty_proto_new (FTY_PROTO_ALERT);

    //  Check that _dup works on empty message
    copy = fty_proto_dup (self);
    assert (copy);
    fty_proto_destroy (&copy);

    fty_proto_aux_insert (self, "Name", "Brutus");
    fty_proto_aux_insert (self, "Age", "%d", 43);
    fty_proto_set_time (self, 123);
    fty_proto_set_ttl (self, 123);
    fty_proto_set_rule (self, "Life is short but Now lasts for ever");
    fty_proto_set_name (self, "Life is short but Now lasts for ever");
    fty_proto_set_state (self, "Life is short but Now lasts for ever");
    fty_proto_set_severity (self, "Life is short but Now lasts for ever");
    fty_proto_set_description (self, "Life is short but Now lasts for ever");
    fty_proto_action_append (self, "Name: %s", "Brutus");
    fty_proto_action_append (self, "Age: %d", 43);

    const char* METADATA = "{ \"Life\": \"is short but Now lasts for ever\" }";
    fty_proto_set_metadata (self, METADATA);

    // convert to zpl
    config = fty_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice from same object
    fty_proto_send_again (self, output);
    fty_proto_send (&self, output);

    for (instance = 0; instance < 3; instance++) {
        if (instance < 2)
            self = fty_proto_recv (input);
        else {
            self = fty_proto_new_zpl (config);
            zconfig_destroy (&config);
        }
        assert (self);
        if (instance < 2)
            assert (fty_proto_routing_id (self));

        assert (fty_proto_aux_size (self) == 2);
        assert (streq (fty_proto_aux_string (self, "Name", "?"), "Brutus"));
        assert (fty_proto_aux_number (self, "Age", 0) == 43);
        assert (fty_proto_time (self) == 123);
        assert (fty_proto_ttl (self) == 123);
        assert (streq (fty_proto_rule (self), "Life is short but Now lasts for ever"));
        assert (streq (fty_proto_name (self), "Life is short but Now lasts for ever"));
        assert (streq (fty_proto_state (self), "Life is short but Now lasts for ever"));
        assert (streq (fty_proto_severity (self), "Life is short but Now lasts for ever"));
        assert (streq (fty_proto_description (self), "Life is short but Now lasts for ever"));
        assert (fty_proto_action_size (self) == 2);
        assert (streq (fty_proto_action_first (self), "Name: Brutus"));
        assert (streq (fty_proto_action_next (self), "Age: 43"));
        assert (streq(fty_proto_metadata (self), METADATA));
        fty_proto_destroy (&self);
    }


    self = fty_proto_new (FTY_PROTO_ASSET);

    //  Check that _dup works on empty message
    copy = fty_proto_dup (self);
    assert (copy);
    fty_proto_destroy (&copy);

    fty_proto_aux_insert (self, "Name", "Brutus");
    fty_proto_aux_insert (self, "Age", "%d", 43);
    fty_proto_set_name (self, "Life is short but Now lasts for ever");
    fty_proto_set_operation (self, "Life is short but Now lasts for ever");
    fty_proto_ext_insert (self, "Name", "Brutus");
    fty_proto_ext_insert (self, "Age", "%d", 43);
    // convert to zpl
    config = fty_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice from same object
    fty_proto_send_again (self, output);
    fty_proto_send (&self, output);

    for (instance = 0; instance < 3; instance++) {
        if (instance < 2)
            self = fty_proto_recv (input);
        else {
            self = fty_proto_new_zpl (config);
            zconfig_destroy (&config);
        }
        assert (self);
        if (instance < 2)
            assert (fty_proto_routing_id (self));

        assert (fty_proto_aux_size (self) == 2);
        assert (streq (fty_proto_aux_string (self, "Name", "?"), "Brutus"));
        assert (fty_proto_aux_number (self, "Age", 0) == 43);
        assert (streq (fty_proto_name (self), "Life is short but Now lasts for ever"));
        assert (streq (fty_proto_operation (self), "Life is short but Now lasts for ever"));
        assert (fty_proto_ext_size (self) == 2);
        assert (streq (fty_proto_ext_string (self, "Name", "?"), "Brutus"));
        assert (fty_proto_ext_number (self, "Age", 0) == 43);
        assert (fty_proto_metadata (self) == NULL); // out of scope
        fty_proto_destroy (&self);
    }

    zconfig_destroy (&config);
    zsock_destroy (&input);
    zsock_destroy (&output);
#if defined (__WINDOWS__)
    zsys_shutdown();
#endif
    //  @end

    printf ("OK\n");
}
