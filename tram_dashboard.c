#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include "tram_dashboard.h"

extern trams_database* create_trams_database(int db_size);
extern void delete_trams_database(trams_database *db);
extern int retrieve_tram_ids(char *file_name, trams_database *db);
extern char is_valid_tram_id(trams_database *db, char *tram_id);
extern int insert_tram_location(trams_database *db, char *tram_id, char *location);
extern int insert_tram_passenger_count(trams_database *db, char *tram_id, int passenger_count);
extern void display_tram_info(trams_database *db, char *tram_id);

trams_database db[MAX_TRAM_NUMBERS];

void error(char* msg) {
    perror(msg);
    exit(1);
}

/*
Replaces the message length byte with a '\0' character to convert the
byte stream into individual strings and also ensures the packet contains 
all the required segments.

Returns 0 upon success, -1 otherwise.
*/
static int convert_byte_streams_to_string_streams(char *msg, int msgLength)
{
	int index = 0;
	int byteCount = 0;
	int packet_segment_count = 0;
	while(byteCount < msgLength)
	{		
		byteCount += MSG_LEN_BYTE + msg[index];
		msg[index] = NULL_CHAR;
		index = byteCount;
		packet_segment_count++;
	}
	
	if((byteCount > msgLength) || 
	   (packet_segment_count < MIN_PACKET_SEGMENTS))
		return -1;
	else	
		return 0;
}

/*
Ensures the individual segments contain the desired keywords and
extract the tram ID in the process.

Returns 0 upon success, -1 otherwise.
*/
static int error_check_and_extract_tram_id(char *msg, char *tram_id)
{	
	if((NULL == msg) || (NULL == tram_id))
		return -1;
	
	if(0 != strcmp(msg, "MSGTYPE"))
		return -1;

	msg += strlen(msg) + NULL_CHAR_BYTE;
	if(!((0 == strcmp(msg, "LOCATION")) || (0 == strcmp(msg, "PASSENGER_COUNT"))))
		return -1;
	
	msg += strlen(msg) + NULL_CHAR_BYTE;
	if(0 != strcmp(msg, "TRAM_ID"))
		return -1;

	msg += strlen(msg) + NULL_CHAR_BYTE;	
	size_t tram_id_len = strlen(msg);
	if((0 == tram_id_len) || (tram_id_len > MAX_TRAM_ID_LEN))
		return -1;
	else
		strncpy(tram_id, msg, tram_id_len);
	
	msg += strlen(msg) + NULL_CHAR_BYTE;
	if(0 != strcmp(msg, "VALUE"))
		return -1;

	msg += strlen(msg) + NULL_CHAR_BYTE;
	if(0 == strlen(msg))
		return -1;

	return 0;
}


/*
Extract the location or passenger count info.

Returns 0 upon success, -1 otherwise.
*/
static int extract_value_and_store(char *msg, char *tram_id, trams_database *db)
{
	if((NULL == msg) || (NULL == tram_id))
		return -1;
	
	if(0 == strcmp(msg + OFFSET_MSGTYPE, "LOCATION"))
	{
		char location[MAX_LOCATION_LEN] = {0};
		int location_value_offset = OFFSET_LOCATION(strlen(tram_id) + NULL_CHAR_BYTE);
		int location_len = strlen(msg + location_value_offset);

		if(location_len > MAX_LOCATION_LEN)
		{
			perror("Location length exceeds maximum allowed length");
			return -1;
		}
		strcpy(location, msg + location_value_offset);
		if(0 != insert_tram_location(db, tram_id, location))
		{
			perror("Inserting location value failed.");
			return -1;
		}
	}
	else
	{
		int passenger_count = 0;
		int passenger_count_value_offset = 
		                     OFFSET_PASSENGER_COUNT(strlen(tram_id) + NULL_CHAR_BYTE);
		int passenger_count_len = strlen(msg + passenger_count_value_offset);

		if(passenger_count_len > MAX_PASSENGER_COUNT_LEN)
		{
			perror("Passenger count exceeds maximum allowed value");
			return -1;
		}
		passenger_count = atoi(msg + passenger_count_value_offset);
		if(0 != insert_tram_passenger_count(db, tram_id, passenger_count))
		{
			perror("Inserting passenger count value failed.");
			return -1;
		}
	}

	return 0;
}


void parse_msg(char* msg, int msgLength, trams_database *db)
{
	char tram_id[MAX_TRAM_ID_LEN + NULL_CHAR_BYTE] = {0};
	char location[MAX_LOCATION_LEN + NULL_CHAR_BYTE] = {0};
	int passenger_count = 0;
	
	if((NULL == msg) || (msgLength <= 0) || (msgLength > MAX_READ_MSG_LEN))
	{
		perror("Invalid input!!!");
		return;
	}
	
	if(0 != convert_byte_streams_to_string_streams(msg, msgLength))
	{
		perror("Corrupted/Incomplete packet");
		return;
	}
	
	if(0 != error_check_and_extract_tram_id(msg + MSG_LEN_BYTE, tram_id))
	{
		perror("Doesn't match expected data");
		return;
	}

	if(TRUE != is_valid_tram_id(db, tram_id))
	{
		perror("Invalid Tram ID");
		return;
	}

	if(0 != extract_value_and_store(msg, tram_id, db))
		return;

	display_tram_info(db, tram_id);
}

/*
void fetch_one_complete_msg_frame(char **msg, int *msg_length, char *msg_frame, bool *packet_parsing_complete)
{
	if((NULL == msg) || (NULL == *msg) || (NULL == msg_length) || (msg_length <= 0) || (NULL == msg_frame) || (NULL == packet_parsing_complete))
	{
		perror("Invalid input to parse.");
		*packet_parsing_complete = TRUE;
		return;
	}

	char *ptr_msg = *msg;

	int packet_segment_count = MIN_PACKET_SEGMENTS, index = 0;
	char *token = strtok(ptr_msg + MSG_LEN_BYTE, ",");

	if((strlen("MSGTYPE") == *token) && ("MSGTYPE" == *(token + MSG_LEN_BYTE)))

}
*/

static int write_socket_data_into_dump_file(char *dump_file_path, char *buffer, int bytesRead)
{
	if((NULL == dump_file_path) || (NULL == buffer) || (bytesRead <= 0))
	{
		printf("\nInvalid input to write into dump file");
		return -1;
	}

	FILE *fp = fopen(dump_file_path, "at");
    if (NULL == fp)
    {
        perror("Can't open file.");
        return -1;
    }

	int byte_written = fwrite(buffer, 1, bytesRead, fp);
	//printf("\nbyte_written in dump file: %d", byte_written);
	fwrite("\n\n", 2, sizeof("\n\n"), fp);

	fclose(fp);

	return 0;
}

int main(int argc, char *argv[]){
	if(argc < 3){
        fprintf(stderr,"No port provided\n");
        exit(1);
	}

	char *token = NULL;
	char *argv_portno = NULL;
	char *tram_db_path = NULL;
	char *dump_file_path = NULL;
	int index = 1;
	
	while(argc > 1)
	{
		token = strtok(argv[index], "=");
		if(0 == strcmp("--tram-db", token))
			tram_db_path = argv[index] + strlen(token) + DELIMITER_BYTE_LEN;
		else if(0 == strcmp("--portno", token))
			argv_portno = argv[index] + strlen(token) + DELIMITER_BYTE_LEN;
		else if(0 == strcmp("--dump-file", token))
			dump_file_path = argv[index] + strlen(token) + DELIMITER_BYTE_LEN;

		index++;
		argc--;
	}

	printf("\ntram_db_path: %s", tram_db_path);
	printf("\nargv_portno: %s", argv_portno);
	printf("\ndump_file_path: %s", dump_file_path);

	if((NULL == tram_db_path) || (NULL == argv_portno))
	{
		fprintf(stderr,"Tram database file path/server portno not provided.");
        exit(1);
	}

	trams_database *db = create_trams_database(MAX_TRAM_NUMBERS);
	if(NULL == db)
	{
		error("Trams database couldn't be created.\n");
	}

	if(0 != retrieve_tram_ids(tram_db_path, db))
	{
		error("Couldn't retrieve tram routes info.\n");
	}

	int sockfd, portno, bytesRead;	
	struct sockaddr_in serv_addr;
	struct hostent* server;
	
	portno = atoi(argv_portno);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0){
		error("Socket failed \n");
	}
	
	server = gethostbyname("127.0.0.1");
	if(server == NULL){
		error("No such host\n");
	}
	
	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	
	if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr))<0)
		error("Connection failed\n");

	char buffer[MAX_BUFFER_SIZE];
	char msg_frame[MAX_MSG_FRAME_LEN];
	char *ptr_buffer = buffer;
	bool packet_parsing_complete = FALSE;
	while(1){
		bzero(buffer, MAX_BUFFER_SIZE);
		bzero(msg_frame, MAX_MSG_FRAME_LEN);
		bytesRead = read(sockfd, buffer, MAX_READ_MSG_LEN);
		if(bytesRead <0)
			error("Error reading from Server");
		//while(!packet_parsing_complete)
		//{
			//fetch_one_complete_msg_frame(&ptr_buffer, &bytesRead, ptr_msg_frame, &packet_parsing_complete);
			//Write file function to write the raw data received from socket.
			if(NULL != dump_file_path)
				write_socket_data_into_dump_file(dump_file_path, buffer, bytesRead);
			parse_msg(buffer, bytesRead, db);
		//}
	}

	delete_trams_database(db);
	return 0;
}