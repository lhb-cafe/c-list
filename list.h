#ifndef C_LIST_H
#define C_LIST_H

#ifndef NULL
#define NULL 0
#endif

#ifndef offset_of
#define offset_of(TYPE, MEMBER)	((size_t)&((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
#define container_of(PTR, TYPE, MERBER) ((TYPE *)((void *)(PTR) - offset_of(TYPE, MERBER)))
#endif

/*
 * This is an modified version of the linux kernel list
 * The goal of the modification is to hide the list details
 *
 * The new use case:
 * Define struct foo as a list node:
 *		struct foo {
 *			DECLARE_LIST_NODE; // this can be placed anywhere inside the struct
 *			int bar;
 *		};
 *
 * Declare a list for struct foo:
 *		list_t(struct foo) foo_list;
 *
 * Initialize the list:
 *		list_init(foo_list);
 * 
 * Insert an element:
 *		struct foo *foo_instance = malloc(sizeof(struct foo));
 *		list_add(foo_instance, foo_list);
 *
 * Get/Delete an element:
 * 		// the list is assumed to be non-empty
 *		struct foo *foo_instance = list_first_entry(foo_list);
 *		list_del(foo_instance);
 *
 * Traverse the list:
 * 		struct foo *foo_instance;
 *		list_for_each(foo_instance, foo_list) {
 *			// ...
 *		}
 *
 * HL: the only annoying part is the 'DECLARE_LIST_NODE' macro, but
 * to the best of my knowledge I am not able to make it look better
 * without changing the conventional syntax of c struct declaration
 */

/**
 * List internals
 */
 
struct list_head;

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_MAGIC_VAR \
	this_turns_container_into_list
#define TO_LIST_NODE_MARK(ptr, mark) \
	&(ptr)->mark
#define TO_LIST_NODE(ptr) \
	TO_LIST_NODE_MARK(ptr, LIST_MAGIC_VAR)

#define list_entry(ptr, type, mark) \
	container_of(ptr, type, mark)

/**
 * List declaration
 */

// the purpose of 'dummy' is for gcc to know the list type
// this is useful in list getter
#define list_t(TYPE)			\
	union {						\
		struct list_head head;	\
		typeof(TYPE) *dummy;	\
	}
#define list_type(list)	typeof(*(list).dummy)
#define list2head(list) ((struct list_head*)&(list))
// one downside to using union is we can't assign list by another 
// list directly if their declared separately
// hopefully this is a rare use case
#define listcopy(dst, src) ((dst).head = (src).head)

/**
 * Initialization
 */
#define list_init(list) __list_init(list2head(list))
static inline void __list_init(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

#define list_empty(list) __list_empty(list2head(list))
static inline int __list_empty(const struct list_head *head)
{
	return head->next == head;
}

/**
 * List node definition
 */

#define DECLARE_LIST_NODE \
	struct list_head LIST_MAGIC_VAR
// in case a structure is needed in multiple lists
// struct list_head is still opaque in this case, so instead of writing:
// struct list_head list1, list2, list3;
// we now write:
// LIST_NODE_MARKS(list1, list2, list3)
#define DECLARE_LIST_NODE_MARKS(...) \
	list_head __VA_ARGS__

/**
 * Iteration
 */
#define __list_for_each_marked(pos, head, mark)				\
	for ((pos) = list_entry((head)->next, typeof(*(pos)), mark);	\
	     &(pos)->mark != (head);			\
	     (pos) = list_entry((pos)->mark.next, typeof(*(pos)), mark))

#define __list_for_each_marked_safe(pos, head, mark, tmp) \
	for ((pos) = list_entry((head)->next, typeof(*(pos)), mark), tmp = (void*)(pos)->mark.next; \
	     &(pos)->mark != (head); \
	     (pos) = list_entry(tmp, typeof(*(pos)), mark), tmp = (void*)(pos)->mark.next)
// pos: pointer to a list node
// list: the list
#define list_for_each_marked(pos, list, mark)				\
	__list_for_each_marked(pos, list2head(list), mark)
#define list_for_each(pos, list) \
	list_for_each_marked(pos, list, LIST_MAGIC_VAR)
// safe version, i.e., you can modify list internals (e.g. delete a node) inside the loop
//
// 'tmp' is anything of pointer size. I don't know how to elegantly hide it from user
// One idea I had is to declare a per-list temporary pointer in the list_t macro as
// a struct member, with atomics we could even make it thread safe.
// However this would make each list_t 0.5 times larger in space, and I am not sure how
// to incorporate it into the macro properly.
#define list_for_each_marked_safe(pos, list, mark, tmp)				\
	__list_for_each_marked_safe(pos, list2head(list), mark, tmp)
#define list_for_each_safe(pos, list, tmp) \
	list_for_each_marked_safe(pos, list, LIST_MAGIC_VAR, tmp)

/**
 * Getter
 */
#define list_first_entry_mark(list, mark) \
	list_entry(list2head(list)->next, list_type(list), mark)
#define list_last_entry_mark(list, mark) \
	list_entry(list2head(list)->prev, list_type(list), mark)

#define list_pop_first_marked(list, mark) \
({	\
	list_type(list) *node = list_first_entry_mark(list, mark);	\
	list_del(node);	\
	node;	\
})
#define list_pop_last_marked(list, mark) \
({	\
	list_type(list) *node = list_last_entry_mark(list, mark);	\
	list_del(node);	\
	node;	\
})

#define list_first_entry(list) \
	list_first_entry_mark(list, LIST_MAGIC_VAR)
#define list_last_entry(list) \
	list_last_entry_mark(list, LIST_MAGIC_VAR)

#define list_pop_first(list) \
	list_pop_first_marked(list, LIST_MAGIC_VAR)
#define list_pop_last(list) \
	list_pop_last_marked(list, LIST_MAGIC_VAR)

/**
 * Setter
 */
#define list_add_marked(new, mark, list) __list_add(TO_LIST_NODE_MARK(new, mark), list2head(list))
#define list_add(new, list) list_add_marked(new, LIST_MAGIC_VAR, list)
static inline void __list_add(struct list_head *new, struct list_head *head)
{
	new->next = head->next;
	new->prev = head;
	head->next->prev = new;
	head->next = new;
}

#define list_add_tail_marked(new, mark, list) __list_add_tail(TO_LIST_NODE_MARK(new, mark), list2head(list))
#define list_add_tail(new, list) list_add_tail_marked(new, LIST_MAGIC_VAR, list)
static inline void __list_add_tail(struct list_head *new, struct list_head *head)
{
	new->next = head;
	new->prev = head->prev;
	head->prev->next = new;
	head->prev = new;
}

#define list_del_marked(old, mark) __list_del(TO_LIST_NODE_MARK(old, mark))
#define list_del(old) list_del_marked(old, LIST_MAGIC_VAR)
static inline void __list_del(struct list_head *old)
{
	old->next->prev = old->prev;
	old->prev->next = old->next;
	old->next = NULL;
	old->prev = NULL;
}

#endif /* !C_LIST_H */
