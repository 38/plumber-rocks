/**
 * Copyright (C) 2017, Hao Hou
 **/
#include <string.h>
#include <pstd.h>
#include <jsonschema.h>

#include <options.h>


static int _set_database_path(pstd_option_data_t data)
{
	options_t* ctx = (options_t*)data.cb_data;
	ctx->db_path = data.param_array[0].strval;
	return 0;
}

static int _option(pstd_option_data_t data)
{
	options_t* ctx = (options_t*)data.cb_data;
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
	options_t* ctx = (options_t*)data.cb_data;
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

static int _mode(pstd_option_data_t data)
{
	options_t* ctx = (options_t*)data.cb_data;
	const char* mode = data.param_array[0].strval;

	if(strcmp(mode, "rest") == 0)
	{
		ctx->mode = OPTIONS_REST_MODE;
		return 0;
	}
	else ERROR_RETURN_LOG(int, "Invalid arguments: unrecognized working mode %s", mode);
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
	},
	{
		.long_opt  = "mode",
		.short_opt = 'M',
		.pattern   = "S",
		.description = "Indicates we are in the RESTful storage controller mode: rest",
		.handler   = _mode,
		.args      = NULL
	}
};

int options_parse(uint32_t argc, char const* const* argv, options_t* buf)
{
	if(NULL == buf) ERROR_RETURN_LOG(int, "Invalid arguments");

	if(ERROR_CODE(int) == pstd_option_sort(_options, sizeof(_options) / sizeof(_options[0])))
		ERROR_RETURN_LOG(int, "Cannot sort the options");

	buf->mode = OPTIONS_REST_MODE;

	uint32_t rc;
	if(ERROR_CODE(uint32_t) == (rc = pstd_option_parse(_options, sizeof(_options) / sizeof(_options[0]), argc, argv, buf)))
		ERROR_RETURN_LOG(int, "Cannot parse the command line param");

	if(argc != rc) ERROR_RETURN_LOG(int, "Invalid arguments: can not parse all the params");
	
	if(buf->db_path == NULL)
		ERROR_RETURN_LOG(int, "Invalid arguments: DB Path is missing");

	return 0;
}

int options_free(options_t* opt)
{
	if(opt == NULL) ERROR_RETURN_LOG(int, "Invalid arguments");

	int rc = 0;

	if(opt->schema != NULL && ERROR_CODE(int) == jsonschema_free(opt->schema))
		rc = ERROR_CODE(int);

	return rc;
}
