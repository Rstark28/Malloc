# Custom Malloc Allocator (with Red-Black Tree)

This is a `malloc` clone.
Designed to manage a memory region itself and tracks free blocks in a **red-black tree**.  

## What it does
- Provides replacements for `malloc`, `free`, and `realloc` (named `rb_malloc`, `rb_free`, `rb_realloc` in the code).  
- Uses a red-black tree to keep free blocks ordered by size for O(logn) lookup.  

## What’s missing / TODO
- **Splitting**: right now, if you request less than the block size, it doesn’t split the block into a used + smaller free piece.  
- **Coalescing**: when freeing, adjacent blocks aren’t merged back together yet.  
- **Stress testing**: I really didn't test this all that much. I know for a fact that it won't work with multiple threads as it uses sbrk.

## Credits
- The red-black tree **delete fixup** function wasn’t entirely written by me — I borrowed an implementation. Really just a pain to write and I don't have that much time to do it now.  

---