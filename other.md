# Handling Cycles
We built a simple reference counting garbage collector. It can handle:

* Simple types, like INT and FLOAT.
* Dynamically allocated types, like STRING.
* Static Container types, like VECTOR3.
* Dynamic Container types, like ARRAY.
However, there's one little problem with our implementation. Take a look at this code:
```c
snek_object_t *first = new_snek_array(1);
snek_object_t *second = new_snek_array(1);
// refcounts: first = 1, second = 1
snek_array_set(first, 0, second);
// refcounts: first = 1, second = 2
refcount_dec(first);
// refcounts: first = 0, second = 1
refcount_dec(second);
// refcounts: first = 0, second = 0
// all free!
```
We create a first array, and shove the second array inside of it. Everything here works as expected. The trouble arises when we introduce a cycle: for example, first contains second but second also contains first...

# Assignment
Run the code in its current state. Notice that the assertions fail. Even though we decremented both the first and second arrays' refcounts, neither were freed (refcount = 0).

Fix the assertions to pass by updating them to match the sad reality of our current implementation.

Observe
The reason both refcounts are stuck at one after being decremented, is because when first has its refcount decremented it already had 2. So it only drops to 1, which does not trigger a "free" of the second array:
```c
void refcount_dec(snek_object_t *obj) {
  if (obj == NULL) {
    return;
  }
  obj->refcount--;
  if (obj->refcount == 0) {
    // this doesn't happen when refcount is 1
    return refcount_free(obj);
  }
  return;
}
```
And because second still has 2 refcounts, it also only drops to 1, which fails to trigger a "free" of the first array.

In other words, we have a cycle, and our simple reference counting garbage collector can't handle it.

<hr>

# Pros and Cons
To solve our cyclic reference issue (and to force you to implement another GC algorithm, HA!) we're going to implement a Mark and Sweep garbage collector.

# Pros of MaS
* Can detect cycles, and thus prevent memory leaks in certain cases
* Less on-demand bookkeeping (Remember: all work done by the GC is "wasted" - it doesn't make your GPT-4 wrapper custom AI product run any faster)
* Reduces potential performance degradation in highly multithreaded programs (refcounting requires atomic updates for thread safety)
# Cons of MaS
* More complex to implement (heh, you'll see)
* Can cause "stop-the-world" pauses when lots of objects exist and must be freed (resulting in poor performance)
* Higher memory overhead
* Less predictable performance
# Assignment
We'll be using a vm_t struct which stands for Virtual Machine Type. This vm_t simulates what would normally be tracked if Sneklang were a fully functional interpreted language (it's not). This virtual machine is much simpler than a real one because all we care about is demonstrating the garbage collection aspects.

You can spam Lane if you want a full "write your own programming language" course on boot.dev

Please don't. I have a wife and kids.

Open vm.h and take a look at the vm_t struct. The frames field holds a stack of frames, which are pushed and popped as we enter and exit new scopes. For example:
```c
msg1 = "This is in scope 1"
def outer_func():
    msg2 = "This is in scope 2"
    def inner_func():
        msg2 = "This is in scope 3"
        return
    return
```
At each of the scope entrances (in this case function calls), a new stack frame is pushed onto the frames stack. When we exit a scope (a function returns), we pop the stack frame off the frames stack. Because we use void * to work with generics in C, you can't actually tell what the data type held by the stack_t is for each field. We'll write some wrapper functions later to help us make sure that we don't accidentally push the wrong kinds of data into our stacks (yay, C!).

The objects field is also a stack, but it holds snek_object_t pointers.

1. In vm.c complete the vm_new function. It should:
    1. allocate space for a vm_t struct on the heap
    2. initialize the frames stack with a capacity of 8 and the objects stack with a capacity of 8 using the stack_new function
    3. return the pointer to our new vm_t struct
2. Complete the vm_free function. It should:
    1. free the frames and objects stacks (remember, we wrote a special stack_free function for this)
    2. free the vm_t struct itself

    <hr>

# Stack Frames
Think back to the warnings I gave you about working with void* data types. It's easy to push the wrong kinds of data onto our stack_t because, like Starbucks Wi-Fi, it will let anything in.

So, to prevent us from shooting ourselves in the foot, we will create some functions that are more type safe and make it much more difficult to do the wrong thing when interacting with our vm_t. Wrong things like:
```c
// The C compiler won't stop us :'(
stack_push(vm->frames, (void *)7);
stack_push(vm->frames, (void *)"uh oh");
```
But we want to make it easier on ourselves to only push frame_t * types onto vm->frames, so we'll write some wrapper functions to help us out.

# Assignment
Take a look at the frame_t type in vm.h. It's a simple struct that holds a stack_t of snek_object_t * pointers that represent the object references in the frame.

1. Complete the vm_frame_push function in vm.c. It should stack_push a frame_t * onto the vm->frames stack.
2. Complete the vm_new_frame function in vm.c. It should:
    1. Allocate a new frame_t on the heap.
    2. Initialize the frame's references with a stack_new of capacity 8.
    3. Push the newly allocated frame onto the vm->frames stack.
    4. Return the newly allocated frame.
3. Complete the frame_free function in vm.c. It should:
    1. Free the frame_t's references stack (we have a special function for this)
    2. Free the frame_t struct.

<hr>

# Tracking Objects
Our virtual machine needs to track every Snek object that gets created.

We are no longer going to track how many times an object is referenced, but instead check at garbage collection time if each object is still referenced at all. If it is, keep it. If not, free it.

# Assignment
1. In vm.c complete the vm_track_object, which adds an object to our vm->objects stack in a more type-safe way.
2. In sneknew.c update the _new_snek_object function. Before returning the newly allocated object, it should ensure it's tracked by the VM.

<hr>


 # Free
Recall the refcount_free function that you wrote in the previous chapter. It should have looked something like this:
```c
void refcount_free(snek_object_t *obj) {
  switch (obj->kind) {
    case INTEGER:
    case FLOAT:
      break;
    case STRING:
      free(obj->data.v_string);
      break;
    case VECTOR3: {
      snek_vector_t vec = obj->data.v_vector3;
      refcount_dec(vec.x);
      refcount_dec(vec.y);
      refcount_dec(vec.z);
      break;
    }
    case ARRAY: {
      snek_array_t *array = &obj->data.v_array;
      for (size_t i = 0; i < array->size; i++) {
        refcount_dec(array->elements[i]);
      }
      free(array->elements);
      break;
    }
  }
  free(obj);
}
```
# Assignment
Let's rewrite our free-ing logic for mark-and-sweep. Because the virtual machine is the one tracking objects, all of the refcount_dec work can be removed! There's some very cool tricks coming up for mark-and-sweep to manage this, but for now you can just trust me that we'll correctly free any of the contained elements if they are no longer alive.

1. In snekobject.c complete the snek_object_free function.
    1. INTEGER, FLOAT, and STRING objects should all work the same as the refcount version (non container types).
    2. VECTOR3 doesn't need to do anything, because the mark-and-sweep will handle the contained objects.
    3. ARRAY should free the elements array, but not the objects themselves because the mark-and-sweep will handle that.
2. In vm.c complete the vm_free function.
    1. Iterate over all the VM's frames and free them using frame_free.
    2. Free the frames stack itself using stack_free.
    3. Iterate over all the VM's objects and free them using snek_object_free.
    4. Free the objects stack itself using stack_free.
    5. Free the VM struct itself.


    <hr>

# Mark and Sweep
We finally have enough machinery in place to start thinking about the "Mark and Sweep" part of our garbage collector.

# The Algorithm
Mark and Sweep garbage collection was first described by John McCarthy in 1960, primarily for managing memory in ((lisp)). It's a two-phase algorithm:

1. Mark Phase: Traverses the object graph, marking all reachable objects.
2. Sweep Phase: Scan memory, collecting all unmarked objects, which are considered garbage.
Note! We don't keep track of how many times a particular object is referenced, like we did with reference counting! Instead, we keep track of which objects are referenced in each stack frame and then traverse our container objects looking for any other referenced objects. That's what "traverse the object graph" means - a fancy way of saying "look for objects".

# Assignment
1. In snekobject.h add an is_marked boolean to our snek_object_t struct.
2. In sneknew.c ensure the is_marked field is set to false when we create a _new_snek_object.

<hr>

# Mark
``TODO: Would it be funny to just have a video clip of me going "MAAARRKKKK" and that's it?``

``I'd advise against it.``

In some mark and sweep implementations you'll see different ways to mark "root objects" - the objects directly referenced by stack frames. However, in our simplistic VM it's a bit easier to find and mark all of the directly referenced objects. You'll see why when you write it in a moment.

# Assignment
 Complete the mark function in vm.c. It should set every object that's directly referenced by a stack frame to is_marked = true.
1. Iterate over each frame in the VM
2. Iterate over each references object in each frame
3. Mark the objects as is_marked = true


<hr>

# Trace

Now that we've done the first (and simplest) part: marking, we can trace through all of our objects and determine which ones are connected to the roots. For example:
```py
def get_list():
  a = 5
  return [a]

print(get_list())
```
If we run this code it will return a list with the integer a inside of it. Our current mark function will mark the list, but it won't mark the integer a. Which means that when we go to sweep the memory, we will mark the list, but not the integer a. We'd then free a while it's still being used, and the operating system could then fill that memory with something else... which would be very bad!
So we need to prevent this!

But we also have another problem:
```py
def get_list():
    a = []
    a.append(a)
    return [5]

print(get_list())
```
In the above (very dumb) example, we create a list that references itself and then return a completely unrelated list.

If our trace function looks for any object that is referenced by any other object it will consider a alive because it has a reference (albeit, to itself in this case). In fact, a is unreachable because when get_list returns, a is no longer used anywhere.

``Tracing solves these problems. To be clear, tracing is part of the "mark" phase of mark and sweep. It's where we mark all the objects referenced by our root objects.``

# Assignment
This is one of the larger assignments, take your time. You're almost done with the course!

1. Complete the trace_mark_object function in vm.c.
    1. If the object is NULL or already marked, return immediately without doing anything.
    2. Otherwise, mark the object and push it onto the gray_objects stack.
2. Complete the trace_blacken_object function in vm.c.
    1. If the object is an INTEGER, FLOAT or STRING do nothing. These don't contain references to other objects.
    2. f it's a VECTOR3, call trace_mark_object on the x, y, and z fields.
    3. If it's an ARRAY, call trace_mark_object on each element.
3. Complete the trace function in vm.c.
    1. Create a new stack (stack_new) with a capacity of 8 called gray_objects. If the allocation fails, return.
    2. Iterate over each of the objects in the VM: if the object is marked, push it onto the gray_objects stack.
3. While the gray_objects stack is not empty:
    1. Pop an object off the gray_objects stack.
    2. Call trace_blacken_object on the object.
4. Free the gray_objects stack (stack_free)

<hr>

# Sweep
Sweep is easy! Trace was probably the hardest part of the garbage collector.

Every object now has an is_marked field that we can use to determine if an object is reachable or not. All we need to do is iterate over all the objects in the VM and free any object that is not marked. Once it's freed, we can also remove it from our VM.

''One more thing that's not obvious about sweep(): Any object that is marked (we don't want to free it right now) needs to be reset to is_marked = false. That way the next time the mark phase runs, if it's not marked again it will be freed in the next cycle.''

# Assignment
1. Complete the sweep function.
    1. Iterate over all of the VM's objects:
        1. If the object is marked, reset is_marked = false and continue.
        2. Otherwise, free the object and set the data at that position in the stack to NULL.
    2. Call stack_remove_nulls to remove any NULL objects from the VM's objects stack.
2. Complete the vm_collect_garbage function. This is it! This is the function that runs the GC.
    1. Call mark.
    2. Call trace.
    3. Call sweep.
Hope you liked the course! Great work.

### Hint
You can use the snek_object_free function to free the objects