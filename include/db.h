/**
 * Copyright (C) 2017, Hao Hou
 **/
#ifndef __DB_H__

/**
 * @brief Acquire the database defined by the option set
 * @param options The servlet options
 * @return The rocksdb insance
 * @note We shares the rocksdb instance amamong different servlets if
 *       they are using the same one
 **/
rocksdb_t* db_acquire(const options_t* options);

/**
 * @brief Release the rocksdb pointer, if there's multiple user, and 
 *        it won't get disposed until the last user release the DB
 * @param db The db to dispose
 * @return status code
 **/
int db_release(rocksdb_t* db);

/**
 * @brief Read data from the database
 * @param db The database to read
 * @param key The key we want to read
 * @param valbuf The value buffer
 * @return the size has been read or error code
 * @note The function will creates a new pointer, so the caller should dispose it 
 *       by calling free(mem) after done
 **/
size_t db_read(rocksdb_t* db, const void* key, size_t key_size, void** valbuf);

/**
 * @brief Write the data to the database
 * @param db The database isntance
 * @param key The key to write
 * @param key_size The size of the key
 * @param val_size The value size
 * @return the status code
 **/
int db_write(rocksdb_t* db, const void* key,  size_t key_size, const void* val, size_t val_size);

#endif /* __DB_H__ */
