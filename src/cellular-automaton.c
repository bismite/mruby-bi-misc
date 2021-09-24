#include <mruby.h>
#include <mruby/class.h>
#include <mruby/array.h>
#include <mruby/value.h>

static mrb_int get_xy_torus(mrb_value *ary, mrb_int w, mrb_int h, mrb_int x, mrb_int y)
{
  x = x % w;
  y = y % h;
  if( x < 0 ) { x = w + x; }
  if( y < 0 ) { y = h + y; }
  return mrb_fixnum( RARRAY_PTR(*ary)[ y * w + x ] ) > 0 ? 1 : 0;
}

static mrb_value mrb_cellular_automaton_step(mrb_state *mrb, mrb_value ary)
{
  mrb_int width, height;
  mrb_value birth, death;
  mrb_value source, result;
  mrb_int count, x, y;
  mrb_bool eight_connected;
  int len;

  mrb_get_args(mrb, "AiAAb",
    &source,
    &width,
    &birth,
    &death,
    &eight_connected
  );

  len = RARRAY_LEN(source);
  result = mrb_ary_new_capa(mrb, len);
  height = len / width;

  for (int i = 0; i < len; i++) {
    count = 0;
    x = i % width;
    y = i / width;

    count += get_xy_torus(&source,width,height,x  ,y-1);
    count += get_xy_torus(&source,width,height,x-1,y  );
    count += get_xy_torus(&source,width,height,x+1,y  );
    count += get_xy_torus(&source,width,height,x  ,y+1);

    if(eight_connected){
      count += get_xy_torus(&source,width,height,x-1,y-1);
      count += get_xy_torus(&source,width,height,x+1,y-1);
      count += get_xy_torus(&source,width,height,x-1,y+1);
      count += get_xy_torus(&source,width,height,x+1,y+1);
    }

    if( mrb_fixnum( RARRAY_PTR(source)[i] ) == 0 ){
      mrb_ary_set(mrb,result,i, mrb_fixnum_value(0) );
      for(int j=0; j<RARRAY_LEN(birth);j++){
        if(count == mrb_fixnum(RARRAY_PTR(birth)[j])){
          mrb_ary_set(mrb,result,i, mrb_fixnum_value(1) );
          break;
        }
      }
    } else {
      mrb_ary_set(mrb,result,i, mrb_fixnum_value(1) );
      for(int j=0; j<RARRAY_LEN(death);j++){
        if(count == mrb_fixnum(RARRAY_PTR(death)[j])){
          mrb_ary_set(mrb,result,i, mrb_fixnum_value(0) );
          break;
        }
      }
    }
  }
  return result;
}

void mrb_mruby_cellular_automaton_gem_init(mrb_state *mrb)
{
  struct RClass *ca = mrb_define_class(mrb, "CellularAutomaton", mrb->object_class);
  mrb_define_class_method(mrb, ca, "step", mrb_cellular_automaton_step, MRB_ARGS_REQ(6));
}
