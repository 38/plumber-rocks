#include <string.h>
#include <pservlet.h>
#include <pstd.h>
#include <proto.h>
#include <jsonschema/log.h>
#include <jsonschema.h>
#include <rocksdb/c.h>

typedef struct {
	uint32_t json_mode:1;        /*!< The JSON mode */
	uint32_t modify_time:1;      /*!< If we need change time */
	uint32_t create_time:1;      /*!< If we need the creating time */
	uint32_t create_db:1;        /*!< If we need create the DB base */
	const char* db_path;         /*!< The data base path */
	jsonschema_t* schema;        /*!< The schema we are using */
	rocksdb_t* db;               /*!< The database instance */
	rocksdb_options_t* db_opt;   /*!< The database options */
	pipe_t p_command;
} context_t;

static int _set_database_path(pstd_option_data_t data)
{
	context_t* ctx = (context_t*)data.cb_data;
	ctx->db_path = data.param_array[0].strval;
	return 0;
}

static int _option(pstd_option_data_t data)
{
	context_t* ctx = (context_t*)data.cb_data;
	switch(data.current_option->short_opt)
	{
		case 'c':
			ctx->create_time = 1;
			break;
		case 'm':
			ctx->modify_time = 1;
			break;
		case 'C':
			ctx->create_db = 1;
			break;
		default:
			ERROR_RETURN_LOG(int, "Invalid command line parameter");
	}
	return 0;
}

static int _process_json_schema(pstd_option_data_t data)
{
	context_t* ctx = (context_t*)data.cb_data;
	ctx->json_mode = 1;
	if(data.param_array_size == 0)
	{
		/* The schemaless mode */
		ctx->schema = NULL;
		LOG_DEBUG("The servlet is configured to a schemaless mode");
	}
	else if(data.param_array_size != 1 || data.param_array == NULL || data.param_array[0].type != PSTD_OPTION_STRING)
		ERROR_RETURN_LOG(int, "Invalid arguments use --help to see the usage");
	else
	{
		const char* schema_file = data.param_array[0].strval;

		if(NULL == (ctx->schema = jsonschema_from_file(schema_file)))
			return ERROR_CODE(int);
	}
	
	return 0;
}

static pstd_option_t _options[] = {
	{
		.long_opt  = "help",
		.short_opt = 'h',
		.pattern   = "",
		.description = "Print this help message",
		.handler = pstd_option_handler_print_help,
		.args = NULL
	},
	{
		.long_opt  = "json",
		.short_opt = 'j',
		.pattern   = "?S",
		.description = "The servlet process JSON data with the given data schema",
		.handler     = _process_json_schema,
		.args        = NULL
	},
	{
		.long_opt  = "create-timestamp",
		.short_opt = 'c',
		.pattern   = "",
		.description = "If we need to add the creation timestamp autoamtically to the data",
		.handler     = _option,
		.args        = NULL
	},
	{
		.long_opt  = "modify-timestamp",
		.short_opt = 'm',
		.pattern   = "",
		.description = "If we need to add modification timestamp automatically to the data",
		.handler     = _option,
		.args        = NULL
	},
	{
		.long_opt   = "create-db",
		.short_opt  = 'C',
		.pattern    = "",
		.description = "Indicates if we need to create the database if it doesn't exist",
		.handler     = _option,
		.args        = NULL
	},
	{
		.long_opt  = "path",
		.short_opt = 'p',
		.pattern   = "S",
		.description = "The path to the RocksDB data file",
		.handler  = _set_database_path,
		.args     = NULL
	}
};

static int _init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;
	ctx->schema = NULL;

	memset(ctx, 0, sizeof(ctx[0]));

	jsonschema_log_set_write_callback(RUNTIME_ADDRESS_TABLE_SYM->log_write);

	if(ERROR_CODE(int) == pstd_option_sort(_options, sizeof(_options) / sizeof(_options[0])))
		ERROR_RETURN_LOG(int, "Cannot sort the options");

	uint32_t rc;
	if(ERROR_CODE(uint32_t) == (rc = pstd_option_parse(_options, sizeof(_options) / sizeof(_options[0]), argc, argv, ctx)))
		ERROR_RETURN_LOG(int, "Cannot parse the command line param");

	if(ctx->db_path == NULL)
		ERROR_RETURN_LOG(int, "Invalid arguments: DB Path is missing");
	
	ctx->p_command = pipe_define("command", PIPE_INPUT, "plumber/std_servlet/rest/controller/v0/Command");

	if(NULL == (ctx->db_opt = rocksdb_options_create()))
		ERROR_RETURN_LOG(int, "Cannot create rocksdb option");

	if(ctx->create_db)
		rocksdb_options_set_create_if_missing(ctx->db_opt, 1);

	char* err = NULL;

	if(NULL == (ctx->db = rocksdb_open(ctx->db_opt, ctx->db_path, &err)) || err != NULL)
		ERROR_RETURN_LOG(int, "Cannot open rocksdb database: %s", err);


	return 0;
}

static int _unload(void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	if(NULL != ctx->schema && ERROR_CODE(int) == jsonschema_free(ctx->schema))
		ERROR_RETURN_LOG(int, "Cannot dispose the JSON schema");

	if(NULL != ctx->db_opt) 
		rocksdb_options_destroy(ctx->db_opt);

	if(NULL != ctx->db)
		rocksdb_close(ctx->db);

	return 0;
}

SERVLET_DEF = {
	.desc = "Plumber-RocksDB binding",
	.version = 0,
	.size = sizeof(context_t),
	.init = _init,
	.unload = _unload
};
