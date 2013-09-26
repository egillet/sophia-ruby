#include "ruby.h"
#include "sophia.h"

#define GetSophia(object, sophia) {                         \
        Data_Get_Struct((object), Sophia, (sophia));        \
        if ((sophia) == NULL) {                             \
            rb_raise(rb_eStandardError, "closed object");   \
        }                                                   \
        if ((sophia)->env == NULL) {                        \
            rb_raise(rb_eStandardError, "closed object");   \
        }                                                   \
        if ((sophia)->db == NULL) {                         \
            rb_raise(rb_eStandardError, "closed object");   \
        }                                                   \
}

static VALUE rb_cSophia;

typedef struct {
    void* env;
    void* db;
} Sophia;

static void
sophia_free(Sophia *sophia)
{
    if (sophia) {
        if (sophia->db) {
            sp_destroy(sophia->db);
        }
        if (sophia->env) {
            sp_destroy(sophia->env);
        }
        xfree(sophia);
    }
}

static void
sophia_mark(Sophia *sophia)
{
}

static VALUE
sophia_alloc(VALUE klass)
{
    Sophia *sophia = ALLOC(Sophia);

    return Data_Wrap_Struct(klass, sophia_mark, sophia_free, sophia);
}

/*
 * call-seq:
 *   Sophia.new("/path/to/db") -> sophia
 */
static VALUE
sophia_initialize(int argc, VALUE *argv, VALUE self)
{
    Sophia *sophia;
    VALUE file;

    Data_Get_Struct(self, Sophia, sophia);

    rb_scan_args(argc, argv, "1", &file);

    FilePathValue(file);

    sophia->env = sp_env();

    if (sophia->env == NULL) {
        rb_raise(rb_eStandardError, "sp_env(3)");
    }

    if (sp_ctl(sophia->env, SPDIR, SPO_CREAT|SPO_RDWR, RSTRING_PTR(file))) {
        rb_raise(rb_eStandardError, sp_error(sophia->env));
    }

    sophia->db = sp_open(sophia->env);

    if (sophia->db == NULL) {
        rb_raise(rb_eStandardError, sp_error(sophia->env));
    }

    return Qnil;
}

/*
 * call-seq:
 *   sophia.close -> nil
 */
static VALUE
sophia_close(VALUE self)
{
    Sophia *sophia;

    GetSophia(self, sophia);

    if (sophia->db) {
        if (sp_destroy(sophia->db)) {
            rb_raise(rb_eStandardError, sp_error(sophia->env));
        }
        sophia->db = NULL;
    }

    if (sophia->env) {
        if (sp_destroy(sophia->env)) {
            rb_raise(rb_eStandardError, sp_error(sophia->env));
        }
        sophia->env = NULL;
    }

    return Qnil;
}

/*
 * call-seq:
 *   sophia.closed? -> true or false
 */
static VALUE
sophia_closed_p(VALUE self)
{
    Sophia *sophia;

    Data_Get_Struct(self, Sophia, sophia);

    if (sophia == NULL) {
        return Qtrue;
    }

    if (sophia->env == NULL) {
        return Qtrue;
    }

    if (sophia->db == NULL) {
        return Qtrue;
    }

    return Qfalse;
}

static VALUE
sophia_set(VALUE self, VALUE key, VALUE value)
{
    Sophia *sophia;

    GetSophia(self, sophia);

    key   = rb_obj_as_string(key);
    value = rb_obj_as_string(value);

    if (sp_set(sophia->db,
               RSTRING_PTR(key), RSTRING_LEN(key),
               RSTRING_PTR(value), RSTRING_LEN(value))) {
        rb_raise(rb_eStandardError, sp_error(sophia->env));
    }

    return value;
}

static VALUE
sophia_get(VALUE self, VALUE key, VALUE ifnone)
{
    Sophia *sophia;
    void   *value;
    size_t  vsize;
    int     result;

    GetSophia(self, sophia);

    ExportStringValue(key);

    result = sp_get(sophia->db,
                    RSTRING_PTR(key), RSTRING_LEN(key),
                    &value, &vsize);

    if (result == 1) {
        return rb_str_new(value, vsize);
    } else if (result == 0) {
        if (NIL_P(ifnone) && rb_block_given_p()) {
            return rb_yield(key);
        } else {
            return ifnone;
        }
    } else if (result == -1) {
        rb_raise(rb_eStandardError, sp_error(sophia->env));
    }

    rb_raise(rb_eStandardError, "error");
    return Qnil;
}

static VALUE
sophia_aref(VALUE self, VALUE key)
{
    return sophia_get(self, key, Qnil);
}

static VALUE
sophia_fetch(int argc, VALUE *argv, VALUE self)
{
    VALUE key, ifnone;
    rb_scan_args(argc, argv, "11", &key, &ifnone);

    return sophia_get(self, key, ifnone);
}

static VALUE
sophia_each(VALUE self)
{
  Sophia *sophia;
  GetSophia(self, sophia);
  
  void *cursor = sp_cursor(sophia->db, SPGT, NULL, 0);
  if (cursor == NULL) 
  {
    rb_raise(rb_eStandardError, sp_error(sophia->env));
  }  
  while (sp_fetch(cursor))
  {
    VALUE indices = rb_ary_new2(2);
    const char *key = sp_key(cursor);
    size_t key_size = sp_keysize(cursor);
    rb_ary_store(indices, 0, rb_str_new(key, key_size));
    const char *value = sp_value(cursor);
    size_t value_size = sp_valuesize(cursor);
    rb_ary_store(indices, 1, rb_str_new(value, value_size));
    rb_yield(indices);
  }
  sp_destroy(cursor);
  return self;
}

static VALUE
sophia_to_a(VALUE self)
{
  Sophia *sophia;
  GetSophia(self, sophia);
  VALUE result = rb_ary_new();
  void *cursor = sp_cursor(sophia->db, SPGT, NULL, 0);
  if (cursor == NULL) 
  {
    rb_raise(rb_eStandardError, sp_error(sophia->env));
  }
  int idx = 0;
  while (sp_fetch(cursor))
  {
    VALUE indices = rb_ary_new2(2);
    const char *key = sp_key(cursor);
    size_t key_size = sp_keysize(cursor);
    const char *value = sp_value(cursor);
    size_t value_size = sp_valuesize(cursor);
    rb_ary_store(indices, 0, rb_str_new(key, key_size));
    rb_ary_store(indices, 1, rb_str_new(value, value_size));
    rb_ary_store(result, idx++, indices);
  }
  sp_destroy(cursor);
  return result;
}

static VALUE
sophia_to_h(VALUE self)
{
  Sophia *sophia;
  GetSophia(self, sophia);
  VALUE result = rb_hash_new();
  void *cursor = sp_cursor(sophia->db, SPGT, NULL, 0);
  if (cursor == NULL) 
  {
    rb_raise(rb_eStandardError, sp_error(sophia->env));
  }  
  while (sp_fetch(cursor))
  {
    const char *key = sp_key(cursor);
    size_t key_size = sp_keysize(cursor);
    const char *value = sp_value(cursor);
    size_t value_size = sp_valuesize(cursor);
    rb_hash_aset(result, rb_str_new(key, key_size),  rb_str_new(value, value_size));
  }
  sp_destroy(cursor);
  return result;
}

void
Init_sophia()
{
    rb_cSophia = rb_define_class("Sophia", rb_cObject);
    rb_define_alloc_func(rb_cSophia, sophia_alloc);
    rb_define_private_method(rb_cSophia, "initialize", sophia_initialize, -1);
    rb_define_method(rb_cSophia, "close",   sophia_close, 0);
    rb_define_method(rb_cSophia, "closed?", sophia_closed_p, 0);
    rb_define_method(rb_cSophia, "set",     sophia_set, 2);
    rb_define_method(rb_cSophia, "[]=",     sophia_set, 2);
    rb_define_method(rb_cSophia, "get",     sophia_aref, 1);
    rb_define_method(rb_cSophia, "[]",      sophia_aref, 1);
    rb_define_method(rb_cSophia, "fetch",   sophia_fetch, -1);
    rb_define_method(rb_cSophia, "each", sophia_each, 0);
    rb_define_method(rb_cSophia, "to_a", sophia_to_a, 0);
    rb_define_method(rb_cSophia, "to_h", sophia_to_h, 0);
    rb_require("sophia/version");
}
