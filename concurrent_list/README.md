# Lock-free concurrent list using RAW(Read After Write) pattern.

This list is an implementation of list on the papaer 'A Scalable Lock Manager for Multicores, Hyungsoo Jung, '.

H. Jung, H. Han, A. Fekete, G. Heiser, and H. Y. Yeom, “A Scalable Lock Manager for Multicores,” in *SIGMOD*, New York, NY, USA, 2013.


## Abstract Data Type

- node_t\* push_back(int) : Insert a node at the end of the list. Return the address of the inserted node.
- void erase(node_t\*) : Delete a node in the list.

This list is only able to insert a node at the end of the list, not front or middle.

Deletion can be conducted anywhere.
