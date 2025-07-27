#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cb0r.h"
#include "unit_test.h"

static inline const char hexcode(const char x)
{
    if (x >= '0' && x <= '9')         /* 0-9 is offset by hex 30 */
      return (x - 0x30);
    else if (x >= 'A' && x <= 'F')    /* A-F offset by hex 37 */
      return(x - 0x37);
    else if (x >= 'a' && x <= 'f')    /* a-f offset by hex 37 */
      return(x - 0x57);
    else {                            /* Otherwise, an illegal hex digit */
      return x;
    }
}

uint32_t unhex(const char *in, uint8_t *out)
{
  uint32_t j;
  uint8_t *c = out;
  if(!out || !in) return 0;
  uint32_t len = strlen(in);

  for(j=0; (j+1)<len; j+=2)
  {
    *c = ((hexcode(in[j]) * 16) & 0xF0) + (hexcode(in[j+1]) & 0xF);
    c++;
  }
  return len/2;
}

int main(int argc, char **argv)
{
  struct cb0r_s res_s = {0,};
  cb0r_t res = &res_s;
  uint8_t *end = NULL;
  uint8_t start[256] = {0,};
  uint32_t len = 0;

  len = unhex("01",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_INT);
  fail_unless(res->value == 1);

  len = unhex("17",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_INT);
  fail_unless(res->value == 23);

  len = unhex("1818",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_INT);
  fail_unless(res->value == 24);

  len = unhex("190100",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_INT);
  fail_unless(res->value == 256);

  len = unhex("1a00010000",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_INT);
  fail_unless(res->value == 65536);

  len = unhex("1b00000000ffffffff",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_INT);
  fail_unless(res->value == 4294967295);

  len = unhex("20",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_NEG);
  fail_unless(-1 - res->value == -1);

  len = unhex("390100",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_NEG);
  fail_unless(-1 - res->value == -257);

  len = unhex("4161",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_BYTE);
  fail_unless(memcmp(res->start+res->header,"a",res->length) == 0);

  len = unhex("4568656c6c6f",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_BYTE);
  fail_unless(memcmp(res->start+res->header,"hello",res->length) == 0);

  len = unhex("581868656c6c6f6f6f6f6f6f6f6f6f6f6f6f6f6f206e75727365",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_BYTE);
  fail_unless(memcmp(res->start+res->header,"helloooooooooooooo nurse",res->length) == 0);

  len = unhex("6161",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_UTF8);
  fail_unless(memcmp(res->start+res->header,"a",res->length) == 0);

  len = unhex("80",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_ARRAY);
  fail_unless(res->count == 0);

  len = unhex("8100",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_ARRAY);
  fail_unless(res->count == 1);

  len = unhex("820102",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_ARRAY);
  fail_unless(res->count == 2);

  len = unhex("818100",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_ARRAY);
  fail_unless(res->count == 1);

  len = unhex("a1616100",start);
  end = cb0r(start, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_MAP);
  fail_unless(res->count == 2);

  len = unhex("0117",start);
  end = cb0r(start, start+len, 0, NULL);
  end = cb0r(end, start+len, 0, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_INT);
  fail_unless(res->value == 23);

  len = unhex("0117",start);
  end = cb0r(start, start+len, 1, res);
  fail_unless(end == start+len);
  fail_unless(res->type == CB0R_INT);
  fail_unless(res->value == 23);

  return 0;
}

