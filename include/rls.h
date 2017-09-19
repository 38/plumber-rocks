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
 * @param sizebuf If given a non-NULL pointer, we returns the size of the
 *        RLS object via this pointer
 * @return The actual string content
 **/
static inline const char* rls_read_string(pstd_type_instance_t* inst, pstd_type_accessor_t accessor, size_t* sizebuf)
{
       scope_token_t token;
       if(ERROR_CODE(scope_token_t) == (token = PSTD_TYPE_INST_READ_PRIMITIVE(scope_token_t, inst, accessor)))
           ERROR_PTR_RETURN_LOG("Cannot access token field");
       
       const pstd_string_t* pstr = pstd_string_from_rls(token);
       if(NULL == pstr) ERROR_PTR_RETURN_LOG("Cannot retrive string object from the RLS");

       if(sizebuf != NULL) *sizebuf = pstd_string_length(pstr) + 1;

       return pstd_string_value(pstr);

}
#endif
