#include <string.h>
#include <pservlet.h>
#include <pstd.h>
#include <proto.h>
#include <jsonschema/log.h>
#include <jsonschema.h>
#include <rocksdb/c.h>

#include <options.h>
#include <db.h>

typedef struct {
	options_t  options;          /*!< The options */
	rocksdb_t* db;               /*!< The database instance */
	pipe_t p_command;            /*!< The command pipe */
} context_t;


static int _init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	memset(ctx, 0, sizeof(ctx[0]));

	jsonschema_log_set_write_callback(RUNTIME_ADDRESS_TABLE_SYM->log_write);

	if(ERROR_CODE(int) == options_parse(argc, argv, &ctx->options))
		ERROR_RETURN_LOG(int, "Invalid servlet options");

	if(ctx->options.mode == OPTIONS_REST_MODE)
	{
		ctx->p_command = pipe_define("command", PIPE_INPUT, "plumber/std_servlet/rest/controller/v0/Command");
		
		if(NULL == (ctx->db = db_acquire(&ctx->options)))
			ERROR_RETURN_LOG(int, "Cannot acquire the DB instance");
	}
	else
		ERROR_RETURN_LOG(int, "Fixme: Currently we only support the REST controller mode");


	return 0;
}

static int _unload(void* ctxbuf)
{
	int rc = 0;
	context_t* ctx = (context_t*)ctxbuf;

	if(ERROR_CODE(int) == options_free(&ctx->options))
		rc = ERROR_CODE(int);

	if(ctx->db != NULL && ERROR_CODE(int) == db_release(ctx->db))
		rc = ERROR_CODE(int);

	return rc;
}

SERVLET_DEF = {
	.desc = "Plumber-RocksDB binding",
	.version = 0,
	.size = sizeof(context_t),
	.init = _init,
	.unload = _unload
};
