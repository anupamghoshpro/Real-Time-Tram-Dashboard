#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tram_dashboard.h"

static int insert_node(linked_list **head, tram_info *node)
{
    linked_list *new_entry = (linked_list*) malloc(sizeof(linked_list));
    if(NULL == new_entry)
        return -1;
    new_entry->node = node;
    new_entry->next = (*head);
    (*head) = new_entry;
  
    return 0;
}

static void delete_linked_list(linked_list *head)
{
    if(NULL == head)
        return;
    
    linked_list *trav = head;
    while(NULL != head)
    {
        trav = head;
        head = head->next;
        free(trav->node);
        free(trav);
    }
}

int resolve_trams_entry_collision(trams_database *db, unsigned int index, tram_info *node)
{
    linked_list *head = db->separate_chaining[index];

    if(NULL == head)
    {
        head = (linked_list*) malloc(sizeof(linked_list));;
        if(NULL == head)
            return -1;
        
        head->node = node;
        head->next = NULL;
    }
    else
    {
        linked_list *current = db->separate_chaining[index];
        while(NULL != current)
        {
             if (0 == strcmp(current->node->tram_id, node->tram_id)) 
             {
                free(current->node);
                current->node = node;
                return 0;
             }
             current = current->next;
        }
        
        if(0 != insert_node(&head, node))
            return -1;             
    }

    db->separate_chaining[index] = head;
    return 0;
}

linked_list** create_separate_chaining(trams_database *db)
{
    linked_list **nodes = (linked_list**)malloc(db->size * sizeof(linked_list*));
    if(NULL == nodes)
        return NULL;

    for (int index = 0; index < db->size; index++)
        nodes[index] = NULL;

    return nodes;
}

void delete_separate_chaining(trams_database *db)
{
    linked_list **nodes = db->separate_chaining;

    for(int index = 0; index < db->size; index++)
        delete_linked_list(nodes[index]);    
    free(nodes);
}