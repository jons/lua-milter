/**
 *
 *
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "libmilter/mfapi.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


typedef struct envelope envelope_t;
typedef struct lmdesc lmdesc_t;

struct envelope
{
  char sid[2];
  //
};

struct lmdesc
{
  char *connstr;
  int flags;
};


sfsistat fi_negotiate (SMFICTX *context,
                           unsigned long f0,
                           unsigned long f1,
                           unsigned long f2,
                           unsigned long f3,
                           unsigned long *pf0,
                           unsigned long *pf1,
                           unsigned long *pf2,
                           unsigned long *pf3)
{
  int r = SMFIS_ALL_OPTS;
  envelope_t *envelope = (envelope_t *)malloc(sizeof(envelope_t));
  smfi_setpriv(context, envelope);
  //r = fire_event(...);
  return r;
}


sfsistat fi_connect (SMFICTX *context, char *host, _SOCK_ADDR *sa)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  return r;
}


sfsistat fi_unknown (SMFICTX *context, const char *command)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  return r;
}

sfsistat fi_helo (SMFICTX *context, char *helo)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  return r;
}

sfsistat fi_envfrom (SMFICTX *context, char **argv)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  return r;
}

sfsistat fi_envrcpt (SMFICTX *context, char **argv)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  return r;
}

sfsistat fi_data (SMFICTX *context)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  return r;
}

sfsistat fi_header (SMFICTX *context, char *name, char *value)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  return r;
}


sfsistat fi_eoh (SMFICTX *context)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  return r;
}


sfsistat fi_body (SMFICTX *context, unsigned char *segment, size_t len)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  return r;
}


sfsistat fi_eom (SMFICTX *context)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  return r;
}


sfsistat fi_abort (SMFICTX *context)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  return r;
}

sfsistat fi_close (SMFICTX *context)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  free(envelope);
  smfi_setpriv(context, NULL);
  return r;
}


/**
 */
const char *g_name = "lua-bindings";



/**
 * start the milter service
 * blocking fcall to smfi_main
 */
void milter_start (char *name, char *connstr, int flags)
{
  int r;
  struct smfiDesc desc;

  desc.xxfi_name      = name;
  desc.xxfi_version   = SMFI_VERSION;
  desc.xxfi_flags     = flags;
  desc.xxfi_negotiate = fi_negotiate;
  desc.xxfi_connect   = fi_connect;
  desc.xxfi_unknown   = fi_unknown;
  desc.xxfi_helo      = fi_helo;
  desc.xxfi_envfrom   = fi_envfrom;
  desc.xxfi_envrcpt   = fi_envrcpt;
  desc.xxfi_data      = fi_data;
  desc.xxfi_header    = fi_header;
  desc.xxfi_eoh       = fi_eoh;
  desc.xxfi_body      = fi_body;
  desc.xxfi_eom       = fi_eom;
  desc.xxfi_abort     = fi_abort;
  desc.xxfi_close     = fi_close;
#if 0
  // signal handler is not implemented yet
  desc.xxfi_signal    = fi_signal;
#endif

  if (MI_SUCCESS == (r = smfi_register(desc)))
  {
    if (MI_SUCCESS == (r = smfi_setconn(connstr)))
    {
      r = smfi_main();// blocks. put it in a thread!
    }
  }
  return r;
}


void milter_stop ()
{
  int r;
  r = smfi_stop();
}

static const char *Milter_typename = "Milter";


static int Milter_start (lua_State *L)
{
  lmdesc_t *d;
  char *s;
  int f = 0, n = lua_gettop(L);
  if (n < 1)
  {
    lua_pushliteral(L, "missing argument");
    lua_error(L);
  }
  if (!lua_isstring(L, 1))
  {
    lua_pushliteral(L, "invalid argument, string expected");
    lua_error(L);
  }
  if (n > 1)
  {
    if (!lua_isnumber(L, 2))
    {
      lua_pushliteral(L, "invalid argument, number expected");
      lua_error(L);
    }
    f = lua_tonumber(L, 2);
  }
  s = (char *)lua_tostring(L, 1);

  fprintf(stderr, "milter_start(\"%s\", \"%s\", %d)\n", g_name, s, f);

  d = lua_newuserdata(L, sizeof(lmdesc_t));
  d->connstr = s;
  d->flags = f;
  luaL_setmetatable(L, Milter_typename);
  return 1;
}

static int Milter_connstr (lua_State *L)
{
  lmdesc_t *d = luaL_checkudata(L, 1, Milter_typename);
  if (NULL == d)
    luaL_error(L, "bad type %s\n", luaL_typename(L, 1));
  lua_pushstring(L, d->connstr);
  return 1;
}


static int Milter_gc (lua_State *L)
{
  lmdesc_t *d = luaL_checkudata(L, 1, Milter_typename);
  const char *t = luaL_typename(L, 1);
  if (NULL == d)
    luaL_error(L, "gc failed type %s\n", t);
  fprintf(stderr, "freeing %s\n", t);
  //free(d);
  return 0;
}


static int luaopen_Milter (lua_State *L)
{
  static const luaL_Reg Milter_objdesc[] =
  {
    { "connstr", &Milter_connstr },
    { NULL, NULL }
  };
  static const luaL_Reg Milter_libdesc[] =
  {
    { "start", &Milter_start },
    { NULL, NULL }
  };

  luaL_newlib(L, Milter_libdesc);
  lua_pushstring(L, "SMFIS_ACCEPT");
  lua_pushinteger(L, SMFIS_ACCEPT);
  lua_settable(L, -3);
  lua_pushstring(L, "SMFIS_CONTINUE");
  lua_pushinteger(L, SMFIS_CONTINUE);
  lua_settable(L, -3);
  lua_pushstring(L, "SMFIS_DISCARD");
  lua_pushinteger(L, SMFIS_DISCARD);
  lua_settable(L, -3);
  lua_pushstring(L, "SMFIS_REJECT");
  lua_pushinteger(L, SMFIS_REJECT);
  lua_settable(L, -3);
  lua_pushstring(L, "SMFIS_TEMPFAIL");
  lua_pushinteger(L, SMFIS_TEMPFAIL);
  lua_settable(L, -3);
  lua_pushstring(L, "SMFIS_SKIP");
  lua_pushinteger(L, SMFIS_SKIP);
  lua_settable(L, -3);
  lua_pushstring(L, "SMFIS_NOREPLY");
  lua_pushinteger(L, SMFIS_NOREPLY);
  lua_settable(L, -3);

  luaL_newmetatable(L, Milter_typename);

  luaL_newlib(L, Milter_objdesc);
  lua_setfield(L, -2, "__index");
  lua_pushstring(L, "__gc");
  lua_pushcfunction(L, Milter_gc);
  lua_settable(L, -3);
  lua_pop(L, 1);
  return 1;
}



int main (int argc, char **argv)
{
  lua_State *L;
  int r;

  if (argc < 2)
  {
    fprintf(stderr, "usage: %s <script>\n", argv[0]);
    return -1;
  }
  if (access(argv[1], R_OK))
  {
    fprintf(stderr, "%s: cannot read %s\n", argv[0], argv[1]);
    return -1;
  }

  L = luaL_newstate();
  luaL_openlibs(L);
  luaL_requiref(L, Milter_typename, &luaopen_Milter, 1);
  lua_pop(L, 1);

  r = luaL_loadfile(L, argv[1]);
  if (r)
  {
    fprintf(stderr, "%s: luaL_loadfile: %s\n", argv[0], lua_tostring(L, -1));
    lua_close(L);
    return 0;
  }
  r = lua_pcall(L, 0, LUA_MULTRET, 0);
  if (r)
    fprintf(stderr, "%s: script error: %s\n", argv[0], lua_tostring(L, -1));

  lua_getglobal(L, "helo");
  lua_newtable(L);
  lua_pcall(L, 1, 1, 0);
  lua_pop(L, 1);

  lua_close(L);

  return 0;
}
