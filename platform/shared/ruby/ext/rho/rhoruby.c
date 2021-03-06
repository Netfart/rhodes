/*
 *  rhoruby.c
 *  rhorubylib
 *
 *  Created by evgeny vovchenko on 10/1/08.
 *  Copyright 2008 RhoMobile. All rights reserved.
 *
 */
#undef RUBY_EXPORT
#include "ruby.h"
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include "rhoruby.h"
#include "vm_core.h"

#ifdef ENABLE_RUBY_VM_STAT
#include "../stat/stat.h"
#endif

#include "logging/RhoLog.h"
#undef DEFAULT_LOGCATEGORY
#define DEFAULT_LOGCATEGORY "RhoRuby"
extern void Init_strscan();
extern void Init_sqlite3_api();
extern void Init_GeoLocation(void);
extern void Init_SyncEngine(void);
extern void print_profile_report();
extern void enable_gc_profile(void);
extern void Init_System(void);
extern void Init_Phonebook(void);
extern void Init_WebView(void);
extern void Init_RhoConf(void);
extern void Init_Alert(void);
extern void Init_Camera(void);
extern void Init_stringio(void);
extern void Init_DateTimePicker(void);
extern void Init_NativeBar(void);
extern void Init_MapView(void); 

//RhoSupport extension
extern void Init_RhoSupport(void);
extern VALUE require_compiled(VALUE fname, VALUE* result);
extern VALUE RhoPreparePath(VALUE path);
//extern void RhoSetCurAppPath(char* path);

static VALUE  framework;
static ID framework_mid;
static ID framework_mid2;
static ID initApp_mid;

static char* rb_type_to_s(VALUE obj);
//extern int ruby_thread_set_native(rb_thread_t *th);
//extern int native_mutex_lock(rb_thread_lock_t *);
//extern void Init_native_thread2(rb_thread_t *th);

/*rb_thread_t * __getCurrentThread()
{
    rb_thread_t * res = ruby_thread_from_native();
    if ( res )
        return res;
	
    return ruby_current_thread;
}*/
/*
void RhoRubyThreadStart()
{
    rb_thread_t *th;

    VALUE self = rb_thread_alloc(rb_cThread);
    GetThreadPtr(self, th);

#if defined( OS_WINDOWS ) || defined( OS_WINCE )	
    ruby_thread_init_stack(th);
#endif	
    ruby_thread_set_native(th);

    rb_gc_register_mark_object(self);

    Init_native_thread2(th);

    //st_insert(th->vm->living_threads, 0, (st_data_t) th->thread_id);
//	RhoRuby_RhomAttribManager_add_attrib(0, "test");
    //native_mutex_lock(&th->vm->global_vm_lock);
} */
/*
void RhoRubyThreadStop()
{
    //rb_thread_t *th = GET_THREAD();
    //st_delete(th->vm->living_threads, &(th->self), 0);
    //native_mutex_unlock(&th->vm->global_vm_lock);

    //native_mutex_destroy(&th->interrupt_lock);
} */

void RhoRubyStart()
{
    //VALUE moduleRhom;
#ifdef HAVE_LOCALE_H
    setlocale(LC_CTYPE, "");
#endif
    {
#ifdef ENABLE_RUBY_VM_STAT
    g_collect_stat = 1; 
#endif    
    
	RUBY_INIT_STACK;
	ruby_init();
#if defined(WIN32)
	rb_w32_sysinit(NULL,NULL);
#endif

#if defined(DEBUG)
	//enable_gc_profile();
#endif
	//rb_funcall(rb_mGC, rb_intern("stress="), 1, Qtrue);

	ruby_init_loadpath(RhoGetRootPath());
	Init_strscan();
	Init_sqlite3_api();
	Init_GeoLocation();
	Init_SyncEngine();
	Init_System();
	Init_Phonebook();
	Init_WebView();
	Init_RhoConf();
	Init_Alert();
	Init_Camera();
	Init_stringio();
	Init_DateTimePicker();
	Init_NativeBar();
	Init_RhoSupport();
	Init_MapView();
		
#ifdef ENABLE_RUBY_VM_STAT
    struct timeval  start;
    struct timeval  end;
    
    gettimeofday (&start, NULL);
#endif    

    require_compiled(rb_str_new2("rhoframework"), &framework );

#ifdef ENABLE_RUBY_VM_STAT
    gettimeofday (&end, NULL);
    
    if ( end.tv_sec > 0 )
    	g_require_compiled_msec += (end.tv_sec  - start.tv_sec) * 1000;
    else
    	g_require_compiled_msec += (end.tv_usec - start.tv_usec)/1000; 
    
#endif    

    rb_gc_register_mark_object(framework);

	CONST_ID(framework_mid, "serve");
	CONST_ID(framework_mid2, "serve_index");
	CONST_ID(initApp_mid, "init_app");

    //moduleRhom = rb_const_get(rb_cObject, rb_intern("Rhom"));

#ifdef ENABLE_RUBY_VM_STAT
	g_collect_stat = 0; 
#endif    
		
	}	
}
void RhoRubyInitApp()
{
    rb_funcall(framework, initApp_mid, 0);
}

char* RhoRuby_getRhoDBVersion()
{
    VALUE moduleRhodes = rb_const_get(rb_cObject, rb_intern("Rhodes"));
    VALUE valVer = rb_const_get(moduleRhodes, rb_intern("DBVERSION"));
    return RSTRING_PTR(valVer);
}

#if defined(WIN32)
#if defined(_WIN32_WCE)
extern DWORD GetModuleFileNameA(HMODULE hModule,LPSTR lpFileName,DWORD size);
#endif
static int _root_loaded = 0;
static char _rootpath[MAX_PATH];
#if !defined(_WIN32_WCE)
void __setRootPath(const char* path) {
	strcpy(_rootpath,path);
	_root_loaded = 1;
}
#endif
const char* RhoGetRootPath() {
  int len;
  if (_root_loaded) {
    return _rootpath;
  }
  if( (len = GetModuleFileNameA(NULL,_rootpath,MAX_PATH)) == 0 )
  {
    strcpy(_rootpath,".");
  }
  else
  {
    while( !(_rootpath[len] == '\\'  || _rootpath[len] == '/') )
      len--;
    _rootpath[len]=0;
    sprintf(_rootpath,"%s\\rho\\",_rootpath);
  }
  _root_loaded = 1;
  return _rootpath;
}

#endif// WIN32

const char* RhoGetRelativeBlobsPath() 
{
    return "apps/public/db-files";
}

const wchar_t* RhoGetRelativeBlobsPathW() 
{
    return L"apps/public/db-files";
}

void RhoRubyStop()
{
	//TBD: clenup framework, etc.
	ruby_cleanup(0);
}

char* makeControllerCall(char* classname, char* methodname);

VALUE getnil() {
	return Qnil;
}

VALUE rho_ruby_create_array()
{
    return rb_ary_new();
}

VALUE rho_ruby_create_string(const char* szVal)
{
    return rb_str_new2(szVal);
}

void rho_ruby_add_to_array(VALUE ar, VALUE val)
{
    rb_ary_push(ar,val);
}

VALUE createHash() {
	return rb_hash_new();
}

VALUE addTimeToHash(VALUE hash, const char* key, time_t val) {
	return rb_hash_aset(hash, rb_str_new2(key), rb_time_new(val,0));
}

VALUE addIntToHash(VALUE hash, const char* key, int val) {
	return rb_hash_aset(hash, rb_str_new2(key), INT2FIX(val));
}

VALUE addStrToHash(VALUE hash, const char* key, const char* val, int len) {
	return rb_hash_aset(hash, rb_str_new2(key), rb_str_new(val, len));
}

VALUE addHashToHash(VALUE hash, const char* key, VALUE val) {
	return rb_hash_aset(hash, rb_str_new2(key), val);	
}

char* getStringFromValue(VALUE val){
    return RSTRING_PTR(val);
}

int getStringLenFromValue(VALUE val){
    return RSTRING_LEN(val);
}

void  releaseValue(VALUE val){
    VALUE ary = GET_THREAD()->vm->mark_object_ary;
    int i = RARRAY_LEN(ary)-1;
    for ( ; i >= 0; i-- ) {
        if ( RARRAY_PTR(ary)[i]== val )
            break;
    }
    if ( i >= 0 )
        rb_ary_delete_at(ary,i);
}

VALUE callFramework(VALUE hashReq) {
    VALUE callres = rb_funcall(framework, framework_mid, 1, hashReq);
	
	if (TYPE(callres)!=T_STRING) {
		RAWLOG_INFO1("Method call result type = %s", rb_type_to_s(callres));
		return rb_str_new2("Error");//TBD: Supply html description of the error
	}

    rb_gc_register_mark_object(callres);
	//TBD: need to cleanup memory
	rb_gc();

	return callres;
}

VALUE callServeIndex(char* index_name) {
    VALUE callres;
    //RhoSetCurAppPath(index_name);
	callres = rb_funcall(framework, framework_mid2, 1, RhoPreparePath(rb_str_new2(index_name)));
	
	if (TYPE(callres)!=T_STRING) {
		RAWLOG_INFO1("Method call result type = %s", rb_type_to_s(callres));
		return rb_str_new2("Error");//TBD: Supply html description of the error
	}
    rb_gc_register_mark_object(callres);

	//TBD: need to cleanup memory
	rb_gc();
	
	return callres;
}

/*static int s_gcWasDisabled;
//extern void native_sleep(rb_thread_t *th, struct timeval *tv);
extern void sleep_for_polling(rb_thread_t *th);
static void start_ruby_call()
{
    rb_thread_t *th = GET_THREAD();

    while( rb_during_gc() )
    {
        sleep_for_polling(th);
    }

    s_gcWasDisabled = rb_gc_disable();
}
static void end_ruby_call()
{
    if ( !s_gcWasDisabled )
        rb_gc_enable();
}*/

static char*
rb_type_to_s(VALUE obj)
{
  VALUE type;

  if (IMMEDIATE_P(obj)) {
		if (FIXNUM_P(obj)) return "T_FIXNUM";
		if (obj == Qtrue) return "T_TRUE";
		if (SYMBOL_P(obj)) return "T_SYMBOL";
		if (obj == Qundef) return "T_UNDEF";
  } else if (!RTEST(obj)) {
		if (obj == Qnil) return "T_NIL";
		if (obj == Qfalse) return "T_FALSE";
  }

  type = BUILTIN_TYPE(obj);
	switch(type) {
		case T_NONE:
			return "RUBY_T_NONE";
		case T_NIL:
			return "RUBY_T_NIL";
		case T_OBJECT:
			return "RUBY_T_OBJECT";
		case T_CLASS:
			return "RUBY_T_CLASS";
		case T_ICLASS: 
			return "RUBY_T_ICLASS";
		case T_MODULE:
			return "RUBY_T_MODULE";
		case T_FLOAT:  
			return "RUBY_T_FLOAT";
		case T_STRING:
			return "RUBY_T_STRING";
		case T_REGEXP:
			return "RUBY_T_REGEXP";
		case T_ARRAY:
			return "RUBY_T_ARRAY";
		case T_HASH:
			return "RUBY_T_HASH";
		case T_STRUCT:
			return "RUBY_T_STRUCT";
		case T_BIGNUM:
			return "RUBY_T_BIGNUM";
		case T_FILE:
			return "RUBY_T_FILE";
		case T_FIXNUM:
			return "RUBY_T_FIXNUM";
		case T_TRUE:
			return "RUBY_T_TRUE";
		case T_FALSE:
			return "RUBY_T_FALSE";
		case T_DATA:
			return "RUBY_T_DATA";
		case T_MATCH:
			return "RUBY_T_MATCH";
		case T_SYMBOL:
			return "RUBY_T_SYMBOL";
		case T_RATIONAL:
			return "RUBY_T_RATIONAL";
		case T_COMPLEX:
			return "RUBY_T_COMPLEX";
		case T_UNDEF:
			return "RUBY_T_UNDEF";
		case T_NODE:
			return "RUBY_T_NODE";
		case T_ZOMBIE: 
			return "RUBY_T_ZOMBIE";
		default:
			return "Unknown";
	}
}

