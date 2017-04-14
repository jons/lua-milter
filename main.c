/**
 *
 *
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "libmilter/mfapi.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define EH_NEGOTIATE  (0)
#define EH_CONNECT    (1)
#define EH_UNKNOWN    (2)
#define EH_HELO       (3)
#define EH_ENVFROM    (4)
#define EH_ENVRCPT    (5)
#define EH_DATA       (6)
#define EH_HEADER     (7)
#define EH_EOH        (8)
#define EH_BODY       (9)
#define EH_EOM        (10)
#define EH_ABORT      (11)
#define EH_CLOSE      (12)
#define NUMEHRS       (13)//number of event handler references

typedef struct envelope envelope_t;
typedef struct lmdesc lmdesc_t;

/**
 */
struct envelope
{
  lua_State *T;
  int Tref;
  char sid[2];
};

/**
 * the Milter object that is injected into our lua state
 */
struct lmdesc
{
  char *connstr;
  int flags;
};


/**
 */
const char *g_name = "lua-bindings";

/**
 */
static const char *Milter_typename = "Milter";

/**
 */
lua_State *L;
int L_refs[NUMEHRS] = { LUA_REFNIL };
pthread_mutex_t lock_L = PTHREAD_MUTEX_INITIALIZER;


/**
 */
static int setwrap (lua_State *S, const char *fcall, const int eh)
{
  int n = lua_gettop(S);
  if (n < 1)
  {
    luaL_error(S, "set%s: missing argument\n", fcall);
  }
  else if (!lua_isfunction(S, n))
  {
    luaL_error(S, "set%s: invalid type %s, function expected\n", fcall, luaL_typename(S, n));
  }
  else
//  if (pthread_mutex_lock(&lock_L))
    L_refs[eh] = luaL_ref(S, LUA_REGISTRYINDEX);
//    pthread_mutex_unlock(&lock_L);
}


/**
 */
static int Milter_set_negotiate (lua_State *S)
{
  setwrap(S, "Negotiate", EH_NEGOTIATE);
  return 0;
}


/**
 */
static int Milter_set_connect (lua_State *S)
{
  setwrap(S, "Connect", EH_CONNECT);
  return 0;
}


/**
 */
static int Milter_set_unknown (lua_State *S)
{
  setwrap(S, "Unknown", EH_UNKNOWN);
  return 0;
}


/**
 */
static int Milter_set_helo (lua_State *S)
{
  setwrap(S, "HELO", EH_HELO);
  return 0;
}


/**
 */
static int Milter_set_envfrom (lua_State *S)
{
  setwrap(S, "ENVFROM", EH_ENVFROM);
  return 0;
}


/**
 */
static int Milter_set_envrcpt (lua_State *S)
{
  setwrap(S, "ENVRCPT", EH_ENVRCPT);
  return 0;
}


/**
 */
static int Milter_set_data (lua_State *S)
{
  setwrap(S, "DATA", EH_DATA);
  return 0;
}


/**
 */
static int Milter_set_header (lua_State *S)
{
  setwrap(S, "Header", EH_HEADER);
  return 0;
}


/**
 */
static int Milter_set_eoh (lua_State *S)
{
  setwrap(S, "EOH", EH_EOH);
  return 0;
}


/**
 */
static int Milter_set_body (lua_State *S)
{
  setwrap(S, "Body", EH_BODY);
  return 0;
}


/**
 */
static int Milter_set_eom (lua_State *S)
{
  setwrap(S, "EOM", EH_EOM);
  return 0;
}


/**
 */
static int Milter_set_abort (lua_State *S)
{
  setwrap(S, "Abort", EH_ABORT);
  return 0;
}


/**
 */
static int Milter_set_close (lua_State *S)
{
  setwrap(S, "Close", EH_CLOSE);
  return 0;
}


/**
 */
static int Milter_create (lua_State *S)
{
  lmdesc_t *d;
  d = lua_newuserdata(S, sizeof(lmdesc_t));
  if (NULL == d)
  {
    fprintf(stderr, "milter_create failed!\n");
    return 0;
  }
  d->connstr = NULL;
  d->flags = 0; //optional
  luaL_setmetatable(S, Milter_typename);
  return 1;
}


/**
 */
static int Milter_set_connstr (lua_State *S)
{
  lmdesc_t *d;
  char *s;
  size_t len;
  int n = lua_gettop(S);

  d = luaL_checkudata(S, 1, Milter_typename);
  if (NULL == d)
    luaL_error(S, "bad type %s\n", luaL_typename(S, 1));
  if (n < 1)
  {
    lua_pushliteral(S, "missing argument");
    lua_error(S);
  }
  if (!lua_isstring(S, 2))
  {
    lua_pushliteral(S, "invalid argument, string expected");
    lua_error(S);
  }
  s = (char *)lua_tolstring(S, 2, &len);
  if (NULL == s)
    fprintf(stderr, "lua_tolstring failed!\n");
  d->connstr = s;//(char *)malloc(strlen(s)+1); copy?
  return 1;
}


/**
 */
static int Milter_set_flags (lua_State *S)
{
  lmdesc_t *d;
  int n = lua_gettop(S);
  d = luaL_checkudata(S, 1, Milter_typename);
  if (NULL == d)
    luaL_error(S, "bad type %s\n", luaL_typename(S, 1));
  if (n < 1)
  {
    lua_pushliteral(S, "missing argument");
    lua_error(S);
  }
  if (!lua_isnumber(S, 2))
  {
    lua_pushliteral(S, "invalid argument, number expected");
    lua_error(S);
  }
  d->flags = lua_tonumber(S, 2);
  return 1;
}


/**
 */
static int Milter_gc (lua_State *S)
{
  lmdesc_t *d = luaL_checkudata(S, 1, Milter_typename);
  const char *t = luaL_typename(S, 1);
  if (NULL == d)
    luaL_error(S, "gc failed type %s\n", t);
  return 0;
}


/**
 */
static int luaopen_Milter (lua_State *S)
{
  static const luaL_Reg Milter_objdesc[] =
  {
    { "setConnection", &Milter_set_connstr },
    { "setFlags",      &Milter_set_flags },
    { "setNegotiate",  &Milter_set_negotiate },
    { "setConnect",    &Milter_set_connect },
    { "setUnknown",    &Milter_set_unknown },
    { "setHELO",       &Milter_set_helo },
    { "setENVFROM",    &Milter_set_envfrom },
    { "setENVRCPT",    &Milter_set_envrcpt },
    { "setDATA",       &Milter_set_data },
    { "setHeader",     &Milter_set_header },
    { "setEOH",        &Milter_set_eoh },
    { "setBody",       &Milter_set_body },
    { "setEOM",        &Milter_set_eom },
    { "setAbort",      &Milter_set_abort },
    { "setClose",      &Milter_set_close },
    { NULL, NULL }
  };
  static const luaL_Reg Milter_libdesc[] =
  {
    { "create", &Milter_create },
    { NULL, NULL }
  };

  luaL_newlib(S, Milter_libdesc);
  lua_pushstring(S, "SMFIS_ACCEPT");
  lua_pushinteger(S, SMFIS_ACCEPT);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIS_CONTINUE");
  lua_pushinteger(S, SMFIS_CONTINUE);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIS_DISCARD");
  lua_pushinteger(S, SMFIS_DISCARD);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIS_REJECT");
  lua_pushinteger(S, SMFIS_REJECT);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIS_TEMPFAIL");
  lua_pushinteger(S, SMFIS_TEMPFAIL);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIS_SKIP");
  lua_pushinteger(S, SMFIS_SKIP);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIS_NOREPLY");
  lua_pushinteger(S, SMFIS_NOREPLY);
  lua_settable(S, -3);
  luaL_newmetatable(S, Milter_typename);

  luaL_newlib(S, Milter_objdesc);
  lua_setfield(S, -2, "__index");
  lua_pushstring(S, "__gc");
  lua_pushcfunction(S, Milter_gc);
  lua_settable(S, -3);
  lua_pop(S, 1);
  return 1;
}


/**
 */
static int trace (lua_State *S)
{
  fprintf(stderr, "Error: %s\n", lua_tostring(S, 1));
  luaL_traceback(S, S, NULL, -1);
  return 0;
}


/**
 */
int fire (lua_State *T, const int ref)
{
  int ti, r, retval = SMFIS_CONTINUE;
  lua_pushcfunction(T, trace);
  ti = lua_gettop(T);
  r = lua_rawgeti(T, LUA_REGISTRYINDEX, ref);
  if (LUA_TFUNCTION == r)
  {
    // TODO: need a function to create and populate table for envelope param
    // TODO: a de/serializer would be good maybe. maybe just ref it?
    lua_newtable(T);
    // TODO: push the rest of the args for the event, set the pcall nargs correctly
    r = lua_pcall(T, 1, 1, ti);
    ti = lua_gettop(T);
    if (LUA_OK == r && lua_isinteger(T, ti))
      retval = lua_tointeger(T, ti);
  }
  lua_pop(T, 2);
  return retval;
}


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
  int f, r = SMFIS_ALL_OPTS;
  envelope_t *envelope = (envelope_t *)malloc(sizeof(envelope_t));
  smfi_setpriv(context, envelope);
  if (NULL == envelope)
  {
    fprintf(stderr, "smfi_negotiate: failed to allocate envelope, continuing\n");
  }
  else if (pthread_mutex_lock(&lock_L))
  {
    fprintf(stderr, "smfi_negotiate: lua_State lock failed, continuing\n");
  }
  else
  {
    // negotiate is the first event of a new mail session and only fires once
    // so we create a lua thread here that will service all of its events
    envelope->T = lua_newthread(L);
    envelope->Tref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pop(L, 1);
    f = L_refs[EH_NEGOTIATE];
    pthread_mutex_unlock(&lock_L);

    fprintf(stderr, "fire(negotiate)\n");
    r = fire(envelope->T, f);
  }
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
  int f = LUA_REFNIL, r = SMFIS_CONTINUE;

  if (!pthread_mutex_lock(&lock_L))
  {
    f = L_refs[EH_CLOSE];
    pthread_mutex_unlock(&lock_L);
  }

  fprintf(stderr, "fire(close)\n");
  r = fire(envelope->T, f);

  if (!pthread_mutex_lock(&lock_L))
  {
    luaL_unref(L, LUA_REGISTRYINDEX, envelope->Tref);
    pthread_mutex_unlock(&lock_L);
  }
  free(envelope);
  smfi_setpriv(context, NULL);
  return r;
}


/**
 */
int main (int argc, char **argv)
{
  struct smfiDesc desc;
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
    return -1;
  }
  r = lua_pcall(L, 0, LUA_MULTRET, 0);
  if (LUA_OK != r)
  {
    fprintf(stderr, "%s: script error: %s\n", argv[0], lua_tostring(L, -1));
    lua_close(L);
    return -1;
  }

  lmdesc_t *milterdesc = luaL_testudata(L, 1, Milter_typename);
  if (NULL == milterdesc)
  {
    fprintf(stderr, "%s: script %s must return milter object\n", argv[0], argv[1]);
    lua_close(L);
    return -1;
  }

  // the connection string and flags are available now
  // hand over the main thread to libmilter
  desc.xxfi_name      = g_name;
  desc.xxfi_version   = SMFI_VERSION;
  desc.xxfi_flags     = milterdesc->flags;
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
  r = smfi_register(desc);
  if (MI_SUCCESS != r)
  {
    fprintf(stderr, "%s: smfi_register failed\n", argv[0]);
    lua_close(L);
    return -1;
  }
  r = smfi_setconn(milterdesc->connstr);
  if (MI_SUCCESS != r)
  {
    fprintf(stderr, "%s: smfi_setconn(%s) failed\n", argv[0], milterdesc->connstr);
    lua_close(L);
    return -1;
  }

  // smfi_main blocks until an event or signal handler calls smfi_stop
  r = smfi_main();

  // probably don't have to join lua threads before close, but maybe.
  lua_close(L);

  return 0;
}
