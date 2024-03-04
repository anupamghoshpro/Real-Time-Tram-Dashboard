#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tram_dashboard.h"

extern int resolve_trams_entry_collision(trams_database *db, unsigned int index, tram_info *node);
extern linked_list** create_separate_chaining(trams_database *db);
extern void delete_separate_chaining(trams_database *db);

static void remove_unwanted_space(char **token, int token_size)
{
    if((NULL == token) || (token_size <= 0))
        return;

    char *ptr = *token;
    int it1 = 0, it2 = token_size;
    bool found_front_alphanum = FALSE, found_rear_alphanum = FALSE;

    while((it1 < it2) && !(found_front_alphanum && found_rear_alphanum))
    {
        if(!found_front_alphanum)
        {
            if(0 == isalnum(ptr[it1]))
                it1++;
            else
                found_front_alphanum = TRUE;
        }

        if(!found_rear_alphanum)
        {
            if(0 == isalnum(ptr[it2]))
                it2--;
            else
                found_rear_alphanum = TRUE;
        }
    }

    ptr[it2 + 1] = NULL_CHAR;
    *token = ptr + it1;
}

static unsigned int BKDRHash(const char* str, unsigned int length)
{
   unsigned int seed = 131; /* 31 131 1313 13131 131313 etc.. */
   unsigned int hash = 0;
   unsigned int i    = 0;

   for (i = 0; i < length; ++str, ++i)
   {
      hash = (hash * seed) + (*str);
   }

   return (hash % MAX_TRAM_NUMBERS);
}

static tram_info* create_node(char *tram_id)
{
    if(NULL == tram_id)
        return NULL;

    tram_info *node = (tram_info*)malloc(sizeof(tram_info));
    if(NULL != node)
        strcpy(node->tram_id, tram_id);
    
    return node;
}

static int create_tram_id_entry(trams_database *db, char *tram_id)
{
    if((NULL == db) || (NULL == tram_id))
        return -1;
    
    int ret = 0;
    tram_info *node = create_node(tram_id);
    if(NULL == node)
        return -1;

    unsigned int index = BKDRHash(tram_id, strlen(tram_id));
    if(NULL == db->tram_id_entry[index])
    {
        /* Database is full. */
        if (db->entry_count == db->size) 
        {
            free(node);
            return -1;
        }
        db->tram_id_entry[index] = node;
        bzero(db->tram_id_entry[index]->value.location, sizeof(db->tram_id_entry[index]->value.location));
        db->tram_id_entry[index]->value.passenger_count = -1;
        db->entry_count++;
    }
    else
    {
        printf("\nCollision!!!!!\n");
        ret = resolve_trams_entry_collision(db, index, node);
    }

    return ret;
}

trams_database* create_trams_database(int db_size)
{
    if(0 == db_size)
        NULL;

    trams_database *db = (trams_database*)malloc(sizeof(trams_database));
    if(NULL == db)
    {
        perror("Memory error in creating tram database.");
        return NULL;
    }

    db->size = db_size;
    db->entry_count = 0;
    db->tram_id_entry = (tram_info**)malloc(db_size * sizeof(tram_info));    
    if(NULL == db->tram_id_entry)
    {
        perror("Memory error in creating tram id entry.");
         return NULL;
    }       

    for (int index = 0; index < db->size; index++)
        db->tram_id_entry[index] = NULL;

    db->separate_chaining = create_separate_chaining(db);    
    if(NULL == db->separate_chaining)
    {
        perror("Memory error in Creating separate chaining.");
        return NULL;
    }

    return db;
}

void delete_trams_database(trams_database *db) 
{
    // Frees the table
    for (int index = 0; index < db->size; index++) 
    {
        tram_info* node = db->tram_id_entry[index];
        if (node != NULL)
            free(node);
    }

    free(db->tram_id_entry);
    delete_separate_chaining(db);
    free(db);
}

int retrieve_tram_ids(char *file_name, trams_database *db)
{
    if(NULL == file_name)
    {
        perror("Invalid file path/name.");
        return -1;
    }

    FILE *fp = fopen(file_name, "r");
    if (NULL == fp)
    {
        perror("Can't open file.");
        return -1;
    }

    fseek (fp , 0 , SEEK_END);
    int file_size = ftell (fp);
    rewind (fp);

    char *buffer = (char*) malloc (sizeof(char)*file_size);
    if (buffer == NULL) {
        perror("Memory error."); 
        return -1;
    }

    bzero(buffer, sizeof(buffer));
    if (fread(buffer, 1, file_size, fp) != file_size) 
    {
        perror("File read error"); 
        return -1;
    }

    char* tram_id = strtok(buffer, ",");
    while(NULL != tram_id)
    {
        remove_unwanted_space(&tram_id, strlen(tram_id));
        if(0 != create_tram_id_entry(db, tram_id))
        {
            perror("Tram database entry failed."); 
            return -1;
        }
        tram_id = strtok(NULL, ",");
    }

    fclose (fp);
    free (buffer);
    return 0;
}

char is_valid_tram_id(trams_database *db, char *tram_id)
{
    if((NULL == db) || (NULL == tram_id))
        return FALSE;
    if(strlen(tram_id) < strlen("Tram"))
        return FALSE;
    tram_id += strlen("Tram") + 1;
    int index = BKDRHash(tram_id, strlen(tram_id));
    tram_info *tie = db->tram_id_entry[index];
    linked_list *sc = db->separate_chaining[index];

    while(NULL != tie)
    {
        if(0 == strcmp(tie->tram_id, tram_id))
            return TRUE;
        if(NULL == sc)
            return FALSE;
        tie = sc->node;
        sc = sc->next;
    } 

    return FALSE;
}


int insert_tram_location(trams_database *db, char *tram_id, char *location)
{
    if((NULL == db) || (NULL == tram_id) || (NULL == location))
        return -1;

    if(strlen(tram_id) < strlen("Tram"))
        return -1;
    tram_id += strlen("Tram") + 1;
    
    unsigned int index = BKDRHash(tram_id, strlen(tram_id));
    if(NULL == db->tram_id_entry[index])
    {
        printf("\nTram ID for location data doesn't exist\n");
        return -1;
    }
    strcpy(db->tram_id_entry[index]->value.location, location);
   //printf("\nlocation: %s\n", db->tram_id_entry[index]->value.location);

    return 0;
}

int insert_tram_passenger_count(trams_database *db, char *tram_id, int passenger_count)
{
    if((NULL == db) || (NULL == tram_id) || (passenger_count < 0))
        return -1;

    if(strlen(tram_id) < strlen("Tram"))
        return -1;
    tram_id += strlen("Tram") + 1;
    
    unsigned int index = BKDRHash(tram_id, strlen(tram_id));
    if(NULL == db->tram_id_entry[index])
    {
        printf("\nTram ID for passenger count doesn't exist\n");
        return -1;
    }
    db->tram_id_entry[index]->value.passenger_count = passenger_count;
    //printf("\npassenger_count: %d\n", db->tram_id_entry[index]->value.passenger_count);

    return 0;
}

void display_tram_info(trams_database *db, char *tram_id)
{
	if(NULL == tram_id)
		perror("Invalid Tram ID input for display");

	if(strlen(tram_id) < strlen("Tram"))
       perror("Incomplete Tram Id value.");

    tram_id += strlen("Tram") + 1;

	unsigned int index = BKDRHash(tram_id, strlen(tram_id));
    if(NULL == db->tram_id_entry[index])
        printf("\nTram ID for display doesn't exist.\n");
        

	if((0 != strlen(db->tram_id_entry[index]->value.location)) && (db->tram_id_entry[index]->value.passenger_count >= 0))
	{
		printf("\nTram %s:", tram_id);
		printf("\n	Location: %-5s", db->tram_id_entry[index]->value.location);
		printf("\n	Passenger Count: %-5d", db->tram_id_entry[index]->value.passenger_count);
        printf("\n\n");
	}
}

int delete_tram_info(tram_info *node)
{
    free(node);
}

