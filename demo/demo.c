#include <stdio.h>
#include <list.h>

#define NUM_NODE_ON_STACK 3

struct key_node {
	int key;
	DECLARE_LIST_NODE;
};

void main() {
	struct key_node nodes[NUM_NODE_ON_STACK];
	int i;
	list_t(struct list_node) key_list;
	struct key_node *node, *tmp;

	list_init(key_list);

	for (i = 0; i < NUM_NODE_ON_STACK; i++) {
		nodes[i].key = i;
		list_add_tail(&nodes[i], key_list);
	}

	i = 0;
	list_for_each_safe(node, key_list, tmp) {
		printf("node %d has key %d\n", i++, node->key);
		list_del(node);
	}
}

