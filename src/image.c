#include <mruby.h>
#include <mruby/data.h>
#include <mruby/class.h>
#include <mruby/string.h>
#include <bi/bi_sdl.h>
#include <bi_core.h>

void mrb_image_free(mrb_state *mrb,void* p){ SDL_FreeSurface(p); }
static struct mrb_data_type const mrb_image_data_type = { "Image", mrb_image_free };

static mrb_value mrb_bi_image_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_int w,h;
  mrb_get_args(mrb, "ii", &w,&h);
  SDL_Surface *img = SDL_CreateRGBSurfaceWithFormat( 0,w,h,32,SDL_PIXELFORMAT_RGBA32);
  DATA_PTR(self) = img;
  DATA_TYPE(self) = &mrb_image_data_type;
  return self;
}

static mrb_value mrb_bi_image_read(mrb_state *mrb, mrb_value self)
{
  mrb_value path;
  mrb_get_args(mrb, "S", &path);
  SDL_Surface *img = IMG_Load( mrb_string_value_cstr(mrb,&path) );

  struct RClass *bi = mrb_class_get(mrb, "Bi");
  struct RClass *klass = mrb_class_get_under(mrb,bi,"Image");
  struct RData *data = mrb_data_object_alloc(mrb,klass,img,&mrb_image_data_type);
  mrb_value val = mrb_obj_value(data);
  return val;
}


static mrb_value mrb_bi_image_w(mrb_state *mrb, mrb_value self)
{
  SDL_Surface *img = DATA_PTR(self);
  return mrb_fixnum_value(img->w);
}

static mrb_value mrb_bi_image_h(mrb_state *mrb, mrb_value self)
{
  SDL_Surface *img = DATA_PTR(self);
  return mrb_fixnum_value(img->h);
}

static mrb_value mrb_bi_image_to_texture(mrb_state *mrb, mrb_value self)
{
  mrb_bool antialias;
  mrb_get_args(mrb, "b", &antialias);
  SDL_Surface *img = DATA_PTR(self);
  return create_bi_texture_from_pixels(mrb, img->w, img->h, img->pixels, antialias);
}

static mrb_value mrb_bi_image_blit(mrb_state *mrb, mrb_value self)
{
  mrb_value src_obj;
  mrb_int x,y;
  mrb_get_args(mrb, "oii", &src_obj,&x,&y);

  SDL_Surface *dst = DATA_PTR(self);
  SDL_Surface *src = DATA_PTR(src_obj);
  SDL_Rect dst_rect = {x,y,0,0};

  SDL_BlitSurface(src,NULL,dst,&dst_rect);

  return self;
}

static mrb_value mrb_bi_image_save(mrb_state *mrb, mrb_value self)
{
  mrb_value path;
  mrb_get_args(mrb, "S", &path);
  SDL_Surface *img = DATA_PTR(self);
  IMG_SavePNG(img,mrb_string_value_cstr(mrb,&path));
  return self;
}

void mrb_mruby_bi_image_gem_init(mrb_state* mrb)
{
  struct RClass *bi = mrb_class_get(mrb, "Bi");
  struct RClass *image = mrb_define_class_under(mrb, bi, "Image", mrb->object_class);
  MRB_SET_INSTANCE_TT(image, MRB_TT_DATA);

  mrb_define_class_method(mrb, image, "read", mrb_bi_image_read, MRB_ARGS_REQ(1) ); // path
  mrb_define_method(mrb, image, "initialize", mrb_bi_image_initialize, MRB_ARGS_REQ(2) ); // w,h

  mrb_define_method(mrb, image, "w", mrb_bi_image_w, MRB_ARGS_NONE() );
  mrb_define_method(mrb, image, "h", mrb_bi_image_h, MRB_ARGS_NONE() );

  mrb_define_method(mrb, image, "to_texture", mrb_bi_image_to_texture, MRB_ARGS_REQ(1) ); // antialiase
  mrb_define_method(mrb, image, "blit!", mrb_bi_image_blit, MRB_ARGS_REQ(3) ); // Image,x,y
  mrb_define_method(mrb, image, "save", mrb_bi_image_save, MRB_ARGS_REQ(1) ); // path
}
