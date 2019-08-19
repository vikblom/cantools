
add_library(canhash hashtable.c hashtable.h
  hashtable_itr.c hashtable_itr.h hashtable_private.h)
target_include_directories(canhash INTERFACE .) # TODO: Limit public
