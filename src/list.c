/* list.c
 * A simple double-linked-list data-structure and manipulation functions
 *
 * Copyright (C) 2012, Joe Bew <joebew42@gmail.com>,
 *                     Vincenzo Di Cicco <enzodicicco@gmail.com>
 *
 * This file is part of cwatch
 *
 * cwatch is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * cwatch is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>

#include "list.h"

Queue *queue_init()
{
    Queue *list = malloc(sizeof(Queue));
    if (list == NULL)
        return NULL;

    list->first = list->last = NULL;

    return list;
}

QueueNode *queue_enqueue(Queue *list, void *data)
{
    QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
    if (node == NULL)
        return NULL;

    node->data = data;
    node->next = node->prev = NULL;

    if (list->first == NULL)
    {
        list->first = list->last = node;
    }
    else
    {
        node->prev = list->last;
        list->last->next = node;
        list->last = node;
    }

    return node;
}

void *queue_dequeue(Queue *list)
{
    if (list->first == NULL)
        return NULL;

    void *data = list->first->data;

    QueueNode *node = list->first;

    if (list->first == list->last)
    {
        list->first = list->last = NULL;
    }
    else
    {
        list->first = node->next;
        list->first->prev = NULL;
    }

    free(node);

    return data;
}

void queue_remove(Queue *list, QueueNode *node)
{
    if (list->first == NULL)
        return;

    if (node->prev == NULL)
        list->first = node->next;
    else
        node->prev->next = node->next;

    if (node->next == NULL)
        list->last = node->prev;
    else
        node->next->prev = node->prev;

    free(node);
}

int queue_size(Queue *list)
{
    if (list == NULL || list->first == NULL)
        return 0;

    int size = 0;
    QueueNode *node = list->first;
    while (node != NULL)
    {
        node = node->next;
        ++size;
    }

    return size;
}

void queue_free(Queue *list)
{
    if (list == NULL)
        return;

    QueueNode *node = list->first;

    while (node)
    {
        QueueNode *next = node->next;
        free(node);
        node = next;
    }

    free(list);
}
