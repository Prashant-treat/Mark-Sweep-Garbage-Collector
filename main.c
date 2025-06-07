#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "sneknew.h"
#include "snekobject.h"
#include "vm.h"

void test_simple() {
  printf("==== Running test_simple ====\n");

  vm_t *vm = vm_new();
  printf("[init] VM created. Object count: %d\n", vm->objects->count);

  frame_t *f1 = vm_new_frame(vm);
  printf("[frame] Frame f1 created.\n");

  snek_object_t *s = new_snek_string(vm, "I wish I knew how to read.");
  printf("[alloc] String object allocated and referenced in f1.\n");
  frame_reference_object(f1, s);

  vm_collect_garbage(vm);
  printf("[gc] Garbage collected. Object count: %d\n", vm->objects->count);
  assert(vm->objects->count == 1);

  frame_free(vm_frame_pop(vm));
  printf("[frame] Frame f1 popped and freed.\n");

  vm_collect_garbage(vm);
  printf("[gc] Garbage collected. Object count: %d\n", vm->objects->count);
  assert(vm->objects->count == 0);

  vm_free(vm);
  printf("==== test_simple passed ====\n\n");
}

void test_full() {
  printf("==== Running test_full ====\n");

  vm_t *vm = vm_new();
  printf("[init] VM created.\n");

  frame_t *f1 = vm_new_frame(vm);
  frame_t *f2 = vm_new_frame(vm);
  frame_t *f3 = vm_new_frame(vm);
  printf("[frame] Frames f1, f2, f3 created.\n");

  snek_object_t *s1 = new_snek_string(vm, "frame 1");
  frame_reference_object(f1, s1);
  printf("[alloc] s1 assigned to f1.\n");

  snek_object_t *s2 = new_snek_string(vm, "frame 2");
  frame_reference_object(f2, s2);
  printf("[alloc] s2 assigned to f2.\n");

  snek_object_t *s3 = new_snek_string(vm, "frame 3");
  frame_reference_object(f3, s3);
  printf("[alloc] s3 assigned to f3.\n");

  snek_object_t *i1 = new_snek_integer(vm, 69);
  snek_object_t *i2 = new_snek_integer(vm, 420);
  snek_object_t *i3 = new_snek_integer(vm, 1337);
  snek_object_t *v = new_snek_vector3(vm, i1, i2, i3);
  printf("[alloc] vector v created from i1, i2, i3.\n");

  frame_reference_object(f2, v);
  frame_reference_object(f3, v);
  printf("[ref] vector v referenced in f2 and f3.\n");

  printf("[info] Object count before GC: %d\n", vm->objects->count);
  assert(vm->objects->count == 7);

  frame_free(vm_frame_pop(vm)); // frees f3
  printf("[frame] Frame f3 popped and freed.\n");

  vm_collect_garbage(vm);
  printf("[gc] GC after freeing f3. Object count: %d\n", vm->objects->count);
  assert(vm->objects->count == 6);

  frame_free(vm_frame_pop(vm)); // frees f2
  printf("[frame] Frame f2 popped and freed.\n");

  frame_free(vm_frame_pop(vm)); // frees f1
  printf("[frame] Frame f1 popped and freed.\n");

  vm_collect_garbage(vm);
  printf("[gc] Final GC. Object count: %d\n", vm->objects->count);
  assert(vm->objects->count == 0);

  vm_free(vm);
  printf("==== test_full passed ====\n\n");
}

void test_shared_reference() {
  printf("==== Running test_shared_reference ====\n");

  vm_t *vm = vm_new();
  frame_t *f1 = vm_new_frame(vm);
  frame_t *f2 = vm_new_frame(vm);
  printf("[frame] Frames f1 and f2 created.\n");

  snek_object_t *shared = new_snek_string(vm, "I'm shared!");
  frame_reference_object(f1, shared);
  frame_reference_object(f2, shared);
  printf("[ref] Shared string referenced in both f1 and f2.\n");

  vm_collect_garbage(vm);
  printf("[gc] After initial GC. Object count: %d\n", vm->objects->count);
  assert(vm->objects->count == 1);

  frame_free(vm_frame_pop(vm)); // f2
  printf("[frame] Frame f2 popped and freed.\n");

  vm_collect_garbage(vm);
  printf("[gc] GC after freeing f2. Object count: %d\n", vm->objects->count);
  assert(vm->objects->count == 1); // Still referenced by f1

  frame_free(vm_frame_pop(vm)); // f1
  printf("[frame] Frame f1 popped and freed.\n");

  vm_collect_garbage(vm);
  printf("[gc] GC after freeing f1. Object count: %d\n", vm->objects->count);
  assert(vm->objects->count == 0);

  vm_free(vm);
  printf("==== test_shared_reference passed ====\n\n");
}

int main() {
  test_simple();
  test_full();
  test_shared_reference();

  printf(" All tests passed successfully.\n");
  return 0;
}
