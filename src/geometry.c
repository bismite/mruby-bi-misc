#include <mruby.h>
#include <mruby/class.h>
#include <mruby/array.h>
#include <mruby/numeric.h>
#include <stdbool.h>

#define f(a,x) mrb_to_flo(mrb,RARRAY_PTR(a)[x])

//
// inner methods
//

static bool _include_point(mrb_float rx, mrb_float ry, mrb_float rw, mrb_float rh, mrb_float px, mrb_float py)
{
  if( rw < 0 ) {
    rw = - rw;
    rx = rx - rw;
  }
  if( rh < 0 ) {
    rh = - rh;
    ry = ry - rh;
  }
  return( rx <= px && px <= rx+rw && ry <= py && py <= ry+rh );
}

static mrb_float _cross_product(mrb_float x1, mrb_float y1, mrb_float x2, mrb_float y2,
                                mrb_float x3, mrb_float y3, mrb_float x4, mrb_float y4)
{
  return( (x2-x1) * (y4-y3) - (y2-y1) * (x4-x3) );
}

static bool _intersection(mrb_float x1, mrb_float y1, mrb_float x2, mrb_float y2,
                          mrb_float x3, mrb_float y3, mrb_float x4, mrb_float y4, mrb_float *ix, mrb_float *iy)
{
  double dx1 = x2 - x1;
  double dy1 = y2 - y1;
  double dx2 = x4 - x3;
  double dy2 = y4 - y3;

  double tc = dx1 * (y1-y3) - dy1 * (x1-x3);
  double td = dx1 * (y1-y4) - dy1 * (x1-x4);
  double ta = dx2 * (y3-y1) - dy2 * (x3-x1);
  double tb = dx2 * (y3-y2) - dy2 * (x3-x2);

  if( tc * td < 0 && ta * tb < 0 ) {
    double t = -ta / (dx1*dy2 - dy1*dx2);
    *ix = x1 + (t * dx1);
    *iy = y1 + (t * dy1);
    return true;
  }
  return false;
}

static bool _on(mrb_float x1, mrb_float y1, mrb_float x2, mrb_float y2, mrb_float px, mrb_float py)
{
  return( _include_point(x1,y1,x2-x1,y2-y1,px,py) && _cross_product(x1,y1,x2,y2,x1,y1,px,py) == 0 );
}

static int _compare_length(mrb_float x1, mrb_float y1, mrb_float x2, mrb_float y2,
                           mrb_float x3, mrb_float y3, mrb_float x4, mrb_float y4)
{
    mrb_float a = ( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
    mrb_float b = ( (x4-x3)*(x4-x3) + (y4-y3)*(y4-y3) );
    if( a < b ) return -1;
    if( a > b ) return 1;
    return 0;
}

//
// Bi::Line class methods
//

static mrb_value mrb_bi_line_cross_product(mrb_state *mrb, mrb_value self)
{
  mrb_float x1,y1, x2,y2, x3,y3, x4,y4;
  mrb_get_args(mrb, "ffffffff", &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4 );
  return mrb_float_value(mrb, _cross_product(x1,y1, x2,y2, x3,y3, x4,y4) );
}

static mrb_value mrb_bi_line_intersection(mrb_state *mrb, mrb_value self)
{
  mrb_float x1,y1, x2,y2, x3,y3, x4,y4;
  mrb_float ix,iy;
  mrb_get_args(mrb, "ffffffff", &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4 );

  if( _intersection(x1,y1, x2,y2, x3,y3, x4,y4, &ix, &iy) ){
    mrb_value v[2];
    v[0] = mrb_float_value(mrb, ix );
    v[1] = mrb_float_value(mrb, iy );
    return mrb_ary_new_from_values(mrb,2,v);
  }

  return mrb_nil_value();
}

static mrb_value mrb_bi_line_on(mrb_state *mrb, mrb_value self)
{
  mrb_float x1,y1, x2,y2, px,py;
  mrb_get_args(mrb, "ffffff", &x1, &y1, &x2, &y2, &px, &py );
  return mrb_bool_value( _on(x1,y1, x2,y2, px,py) );
}

static mrb_value mrb_bi_line_compare_length(mrb_state *mrb, mrb_value self)
{
  mrb_float x1,y1, x2,y2, x3,y3, x4,y4;
  mrb_get_args(mrb, "ffffffff", &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4 );
  return mrb_fixnum_value( _compare_length(x1,y1, x2,y2, x3,y3, x4,y4) );
}

static mrb_value mrb_bi_line_nearest_intersection(mrb_state *mrb, mrb_value self)
{
  mrb_int i,len;
  mrb_value v;
  mrb_float ix,iy;
  mrb_float nearest_x, nearest_y;
  mrb_float sx,sy, dx,dy;
  mrb_value subjects;
  bool new_intersection, nearest_available;
  mrb_float px,py;
  mrb_get_args(mrb, "ffffA", &sx, &sy, &dx, &dy, &subjects );

  len = RARRAY_LEN(subjects);
  nearest_available = false;
  for (i = 0; i < len; i++) {
    new_intersection = false;
    v = RARRAY_PTR(subjects)[i];
    switch (RARRAY_LEN(v)) {
    case 2:
      px = f(v,0);
      py = f(v,1);
      new_intersection = _on(sx,sy,dx,dy, px, py );
      if(new_intersection){
        ix = px;
        iy = py;
      }
      break;
    case 4:
      new_intersection = _intersection(sx,sy,dx,dy,
                                       f(v,0), f(v,1), f(v,2), f(v,3),
                                       &ix, &iy );
      break;
    }

    if(new_intersection){
      if(nearest_available) {
        if( _compare_length(sx,sy,nearest_x,nearest_y,sx,sy,ix,iy) > 0 ) {
          nearest_x = ix;
          nearest_y = iy;
        }
      }else{
        nearest_available = true;
        nearest_x = ix;
        nearest_y = iy;
      }
    }
  }

  if(nearest_available){
    mrb_value result[2];
    result[0] = mrb_float_value(mrb, nearest_x);
    result[1] = mrb_float_value(mrb, nearest_y);
    return mrb_ary_new_from_values(mrb, 2, result);
  }
  return mrb_nil_value();
}

//
// Bi::Rectangle class methods
//

static mrb_value mrb_bi_rect_collide(mrb_state *mrb, mrb_value self)
{
  mrb_int x1,y1, w1,h1, x2,y2, w2,h2;
  mrb_get_args(mrb, "iiiiiiii", &x1, &y1, &w1, &h1,  &x2, &y2, &w2, &h2);

  mrb_int l1 = x1;
  mrb_int r1 = x1 + w1 - 1;
  mrb_int b1 = y1;
  mrb_int t1 = y1 + h1 - 1;
  mrb_int l2 = x2;
  mrb_int r2 = x2 + w2 - 1;
  mrb_int b2 = y2;
  mrb_int t2 = y2 + h2 - 1;

  return  mrb_bool_value( l2 <= r1 && l1 <= r2 && b2 <= t1 && b1 <= t2 );
}

static mrb_value mrb_bi_rect_include_point(mrb_state *mrb, mrb_value self)
{
  mrb_int rx,ry,rw,rh, px,py;
  mrb_get_args(mrb, "iiiiii", &rx, &ry, &rw, &rh, &px, &py);
  return mrb_bool_value( _include_point(rx,ry,rw,rh, px,py) );
}

//
// ----
//

void mrb_mruby_bi_geometry_gem_init(mrb_state* mrb)
{
  struct RClass *bi = mrb_class_get(mrb,"Bi");
  struct RClass *line = mrb_define_module_under(mrb, bi, "Line");
  struct RClass *rect = mrb_define_module_under(mrb, bi, "Rectangle");
  mrb_define_class_method(mrb, line, "intersection", mrb_bi_line_intersection, MRB_ARGS_REQ(8));
  mrb_define_class_method(mrb, line, "cross_product", mrb_bi_line_cross_product, MRB_ARGS_REQ(8));
  mrb_define_class_method(mrb, line, "on?", mrb_bi_line_on, MRB_ARGS_REQ(8));
  mrb_define_class_method(mrb, line, "compare_length", mrb_bi_line_compare_length, MRB_ARGS_REQ(8));
  mrb_define_class_method(mrb, line, "nearest_intersection", mrb_bi_line_nearest_intersection, MRB_ARGS_REQ(5));
  mrb_define_class_method(mrb, rect, "collide?", mrb_bi_rect_collide, MRB_ARGS_REQ(8));
  mrb_define_class_method(mrb, rect, "include_point?", mrb_bi_rect_include_point, MRB_ARGS_REQ(6));
}
