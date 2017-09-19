/**
 * Copyright (C) 2017, Hao Hou
 **/
#include <rocksdb/c.h>

#include <pstd.h>
#include <pstd/types/string.h>

#include <jsonschema.h>

#include <simple.h>
#include <options.h>
#include <db.h>
#include <rls.h>

int simple_ctx_init(simple_t* ctx, pstd_type_model_t* type_model, int readonly)
{
	if(NULL == ctx || NULL == type_model) ERROR_RETURN_LOG(int, "Invalid arguments");

	if(ERROR_CODE(pipe_t) == (ctx->cmd = pipe_define("command", PIPE_INPUT,     "plumber/std/request_local/String")))
		ERROR_RETURN_LOG(int, "Cannot create the command pipe");

	ctx->data_in = ERROR_CODE(pipe_t);
	if(!readonly && 
	   ERROR_CODE(pipe_t) == (ctx->data_in = pipe_define("data_in", PIPE_INPUT, "plumber/std/request_local/String")))
		ERROR_RETURN_LOG(int, "Cannot create the data input pipe");

	if(ERROR_CODE(pipe_t) == (ctx->data_out = pipe_define("data_out", PIPE_OUTPUT, "plumber/std/request_local/String")))
		ERROR_RETURN_LOG(int, "Cannot create the data output pipe");

	if(ERROR_CODE(pstd_type_accessor_t) == (ctx->cmd_tk_ac = pstd_type_model_get_accessor(type_model, ctx->cmd, "token")))
		ERROR_RETURN_LOG(int, "Cannot get the accessor for the RLS token field");
	
	if(!readonly &&
	   ERROR_CODE(pstd_type_accessor_t) == (ctx->din_tk_ac = pstd_type_model_get_accessor(type_model, ctx->data_in, "token")))
		ERROR_RETURN_LOG(int, "Cannot get the accessor for the RLS token field");
	
	if(ERROR_CODE(pstd_type_accessor_t) == (ctx->dout_tk_ac = pstd_type_model_get_accessor(type_model, ctx->data_out, "token")))
		ERROR_RETURN_LOG(int, "Cannot get the accessor for the RLS token field");

	return 0;
}

int simple_sync_exec(simple_t* ctx, pstd_type_instance_t* inst, rocksdb_t* db)
{
	if(NULL == ctx || NULL == inst || NULL == db) ERROR_RETURN_LOG(int, "Invalid arguments");
	size_t keysize;
	const char* cmd;
	if(ERROR_CODE(int) == rls_read_string(inst, ctx->cmd_tk_ac, &cmd, &keysize))
		ERROR_RETURN_LOG(int, "Cannot read the command string from the RLS");

	uint32_t val = 0, i;
	for(i = 0; i < 4 && cmd[i]; i ++)
		val |= (((uint32_t)cmd[i]) << (i * 8));

	switch(val)
	{
		case 'G' | ('E' << 8) | ('T' << 16) | (' ' << 24):
		{
			char* val;
			size_t val_size = db_read(db, cmd + 4, keysize - 4, (void**)&val);
			if(val_size == ERROR_CODE(size_t)) ERROR_RETURN_LOG(int, "Cannot read data from the RocksDB");

			if(val == NULL) return 0;

			pstd_string_t* ps = pstd_string_from_onwership_pointer(val, val_size - 1);
			if(NULL == ps) ERROR_RETURN_LOG(int, "Cannot create result string RLS object");

			scope_token_t token = pstd_string_commit(ps);
			if(ERROR_CODE(scope_token_t) == token) ERROR_RETURN_LOG(int, "Cannot commit string object to RLS");

			if(ERROR_CODE(int) == PSTD_TYPE_INST_WRITE_PRIMITIVE(inst, ctx->dout_tk_ac, token))
				ERROR_RETURN_LOG(int, "Cannot write the RLS token to the output");

			return 0;
		}
		case 'P' | ('U' << 8) | ('T' << 16) | (' ' << 24):
		{
			if(ctx->data_in == ERROR_CODE(pipe_t)) 
				ERROR_RETURN_LOG(int, "Invalid command: cannot write to a readonly database");
			size_t val_size;
			const char* val;
			
			if(ERROR_CODE(int) == rls_read_string(inst, ctx->din_tk_ac, &val, &val_size))
				ERROR_RETURN_LOG(int, "Cannot read input data");

			if(ERROR_CODE(int) == db_write(db, cmd + 4, keysize - 4, val, val_size))
				ERROR_RETURN_LOG(int, "Cannot write Rocksdb");

			return 0;
		}
		default:
			ERROR_RETURN_LOG(int, "Invalid command");
	}
}

int simple_async_setup(simple_t* ctx, pstd_type_instance_t* inst, rocksdb_t* db, async_handle_t* async_handle, simple_async_buf_t* async_buf)
{
	if(NULL == ctx || NULL == inst || NULL == async_handle || NULL == async_buf)
		ERROR_RETURN_LOG(int, "Invalid arguments");

	size_t keysize;
	const char* cmd;
	if(ERROR_CODE(int) == rls_read_string(inst, ctx->cmd_tk_ac, &cmd, &keysize))
		ERROR_RETURN_LOG(int, "Cannot read the command string from the RLS");

	uint32_t val = 0, i;
	for(i = 0; i < 4 && cmd[i]; i ++)
		val |= (((uint32_t)cmd[i]) << (i * 8));

	async_buf->db = db;
	async_buf->out_val = NULL;
	async_buf->in_val = NULL;

	switch(val)
	{
		case 'P' | ('U' << 8) | ('T' << 16) | (' ' << 24):
			if(ERROR_CODE(pipe_t) == ctx->data_in)
				ERROR_RETURN_LOG(int, "Cannot write a readonly database");
			if(ERROR_CODE(int) == rls_read_string(inst, ctx->din_tk_ac, (char const**)&async_buf->in_val, &async_buf->in_val_size))
				ERROR_RETURN_LOG(int, "Cannot read the value string from RLS");
		case 'G' | ('E' << 8) | ('T' << 16) | (' ' << 24):
			async_buf->key = (void*)(cmd + 4);
			async_buf->key_size = keysize;
		default:
			ERROR_RETURN_LOG(int, "Invalid arguments");
	}

	return 0;
}

int simple_async_exec(async_handle_t* async_handle, simple_async_buf_t* async_buf)
{
	if(NULL == async_handle || NULL == async_buf)
		ERROR_RETURN_LOG(int, "Invalid arguments");

	if(async_buf->in_val == NULL)
	{
		/* We need to read the key */
		async_buf->out_val_size = db_read(async_buf->db, async_buf->key, async_buf->key_size, &async_buf->out_val);
		if(ERROR_CODE(size_t) == async_buf->out_val_size)
			ERROR_RETURN_LOG(int, "Cannot read data from the database");
		if(async_buf->out_val == NULL) async_buf->out_val_size = 0;

		return 0;
	}
	else
	{
		/* We need to write the key */
		if(ERROR_CODE(int) == db_write(async_buf->db, async_buf->key, async_buf->key_size, async_buf->in_val, async_buf->in_val_size))
			ERROR_RETURN_LOG(int, "Cannot write Rocksdb");
		return 0;
	}
}

int simple_async_cleanup(simple_t* ctx, pstd_type_instance_t* inst, async_handle_t* async_handle, simple_async_buf_t* async_buf)
{
	if(NULL == ctx || NULL == inst || NULL == async_handle || NULL == async_buf)
		ERROR_RETURN_LOG(int, "Invalid arguments");

	int rc;
	if(async_cntl(async_handle, ASYNC_CNTL_RETCODE, &rc) == ERROR_CODE(int))
		ERROR_RETURN_LOG(int, "Cannot get the async status");

	if(ERROR_CODE(int) == rc) return 0;

	if(NULL != async_buf->out_val)
	{
		pstd_string_t* ps = pstd_string_from_onwership_pointer(async_buf->out_val, async_buf->out_val_size - 1);
		if(NULL == ps) ERROR_RETURN_LOG(int, "Cannot create result string RLS object");

		scope_token_t token = pstd_string_commit(ps);
		if(ERROR_CODE(scope_token_t) == token) ERROR_RETURN_LOG(int, "Cannot commit string object to RLS");

		if(ERROR_CODE(int) == PSTD_TYPE_INST_WRITE_PRIMITIVE(inst, ctx->dout_tk_ac, token))
			ERROR_RETURN_LOG(int, "Cannot write the RLS token to the output");

	}

	return 0;
}
