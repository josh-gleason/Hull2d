#ifndef STACK_JG_H
#define STACK_JG_H

// A simple array based stack implmementation

#include "defines.h"

typedef struct stack_s {
    int32_t top;
    int32_t maxItems;
    int32_t itemSize;
    char* data;
} stack_t;

/**
* @brief Initialize a stack object
* @param[in/out] stack Pointer to an uninitialized stack object
* @param[in] maxItems  The maximum number of items
* @param[in] itemSize  The size of an item
*/
bool_t stack_init(stack_t* stack, int32_t maxItems, int32_t itemSize);

/**
* @brief Reset stack to the initialized state
* @param[in/out] stack Pointer to the stack object
*/
void stack_clear(stack_t* stack);

/**
* @brief Push onto the stack
* @param[in/out] stack Pointer to the stack
* @param[in] item      Pointer to the item to put on the stack
* @return Returns false if the stack is full after inserting the item
*/
bool_t stack_push(stack_t* stack, const void* item);

/**
* @brief Pop the top item from the stack
* @param[in/out] stack Pointer to the stack
* @return Returns false if the stack is empty after poping
*/
bool_t stack_pop(stack_t* stack);

/**
* @brief Get a copy of an item in the stack <idx> levels from the top
* @param[in]  stack Pointer to the stack object
* @param[in]  idx   Index to peek relative to top of stack
* @param[out] item  Pointer to an empty data store to put peeked data in
* @return Returns false if requested index isn't in stack
*/
bool_t stack_peek(const stack_t* stack, int32_t idx, void* item);

/**
* @brief Get the number of items in the stack
* @param[in] stack Pointer to the stack object
* @return Returns the number of items in the stack
*/
int32_t stack_count(const stack_t* stack);

#endif
