#include  <string.h>
#include  <errno.h>
#include  <ctype.h>
#include  <stddef.h>

#ifdef  _TARGET_PC_WIN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // WinXP by default
#endif
#endif
#if _TARGET_PC_WIN|_TARGET_XBOX
# include <Ws2tcpip.h>
# if (NTDDI_VERSION >= NTDDI_VISTA)
#   define HAVE_NTOP 1
# endif
#else
# include <sys/socket.h>
# include <sys/types.h>
#endif

#if !defined(HAVE_NTOP)

#define NS_INADDRSZ 4

/*
 * copied from bionic libc which under BSD license
 */
static int  inet_pton4(const char *src, unsigned char *dst)
{
  unsigned int val;
  unsigned int digit, base;
  ptrdiff_t n;
  unsigned char c;
  unsigned int parts[4];
  unsigned int *pp = parts;

  c = *src;
  for (;;) {
    /*
     * Collect number up to ``.''.
     * Values are specified as for C:
     * 0x=hex, 0=octal, isdigit=decimal.
     */
    if (!isdigit(c))
      return (0);
    val = 0; base = 10;
    if (c == '0') {
      c = *++src;
      if (c == 'x' || c == 'X')
        base = 16, c = *++src;
      else if (isdigit(c) && c != '9')
        base = 8;
    }
    /* inet_pton() takes decimal only */
    if (base != 10)
      return (0);
    for (;;) {
      if (isdigit(c)) {
        digit = c - '0';
        if (digit >= base)
          break;
        val = (val * base) + digit;
        c = *++src;
      } else if (base == 16 && isxdigit(c)) {
        digit = c + 10 - (islower(c) ? 'a' : 'A');
        if (digit >= 16)
          break;
        val = (val << 4) | digit;
        c = *++src;
      } else
        break;
    }
    if (c == '.') {
      /*
       * Internet format:
       *  a.b.c.d
       *  a.b.c (with c treated as 16 bits)
       *  a.b (with b treated as 24 bits)
       *  a (with a treated as 32 bits)
       */
      if (pp >= parts + 3)
        return (0);
      *pp++ = val;
      c = *++src;
    } else
      break;
  }
  /*
   * Check for trailing characters.
   */
  if (c != '\0' && !isspace(c))
    return (0);
  /*
   * Concoct the address according to
   * the number of parts specified.
   */
  n = pp - parts + 1;
  /* inet_pton() takes dotted-quad only.  it does not take shorthand. */
  if (n != 4)
    return (0);
  switch (n) {

  case 0:
    return (0);   /* initial nondigit */

  case 1:       /* a -- 32 bits */
    break;

  case 2:       /* a.b -- 8.24 bits */
    if (parts[0] > 0xff || val > 0xffffff)
      return (0);
    val |= parts[0] << 24;
    break;

  case 3:       /* a.b.c -- 8.8.16 bits */
    if ((parts[0] | parts[1]) > 0xff || val > 0xffff)
      return (0);
    val |= (parts[0] << 24) | (parts[1] << 16);
    break;

  case 4:       /* a.b.c.d -- 8.8.8.8 bits */
    if ((parts[0] | parts[1] | parts[2] | val) > 0xff)
      return (0);
    val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
    break;
  }
  if (dst) {
    val = htonl(val);
    memcpy(dst, &val, NS_INADDRSZ);
  }
  return (1);
}


/* int
 * inet_pton(af, src, dst)
 *  convert from presentation format (which usually means ASCII printable)
 *  to network format (which is usually some kind of binary format).
 * return:
 *  1 if the address was valid for the specified address family
 *  0 if the address wasn't valid (`dst' is untouched in this case)
 *  -1 if some other error occurred (`dst' is untouched in this case, too)
 * author:
 *  Paul Vixie, 1996.
 */
int inet_pton_compat(int af, const char *src, void *dst)
{
  switch (af) 
  {
  case AF_INET:
    return (inet_pton4(src, dst));
  default:
    errno = EAFNOSUPPORT;
    return (-1);
  }
}

#endif
