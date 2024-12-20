# mruby-bi-misc

- Bi::Archive
- Bi::Line and Bi::Rectangle
- CellularAutomaton
- dlopen
- stable_sort
- rand() with Range object
- coinflip()

# Changelog
## 5.0.0 - 2024/12/21
- Remove code that process crc. (Use mruby-bi-crc)
## 4.2.0 - 2024/10/10
- add `Kernel#execvp`
## 4.1.0 - 2023/04/09
- add `Bi::get_platform` `Bi::get_pointer_size` `Bi::little_endian?`
## 4.0.1 - 2023/04/07
- remove Sound module (fix)
## 4.0.0 - 2023/04/06
- remove Sound module
## 3.0.0 - 2023/03/23
- license changed : MIT License
## 2.1.1 - 2023/03/19
- src/geometry.c : fix by mrb_as_float.
## 2.1.0 - 2023/03/08
- src/geometry.c : replaced mrb_to_flo with mrb_float.
- mrblib/bi_archive.rb : use File.file? and return self in load function.
- src/bi_misc.c : add save_screenshot().
## 2.0.1 - 2022/11/19
- Archive: fix file read
## 2.0.0 - 2022/11/14
- archive ver3
- Archive sets error attribute when failed parse index
## 1.0.0 - 2022/11/14
- Use json instead of msgpack for assets archive.
## 0.6.1 - 2022/10/27
- fix: rand() default argument is 0
## 0.6.0 - 2022/6/8
- add MRB_FIXNUM_MIN and MRB_FIXNUM_MAX
## 0.5.1 - 2022/1/9
- fix archive decrypt
## 0.5.0 - 2021/12/22
- for libbismite 1.2
## 0.4.0 - 2021/12/18
- remove Bi::Image
## 0.3.0
- extern bi_crc64() and bi_crc32().
## 0.2.0
- implements crc32() internally.

# License
Copyright 2021-2024 kbys <work4kbys@gmail.com>

MIT License
