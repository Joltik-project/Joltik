#include "heap.h"

void initMinHeap(minHeap *heap, uint32_t size) { heap->size = 0; }

/*
    Function to swap data within two nodes of the min heap using pointers
*/
void swap(node *n1, node *n2) {
    node temp = *n1;
    *n1 = *n2;
    *n2 = temp;
}

/*
    Heapify function is used to make sure that the heap property is never
   violated In case of deletion of a node, or creating a min heap from an array,
   heap property may be violated. In such cases, heapify function can be called
   to make sure that heap property is never violated
*/
void heapify(minHeap *hp, int i) {
    int smallest =
        (LCHILD(i) < hp->size && hp->elem[LCHILD(i)].count < hp->elem[i].count)
            ? LCHILD(i)
            : i;
    if (RCHILD(i) < hp->size &&
        hp->elem[RCHILD(i)].count < hp->elem[smallest].count) {
        smallest = RCHILD(i);
    }
    if (smallest != i) {
        swap(&(hp->elem[i]), &(hp->elem[smallest]));
        heapify(hp, smallest);
    }
}

/*
    Build a Min Heap given an array of numbers
    Instead of using insertNode() function n times for total complexity of
   O(nlogn), we can use the buildMinHeap() function to build the heap in O(n)
   time
*/
void buildMinHeap(minHeap *hp, my_int *arr, int size) {
    int i;

    // Insertion into the heap without violating the shape property
    for (i = 0; i < size; i++) {
        if (hp->size) {
            hp->elem = (node *)realloc(hp->elem, (hp->size + 1) * sizeof(node));
        } else {
            hp->elem = (node *)malloc(sizeof(node));
        }
        node nd;
        nd.count = arr[i];
        hp->elem[(hp->size)++] = nd;
    }

    // Making sure that heap property is also satisfied
    for (i = (hp->size - 1) / 2; i >= 0; i--) {
        heapify(hp, i);
    }
}

/*
    Function to insert a node into the min heap, by allocating space for that
   node in the heap and also making sure that the heap property and shape
   propety are never violated.
*/
void insertNode(minHeap *hp, my_key _key, my_int _counter) {
    if (hp->size) {
        hp->elem = (node *)realloc(hp->elem, (hp->size + 1) * sizeof(node));
    } else {
        hp->elem = (node *)malloc(sizeof(node));
    }

    node nd;
    nd.key = _key;
    nd.count = _counter;

    uint32_t i = (hp->size)++;
    while (i && nd.count < hp->elem[PARENT(i)].count) {
        hp->elem[i] = hp->elem[PARENT(i)];
        i = PARENT(i);
    }
    hp->elem[i] = nd;
}

/*
    Function to delete a node from the min heap
    It shall remove the root node, and place the last node in its place
    and then call heapify function to make sure that the heap property
    is never violated
*/
void deleteNode(minHeap *hp) {
    if (hp->size) {
        // printf("Deleting node %d\n\n", hp->elem[0].data) ;
        hp->elem[0] = hp->elem[--(hp->size)];
        hp->elem = (node *)realloc(hp->elem, hp->size * sizeof(node));
        heapify(hp, 0);
    } else {
        // printf("\nMin Heap is empty!\n") ;
        free(hp->elem);
    }
}

/*
    Function to get maximum node from a min heap
    The maximum node shall always be one of the leaf nodes. So we shall
   recursively move through both left and right child, until we find their
   maximum nodes, and compare which is larger. It shall be done recursively
   until we get the maximum node
*/
uint32_t getMaxNode(minHeap *hp, uint32_t i) {
    if (LCHILD(i) >= hp->size) {
        return hp->elem[i].count;
    }

    uint32_t l = getMaxNode(hp, LCHILD(i));
    uint32_t r = getMaxNode(hp, RCHILD(i));

    if (l >= r) {
        return l;
    } else {
        return r;
    }
}

void deleteMinHeap(minHeap *hp) { free(hp->elem); }

/*
    Function to find the item in min heap
*/
int find(minHeap *hp, my_key _key) {
    for (uint32_t i = 0; i < hp->size; ++i) {
        if (hp->elem[i].key == _key) {
            return i;
        }
    }
    return -1;
}
