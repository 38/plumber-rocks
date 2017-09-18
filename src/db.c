/**
 * Copyright (C) 2017, Hao Hou
 **/
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <rocksdb/c.h>

#include <jsonschema.h>

#include <pstd.h>

#include <options.h>
#include <db.h>

/**
 * @brief A used DB instance
 **/
typedef struct _db_t {
	struct _db_t* next;   /*!< The linked list pointer */
	rocksdb_t*    db;     /*!< The actual DB instance */
	rocksdb_options_t* options; /*!< The database options */
	const char*   path;   /*!< The path to the DB */
	uint32_t      refcnt; /*!< How many servlet are using this DB */
} _db_t;

static _db_t* _db_list = NULL;

rocksdb_t* db_acquire(const options_t* options)
{
	if(NULL == options) ERROR_PTR_RETURN_LOG( "Invalid arguments");

	_db_t* ptr;
	for(ptr = _db_list; NULL != _db_list && strcmp(ptr->path, options->db_path) != 0; ptr = ptr->next);

	if(ptr == NULL)
	{
		char* err = NULL;
		
		if(NULL == (ptr = (_db_t*)calloc(1, sizeof(_db_t))))
			ERROR_PTR_RETURN_LOG_ERRNO("Cannot allocate memory for the db instance list item");

		if(NULL == (ptr->options = rocksdb_options_create()))
			ERROR_LOG_GOTO(CREATE_ERR, "Cannot create rocksdb option");

		if(options->create_db)
			rocksdb_options_set_create_if_missing(ptr->options, 1);

		if(NULL == (ptr->db = rocksdb_open(ptr->options, options->db_path, &err)) || err != NULL)
			ERROR_LOG_GOTO(CREATE_ERR, "Cannot open rocksdb database: %s", err);

		ptr->next = _db_list;
		ptr->path = options->db_path;
		_db_list = ptr;

		LOG_DEBUG("Successfully opened DB at %s", options->db_path);

		goto EXIT;
CREATE_ERR:
		if(NULL != err) free(err);
		if(NULL != ptr) 
		{
			if(ptr->options != NULL) rocksdb_options_destroy(ptr->options);
			if(ptr->db != NULL) rocksdb_close(ptr->db);
			free(ptr);
		}
		return NULL;
	}
	else LOG_DEBUG("DB %s has been opened previously", options->db_path);

EXIT:
	ptr->refcnt ++;
	return ptr->db;
}

int db_release(rocksdb_t* db)
{
	if(NULL == db) ERROR_RETURN_LOG(int, "Invalid arguments");

	_db_t* ptr;
	_db_t* prev = NULL;
	for(ptr = _db_list; ptr != NULL && ptr->db != db; prev = ptr, ptr = ptr->next);

	if(ptr == NULL) ERROR_RETURN_LOG(int, "Invalid arguments: DB instance not found");

	if(ptr->refcnt <= 1)
	{
		LOG_DEBUG("DB instance is longer used, dispose the DB");
		if(ptr->options != NULL) rocksdb_options_destroy(ptr->options);
		if(ptr->db != NULL) rocksdb_close(ptr->db);
		if(NULL != prev) prev->next = ptr->next;
		else _db_list = ptr->next;
		free(ptr);
	}
	else ptr->refcnt --;

	return 0;
}

size_t db_read(rocksdb_t* db, const void* key, size_t key_size, void** valbuf)
{
	size_t ret = 0;
	char* err = NULL;
	if(NULL == db || NULL == key || NULL == valbuf)
		ERROR_RETURN_LOG(size_t, "Invalid arguments");
	rocksdb_readoptions_t* read_options = rocksdb_readoptions_create();

	if(NULL == read_options) ERROR_LOG_GOTO(ERR, "Cannot allocate read options");

	if(NULL == (*valbuf = rocksdb_get(db, read_options, key, key_size, &ret, &err)) && NULL != err)
		ERROR_LOG_GOTO(ERR, "Cannot read the database: %s", err);

	rocksdb_readoptions_destroy(read_options);

	return ret;

ERR:
	if(NULL != *valbuf) free(*valbuf);
	if(NULL != err) free(err);
	if(NULL != read_options) rocksdb_readoptions_destroy(read_options);

	return ERROR_CODE(size_t);
}

int db_write(rocksdb_t* db, const void* key,  size_t key_size, const void* val, size_t val_size)
{
	char* err = NULL;

	if(NULL == db || NULL == key || NULL == val)
		ERROR_RETURN_LOG(int, "Invalid arguments");

	rocksdb_writeoptions_t* write_options = rocksdb_writeoptions_create();

	if(NULL == write_options) ERROR_LOG_GOTO(ERR, "Cannot allocate write options");

	rocksdb_put(db, write_options, key, key_size, val, val_size, &err);

	if(NULL != err) ERROR_LOG_GOTO(ERR, "Cannot write the data to database: %s", err);

	rocksdb_writeoptions_destroy(write_options);

	return 0;

ERR:
	if(NULL != err) free(err);
	if(NULL != write_options) rocksdb_writeoptions_destroy(write_options);

	return ERROR_CODE(int);
}
