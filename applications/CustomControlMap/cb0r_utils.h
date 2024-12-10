#pragma once

inline string cb0r_type_to_string(cb0r_e type)
{
  switch(type)
  {
    case CB0R_INT: return "INT";
    case CB0R_NEG: return "NEG";
    case CB0R_BYTE: return "BYTE";
    case CB0R_UTF8: return "UTF8";
    case CB0R_ARRAY: return "ARRAY";
    case CB0R_MAP: return "MAP";
    case CB0R_TAG: return "TAG";
    case CB0R_SIMPLE: return "SIMPLE";
    case CB0R_TAGS: return "TAGS";
    case CB0R_DATETIME: return "DATETIME";
    case CB0R_EPOCH: return "EPOCH";
    case CB0R_BIGNUM: return "BIGNUM";
    case CB0R_BIGNEG: return "BIGNEG";
    case CB0R_FRACTION: return "FRACTION";
    case CB0R_BIGFLOAT: return "BIGFLOAT";
    case CB0R_BASE64URL: return "BASE64URL";
    case CB0R_BASE64: return "BASE64";
    case CB0R_HEX: return "HEX";
    case CB0R_DATA: return "DATA";
    case CB0R_SIMPLES: return "SIMPLES";
    case CB0R_FALSE: return "FALSE";
    case CB0R_TRUE: return "TRUE";
    case CB0R_NULL: return "NULL";
    case CB0R_UNDEF: return "UNDEF";
    case CB0R_FLOAT: return "FLOAT";
    case CB0R_ERR: return "ERR";
    case CB0R_EPARSE: return "EPARSE";
    case CB0R_EBAD: return "EBAD";
    case CB0R_EBIG: return "EBIG";
    case CB0R_EMAX: return "EMAX";
  }
  return "UNKNOWN";
}

inline bool cb0r_get_check_type(cb0r_t cbor, uint32_t index, cb0r_s* cbor_data, cb0r_e type)
{
  if(!cb0r_get(cbor, index, cbor_data))
  {
    MLOGE("cb0r_get_check_type", "Failed to get action data");
    return false;
  }
  if(cbor_data->type != type)
  {
    MLOGE("cb0r_get_check_type", "Incorrect action data type - Expected: %s, Got: %s", cb0r_type_to_string(type).c_str(), cb0r_type_to_string(cbor_data->type).c_str());
    return false;
  }
  return true;
}

inline bool cb0r_next(cb0r_t array, cb0r_t prev, cb0r_s* cbor_data)
{
  if(array->type != CB0R_ARRAY && array->type != CB0R_MAP)
  {
    MLOGE("cb0r_next", "Invalid array type");
    return false;
  }
  if(!cbor_data)
  {
    MLOGE("cb0r_next", "Invalid result pointer");
    return false;
  }
  if(prev == NULL || array->start == prev->start)
  {
    cb0r(array->start+array->header, array->end, 0, cbor_data); // You can pass in the same array as prev to get the first element. Useful if you don't have a previous element yet in a for loop.
  }
  else
  {
    cb0r(prev->end, array->end, 0, cbor_data);
  }
  if(cbor_data->type >= CB0R_ERR)
  {
    MLOGE("cb0r_next", "Error action data type - Got: %s", cb0r_type_to_string(cbor_data->type).c_str());
    return false;
  }
  return true;
}

inline bool cb0r_next_check_type(cb0r_t array, cb0r_t prev, cb0r_s* cbor_data, cb0r_e type)
{
  if(!cb0r_next(array, prev, cbor_data))
  {
    MLOGE("cb0r_next_check_type", "Failed to get action data");
    return false;
  }
  if(cbor_data->type != type)
  {
    MLOGE("cb0r_next_check_type", "Incorrect action data type - Expected: %s, Got: %s", cb0r_type_to_string(type).c_str(), cb0r_type_to_string(cbor_data->type).c_str());
    return false;
  }

  return true;
}