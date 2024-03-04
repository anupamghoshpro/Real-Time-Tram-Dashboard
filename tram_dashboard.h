#define TRUE    1
#define FALSE   0

#define MAX_ARG_COUNT       10
#define DELIMITER           '='
#define DELIMITER_BYTE_LEN  1
#define NULL_CHAR_BYTE		1
#define MSG_LEN_BYTE		1
#define NULL_CHAR			0x00
#define MAX_READ_MSG_LEN	256
#define MAX_BUFFER_SIZE		(MAX_READ_MSG_LEN + NULL_CHAR_BYTE)


#define MAX_TRAM_ID_LEN				10
#define MAX_LOCATION_LEN			50
#define MAX_PASSENGER_COUNT_LEN		4  /* Max supported passenger count 9999 */
#define MAX_TRAM_ROUTES             50
#define MAX_TRAM_NUMBERS            5000
#define MAX_DUMP_FILE_PATH_LEN      100
#define MIN_PACKET_SEGMENTS			6 /* MSGTYPE, LOCATION/PASSENGER_COUNT, TRAM_ID, <Tram ID>, VALUE and <Value> */
#define MAX_MSG_FRAME_LEN           ((MSG_LEN_BYTE * MIN_PACKET_SEGMENTS) + strlen("MSGTYPE") + strlen("PASSENGER_COUNT") + \
                                    strlen("TRAM_ID") + MAX_TRAM_ID_LEN + sizeof("VALUE") + MAX_LOCATION_LEN)

#define MSGTYPE_SEGMENT_LEN             (MSG_LEN_BYTE + strlen("MSGTYPE"))
#define LOCATION_SEGMENT_LEN            (MSG_LEN_BYTE + strlen("LOCATION"))
#define PASSENGER_COUNT_SEGMENT_LEN     (MSG_LEN_BYTE + strlen("PASSENGER_COUNT"))
#define TRAM_ID_SEGMENT_LEN             (MSG_LEN_BYTE + strlen("TRAM_ID"))
#define VALUE_SEGMENT_LEN               (MSG_LEN_BYTE + strlen("VALUE"))

#define OFFSET_MSGTYPE					(MSG_LEN_BYTE + sizeof("MSGTYPE"))
#define OFFSET_LOCATION_TRAM_ID			(OFFSET_MSGTYPE + sizeof("LOCATION"))
#define OFFSET_PASSENGER_COUNT_TRAM_ID	(OFFSET_MSGTYPE + sizeof("PASSENGER_COUNT"))
#define OFFSET_LOCATION(X)				(OFFSET_LOCATION_TRAM_ID + sizeof("TRAM_ID") + X + sizeof("VALUE"))
#define OFFSET_PASSENGER_COUNT(X)		(OFFSET_PASSENGER_COUNT_TRAM_ID + sizeof("TRAM_ID") + X + sizeof("VALUE"))

typedef unsigned char bool;

typedef struct
{
    char location[MAX_LOCATION_LEN];
    int passenger_count;
}tram_id_value;

typedef struct
{
    char tram_id[MAX_TRAM_ID_LEN];
    tram_id_value value;
}tram_info;

typedef struct LinkedList
{
    tram_info *node;
    struct LinkedList *next;
}linked_list;

typedef struct
{
    tram_info **tram_id_entry;
    linked_list **separate_chaining;
    int size;
    int entry_count;
}trams_database;