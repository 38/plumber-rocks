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
	size_t keysize;
	const char* cmd = rls_read_string(inst, ctx->cmd_tk_ac, &keysize);
	if(NULL == cmd) ERROR_RETURN_LOG(int, "Cannot read the command string from the RLS");

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

			if(val == NULL) 
			{
				LOG_DEBUG("Key not found, exiting");
				return 0;
			}

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
			const char* val = rls_read_string(inst, ctx->din_tk_ac, &val_size);

			if(NULL == val) ERROR_RETURN_LOG(int, "Cannot read the input data");

			if(ERROR_CODE(int) == db_write(db, cmd + 4, keysize - 4, val, val_size))
				ERROR_RETURN_LOG(int, "Cannot write Rocksdb");

			return 0;
		}
		default:
			ERROR_RETURN_LOG(int, "Invalid command");
	}
}
