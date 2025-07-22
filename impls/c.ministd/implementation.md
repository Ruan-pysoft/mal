I'm writing this file because it has become increasingly difficult
to progress cleanly from the base I currently have,
so here I will clarify guidelines, conventions, and future plans
for how to implement mal in c.

# Error handling/reporting

As far as I can tell, there are three possible sources of error:

 1. From the standard library (IO error, allocation error, etc)
 2. From the parsing step (unmatched parentheses, etc)
 3. From the evaluation step (divide by zero, etc)

The standard library already provides an error reporting mechanism:
Errors are represented with the `err_t` enumeration,
and any functions which can error takes an argument `err_t ref err_out`,
which may be `NULL` to ignore errors.

Error handling is then done using the `ERR_*` and `TRY_*` macros,
where `ERR_WITH` and `ERR_VOID` exits the function with the error value supplied,
and `TRY_WITH` and `TRY_VOID` exits the function with the error value supplied,
*provided* that it is not the `ERR_OK` (no error) value.

Error handling for parse errors and runtime errors can be handled similarly.
With the assumption that errors are always to be reported to the user,
and not resolved before then,
then errors can be represented quite simply:
Either it is an stdlib error (`err_t`), possibly `ERR_OK`,
or it is some error message,
which can be implemented as an owned string pointer,
to allow the building of error strings values reliant on runtime values,
and not just static error strings.

# Representing mal values

From the [guide](../../process/guide.md),
the eventual data types that mal values can take on is:

 - Symbol
 - Number
 - List
 - String
 - Nil
 - True
 - False
 - Keyword
 - Vector
 - Hash-Map
 - Function (built-in and mal)

Each mal value will need to be tagged with its type, and its data.
This can be done with an enum for the type and a union for the data.

The values should be representable with four types in c:

 - string type (symbol, string, keyword)
 - bool type (true, false)
 - void [no union member] (nil)
 - list-like type (list, vector)
 - associated list type (hash-map)
 - closure type (function)

All of these can be implemented with a typedef/struct forward declaration
(eg. `typedef struct String String;`),
and only interacting with the values via functions
(eg. `String new_str(const char *cstr);`).
This will allow the gradual expansion
of the internal representations/available functions with each step,
as long as a backwards-compatible api is maintained.

## Basic memory management api

All pass-by-value values should be copyable using simple assignment in c.

For memory managed values, the following api should be used:

```c
typedef const struct Typename_struct own Typename_own;
typedef const struct Typename_struct ref Typename_ref;

/* create a new value of type Typename;
 * an error may occur because of memory allocations.
 * Runtime errors/parse errors should be handled *outside* of this function.
 */
Typename_own typename_new(..., err_t ref err_out);
/* transform a reference to an owned pointer;
 * could also be used to clone an owned pointer
 *
 * essentially, just increment the reference count
 */
Typename_own typename_copy(Typename_ref this);
/* decrement the reference count, if it is zero release all resources;
 * this function should *never* error,
 * prevent errrors in other functions!
 * Also, guard against NULL values
*/
void Typename_free(Typename_own this);
```

Functions that can mutate such a value should have type
`Typename_own _Typename_fn(struct Typename_struct own this, ..., err_t ref err_out)`,
and should always start with an underscore to indicate that it is unsafe.
These should only be used directly after the construction of a new value,
and should exit with `ERR_PERM` if the reference count of the value is not one.
It should return `this` on success,
or `NULL` on failure,
after freeing resources.

# Memory management model

One of the hard things about working in c is how to manage the memory.
It is difficult to say when a value will be last used,
as a value might live on in one or more environments
or as part of another value (eg. as a list element).

One simple way to do this is a combination of two models:

 1. Stack allocations for small and simple values, like ints and bools.
    These will then need to be copied around to be stored at multiple locations.
 2. Simple ref-counting for large or complex values, like strings or lists.

For reference-counted values,
they should *always* be accessed through pointer,
and never as raw values.
Then the user must keep track of which pointers are references (`ref`),
and which are owned pointers (`own`).
A reference aliases an owned pointer,
and a reference does not have to be freed.
However, the user *does* have to make sure the reference
does not outlive the associated owned pointer.
An owned pointer, on the other hand,
should eventually be freed,
either in the function where it is created,
or by being passed as an owned pointer parameter to another function.

That is, if a function takes an owned pointer as an argument,
or it creates an owned value it should do one of the following,
in *any* code path (including errors):

 - Call an associated free function on the value (equivalent to the below option)
 - Pass it to another function as an owned pointer
 - Assign it to an owned pointer member of another type
 - Return it as an owned pointer

## Cycles

A reference-counted model presents us with one vexing problem:
how to deal with cycles.

Cycles could possibly occur in one of the two following circumstances:

 - The creation of an aggregate type (list, vector, hash-map)
   eventually containing itself.

   This should be avoidable by never modifying such a value
   after its initial creation,
   but instead creating a new value which is a copy of the old one,
   with the given modification applied.
 - A function closing over an environment
   being stored in that environment or a child environment.

   I can think of two methods to resolve this:

   1. Every time a function or aggregate value is added to an environment,
      it should be checked if a circular reference has been created
      and if it is so,
      a counter in the environment should be updated.
      When freeing the environment, that counter should be considered.
   2. Every time an environment is freed,
      all of its members (including child environments)
      should be checked for circular references.
      This should then be taken in to account.

   I think I will go with method 1.,
   as I suspect that the runtime costs of 2. will be excessive
   (besides also making the free function complex with unpredictable runtime cost,
   as opposed to essentially constant-time when the resources are not released)
   and will incur a large complexity cost
   (by, for example, having to keep track of all child environments).

# Lists

Up till now, I have implemented lists in a lisp-like manner,
with a list consisting of cells keeping track of the associated value
and a next element.

This is a pleasing method,
as it fits with the lisp conceptual model,
as well as allowing multiple lists with the same end values,
not just in terms of value but also in terms of physical memory location.
(eg. one might have `(let! a '(1 2 3))` and `(let! b (cons 0 (cdr a)))`
in which case the memory of `(2 3)` is shared rather than duplicated)

However, I have found that this becomes quite difficult to work with
if one is not careful,
as it does not quite fit the default way to do things in c.

I think for now,
lists can be implemented with c arrays
and then I can switch over to a more lisp-like model later,
which should be more memory-efficient, in theory.

# Hash-Map

I think the simplest way to implement a hash-map at first
is to simply store an array of keys
and an array of values.
One can then traverse the array of keys to find the associated value.
Later on this can be optimised by writing a hash function for keys
and switching the internal representation of the hash-map over to a red-black tree
or a similar structure.
