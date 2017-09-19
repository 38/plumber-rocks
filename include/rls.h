/**
 * Copyright (C) 2017, Hao Hou
 **/
/**
 * @brief The RLS string related operations
 * @file rls.h
 **/
#ifndef __RLS_H__
/**
 * @brief Read a RLS string content from the RLS
 * @param inst The type instance
 * @param accessor The accessor used for the RLS token
 * @param retbuf The buffer used to return the result
 * @param sizebuf If given a non-NULL pointer, we returns the size of the
 *        RLS object via this pointer
 * @return status code
 **/
static inline int rls_read_string(pstd_type_instance_t* inst, pstd_type_accessor_t accessor, char const**retbuf, size_t* sizebuf)
{
	if(inst == NULL || retbuf == NULL) ERROR_RETURN_LOG(int, "Invalid arguments");

	scope_token_t token;
	if(ERROR_CODE(scope_token_t) == (token = PSTD_TYPE_INST_READ_PRIMITIVE(scope_token_t, inst, accessor)))
	   ERROR_RETURN_LOG(int, "Cannot access token field");

	size_t rc = pstd_type_instance_read(inst, accessor, &token, sizeof(token));

	if(rc == 0)
	{
		*retbuf = NULL;
		if(sizebuf != NULL) *sizebuf = 0;
		return 0;
	}

	const pstd_string_t* pstr = pstd_string_from_rls(token);
	if(NULL == pstr) ERROR_RETURN_LOG(int, "Cannot retrive string object from the RLS");

	if(sizebuf != NULL) *sizebuf = pstd_string_length(pstr) + 1;

	*retbuf = pstd_string_value(pstr);

	return 0;
}
#endif
