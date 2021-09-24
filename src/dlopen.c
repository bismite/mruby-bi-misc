#include <mruby.h>
#include <mruby/data.h>
#include <mruby/class.h>
#include <mruby/string.h>
#include <mruby/variable.h>

#include <bi/bi_sdl.h>

typedef void (*mrb_dl_func)(mrb_state*);

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

typedef struct {
  mrb_state *mrb;
  mrb_value callback;
  char *filename;
  char *symbol;
} dl_wget_context;

static void onload(unsigned int req_handle, void* _context,const char* file)
{
  dl_wget_context* context = (dl_wget_context*)_context;
  void* handle = dlopen( context->filename, RTLD_NOW);
  if(handle){
    mrb_dl_func func = (mrb_dl_func)dlsym(handle, context->symbol);
    if(func){
      func(context->mrb);
      mrb_yield(context->mrb,context->callback,mrb_true_value());

    }else{
      mrb_yield(context->mrb,context->callback,mrb_false_value());
    }
  }else{
    mrb_yield(context->mrb,context->callback,mrb_false_value());
  }
}

static void onerror(unsigned int req_handle, void *_context, int http_status_code)
{
  dl_wget_context *context = (dl_wget_context*)_context;
  mrb_yield(context->mrb,context->callback,mrb_false_value());
}

static mrb_value mrb_dlopen(mrb_state *mrb, mrb_value self)
{
    mrb_value filename,symbol,callback;
    mrb_get_args(mrb, "SS&", &filename, &symbol, &callback );

    dl_wget_context* context = malloc(sizeof(dl_wget_context));
    context->mrb = mrb;
    context->callback = callback;
    context->filename = malloc( strlen(RSTRING_CSTR(mrb,filename)) +1 );
    strcpy(context->filename, RSTRING_CSTR(mrb,filename) );
    context->symbol = malloc( strlen(RSTRING_CSTR(mrb,filename)) +1 );
    strcpy(context->symbol, RSTRING_CSTR(mrb,symbol) );

    emscripten_async_wget2( context->filename, context->filename, "GET", "", context, onload, onerror, NULL);

    return self;
}

#else

static mrb_value mrb_dlopen(mrb_state *mrb, mrb_value self)
{
    mrb_value filename,symbol,callback;
    mrb_get_args(mrb, "SS&", &filename, &symbol, &callback );

    void *handle = SDL_LoadObject(RSTRING_PTR(filename));
    if(handle){
      mrb_dl_func func = (mrb_dl_func)SDL_LoadFunction(handle, RSTRING_PTR(symbol) );
      if(func){
        func(mrb);
        mrb_yield(mrb,callback,mrb_true_value());
      }else{
        mrb_yield(mrb,callback,mrb_false_value());
      }
    }else{
      mrb_yield(mrb,callback,mrb_false_value());
    }

    return self;
}

#endif

void mrb_mruby_bi_dlopen_gem_init(mrb_state* mrb)
{
  struct RClass *kernel;
  kernel = mrb->kernel_module;

  mrb_define_method(mrb, kernel, "dlopen", mrb_dlopen, MRB_ARGS_REQ(3)); // filename, symbol, callback
}

void mrb_mruby_bi_dlopen_gem_final(mrb_state* mrb)
{
}
