#include <string.h>

#include <pservlet.h>
#include <pstd.h>

#include <jsonschema/log.h>
#include <jsonschema.h>

#include <rocksdb/c.h>

#include <options.h>
#include <db.h>

typedef struct {
	options_t          options;              /*!< The options */
	rocksdb_t*         db;                   /*!< The database instance */
	pipe_t             p_command;            /*!< The command pipe */
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
		ctx->p_command = pipe_define("command", PIPE_INPUT, "plumber/std_servlet/rest/controller/v0/Command");
		
		if(NULL == (ctx->db = db_acquire(&ctx->options)))
			ERROR_RETURN_LOG(int, "Cannot acquire the DB instance");
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
	.async_setup = _async_setup
};
