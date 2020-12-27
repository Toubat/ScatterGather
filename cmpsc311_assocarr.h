#ifndef CMPSC311_ASSOCARR_INCLUDED
#define CMPSC311_ASSOCARR_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : cmpsc311_assocarr.h
//  Description    : This is the header file for the implementation of the
//                   CMPSC311 associative array support library.  Here the 
//                   array contains UNIQUE keys mapping to values (which may 
//                   or may not be unique).
//
//  Author         : Patrick McDaniel
//  Change Log     :
//
//     01/25/20 - Created the intial version of the associate array 
//                implementation
//

// Type definitions

// Call back for the comparison of the keys and values
typedef int (*compareCallback)(void *, void *);

// Associate array element structure
typedef struct aaElement {
    void             *key;     // THe key for this element
    void             *value;   // The value for this element
    struct aaElement *next;    // Pointer to next element in array
} AssocArrayElement;

// Associative array structure
typedef struct  {
    compareCallback    keyCompare;  // Function pointer for compare keys
    compareCallback    valCompare;  // Function pointer for value compares
    int                noElements;  // Number of elements
    AssocArrayElement *elements;    // The elements of the array
    AssocArrayElement *iterator;    // The iterator for the array
} AssocArray;

//
// Interfaces for the associatuve array

int init_assoc( AssocArray *arr, compareCallback kcb, compareCallback vcb );
    // Initialize the array structure

int insert_assoc( AssocArray *ar, void *key, void *val );
    // Insert a K/V into the associative array

int delete_assoc( AssocArray *ar, void *key );
    // Remove a key/value pair by key

void * find_assoc( AssocArray *ar, void *key );
    // Find a value by the key in the table

void * pop_assoc( AssocArray *ar, void **key, void **val );
    // Pop an element off the front of the association

void * assocAt( AssocArray *ar, int index );
    // Get an association at a specific index in the array

int clear_assoc( AssocArray *ar, int freeKeys, int freeValues );
    // Clear out the entire array

int nextIterator( AssocArray *ar, void **key, void **value, int reset );
    // Get the next value in the interator (see .c for important note)

/* Generic Callback Functions */

int int64CompareCallback( void *arga, void *argb );
    // Usable callback for 32-bit integer keys/values

int uint64CompareCallback( void *arga, void *argb );
    // Usable callback for 32-bit integer keys/values

int int32CompareCallback( void *arga, void *argb );
    // Usable callback for 32-bit integer keys/values

int uint32CompareCallback( void *arga, void *argb );
    // Usable callback for 32-bit integer keys/values

int int8CompareCallback( void *arga, void *argb );
    // Usable callback for 32-bit integer keys/values

int uint8CompareCallback( void *arga, void *argb );
    // Usable callback for 32-bit integer keys/values

int stringCompareCallback( void *arga, void *argb );
    // Usable callback for "C" string keys/values

int pointerCompareCallback( void *arga, void *argb );
    // Usable callback for generic pointers (order by address)

/* Unit tests */

int unit_test_assoc( void );
    // Unit test the associative array implementation

#endif