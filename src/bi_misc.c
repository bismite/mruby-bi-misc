#include <stdlib.h>
#include <unistd.h>
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/value.h>
#include <mruby/string.h>
#include <mruby/presym.h>
#include <bi/bi_sdl.h>
#include <bi/bi_gl.h>

//
extern void mrb_mruby_bi_archive_gem_init(mrb_state* mrb);
extern void mrb_mruby_bi_geometry_gem_init(mrb_state* mrb);
extern void mrb_mruby_cellular_automaton_gem_init(mrb_state *mrb);
extern void mrb_mruby_bi_dlopen_gem_init(mrb_state* mrb);

//
// ---- stable sort ----
//

typedef struct {
  mrb_int z;
  mrb_int index;
  mrb_value v;
} _stable_sort_container_;

static int container_compare(const void *a, const void *b )
{
  _stable_sort_container_* x = (_stable_sort_container_*)a;
  _stable_sort_container_* y = (_stable_sort_container_*)b;
  return x->z != y->z ? x->z - y->z : x->index - y->index;
}

static mrb_value mrb_sort_ab_sort_ab(mrb_state *mrb, mrb_value ary)
{
  mrb_int i,len,z;
  mrb_value v, result, tmp;
  mrb_sym name;
  _stable_sort_container_ *que;

  mrb_get_args(mrb, "n", &name);
  len = RARRAY_LEN(ary);
  que = malloc(sizeof(_stable_sort_container_)*len);

  for (i = 0; i < len; i++) {
    v = RARRAY_PTR(ary)[i];

    tmp = mrb_funcall_argv(mrb,v,name,0,NULL);

    z = mrb_fixnum(tmp);
    que[i].z = z;
    que[i].index = i;
    que[i].v = v;
  }

  // sorting...
  qsort(que,len,sizeof(_stable_sort_container_),container_compare);

  result = mrb_ary_new_capa(mrb, len);
  for( int i=0; i<len; i++) {
    mrb_ary_push(mrb,result,que[i].v);
  }
  free(que);

  return result;
}

static mrb_value mrb_bi_get_platform(mrb_state* mrb, mrb_value self)
{
  return mrb_str_new_cstr(mrb, SDL_GetPlatform());
}

static mrb_value mrb_bi_get_pointer_size(mrb_state* mrb, mrb_value self)
{
  return mrb_fixnum_value( sizeof(void*)*8 );
}

static mrb_value mrb_bi_is_little_endian(mrb_state* mrb, mrb_value self)
{
  return mrb_bool_value( SDL_BYTEORDER == SDL_LIL_ENDIAN );
}


// Screenshot
static mrb_value save_screenshot(mrb_state* mrb, mrb_value self)
{
  const char* filename;
  mrb_int w,h;
  mrb_get_args(mrb, "zii", &filename, &w, &h );
  uint8_t* tmp = malloc(4*w*h);
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,tmp);
  uint8_t* pixels = malloc(4*w*h);
  const int pitch = 4*w;
  for(int i=1;i<=h;i++) {
    memcpy( &(pixels[(h-i)*pitch]), &(tmp[(i-1)*pitch]), pitch );
  }
  free(tmp);
  SDL_Surface* s = SDL_CreateRGBSurfaceWithFormatFrom(pixels,w,h,32,pitch,SDL_PIXELFORMAT_RGBA32);
  IMG_SavePNG(s,filename);
  SDL_FreeSurface(s);
  free(pixels);
  return self;
}

// Kernel
static mrb_value mrb_execvp(mrb_state* mrb, mrb_value self)
{
  const char* cmd;
  mrb_int cmd_len;
  mrb_value *mrb_argv;
  mrb_int argc;
  char** argv;
  mrb_get_args(mrb, "sa", &cmd,&cmd_len, &mrb_argv,&argc );
  argv = mrb_malloc(mrb, sizeof(char *) * (argc + 1));
  for(int i=0;i<argc;i++){
    argv[i] = RSTRING_CSTR(mrb,mrb_argv[i]);
  }
  argv[argc] = NULL;
  execvp(cmd,argv);
}

//
// ---- mruby gem ----
//

void mrb_mruby_bi_misc_gem_init(mrb_state *mrb)
{
  struct RClass * a = mrb->array_class;
  mrb_define_method(mrb, a, "stable_sort", mrb_sort_ab_sort_ab, MRB_ARGS_REQ(1));

  struct RClass *bi = mrb_class_get(mrb, "Bi");
  mrb_define_class_method(mrb, bi, "get_platform", mrb_bi_get_platform, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, bi, "get_pointer_size", mrb_bi_get_pointer_size, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, bi, "little_endian?", mrb_bi_is_little_endian, MRB_ARGS_NONE());

  // Kernel
  mrb_define_method_id(mrb, mrb->kernel_module, MRB_SYM(execvp), mrb_execvp, MRB_ARGS_REQ(2)); // cmd,args
  mrb_define_const(mrb, mrb->kernel_module, "MRB_FIXNUM_MIN", mrb_fixnum_value(MRB_FIXNUM_MIN));
  mrb_define_const(mrb, mrb->kernel_module, "MRB_FIXNUM_MAX", mrb_fixnum_value(MRB_FIXNUM_MAX));

  mrb_define_class_method(mrb, bi, "save_screenshot", save_screenshot, MRB_ARGS_REQ(3)); // filename,w,h

  //
  mrb_mruby_bi_archive_gem_init(mrb);
  mrb_mruby_bi_geometry_gem_init(mrb);
  mrb_mruby_cellular_automaton_gem_init(mrb);
  mrb_mruby_bi_dlopen_gem_init(mrb);
}

void mrb_mruby_bi_misc_gem_final(mrb_state *mrb)
{
}
