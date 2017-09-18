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

#endif /* __DB_H__ */
