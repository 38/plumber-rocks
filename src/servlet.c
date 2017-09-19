/**
 * Copyright (C) 2017, Hao Hou
 **/
#include <stdlib.h>
#include <string.h>

#include <pservlet.h>
#include <pstd.h>
#include <pstd/types/string.h>

#include <jsonschema/log.h>
#include <jsonschema.h>

#include <rocksdb/c.h>

#include <options.h>
#include <db.h>
#include <simple.h>

typedef struct {
	/* TODO */
} rest_t;

typedef struct {
	options_t          options;              /*!< The options */
	rocksdb_t*         db;                   /*!< The database instance */
	union {
		simple_t       simple_mode;          /*!< The simple mode data */
		rest_t         rest_mode;            /*!< The rest mode data */
	};
	pstd_type_model_t* type_model;           /*!< The type model */
} context_t;

typedef struct {
	int                mode;        /*!< The mode code */
	union{
		simple_async_buf_t simple_mode; /*!< The simple mode */
	};
} async_buf_t;


static int _init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	memset(ctx, 0, sizeof(ctx[0]));

	jsonschema_log_set_write_callback(RUNTIME_ADDRESS_TABLE_SYM->log_write);

	if(ERROR_CODE(int) == options_parse(argc, argv, &ctx->options))
		ERROR_RETURN_LOG(int, "Invalid servlet options");

	if(NULL == (ctx->type_model = pstd_type_model_new()))
		ERROR_RETURN_LOG(int, "Cannot create type model for rocksdb servlet");

	switch(ctx->options.mode)
	{
		case OPTIONS_SIMPLE_MODE:
			if(ERROR_CODE(int) == simple_ctx_init(&ctx->simple_mode, ctx->type_model, ctx->options.read_only))
				ERROR_RETURN_LOG(int, "Cannot initialize the simple mode context");
			break;
		default:
			ERROR_RETURN_LOG(int, "Invalid Servlet Operation mode");
	}

	if(NULL == (ctx->db = db_acquire(&ctx->options)))
		ERROR_RETURN_LOG(int, "Cannot acquire the DB instance");

	return ctx->options.async_ops ? 1 : 0;
}

static int _unload(void* ctxbuf)
{
	int rc = 0;
	context_t* ctx = (context_t*)ctxbuf;

	if(ERROR_CODE(int) == options_free(&ctx->options))
		rc = ERROR_CODE(int);

	if(ctx->db != NULL && ERROR_CODE(int) == db_release(ctx->db))
		rc = ERROR_CODE(int);

	if(NULL != ctx->type_model && ERROR_CODE(int) == pstd_type_model_free(ctx->type_model))
		rc = ERROR_CODE(int);

	return rc;
}


static int _exec(void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	size_t tisize = pstd_type_instance_size(ctx->type_model);
	if(ERROR_CODE(size_t) == tisize) ERROR_RETURN_LOG(int, "Cannot get the size of the tpye instance");
	char tibuf[tisize];
	pstd_type_instance_t* ti = pstd_type_instance_new(ctx->type_model, tibuf);
	if(NULL == ti) ERROR_RETURN_LOG(int, "Cannot create type instance");

	int rc = ERROR_CODE(int);

	switch(ctx->options.mode)
	{
		case OPTIONS_SIMPLE_MODE:
			rc = simple_sync_exec(&ctx->simple_mode, ti, ctx->db);
			break;
		default:
			LOG_ERROR("Invalid mode");
	}
	
	if(ERROR_CODE(int) == pstd_type_instance_free(ti))
		ERROR_RETURN_LOG(int, "Canont dispose the type instance");

	return rc;
}

static int _async_setup(async_handle_t* handle, void* asyncbuf, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	size_t tisize = pstd_type_instance_size(ctx->type_model);
	if(ERROR_CODE(size_t) == tisize) ERROR_RETURN_LOG(int, "Cannot get the size of the tpye instance");
	char tibuf[tisize];
	pstd_type_instance_t* ti = pstd_type_instance_new(ctx->type_model, tibuf);
	if(NULL == ti) ERROR_RETURN_LOG(int, "Cannot create type instance");

	int rc = ERROR_CODE(int);

	async_buf_t* buf = (async_buf_t*)asyncbuf;

	buf->mode = ctx->options.mode;

	switch(ctx->options.mode)
	{
		case OPTIONS_SIMPLE_MODE:
			rc = simple_async_setup(&ctx->simple_mode, ti, ctx->db, handle, &buf->simple_mode);
			break;
		default:
			LOG_ERROR("Invalid mode");
	}

	if(ERROR_CODE(int) == pstd_type_instance_free(ti))
		ERROR_RETURN_LOG(int, "Canont dispose the type instance");

	return rc;
}

static int _async_exec(async_handle_t* handle, void* asyncbuf)
{
	(void)handle;
	async_buf_t* buf = (async_buf_t*)asyncbuf;
	switch(buf->mode)
	{
		case OPTIONS_SIMPLE_MODE:
			return simple_async_exec(handle, &buf->simple_mode);
		default:
			ERROR_RETURN_LOG(int, "Invalid mode");
	}
}

static int _async_cleanup(async_handle_t* handle, void* asyncbuf, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	size_t tisize = pstd_type_instance_size(ctx->type_model);
	if(ERROR_CODE(size_t) == tisize) ERROR_RETURN_LOG(int, "Cannot get the size of the tpye instance");
	char tibuf[tisize];
	pstd_type_instance_t* ti = pstd_type_instance_new(ctx->type_model, tibuf);
	if(NULL == ti) ERROR_RETURN_LOG(int, "Cannot create type instance");

	int rc = ERROR_CODE(int);

	async_buf_t* buf = (async_buf_t*)asyncbuf;

	buf->mode = ctx->options.mode;

	switch(ctx->options.mode)
	{
		case OPTIONS_SIMPLE_MODE:
			rc = simple_async_cleanup(&ctx->simple_mode, ti, handle, &buf->simple_mode);
			break;
		default:
			LOG_ERROR("Invalid mode");
	}

	if(ERROR_CODE(int) == pstd_type_instance_free(ti))
		ERROR_RETURN_LOG(int, "Canont dispose the type instance");

	return rc;
}


SERVLET_DEF = {
	.desc = "Plumber-RocksDB Binding",
	.version = 0,
	.size = sizeof(context_t),
	.init = _init,
	.unload = _unload,
	.exec   = _exec,
	.async_buf_size = sizeof(async_buf_t),
	.async_setup = _async_setup,
	.async_exec  = _async_exec,
	.async_cleanup = _async_cleanup
};
