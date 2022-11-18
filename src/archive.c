#include <mruby.h>
#include <mruby/data.h>
#include <mruby/class.h>
#include <mruby/variable.h>
#include <mruby/string.h>
#include <mruby/hash.h>
#include <mruby/presym.h>

#include <bi/texture.h>
#include <bi/bi_sdl.h>
#include <bi_core.h>
#include <bi_misc.h>

#include <string.h>
#include <stdlib.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

//
static void decrypt64(void* buf,uint32_t size,uint64_t secret)
{
  uint64_t* b64 = buf;
  for(int i=0;i<size/8;i++){ b64[i] ^= secret; }
  if(size%8>0){
    uint8_t *b = buf;
    uint64_t tmp = 0;
    memcpy(&tmp, &b[size-size%8], size%8);
    tmp ^= secret;
    memcpy(&b[size-size%8], &tmp, size%8);
  }
}

//
// Bi::Archive
//
typedef struct {
  uint32_t file_size;
  uint32_t index_size;
  uint64_t secret;
  void* data; // for emscripten
} mrb_bi_archive_struct;

static void bi_archive_free(mrb_state *mrb, void *p)
{
  free(((mrb_bi_archive_struct*)p)->data);
  mrb_free(mrb,p);
}

static struct mrb_data_type const mrb_archive_data_type = { "Archive", bi_archive_free };

static mrb_value mrb_archive_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value path,secret;
  mrb_get_args(mrb, "SS", &path, &secret );
  mrb_iv_set(mrb, self, mrb_intern_cstr(mrb,"@path"), path );
  mrb_iv_set(mrb, self, mrb_intern_cstr(mrb,"@available"), mrb_false_value() );
  mrb_iv_set(mrb, self, mrb_intern_cstr(mrb,"@index"), mrb_hash_new(mrb) );

  mrb_bi_archive_struct *a = mrb_malloc(mrb,sizeof(mrb_bi_archive_struct));
  a->file_size = 0;
  a->index_size = 0;
  a->data = NULL;
  a->secret = bi_crc64(0,(uint8_t*)RSTRING_PTR(secret),RSTRING_LEN(secret));
  mrb_data_init(self, a, &mrb_archive_data_type);

  return self;
}

static void read_from_rwops(mrb_state *mrb, mrb_value self, SDL_RWops *io)
{
  uint32_t header,index_size;
  mrb_bi_archive_struct *a = DATA_PTR(self);
  a->file_size = SDL_RWsize(io);
  // header
  SDL_RWread(io,&header,4,1);
  if(header!=2) {
    mrb_iv_set(mrb, self, MRB_SYM(error), mrb_str_new_lit(mrb, "archive load failed : header invalid.") );
    return;
  }
  // index length
  SDL_RWread(io,&index_size,4,1);
  a->index_size = index_size;
  // index
  char* buf = malloc(index_size);
  if(SDL_RWread(io,buf,index_size,1)!=1) {
    mrb_iv_set(mrb, self, MRB_SYM(error), mrb_str_new_lit(mrb, "archive load failed: index can not read.") );
    free(buf);
    return;
  }
  decrypt64(buf,index_size,a->secret);
  mrb_value index_str = mrb_str_new_static(mrb,buf,index_size);
  mrb_funcall(mrb,self,"_set_raw_index",1,index_str);
  free(buf);
}

static mrb_value mrb_archive_open(mrb_state *mrb, mrb_value self)
{
  mrb_value path = mrb_iv_get(mrb, self, mrb_intern_cstr(mrb,"@path") );
  SDL_RWops *io = SDL_RWFromFile(mrb_string_value_cstr(mrb,&path),"rb");
  read_from_rwops(mrb,self,io);
  SDL_RWclose(io);
  return self;
}

#ifdef EMSCRIPTEN

typedef struct {
  mrb_state *mrb;
  mrb_value archive;
} fetch_context;

static void onload(unsigned int handle, void* _context, void* data, unsigned int size)
{
  fetch_context* context = (fetch_context*)_context;
  mrb_state *mrb = context->mrb;
  mrb_value self = context->archive;
  mrb_bi_archive_struct *a = DATA_PTR(self);
  a->data = data;

  SDL_RWops* io = SDL_RWFromConstMem(data,size);
  read_from_rwops(mrb,self,io);
  SDL_RWclose(io);

  mrb_iv_set(mrb, self, mrb_intern_cstr(mrb,"@available"), mrb_true_value() );
  mrb_value callback = mrb_iv_get(mrb,self, mrb_intern_cstr(mrb,"@callback"));
  mrb_value argv[1] = { self };
  if( mrb_symbol_p(callback) ){
    mrb_funcall_argv(mrb,self,mrb_symbol(callback),1,argv);
  }else if( mrb_proc_p(callback) ) {
    mrb_yield_argv(mrb,callback,1,argv);
  }

  free(context);
}

static void onprogress(unsigned int handle, void *_context, int loaded, int total)
{
  fetch_context* context = (fetch_context*)_context;
  mrb_state *mrb = context->mrb;
  mrb_value self = context->archive;

  mrb_value callback = mrb_iv_get(mrb,self,mrb_intern_cstr(mrb,"@on_progress"));
  mrb_value argv[3] = {
    self,
    mrb_fixnum_value(loaded),
    mrb_fixnum_value(total)
  };
  if( mrb_symbol_p(callback) ){
    mrb_funcall_argv(mrb,self,mrb_symbol(callback),3,argv);
  }else if( mrb_type(callback) == MRB_TT_PROC ) {
    mrb_yield_argv(mrb,callback,3,argv);
  }
}

static void onerror(unsigned int handle, void *_context, int http_status_code, const char* desc)
{
  fetch_context* context = (fetch_context*)_context;
  mrb_state *mrb = context->mrb;
  mrb_value self = context->archive;

  mrb_value callback = mrb_iv_get(mrb,self,mrb_intern_cstr(mrb,"@callback"));
  mrb_value argv[2] = {
    self,
    mrb_fixnum_value(http_status_code),
  };
  if( mrb_symbol_p(callback) ){
    mrb_funcall_argv(mrb,self,mrb_symbol(callback),2,argv);
  }else if( mrb_type(callback) == MRB_TT_PROC ) {
    mrb_yield_argv(mrb,callback,2,argv);
  }

  free(context);
}

static mrb_value mrb_archive_download(mrb_state *mrb, mrb_value self)
{
  mrb_value callback;
  mrb_value path;
  fetch_context *context;
  mrb_get_args(mrb,"o",&callback);

  mrb_iv_set(mrb, self, mrb_intern_cstr(mrb,"@callback"), callback );
  context = malloc(sizeof(fetch_context));
  context->mrb = mrb;
  context->archive = self;

  path = mrb_iv_get(mrb, self, mrb_intern_cstr(mrb,"@path") );

  emscripten_async_wget2_data(mrb_string_value_cstr(mrb,&path),"GET","",context,FALSE,onload,onerror,onprogress);

  return self;
}

#endif // ifdef EMSCRIPTEN

static mrb_value mrb_archive_texture(mrb_state *mrb, mrb_value self)
{
  mrb_int start,length;
  mrb_bool encrypted,antialias;
  mrb_get_args(mrb, "iibb", &start,&length,&encrypted,&antialias);
  mrb_bi_archive_struct* a = DATA_PTR(self);
  start += 4 + 4 + a->index_size;

  char* buf = NULL;

#ifdef EMSCRIPTEN
  buf = (char*)a->data + start;
  if(encrypted) decrypt64(buf,length,a->secret);
#else
  buf = malloc(length);
  mrb_value path = mrb_iv_get(mrb, self, mrb_intern_cstr(mrb,"@path") );
  FILE *f = fopen(mrb_string_value_cstr(mrb,&path),"rb");
  fseek(f,start,SEEK_SET);
  fread(buf,sizeof(char),length,f);
  fclose(f);
  decrypt64(buf,length,a->secret);
#endif

  mrb_value result = create_bi_texture_from_memory( mrb, buf, length, antialias );

#ifndef EMSCRIPTEN
  free(buf);
#endif

  return result;
}

static mrb_value mrb_archive_read(mrb_state *mrb, mrb_value self)
{
  mrb_int start,length;
  mrb_get_args(mrb, "ii", &start,&length);
  mrb_bi_archive_struct *a = DATA_PTR(self);
  start += 4 + 4 + a->index_size;

  mrb_value buf = mrb_str_new_capa(mrb,length+1);
  char* b = RSTRING_PTR(buf);
#ifdef EMSCRIPTEN
  memcpy(b,(char*)a->data+start,length);
#else
  mrb_value path = mrb_iv_get(mrb, self, mrb_intern_cstr(mrb,"@path") );
  FILE *f = fopen(mrb_string_value_cstr(mrb,&path),"rb");
  fseek(f,start,SEEK_SET);
  fread(b,sizeof(char),length,f);
  fclose(f);
#endif

  decrypt64(b,length,a->secret);
  RSTR_SET_LEN(RSTRING(buf),length);
  b[length] = 0; // sentinel

  return buf;
}

//
// gem init and final
//

void mrb_mruby_bi_archive_gem_init(mrb_state* mrb)
{
  struct RClass *bi = mrb_class_get(mrb,"Bi");
  struct RClass *archive = mrb_define_class_under(mrb, bi, "Archive", mrb->object_class);
  MRB_SET_INSTANCE_TT(archive, MRB_TT_DATA);

  mrb_define_method(mrb, archive, "initialize", mrb_archive_initialize, MRB_ARGS_REQ(2) ); // path,secret

  mrb_define_method(mrb, archive, "_open", mrb_archive_open, MRB_ARGS_NONE());

#ifdef EMSCRIPTEN
  mrb_define_method(mrb, archive, "_download", mrb_archive_download, MRB_ARGS_REQ(1)); // callback
#endif

  mrb_define_method(mrb, archive, "_texture", mrb_archive_texture, MRB_ARGS_REQ(4)); // start, size, encrypted, antialias
  mrb_define_method(mrb, archive, "_read", mrb_archive_read, MRB_ARGS_REQ(2)); // start, size
}
