/**
 * Copyright (C) 2017, Hao Hou
 **/
/**
 * @brief The servlet options 
 * @file options.h
 **/
#ifndef __OPTIONS_H__

/**
 * @brief The servlet options
 **/
typedef struct {
	uint32_t      json_mode:1;        /*!< The JSON mode */
	uint32_t      modify_time:1;      /*!< If we need change time */
	uint32_t      create_time:1;      /*!< If we need the creating time */
	uint32_t      create_db:1;        /*!< If we need create the DB base */
	uint32_t      async_ops:1;        /*!< If this servlet runs in async mode */
	enum {
		OPTIONS_REST_MODE             /*!< This servlet works on the REST controller mode */
	}             mode;               /*!< The servlet mode */
	const char*   db_path;            /*!< The data base path */
	jsonschema_t* schema;             /*!< The schema we are using */
} options_t;

/**
 * @brief Parse the command line options
 * @param argc The arguments count
 * @param argv The arguments value
 * @param buf The options buffer
 * @return status code
 **/
int options_parse(uint32_t argc, char const* const* argv, options_t* buf);

/**
 * @brief dispose The options
 * @note This function assumes the options struct itself is managed by the caller,
 *       so the caller should make sure the struct itself is properly disposed
 * @param options The options to dispose
 * @return status code
 **/
int options_free(options_t* options);

#endif /* __OPTIONS_H__ */
