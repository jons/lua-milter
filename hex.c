
#include <stdlib.h>

/**
 */
int sscanh(unsigned char *dest, size_t maxlen, char *src, size_t len)
{
  char c;
  int i, j = 0;
  if (maxlen <= (len/2))
    return -1;
  for (i = 0; i < len; i+=2)
  {
    if (src[i] < 'A') c = (src[i] - '0');
    else if (src[i] < 'a') c = (src[i] - 'A' + 10);
    else c = (src[i] - 'a' + 10);
    dest[j] = (unsigned char)c << 4;
    if (src[i+1] < 'A') c = (src[i+1] - '0');
    else if (src[i+1] < 'a') c = (src[i+1] - 'A' + 10);
    else c = (src[i+1] - 'a' + 10);
    dest[j++] |= (unsigned char)c;
  }
  return j;
}

/**
 */
int sprinth(char *dest, size_t maxlen, char *src, size_t len)
{
  unsigned char c;
  size_t i, j = 0;
  if (maxlen <= (len*2))
    return -1;
  for (i = 0; i < len; i++)
  {
    // TODO: if j is too close to INT_MAX, stop early
    c = 0xf &((unsigned char)src[i] >> 4);
    if (c < 10) c += '0';
    else if (c < 16) c += 'a'-10;
    dest[j++] = (char)c;
    c = 0xf & (unsigned char)src[i];
    if (c < 10) c += '0';
    else if (c < 16) c += 'a'-10;
    dest[j++] = (char)c;
  }
  dest[j]='\0';
  return j;
}

