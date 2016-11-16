/*
 * stack.c
 *
 *  Created on: 18 Oct 2011
 *  Copyright 2011 Nicolas Melot
 *
 * This file is part of TDDD56.
 *
 *     TDDD56 is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     TDDD56 is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with TDDD56. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef DEBUG
#define NDEBUG
#endif

#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "stack.h"
#include "non_blocking.h"

#if NON_BLOCKING == 0
#warning Stacks are synchronized through locks
#else
#if NON_BLOCKING == 1
#warning Stacks are synchronized through hardware CAS
#else
#warning Stacks are synchronized through lock-based CAS
#endif
#endif

void stack_check(stack_t *stack)
{
// Do not perform any sanity check if performance is bein measured
#if MEASURE == 0
    // Use assert() to check if your stack is in a state that makes sens
    // This test should always pass
    assert(1 == 1);

    // This test fails if the task is not allocated or if the allocation failed
    assert(stack != NULL);
#endif
}

void stack_init(stack_t* stack, int max_size)
{
    int i;
    for(i = 0; i < max_size; i++)
    {
        stack_item_t* new_item = (stack_item_t*) malloc(sizeof(stack_item_t));
        stack_item_t* new_item_unused = (stack_item_t*) malloc(sizeof(stack_item_t));

        new_item->value = 0;
        new_item_unused->value = 0;
        if(i == 0)
        {
            new_item->next = NULL;
            new_item_unused->next = NULL;
        } else
        {
            new_item->next = stack->head;
            new_item_unused->next = stack->unused;
        }
        stack->head = new_item;
        stack->unused = new_item_unused;
    }

#if NON_BLOCKING == 0
    pthread_mutex_init(&(stack->lock), NULL);
#endif
}

int stack_push(stack_t* stack, int value) /* Return the type you prefer */
{
    stack_item_t* new_stack_item;

#if NON_BLOCKING == 0 // using mutexes
    pthread_mutex_lock(&(stack->lock));

    if(stack->unused == NULL)
    {
        pthread_mutex_unlock(&(stack->lock));
        return 0;

    } else
    {
        new_stack_item = stack->unused;
        stack->unused = new_stack_item->next;
    }

    new_stack_item->value = value;
    new_stack_item->next = stack->head;
    stack->head = new_stack_item;

    pthread_mutex_unlock(&(stack->lock));
    return 1;

#elif NON_BLOCKING == 1 // using hardware CAS

    stack_item_t* oldval;

//get a stack item from the unused stack
    do
    {
        oldval = stack->unused;
        if(oldval == NULL)
        {
            return 0;
        }
        new_stack_item = oldval;
    }
    while(cas((size_t*)&(stack->unused), (size_t)oldval, (size_t)oldval->next) != (size_t)oldval);


    new_stack_item->value = value;

    // push this value to the stack
    do
    {
        oldval = stack->head;
        new_stack_item->next = oldval;
    }
    while(cas((size_t*)&(stack->head), (size_t)oldval, (size_t)new_stack_item) != (size_t)oldval);

    return 1;

#endif
}

int stack_pop(stack_t* stack, int* value)  /* Return the type you prefer */
{
    stack_item_t* popped;

#if NON_BLOCKING == 0
    pthread_mutex_lock(&(stack->lock));

    if(stack->head == NULL)
    {
        pthread_mutex_unlock(&(stack->lock));
        return 0;
    }

    popped = stack->head;
    stack->head = popped->next;
    popped->next = stack->unused;
    stack->unused = popped;
    *value = popped->value;

    pthread_mutex_unlock(&(stack->lock));

    return 1;
#elif NON_BLOCKING == 1

    stack_item_t* oldval;
    //get a stack item from the stack
    do
    {
        oldval = stack->head;
        if(oldval == NULL)
        {
            return 0;
        }
        popped = oldval;
    }
    while(cas((size_t*)&(stack->head), (size_t)oldval, (size_t)oldval->next) != (size_t)oldval);

    *value = popped->value;

    // push this item to the unused stack
    do
    {
        oldval = stack->unused;
        popped->next = oldval;
    }
    while(cas((size_t*)&(stack->unused), (size_t)oldval, (size_t)popped) != (size_t)oldval);
    return 1;

#endif
}
