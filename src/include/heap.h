/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

/* Author: David Alvarez
 * heap.h - basic heap with intrusive structures. */

#ifndef HEAP_H
#define HEAP_H

#include "common.h"
#include <stddef.h>

typedef struct heap_node {
	struct heap_node *parent;
	struct heap_node *left;
	struct heap_node *right;
} heap_node_t;

typedef struct head_head {
	struct heap_node *root;
	size_t size;
} heap_head_t;

#define heap_elem(head, type, name) \
	((type *) (((char *) head) - offsetof(type, name)))

#define heap_swap(a, b)                 \
	do {                            \
		heap_node_t *aux = (a); \
		(a) = (b);              \
		(b) = aux;              \
	} while (0)

/* heap_node_compare_t - comparison function.
 * The comparison function cmp(a, b) shall return an integer:
 *  > 0 if a > b
 *  < 0 if a < b
 *  = 0 if a == b
 *
 * Invert the comparison function to get a min-heap instead */
typedef int (*heap_node_compare_t)(heap_node_t *a, heap_node_t *b);

static inline void
heap_init(heap_head_t *head)
{
	head->root = NULL;
	head->size = 0;
}

/* max_heapify maintains the max-heap property
 * When it is called for a node "a", a->left and a->right are max-heaps, but "a"
 * may be smaller than a->left or a->right, violating the max-heap property
 * max_heapify will float "a" down in the max-heap */
static inline void
heap_max_heapify(heap_head_t *head, heap_node_t *a, heap_node_compare_t cmp)
{
	heap_node_t *largest = a;

	if (a->left && cmp(a->left, largest) > 0)
		largest = a->left;
	if (a->right && cmp(a->right, largest) > 0)
		largest = a->right;

	if (largest == a)
		return;

	// Exchange
	largest->parent = a->parent;

	if (a->parent) {
		if (a->parent->left == a)
			a->parent->left = largest;
		else
			a->parent->right = largest;
	}
	a->parent = largest;

	if (head)
		head->root = largest;

	if (a->left == largest) {
		a->left = largest->left;
		if (a->left)
			a->left->parent = a;
		largest->left = a;

		heap_swap(a->right, largest->right);
		if (a->right)
			a->right->parent = a;
		if (largest->right)
			largest->right->parent = largest;
	} else {
		// Right
		a->right = largest->right;
		if (a->right)
			a->right->parent = a;

		largest->right = a;
		heap_swap(a->left, largest->left);
		if (a->left)
			a->left->parent = a;
		if (largest->left)
			largest->left->parent = largest;
	}

	heap_max_heapify(NULL, a, cmp);
}

static inline heap_node_t *
heap_max(heap_head_t *head)
{
	return head->root;
}

static inline int
leading_zeros(size_t x)
{
	/* Call and if()'s optimized by the compiler with -O2 */
	if (sizeof(size_t) == sizeof(unsigned int))
		return __builtin_clz((unsigned int) x);
	else if (sizeof(size_t) == sizeof(unsigned long))
		return __builtin_clzl((unsigned long) x);
	else if (sizeof(size_t) == sizeof(unsigned long long))
		return __builtin_clzll((unsigned long long) x);
	else
		die("cannot find suitable size for __builtin_clz*");
}

/* Get a move to reach a leaf */
static inline int
heap_get_move(size_t *node /*out*/)
{
	size_t aux_node = *node;

	// Round to previous po2
	int shift = (int) sizeof(size_t) * 8 - leading_zeros(aux_node) - 1;
	size_t base = 1ULL << shift;

	aux_node -= base / 2;

	if (aux_node < base) {
		// Left
		*node = aux_node;
		return 0;
	} else {
		// Right
		*node = aux_node - base / 2;
		return 1;
	}
}

/* Travel down the heap to find the correct node */
static inline heap_node_t *
heap_get(heap_head_t *head, size_t node)
{
	heap_node_t *current = head->root;

	while (node != 1) {
		if (heap_get_move(&node))
			current = current->right;
		else
			current = current->left;
	}

	return current;
}

static inline heap_node_t *
heap_pop_max(heap_head_t *head, heap_node_compare_t cmp)
{
	heap_node_t *max = head->root;

	if (!max)
		return NULL;

	size_t size = head->size;
	heap_node_t *change = heap_get(head, size);
	if (change == NULL)
		die("heap_get() failed");

	head->size--;

	// Special case
	if (!change->parent) {
		head->root = NULL;
		return max;
	}

	if (change->parent == max) {
		// Right child
		if (size % 2) {
			change->left = max->left;
			if (change->left)
				change->left->parent = change;
		} else {
			change->right = max->right;
			if (change->right)
				change->right->parent = change;
		}

		change->parent = NULL;
		head->root = change;
	} else {
		// Right child
		if (size % 2)
			change->parent->right = NULL;
		else
			change->parent->left = NULL;

		if (change->left)
			die("change->left not NULL");

		if (change->right)
			die("change->right not NULL");

		change->left = max->left;
		if (change->left)
			change->left->parent = change;
		change->right = max->right;
		if (change->right)
			change->right->parent = change;

		change->parent = NULL;
		head->root = change;
	}

	heap_max_heapify(head, change, cmp);
	return max;
}

static inline void
heap_insert(heap_head_t *head, heap_node_t *node, heap_node_compare_t cmp)
{
	node->left = NULL;
	node->right = NULL;
	node->parent = NULL;
	head->size++;

	if (!head->root) {
		// Easy
		head->root = node;
		return;
	}

	// Insert on size's parent
	size_t insert = head->size / 2;
	heap_node_t *parent = heap_get(head, insert);

	// Right child
	if (head->size % 2) {
		if (parent->right)
			die("parent->right already set");
		parent->right = node;
	} else {
		if (parent->left)
			die("parent->left already set");
		parent->left = node;
	}

	node->parent = parent;

	// Equivalent of HEAP-INCREASE-KEY
	while (parent && cmp(node, parent) > 0) {
		// Bubble up
		node->parent = parent->parent;
		parent->parent = node;

		if (node->parent) {
			if (node->parent->left == parent)
				node->parent->left = node;
			else
				node->parent->right = node;
		}

		if (parent->left == node) {
			parent->left = node->left;
			if (parent->left)
				parent->left->parent = parent;

			node->left = parent;

			heap_swap(node->right, parent->right);
			if (node->right)
				node->right->parent = node;
			if (parent->right)
				parent->right->parent = parent;
		} else {
			parent->right = node->right;
			if (parent->right)
				parent->right->parent = parent;

			node->right = parent;
			heap_swap(node->left, parent->left);
			if (node->left)
				node->left->parent = node;
			if (parent->left)
				parent->left->parent = parent;
		}

		parent = node->parent;
	}

	if (!parent)
		head->root = node;
}

#endif// HEAP_H
