# C List

This is a modified version of the Linux kernel's list implementation. The goal is to simplify the interface and improve readability, while keeping the same space and time complexity.

The internal list implementation details, such as `struct list_head`, is now hidden in user code.

## List interface (diff with the linux kernel's list):

### Declare struct foo as a list node:

To declare a structure as a list node, the original way is to put a `struct list_head` member inside the struct. The user also has to come up with and remember a variable name. I found this a bit annoying, and used a macro `DECLARE_LIST_NODE` to do the same thing.

 ```diff
struct foo {
-	struct list_head list; // user-irrelevant list implementation internals
+	DECLARE_LIST_NODE; // this can be placed anywhere inside the struct
	int bar;
};
```

Declaring a list node type to be inserted into multiple lists is also possible. See `DECLARE_LIST_NODE_MARKS` and the relevant macros in list.h.

TBH I still find this `DECLARE_LIST_NODE` macro annoying, but I don't know how to simplify it further without changing the struct declaration syntax.

### Define and initialize a list for struct foo:

Another thing troubling me in using the linux kernel's list is the same `struct list_head` member being used in the list head and list nodes. C-like, but I dislike it.
My implementation uses a pseudo-type `list_t`, with the type of list nodes specified in parenthesis, so it looks similar to the c++ list declaration (which uses <> instead). This improves readability in multiple ways.
* A list is declared using `list_t`, no more `struct list_head` and confusion between list and list nodes.
* Reader knows the type of list nodes by looking at the list declaration.
* List interface is simplified because the compiler already knows the type of list nodes for any list, so we wouldn't need to specify the node type anymore in, e.g., `list_first_entry()` . See more examples below for the new list interface.

Note that the list node type is declared as a union with the `struct list_head` in the internal `list_t` implementation, so this does not introduce any additional space/time complexity compared with the original Linux kernel implementation. It's only there to provide information for the compiler. See list.h for details.

```diff
-struct list_head foo_list; // same confusing struct as used by a list node
+list_t(struct foo) foo_list; // improved clarity in multiple ways
list_init(foo_list);
```

### Insert an element into the list:

```diff
struct foo *foo_instance = malloc(sizeof(struct foo));
-// c-like, but imho too much irrelavent details
-list_add(&foo_instanc->list, &foo_list)
+list_add(foo_instance, foo_list); // more straight forward
```

### Get/Delete the first element:

`list_first_entry` only requires one argument (the list) now, compared with the original implementation, which takes 3 "arguments".

```diff
// the list is assumed to be non-empty
-struct foo *foo_instance = list_first_entry(&foo_list, struct foo, list);
-list_del(&foo_instance->list)

// the list is assumed to be non-empty
+struct foo *foo_instance = list_first_entry(foo_list);
+list_del(foo_instance);
```

### Traverse the list:

```diff
-struct foo *foo_instance;
-struct list_head *head;
-list_for_each(head, foo_list, list) {
-	foo_instance = list_entry(head, struct foo, list)
-	// ...
-}

+struct foo *foo_instance;
+list_for_each(foo_instance, foo_list) {
+	// ...
+}
```

## Notes

This is a stand-alone header that has no dependency requirement. However, a couple notes:

* The `LIST_MAGIC_VAR` macro declares a `struct list_head this_turns_container_into_list`. It is assumed that there is no other member named `this_turns_container_into_list` directly in the struct.
* The `offset_of()` and `container_of()` macros are used in the implementation. If these macros aren't already defined, list.h defines it for you. However, if these macros already exists, you should make sure it does the same thing as their Linux counterparts.

