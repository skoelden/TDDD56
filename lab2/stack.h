/*
 * stack.h
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
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with TDDD56. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include <pthread.h>

#ifndef STACK_H
#define STACK_H

struct stack_item
{
    int value;
    struct stack_item* next;
};
typedef struct stack_item stack_item_t;

struct stack
{
#if NON_BLOCKING == 0
    pthread_mutex_t lock;
#endif
    stack_item_t* head;
    stack_item_t* unused;
};
typedef struct stack stack_t;

void stack_init(stack_t*, int);
// Pushes an element in a thread-safe manner
int stack_push(stack_t*, int); /* Return the type you prefer */

// Pops an element in a thread-safe manner
int stack_pop(stack_t*, int*);  /* Return the type you prefer */



/* Debug practice: check the boolean expression expr; if it computes to 0, print a warning message on standard error and exit */

// If a default assert is already defined, undefine it first
#ifdef assert
#undef assert
#endif

// Enable assert() only if NDEBUG is not set
#ifndef NDEBUG
#define assert(expr) if(!expr) { fprintf(stderr, "[%s:%s:%d][ERROR] Assertion failure: %s\n", __FILE__, __FUNCTION__, __LINE__, #expr); abort(); }
#else
// Otherwise define it as nothing
#define assert(expr)
#endif

// Debug practice: function that can check anytime is a stack is in a legal state using assert() internally
void stack_check(stack_t *stack);

#endif /* STACK_H */
