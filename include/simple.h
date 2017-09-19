/**
 * Copyright (C) 2017, Hao Hou
 **/
/**
  * @brief The simple mode
  * @file simple.h
  **/
#ifndef __SIMPLE_H__

/**
 * @brief The context of the simple mode
 **/
typedef struct {
	pipe_t                cmd;        /*!< The command pipe */
	pipe_t                data_in;    /*!< The data pipe */
	pipe_t                data_out;   /*!< The data output pipe */
	pstd_type_accessor_t  cmd_tk_ac;  /*!< The command token accessor */
	pstd_type_accessor_t  din_tk_ac;  /*!< The data input token accessor */
	pstd_type_accessor_t  dout_tk_ac; /*!< The data output token accessor */
} simple_t;

/**
 * @brief The async buffer for the simple mode
 **/
typedef struct {
	rocksdb_t*      db;   /*!< The database */
	const void*     key;  /*!< The key for this data */
	size_t          key_size; /*!< The key size */
	const void*     in_val;  /*!< The value pointer, if val is NULL, this means we are performing get operation */
	size_t          in_val_size; /*!< The value size, for the get operation, this is input, for put operation, this is output */
	void*           out_val;   /*!< The out value bufferr */
	size_t          out_val_size; /*!< The output value buffer */
} simple_async_buf_t;

/**
 * @brief Initailize the simple context
 * @param ctx The context buffer
 * @param type_model The type model we used for this servlet
 * @param readonly If this servlet should be in the readonly mode
 * @return status code
 **/
int simple_ctx_init(simple_t* ctx, pstd_type_model_t* type_model, int readonly);

/**
 * @brief The synchnozied servlet execution
 * @param ctx The context
 * @param db The database
 * @param inst THe type instance
 * @return status code
 **/
int simple_sync_exec(simple_t* ctx, pstd_type_instance_t* inst, rocksdb_t* db);

/**
 * @brief Setup the asynchrnozed task execution
 * @param ctx The context
 * @param inst The type instance
 * @param db The database isntance
 * @param async_handle Current async handle
 * @param async_buf The async buffer
 * @return status code
 **/
int simple_async_setup(simple_t* ctx, pstd_type_instance_t* inst, rocksdb_t* db, async_handle_t* async_handle, simple_async_buf_t* async_buf);

/**
 * @brief The execution function
 * @param async_handle The async handle
 * @param async_buf The async buffer
 * @return status code 
 **/
int simple_async_exec(async_handle_t* async_handle, simple_async_buf_t* async_buf);

/**
 * @brief Cleanup the async task
 * @param ctx The simple context
 * @param inst The type instane
 * @param async_handle The async handle
 * @param async_buf The async buffer
 * @return status code
 **/
int simple_async_cleanup(simple_t* ctx, pstd_type_instance_t* inst, async_handle_t* async_handle, simple_async_buf_t* async_buf);


#endif /* __SIMPLE_H__ */
