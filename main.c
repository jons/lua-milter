/**
 *
 *
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "libmilter/mfapi.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "hex.h"


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
  char sid[2];
  lua_State *T; // lua thread
  int event;    // the current event
  int Tref;     // ref for thread to keep it alive
  int Eref;     // ref for envelope table
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
char *g_name = "lua-bindings";

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
static int Milter_smfi_version (lua_State *S)
{
  unsigned int major, minor, patch;
  smfi_version(&major, &minor, &patch);
  lua_pushinteger(S, major);
  lua_pushinteger(S, minor);
  lua_pushinteger(S, patch);
  return 3;
}


/**
 */
static int Milter_smfi_setsymlist (lua_State *S)
{
  SMFICTX *ctx;
  char *macros;
  int stage, r = MI_FAILURE, n = lua_gettop(S);
  if (n < 3)
  {
    lua_pushliteral(S, "smfi_setsymlist: missing argument");
    lua_error(S);
  }
  lua_pushliteral(S, "smfictx");//put this at 4
  lua_rawget(S, 1);
  stage = lua_tointeger(S, 2);
  macros = (char *)lua_tostring(S, 3);
  ctx = (SMFICTX *)lua_topointer(S, 4);
  r = smfi_setsymlist(ctx, stage, macros);
  lua_pushinteger(S, r);
  return 1;
}


/**
 */
static int Milter_smfi_getsymval (lua_State *S)
{
  SMFICTX *ctx;
  char *symname, *symval;
  int n = lua_gettop(S);
  if (n < 2)
  {
    lua_pushliteral(S, "smfi_getsymval: missing argument");
    lua_error(S);
  }
  //symval = smfi_getsymval(ctx, symname);
  return 1;
}


/**
 * this wrapper is used by all the set* functions to let the lua user
 * tell this app which functions are their event handlers
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
  return 0;
}


/**
 */
static int Milter_set_negotiate (lua_State *S)
{
  return setwrap(S, "Negotiate", EH_NEGOTIATE);
}


/**
 */
static int Milter_set_connect (lua_State *S)
{
  return setwrap(S, "Connect", EH_CONNECT);
}


/**
 */
static int Milter_set_unknown (lua_State *S)
{
  return setwrap(S, "Unknown", EH_UNKNOWN);
}


/**
 */
static int Milter_set_helo (lua_State *S)
{
  return setwrap(S, "HELO", EH_HELO);
}


/**
 */
static int Milter_set_envfrom (lua_State *S)
{
  return setwrap(S, "ENVFROM", EH_ENVFROM);
}


/**
 */
static int Milter_set_envrcpt (lua_State *S)
{
  return setwrap(S, "ENVRCPT", EH_ENVRCPT);
}


/**
 */
static int Milter_set_data (lua_State *S)
{
  return setwrap(S, "DATA", EH_DATA);
}


/**
 */
static int Milter_set_header (lua_State *S)
{
  return setwrap(S, "Header", EH_HEADER);
}


/**
 */
static int Milter_set_eoh (lua_State *S)
{
  return setwrap(S, "EOH", EH_EOH);
}


/**
 */
static int Milter_set_body (lua_State *S)
{
  return setwrap(S, "Body", EH_BODY);
}


/**
 */
static int Milter_set_eom (lua_State *S)
{
  return setwrap(S, "EOM", EH_EOM);
}


/**
 */
static int Milter_set_abort (lua_State *S)
{
  return setwrap(S, "Abort", EH_ABORT);
}


/**
 */
static int Milter_set_close (lua_State *S)
{
  return setwrap(S, "Close", EH_CLOSE);
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
    { "create",      &Milter_create },
    { "version",     &Milter_smfi_version },
    { "setsymlist",  &Milter_smfi_setsymlist },
    { NULL, NULL }
  };

  luaL_newlib(S, Milter_libdesc);
  lua_pushstring(S, "SMFI_VERSION");
  lua_pushinteger(S, SMFI_VERSION);
  lua_settable(S, -3);

  lua_pushstring(S, "MI_SUCCESS");
  lua_pushinteger(S, MI_SUCCESS);
  lua_settable(S, -3);
  lua_pushstring(S, "MI_FAILURE");
  lua_pushinteger(S, MI_FAILURE);
  lua_settable(S, -3);

  lua_pushstring(S, "SMFIS_ALL_OPTS");
  lua_pushinteger(S, SMFIS_ALL_OPTS);
  lua_settable(S, -3);
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

  lua_pushstring(S, "SMFIF_ADDHDRS");
  lua_pushinteger(S, SMFIF_ADDHDRS);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIF_CHGHDRS");
  lua_pushinteger(S, SMFIF_CHGHDRS);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIF_CHGBODY");
  lua_pushinteger(S, SMFIF_CHGBODY);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIF_ADDRCPT");
  lua_pushinteger(S, SMFIF_ADDRCPT);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIF_ADDRCPT_PAR");
  lua_pushinteger(S, SMFIF_ADDRCPT_PAR);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIF_DELRCPT");
  lua_pushinteger(S, SMFIF_DELRCPT);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIF_QUARANTINE");
  lua_pushinteger(S, SMFIF_QUARANTINE);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIF_CHGFROM");
  lua_pushinteger(S, SMFIF_CHGFROM);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIF_SETSYMLIST");
  lua_pushinteger(S, SMFIF_SETSYMLIST);
  lua_settable(S, -3);

  lua_pushstring(S, "SMFIP_RCPT_REJ");
  lua_pushinteger(S, SMFIP_RCPT_REJ);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_SKIP");
  lua_pushinteger(S, SMFIP_SKIP);
  lua_settable(S, -3);

  lua_pushstring(S, "SMFIP_NR_CONN");
  lua_pushinteger(S, SMFIP_NR_CONN);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NR_HELO");
  lua_pushinteger(S, SMFIP_NR_HELO);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NR_MAIL");
  lua_pushinteger(S, SMFIP_NR_MAIL);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NR_RCPT");
  lua_pushinteger(S, SMFIP_NR_RCPT);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NR_DATA");
  lua_pushinteger(S, SMFIP_NR_DATA);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NR_UNKN");
  lua_pushinteger(S, SMFIP_NR_UNKN);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NR_EOH");
  lua_pushinteger(S, SMFIP_NR_EOH);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NR_BODY");
  lua_pushinteger(S, SMFIP_NR_BODY);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NR_HDR");
  lua_pushinteger(S, SMFIP_NR_HDR);
  lua_settable(S, -3);

  lua_pushstring(S, "SMFIP_NOCONNECT");
  lua_pushinteger(S, SMFIP_NOCONNECT);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NOHELO");
  lua_pushinteger(S, SMFIP_NOHELO);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NOMAIL");
  lua_pushinteger(S, SMFIP_NOMAIL);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NORCPT");
  lua_pushinteger(S, SMFIP_NORCPT);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NOBODY");
  lua_pushinteger(S, SMFIP_NOBODY);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NOHDRS");
  lua_pushinteger(S, SMFIP_NOHDRS);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NOEOH");
  lua_pushinteger(S, SMFIP_NOEOH);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NOUNKNOWN");
  lua_pushinteger(S, SMFIP_NOUNKNOWN);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIP_NODATA");
  lua_pushinteger(S, SMFIP_NODATA);
  lua_settable(S, -3);

  lua_pushstring(S, "SMFIM_CONNECT");
  lua_pushinteger(S, SMFIM_CONNECT);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIM_HELO");
  lua_pushinteger(S, SMFIM_HELO);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIM_ENVFROM");
  lua_pushinteger(S, SMFIM_ENVFROM);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIM_ENVRCPT");
  lua_pushinteger(S, SMFIM_ENVRCPT);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIM_DATA");
  lua_pushinteger(S, SMFIM_DATA);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIM_EOM");
  lua_pushinteger(S, SMFIM_EOM);
  lua_settable(S, -3);
  lua_pushstring(S, "SMFIM_EOH");
  lua_pushinteger(S, SMFIM_EOH);
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
int setstack_negotiate (lua_State *T, va_list *ap)
{
  unsigned long f;
  f = va_arg(*ap, unsigned long);
  lua_pushinteger(T, f);
  f = va_arg(*ap, unsigned long);
  lua_pushinteger(T, f);
  f = va_arg(*ap, unsigned long);
  lua_pushinteger(T, f);
  f = va_arg(*ap, unsigned long);
  lua_pushinteger(T, f);
  return 4;
}


/**
 */
int fire (lua_State *T, const int ref_fcall, const int ref_env, ...)
{
  va_list ap;
  int ti, r, nargs = 1, retval = SMFIS_CONTINUE;
  lua_pushcfunction(T, trace);
  ti = lua_gettop(T);
  r = lua_rawgeti(T, LUA_REGISTRYINDEX, ref_fcall);
  if (LUA_TFUNCTION == r)
  {
    r = lua_rawgeti(T, LUA_REGISTRYINDEX, ref_env);
    if (LUA_TTABLE == r)
    {
      //
      va_start(ap, ref_env);
      //nargs += SS_funcs[EH_NEGOTIATE](T,ap);
      switch (ref_fcall)
      {
        case EH_NEGOTIATE:
          nargs += setstack_negotiate(T, &ap);
          break;
      }
      va_end(ap);
      r = lua_pcall(T, nargs, 1, ti);
      ti = lua_gettop(T);
      if (LUA_OK == r && lua_isinteger(T, ti))
        retval = lua_tointeger(T, ti);
    }
  }
  // remove the retval/error object pushed by pcall
  // remove the error handler at ti
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
  unsigned short sid = rand();
  char ssid[8] = {'\0'};
  int r = SMFIS_ALL_OPTS;
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
    //f = L_refs[EH_NEGOTIATE];
    pthread_mutex_unlock(&lock_L);

    // before firing the event, create the envelope for this session
    lua_newtable(envelope->T);
    lua_pushliteral(envelope->T, "smfictx");
    lua_pushlightuserdata(envelope->T, context);
    lua_rawset(envelope->T, -3);
    sprinth(ssid, 8, (char *)&sid, sizeof(sid));
    lua_pushliteral(envelope->T, "sid");
    lua_pushstring(envelope->T, ssid);
    lua_rawset(envelope->T, -3);
    envelope->Eref = luaL_ref(envelope->T, LUA_REGISTRYINDEX);

    r = fire(envelope->T, L_refs[EH_NEGOTIATE], envelope->Eref, f0, f1, f2, f3);
  }
  return r;
}


sfsistat fi_connect (SMFICTX *context, char *host, _SOCK_ADDR *sa)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  r = fire(envelope->T, L_refs[EH_CONNECT], envelope->Eref, host);
  return r;
}


sfsistat fi_unknown (SMFICTX *context, const char *command)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  r = fire(envelope->T, L_refs[EH_UNKNOWN], envelope->Eref, command);
  return r;
}


sfsistat fi_helo (SMFICTX *context, char *helo)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  r = fire(envelope->T, L_refs[EH_HELO], envelope->Eref, helo);
  return r;
}


sfsistat fi_envfrom (SMFICTX *context, char **argv)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  r = fire(envelope->T, L_refs[EH_ENVFROM], envelope->Eref);
  return r;
}


sfsistat fi_envrcpt (SMFICTX *context, char **argv)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  r = fire(envelope->T, L_refs[EH_ENVRCPT], envelope->Eref);
  return r;
}


sfsistat fi_data (SMFICTX *context)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  r = fire(envelope->T, L_refs[EH_DATA], envelope->Eref);
  return r;
}


sfsistat fi_header (SMFICTX *context, char *name, char *value)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  r = fire(envelope->T, L_refs[EH_HEADER], envelope->Eref, name, value);
  return r;
}


sfsistat fi_eoh (SMFICTX *context)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  r = fire(envelope->T, L_refs[EH_EOH], envelope->Eref);
  return r;
}


sfsistat fi_body (SMFICTX *context, unsigned char *segment, size_t len)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  r = fire(envelope->T, L_refs[EH_BODY], envelope->Eref, segment, len);
  return r;
}


sfsistat fi_eom (SMFICTX *context)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  r = fire(envelope->T, L_refs[EH_EOM], envelope->Eref);
  return r;
}


sfsistat fi_abort (SMFICTX *context)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
  r = fire(envelope->T, L_refs[EH_ABORT], envelope->Eref);
  return r;
}


sfsistat fi_close (SMFICTX *context)
{
  envelope_t *envelope = (envelope_t *)smfi_getpriv(context);
  int r = SMFIS_CONTINUE;
/*
  if (!pthread_mutex_lock(&lock_L))
  {
    f = L_refs[EH_CLOSE];
    pthread_mutex_unlock(&lock_L);
  }
*/
  r = fire(envelope->T, L_refs[EH_CLOSE], envelope->Eref);

  luaL_unref(envelope->T, LUA_REGISTRYINDEX, envelope->Eref);

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

  srand(time(NULL));
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
