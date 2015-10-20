#include "ucos_ii.h"
#include "bsp_init.h"
#include <stdlib.h>
#include <stdint.h>


#define TASK_STK_SIZE 8192

/*DEFINES*/
/* ************************************************
 *                  TASK IDS
 **************************************************/

#define          UBYTE                     unsigned char
#define          TASK_RECEIVE_ID           6
#define 		 TASK_VERIFICATION_ID      7
#define 		 TASK_STOP_ID              10
#define 		 TASK_RESTART_ID           11
#define          TASK_COMPUTING_ID         8
#define          TASK_FORWARDING_ID        9
#define          TASK_PRINT1_ID            3
#define          TASK_PRINT2_ID            4
#define          TASK_PRINT3_ID            5

/* ************************************************
 *                TASK PRIOS
 **************************************************/

#define          TASK_RECEIVE_PRIO         35
#define 		 TASK_VERIFICATION_PRIO    37
#define 		 TASK_STOP_PRIO            34
#define			 TASK_STATS_PRIO		   36
#define          TASK_COMPUTING_PRIO       38
#define          TASK_FORWARDING_PRIO      41	// making the forwarding task prior to print misc. because vidoe and 
#define          TASK_PRINT1_PRIO          42	// audio packets must not be block in forwarding because of print misc.
#define          TASK_PRINT2_PRIO          40
#define          TASK_PRINT3_PRIO          39



// Routing info.
#define INT1_LOW      0x00000000
#define INT1_HIGH     0x3FFFFFFF
#define INT2_LOW      0x40000000
#define INT2_HIGH     0x7FFFFFFF
#define INT3_LOW      0x80000000
#define INT3_HIGH     0xBFFFFFFF
#define INT4_LOW	  0xC0000000
#define INT4_HIGH     0xFFFFFFFF

// Reject source info.
#define REJECT_LOW1   0x10000000
#define REJECT_HIGH1  0x100FFFFF
#define REJECT_LOW2   0x50000000
#define REJECT_HIGH2  0x500FFFFF
#define REJECT_LOW3   0x60000000
#define REJECT_HIGH3  0x600FFFFF
#define REJECT_LOW4   0xD0000000
#define REJECT_HIGH4  0xD00FFFFF

// Packet types
#define VIDEO_PACKET_TYPE 0
#define AUDIO_PACKET_TYPE 1
#define MISC_PACKET_TYPE 2


typedef struct {
    unsigned int src;
    unsigned int dst;
    unsigned int type;
    unsigned int crc;
    unsigned int data[12];
} Packet;

typedef struct {
    unsigned int interfaceID;
    OS_EVENT *Mbox;
} PRINT_PARAM;

PRINT_PARAM print_param1, print_param2, print_param3;

/*DECLARATION DES EVENTS*/
/* ************************************************
 *                  Mailbox
 **************************************************/

OS_EVENT *Mbox1;
OS_EVENT *Mbox2;
OS_EVENT *Mbox3;

/* ************************************************
 *                  Queues
 **************************************************/

OS_EVENT *inputQ;
void* inputMsg[16];
OS_EVENT *verifQ;
void* verifMsg[10];
OS_EVENT *lowQ;
void* lowMsg[4];
OS_EVENT *mediumQ;
void* mediumMsg[4];
OS_EVENT *highQ;
void* highMsg[4];


/* ************************************************
 *                  Semaphores
 **************************************************/

/* À compléter */

// Event sych.
OS_EVENT* sem_packet_ready;
OS_EVENT* sem_packet_computed;
OS_EVENT* sem_verif_signal;
OS_EVENT* sem_crc_count_check_task_enable;

// Shared var. protect.
OS_EVENT* sem_nbPacket;
OS_EVENT* sem_nbPacketLowRejete;
OS_EVENT* sem_nbPacketMediumRejete;
OS_EVENT* sem_nbPacketHighRejete;
OS_EVENT* sem_nbPacketCRCRejete;
OS_EVENT* sem_nbPacketSourceRejete;
OS_EVENT* sem_nbPacketSent;

/*DECLARATION DES TACHES*/
/* ************************************************
 *                  STACKS
 **************************************************/

OS_STK           TaskReceiveStk[TASK_STK_SIZE];
OS_STK 	         TaskVerificationStk[TASK_STK_SIZE];
OS_STK 	         TaskStopStk[TASK_STK_SIZE];
OS_STK 	         TaskStatsStk[TASK_STK_SIZE];
OS_STK           TaskComputeStk[TASK_STK_SIZE];
OS_STK           TaskForwardingStk[TASK_STK_SIZE];
OS_STK           TaskPrint1Stk[TASK_STK_SIZE];
OS_STK           TaskPrint2Stk[TASK_STK_SIZE];
OS_STK           TaskPrint3Stk[TASK_STK_SIZE];

/*DECLARATION DES COMPTEURS*/
int nbPacket = 0; // Nb de packets total traites
int nbPacketLowRejete = 0; // Nb de packets low rejetes
int nbPacketMediumRejete = 0; // Nb de packets Medium rejetes
int nbPacketHighRejete = 0; // Nb de packets High rejetes
int nbPacketCRCRejete = 0; // Nb de packets rejetes pour mauvais crc
int nbPacketSourceRejete = 0; // Nb de packets rejetes pour mauvaise source
int nbPacketSent = 0;


/* ************************************************
 *              TASK PROTOTYPES
 **************************************************/

void TaskReceivePacket(void *data); // Function prototypes of tasks
void TaskVerification(void *data);
void TaskRestart(void *data);
void TaskStop(void *data);
void TaskComputing(void *data);
void TaskForwarding(void *data);
void TaskPrint(void *data);
void TaskStats(void *pdata);

/* ************************************************
*              Utility functions
**************************************************/

void create_application();
int create_tasks();
int create_events();
unsigned char post_to_verif(Packet* p, INT8U status);
void forward(Packet* p);
void err_msg(char* ,INT8U);
Packet* packetDeepCopy(Packet* p);
void incRejectedPacketType(Packet* p);