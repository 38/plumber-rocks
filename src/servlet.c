#include <string.h>

#include <pservlet.h>
#include <pstd.h>
#include <pstd/types/string.h>

#include <jsonschema/log.h>
#include <jsonschema.h>

#include <rocksdb/c.h>

#include <options.h>
#include <db.h>

typedef struct {
	pipe_t      cmd;   /*!< The command pipe */
} rest_t;

typedef struct {
	pipe_t                cmd;        /*!< The command pipe */
	pipe_t                data_in;    /*!< The data pipe */
	pipe_t                data_out;   /*!< The data output pipe */
	pstd_type_accessor_t  cmd_tk_ac;  /*!< The command token accessor */
	pstd_type_accessor_t  din_tk_ac;  /*!< The data input token accessor */
	pstd_type_accessor_t  dout_tk_ac; /*!< The data output token accessor */
} simple_t;

typedef struct {
	options_t          options;              /*!< The options */
	rocksdb_t*         db;                   /*!< The database instance */
	union {
		simple_t       simple_mode;          /*!< The simple mode data */
		rest_t         rest_mode;            /*!< The rest mode data */
	};
	pstd_type_model_t* type_model;           /*!< The type model */
} context_t;


static int _init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	memset(ctx, 0, sizeof(ctx[0]));

	jsonschema_log_set_write_callback(RUNTIME_ADDRESS_TABLE_SYM->log_write);

	if(ERROR_CODE(int) == options_parse(argc, argv, &ctx->options))
		ERROR_RETURN_LOG(int, "Invalid servlet options");

	if(NULL == (ctx->type_model = pstd_type_model_new()))
		ERROR_RETURN_LOG(int, "Cannot create type model for rocksdb servlet");

	if(ctx->options.mode == OPTIONS_REST_MODE)
	{
		if(ERROR_CODE(pipe_t) == (ctx->rest_mode.cmd = pipe_define("command", PIPE_INPUT, "plumber/std_servlet/rest/controller/v0/Command")))
			ERROR_RETURN_LOG(int, "Cannot create command pipe");
		
		if(NULL == (ctx->db = db_acquire(&ctx->options)))
			ERROR_RETURN_LOG(int, "Cannot acquire the DB instance");
	}
	else if(ctx->options.mode == OPTIONS_SIMPLE_MODE)
	{
		if(ERROR_CODE(pipe_t) == (ctx->simple_mode.cmd = pipe_define("command", PIPE_INPUT, "plumber/std/request_local/String")))
			ERROR_RETURN_LOG(int, "Cannot create the command pipe");

		if(ERROR_CODE(pipe_t) == (ctx->simple_mode.data_in = pipe_define("data_in", PIPE_INPUT, "plumber/std/request_local/String")))
			ERROR_RETURN_LOG(int, "Cannot create the data input pipe");

		if(ERROR_CODE(pipe_t) == (ctx->simple_mode.data_out = pipe_define("data_out", PIPE_OUTPUT, "plubmer/std/request_local/String")))
			ERROR_RETURN_LOG(int, "Cannot create the data output pipe");

		if(ERROR_CODE(pstd_type_accessor_t) == (ctx->simple_mode.cmd_tk_ac = pstd_type_model_get_accessor(ctx->type_model, ctx->simple_mode.cmd, "token")))
			ERROR_RETURN_LOG(int, "Cannot get the accessor for the RLS token field");
		
		if(ERROR_CODE(pstd_type_accessor_t) == (ctx->simple_mode.din_tk_ac = pstd_type_model_get_accessor(ctx->type_model, ctx->simple_mode.data_in, "token")))
			ERROR_RETURN_LOG(int, "Cannot get the accessor for the RLS token field");
		
		if(ERROR_CODE(pstd_type_accessor_t) == (ctx->simple_mode.dout_tk_ac = pstd_type_model_get_accessor(ctx->type_model, ctx->simple_mode.data_out, "token")))
			ERROR_RETURN_LOG(int, "Cannot get the accessor for the RLS token field");
	}
	else
		ERROR_RETURN_LOG(int, "Fixme: Currently we only support the REST controller mode");


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

static const char* _read_string(pstd_type_instance_t* inst, pstd_type_accessor_t accessor)
{
	scope_token_t token;
	if(ERROR_CODE(scope_token_t) == (token = PSTD_TYPE_INST_READ_PRIMITIVE(scope_token_t, inst, accessor)))
	    ERROR_PTR_RETURN_LOG("Cannot access token field");
	
	const pstd_string_t* pstr = pstd_string_from_rls(token);
	if(NULL == pstr) ERROR_PTR_RETURN_LOG("Cannot retrive string object from the RLS");

	return pstd_string_value(pstr);

}

static int _do_simple_mode(context_t* ctx, pstd_type_instance_t* inst)
{
	const char* cmd = _read_string(inst, ctx->simple_mode.cmd_tk_ac);
	if(NULL == cmd) ERROR_RETURN_LOG(int, "Cannot read the command string from the RLS");

	/* TODO parse the comamnd string */
	return 0;
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
			rc = _do_simple_mode(ctx, ti);
			break;
		default:
			LOG_ERROR("Invalid mode");
	}

	if(ERROR_CODE(int) == pstd_type_instance_free(ti))
		ERROR_RETURN_LOG(int, "Canont dispose the type instance");

	return 0;
}

static int _async_setup(async_handle_t* handle, void* asyncbuf, void* ctxbuf)
{
	(void)handle;
	(void)asyncbuf;
	(void)ctxbuf;

	return 0;
}

SERVLET_DEF = {
	.desc = "Plumber-RocksDB Binding",
	.version = 0,
	.size = sizeof(context_t),
	.init = _init,
	.unload = _unload,
	.exec   = _exec,
	.async_setup = _async_setup
};
