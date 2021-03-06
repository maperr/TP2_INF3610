#include "cpu1_uc.h"


// State of the app
bool isAppRunning = false;	// the boolean indicate if the application is running for the external resources (interrupt handlers)
							// true = all tasks are active and the resources are available
							// false = the states of the tasks and resources are undefined and might not be initialized

///////////////////////////////////////////////////////////////////////////////////////
//								Routines d'interruptions
///////////////////////////////////////////////////////////////////////////////////////

void irq_gen_0_isr(void* data) {
	/* � compl�ter */
	INT8U err;

	if(isAppRunning)
	{
		// Enable ReceivePacket task
		err = OSSemPost(sem_packet_ready);
		err_msg("irq_gen_0_isr - OSSemPost(sem_packet_ready)", err);
	}

	Xil_Out32(m_irq_gen_0.Config.BaseAddress, 0);
	XIntc_Acknowledge(&m_axi_intc, RECEIVE_PACKET_IRQ_ID);
}

void irq_gen_1_isr(void* data) {
	/* � compl�ter */
	INT8U err;

	if(isAppRunning)
	{
		// Enable ReceivePacket task
		err = OSSemPost(sem_enable_stats);
		err_msg("irq_gen_1_isr - OSSemPost(sem_enable_stats)", err);
	}


	Xil_Out32(m_irq_gen_1.Config.BaseAddress, 0);
	XIntc_Acknowledge(&m_axi_intc, PRINT_STATS_IRQ_ID);
}
void timer_isr(void* not_valid) {
	if (private_timer_irq_triggered()) {
		private_timer_clear_irq();
		OSTimeTick();
	}                           
}

void fit_timer_1s_isr(void *not_valid) {
	/* � compl�ter */
	INT8U err;

	if(isAppRunning)
	{
		// Enable ReceivePacket task
		err = OSSemPost(sem_crc_count_check_task_enable);
		err_msg("fit_timer_5s_isr - OSSemPost(sem_crc_count_check)", err);
	}
}

void fit_timer_5s_isr(void *not_valid) {
	/* � compl�ter */
	INT8U err;

	if(isAppRunning)
	{
		// Enable ReceivePacket task
		err = OSSemPost(sem_verif_signal);
		err_msg("fit_timer_5s_isr - OSSemPost(sem_verif_signal)", err);
	}
}

///////////////////////////////////////////////////////////////////////////////////////
//								uC/OS-II part
///////////////////////////////////////////////////////////////////////////////////////
int main() {
	initialize_bsp();

	// Initialize uC/OS-II
	OSInit();

	create_application();

	prepare_and_enable_irq();

	xil_printf("*** Starting uC/OS-II scheduler ***\n");

	OSStart();


	cleanup();
	
    return 0;
}

void create_application() {
	int error;

	error = create_tasks();
	if (error != 0)
		xil_printf("Error %d while creating tasks \n", error);

	error = create_events();
	if (error != 0)
		xil_printf("Error %d while creating events \n", error);

	isAppRunning = true;
}

int create_tasks() {
	/* � compl�ter */
	UBYTE err;

	err = OSTaskCreate(TaskReceivePacket, NULL, &TaskReceiveStk[TASK_STK_SIZE-1], TASK_RECEIVE_PRIO);
	err_msg("create_tasks - OSTaskCreate(TaskReceivePacket)", err);

	err = OSTaskCreate(TaskComputing, NULL, &TaskComputeStk[TASK_STK_SIZE - 1], TASK_COMPUTING_PRIO);
	err_msg("create_tasks - OSTaskCreate(TaskComputing)", err);

	err = OSTaskCreate(TaskForwarding, NULL, &TaskForwardingStk[TASK_STK_SIZE - 1], TASK_FORWARDING_PRIO);
	err_msg("create_tasks - OSTaskCreate(TaskComputing)", err);

	err = OSTaskCreate(TaskPrint, &print_param1, &TaskPrint1Stk[TASK_STK_SIZE - 1], TASK_PRINT1_PRIO);
	err_msg("create_tasks - OSTaskCreate(TaskPrint1)", err);

	err = OSTaskCreate(TaskPrint, &print_param2, &TaskPrint2Stk[TASK_STK_SIZE - 1], TASK_PRINT2_PRIO);
	err_msg("create_tasks - OSTaskCreate(TaskPrint2)", err);

	err = OSTaskCreate(TaskPrint, &print_param3, &TaskPrint3Stk[TASK_STK_SIZE - 1], TASK_PRINT3_PRIO);
	err_msg("create_tasks - OSTaskCreate(TaskPrint3)", err);

	err = OSTaskCreate(TaskStop, NULL, &TaskStopStk[TASK_STK_SIZE - 1], TASK_STOP_PRIO);
	err_msg("create_tasks - OSTaskCreate(TaskStop)", err);

	err = OSTaskCreate(TaskVerification, NULL, &TaskVerificationStk[TASK_STK_SIZE - 1], TASK_VERIFICATION_PRIO);
	err_msg("create_tasks - OSTaskCreate(TaskVerification)", err);

	err = OSTaskCreate(TaskStats, NULL, &TaskStatsStk[TASK_STK_SIZE - 1], TASK_STATS_PRIO);
	err_msg("create_tasks - OSTaskCreate(TaskStats)", err);

    return 0;
}

int create_events() {
	INT8U err;

	/*CREATION DES FILES*/
	inputQ = OSQCreate(&inputMsg[0], 16);
	lowQ = OSQCreate(&lowMsg[0], 4);
	mediumQ = OSQCreate(&mediumMsg[0], 4);
	highQ = OSQCreate(&highMsg[0], 4);
	verifQ = OSQCreate(&verifMsg[0], 10);

	/*CREATION DES MAILBOXES*/
	Mbox1 = OSMboxCreate(NULL);
	Mbox2 = OSMboxCreate(NULL);
	Mbox3 = OSMboxCreate(NULL);

	/*ALLOCATION ET DEFINITIONS DES STRUCTURES PRINT_PARAM*/
	print_param1.Mbox = Mbox1;
	print_param1.interfaceID = 1;
	print_param2.Mbox = Mbox2;
	print_param2.interfaceID = 2;
	print_param3.Mbox = Mbox3;
	print_param3.interfaceID = 3;

	/*CREATION DES SEMAPHORES*/
	sem_packet_ready = OSSemCreate(0);
	sem_packet_computed = OSSemCreate(0);
	sem_verif_signal = OSSemCreate(0);
	sem_crc_count_check_task_enable = OSSemCreate(0);
	sem_enable_stats = OSSemCreate(0);

	/* CREATION DES MUTEX*/
	mtx_nbPacket = OSMutexCreate(MUTEX_NBPACKET_PRIO, &err);
	mtx_computingValues = OSMutexCreate(MUTEX_COMPUTING_PRIO, &err);

	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////
//								uC/OS-II part
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
//									TASKS
///////////////////////////////////////////////////////////////////////////////////////


/*
 *********************************************************************************************************
 *                                              computeCRC
   -Calcule la check value d'un paquet en utilisant un CRC (cyclic redudancy check)
 *********************************************************************************************************
 */
unsigned int computeCRC(INT16U* w, int nleft) {
    unsigned int sum = 0;
    INT16U answer = 0;

    // Adding words of 16 bits
    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    // Handling the last byte
    if (nleft == 1) {
        *(unsigned char *) (&answer) = *(const unsigned char *) w;
        sum += answer;
    }

    // Handling overflow
    sum = (sum & 0xffff) + (sum >> 16);
    sum += (sum >> 16);

    answer = ~sum;
    return (unsigned int) answer;
}

/*
 *********************************************************************************************************
 *                                              TaskreceivePacket
 *  -Injecte des paquets dans la InputQ lorsqu'il en recoit de linux
 *********************************************************************************************************
 */
void TaskReceivePacket(void *data) {
    int k;
    INT8U err;

	// Initialize a packet that will be fill
    Packet* packet;

    for (;;) {
    	packet = (Packet*) malloc(sizeof(Packet));

    	// Wait for linux's interruption
    	OSSemPend(sem_packet_ready, 0, &err);

    	err_msg("TaskReceivePacket - OSSemPend(sem_packet_ready)", err);

    	/* � compl�ter : R�ception des paquets de Linux */
    	uint32_t *ll;
		ll = (uint32_t *) (packet);

		// Building the complete packet (16 unsigned)
		for(k = 0; k < 16; k++)
		{
			while(COMM_RX_FLAG == 0); //wait for Linux to produce the value
			*ll = COMM_RX_DATA; // read current part of packet
			ll++;	// point to next part of packet
			COMM_RX_FLAG = 0;
		}

		xil_printf("TaskReceivePacket - Packet RECIEVED \n");	// debug output

		/* � compl�ter: Transmission des paquets dans l'inputQueue */
		err = OSQPost(inputQ, packet);
		err_msg("TaskReceivePacket - OSQPost(inputQ)", err);
    }
}


/*
 *********************************************************************************************************
 *                                              TaskVerification
 *  -R�injecte les paquets rejet�s des files haute, medium et basse dans la inputQ
 *********************************************************************************************************
 */
void TaskVerification(void *data) {
	INT8U statusVerifQ;	// max of INT8U (unused value)
	INT8U statusInputQ;	// max of INT8U (unused value)
	INT8U err;
	Packet *packet = NULL;
	while (1) {
		/* � compl�ter */
		statusVerifQ = OS_ERR_NONE;	// max of INT8U (unused value)
		statusInputQ = OS_ERR_NONE;
		// Wait for LINUX's interruption
		OSSemPend(sem_verif_signal, 0, &err);
		err_msg("TaskVerification - OSSemPend(sem_verif_signal)", err);

		while(statusVerifQ == OS_ERR_NONE && statusInputQ == OS_ERR_NONE)
		{
			packet = OSQAccept(verifQ, &statusVerifQ);
			if(packet != NULL)
			{
				statusInputQ = OSQPost(inputQ, packet);
				if(statusInputQ != OS_ERR_NONE)
				{
					err = OSQPost(verifQ, packet);
				}
			}
		}
	}
}
/*
 *********************************************************************************************************
 *                                              TaskStop
 *  -Stoppe le routeur une fois que 5 paquets ont �t�s rejet�s pour mauvais CRC
 *********************************************************************************************************
 */
void TaskStop(void *data) {
	INT8U err;
	while(1) {
		/* � compl�ter */

		// Wait for interruption (from LINUX)
		OSSemPend(sem_crc_count_check_task_enable, 0, &err);
		err_msg("TaskStop - OSSemPend(sem_nbPacketCRCRejete)", err);

		// If more than 15 packets were corrupted kill all
		OSMutexPend(mtx_computingValues, 0, &err);
		err_msg("TaskStop - OSMutexPend(mtx_computingValues)", err);
		if (nbPacketCRCRejete >= 15)
		{
			xil_printf("TaskStop - STOPPING \n");

			isAppRunning = false;

			err = OSTaskDel(TASK_RECEIVE_PRIO);
			err_msg("TaskStop - OSTaskDel(TASK_RECEIVE_PRIO)", err);

			err = OSTaskDel(TASK_VERIFICATION_PRIO);
			err_msg("TaskStop - OSTaskDel(TASK_VERIFICATION_PRIO)", err);

			err = OSTaskDel(TASK_STATS_PRIO);
			err_msg("TaskStop - OSTaskDel(TASK_STATS_PRIO)", err);

			err = OSTaskDel(TASK_COMPUTING_PRIO);
			err_msg("TaskStop - OSTaskDel(TASK_COMPUTING_PRIO)", err);

			err = OSTaskDel(TASK_FORWARDING_PRIO);
			err_msg("TaskStop - OSTaskDel(TASK_FORWARDING_PRIO)", err);

			err = OSTaskDel(TASK_PRINT1_PRIO);
			err_msg("TaskStop - OSTaskDel(TASK_PRINT1_PRIO)", err);

			err = OSTaskDel(TASK_PRINT2_PRIO);
			err_msg("TaskStop - OSTaskDel(TASK_PRINT2_PRIO)", err);

			err = OSTaskDel(TASK_PRINT3_PRIO);
			err_msg("TaskStop - OSTaskDel(TASK_PRINT3_PRIO)", err);


			// Clean all memory
			cleaningEverything();

			// Delete current task...
			err = OSTaskDel(TASK_STOP_PRIO);
			err_msg("TaskStop - OSTaskDel(TASK_STOP_PRIO)", err);
		}

		// Release sem_nbPacketCRCRejete
		err = OSMutexPost(mtx_computingValues);
		err_msg("TaskComputing - OSMutexPost(mtx_computingValues)", err);

	}
}

/*
 *********************************************************************************************************
 *                                              TaskComputing
 *  -V�rifie si les paquets sont conformes (CRC,Adresse Source)
 *  -Dispatche les paquets dans des files (HIGH,MEDIUM,LOxmd
 *
 *********************************************************************************************************
 */
void TaskComputing(void *pdata) {
	INT8U err;
	Packet* packet = NULL;
	while (1) {
		// Unqueue a packet form the queue (wait till a packet is queued)
		packet = OSQPend(inputQ, 0, &err);
		err_msg("TaskComputing - OSQPend(inputQ)", err);

		// Validate checksum and delete corrupted packet
		if (computeCRC((unsigned short*)packet, 64) != 0)
		{
			xil_printf("TaskComputing - PacketChecksumCorrupted \n");

			// Inc nbPacketCRCRejete
			OSMutexPend(mtx_computingValues, 0, &err);
			err_msg("TaskComputing - OSMutexPend(mtx_computingValues)", err);
			nbPacketCRCRejete++;
			err = OSMutexPost(mtx_computingValues);
			err_msg("TaskComputing - OSSemPost(sem_nbPacketCRCRejete)", err);

			// wont inc nbPacketLowRejete, nbPacketMediumRejete and nbPacketHighRejete because the type might be corrupted

			free(packet);
			packet = NULL;
			continue;
		}

		// Validate the source and reject unknown packet
		if (((packet->src >= REJECT_LOW1) && (packet->src <= REJECT_HIGH1)) ||
			((packet->src >= REJECT_LOW2) && (packet->src <= REJECT_HIGH2)) ||
			((packet->src >= REJECT_LOW3) && (packet->src <= REJECT_HIGH3)) ||
			((packet->src >= REJECT_LOW4) && (packet->src <= REJECT_HIGH4))  )
		{
			xil_printf("TaskComputing - PacketSourceRejected - SourceAddress: %x \n", packet->src);

			incRejectedPacketType(packet);

			// Inc nbPacketSourceRejete
			OSMutexPend(mtx_computingValues, 0, &err);
			err_msg("TaskComputing - OSMutexPend(mtx_computingValues)", err);
			nbPacketSourceRejete++;
			err = OSMutexPost(mtx_computingValues);
			err_msg("TaskComputing - OSSemPost(sem_nbPacketCRCRejete)", err);

			// Destroy packet
			free(packet);
			packet = NULL;
			continue;
		}

		// Transfer to the right type queue
		switch (packet->type)
		{
			case(VIDEO_PACKET_TYPE) :
				err = OSQPost(highQ, packet);
				if(post_to_verif(packet, err) == 0)
				{
					err = OSSemPost(sem_packet_computed);
					err_msg("TaskComputing - OSSemPost(sem_packet_computed)", err);
				}
				break;

			case(AUDIO_PACKET_TYPE) :
				err = OSQPost(mediumQ, packet);
				if(post_to_verif(packet, err) == 0)
				{
					err = OSSemPost(sem_packet_computed);
					err_msg("TaskComputing - OSSemPost(sem_packet_computed)", err);
				}
				break;

			case(MISC_PACKET_TYPE) :
				err = OSQPost(lowQ, packet);
				if(post_to_verif(packet, err) == 0)
				{
					err = OSSemPost(sem_packet_computed);
					err_msg("TaskComputing - OSSemPost(sem_packet_computed)", err);
				}
				break;

			default:
				// Type is invalid
				xil_printf("TaskComputing - Packet COMPUTED but invalid type (packet destroyed) - Type: %d \n", packet->type);	// debug output
				free(packet);
				packet = NULL;
				continue;
				break;
		}
	}
}

/*
 *********************************************************************************************************
 *                                              TaskForwarding1
 *  -traite la priorit� des paquets : si un paquet de haute priorit� est pr�t,
 *   on l'envoie � l'aide de la fonction dispatch, sinon on regarde les paquets de moins haute priorit�
 *********************************************************************************************************
 */
void TaskForwarding(void *pdata) {
    INT8U err;
    Packet *packet = NULL;
    while(1){
        /* � compl�ter */
    	// Wait for a packet to be queued in any Q
    	OSSemPend(sem_packet_computed, 0, &err);
    	err_msg("TaskForwarding - OSSemPend(sem_packet_computed)", err);

    	// Check for video packets
    	packet = OSQAccept(highQ, &err);
    	if(err == OS_ERR_NONE)	// if highQ is not empty
    	{
    		forward(packet);
    	}
    	else if(err == OS_ERR_Q_EMPTY)
    	{
    		// Check for audio packets
    		packet = OSQAccept(mediumQ, &err);
			if(err == OS_ERR_NONE)		// if mediumQ is not empty
			{
				forward(packet);
			}
			else if(err == OS_ERR_Q_EMPTY)
			{
				// Check for misc packets
				packet = OSQAccept(lowQ, &err);
				if(err == OS_ERR_NONE)		// if lowQ is not empty
				{
					forward(packet);
				}
				else if(err == OS_ERR_Q_EMPTY)
				{
					xil_printf("TaskForwarding - ERROR - Semaphore freed while no packets are present in Qs \n");
					continue;
				}
				else
					err_msg("TaskForwarding - OSQPend(lowQ)", err);
			}
    	}
    }
}

/*
 *********************************************************************************************************
 *                                              TaskStats
 *  -Est d�clench�e lorsque le irq_gen_1_isr() lib�re le s�maphore
 *  -Lorsque d�clench�e, Imprime les statistiques du routeur � cet instant
 *********************************************************************************************************
 */
void TaskStats(void *pdata) {
    INT8U err;

    while(1){
    	/* � compl�ter */

    	// Wait for LINUX's interruption
    	OSSemPend(sem_enable_stats, 0, &err);
    	err_msg("TaskStats - OSSemPend(sem_enable_stats)", err);

		xil_printf("TaskStats - Printing stats... \n");

    	// Printing nbPacket
    	OSMutexPend(mtx_nbPacket, 0, &err);
    	err_msg("TaskStats - OSMutexPend(mtx_nbPacket)", err);
		xil_printf("TaskStats - nbPacket: %d /n", nbPacket);
		err = OSMutexPost(mtx_nbPacket);
		err_msg("TaskStats - OSMutexPost(mtx_nbPacket)", err);

		// Printing other stats
    	OSMutexPend(mtx_computingValues, 0, &err);
    	err_msg("TaskStats - OSMutexPend(mtx_computingValues)", err);
		xil_printf("TaskStats - nbPacketLowRejete: %d /n", nbPacketLowRejete);
		xil_printf("TaskStats - nbPacketMediumRejete: %d /n", nbPacketMediumRejete);
		xil_printf("TaskStats - nbPacketHighRejete: %d /n", nbPacketHighRejete);
		xil_printf("TaskStats - nbPacketCRCRejete: %d /n", nbPacketCRCRejete);
		xil_printf("TaskStats - nbPacketSourceRejete: %d /n", nbPacketSourceRejete);
		err = OSMutexPost(mtx_computingValues);
		err_msg("TaskStats - OSMutexPost(mtx_computingValues)", err);

		xil_printf("TaskStats - nbPacketSent: %d /n", nbPacketSent);
    }
}


/*
 *********************************************************************************************************
 *                                              TaskPrint
 *  -Affiche les infos des paquets arriv�s � destination et libere la m�moire allou�e
 *********************************************************************************************************
 */
void TaskPrint(void *data) {
    INT8U err;
    Packet *packet = NULL;
    int intID = ((PRINT_PARAM*)data)->interfaceID;
    OS_EVENT* mb = ((PRINT_PARAM*)data)->Mbox;

    while(1){
        /* � compl�ter */

    	// Retrieve a packet
    	packet = OSMboxPend(mb, 0, &err);
    	err_msg("TaskPrint - OSMboxPend(mb)", err);

    	OSMutexPend(mtx_nbPacket, 0, &err);
    	err_msg("TaskReceivePacket - OSSemPend(sem_nbPacket)", err);

    	xil_printf("\n ***** Interface # %d : Print du Paquet # %d ***** \n", intID, nbPacket++);
    	xil_printf("    -src : %X \n", packet->src);
    	xil_printf("    -dst : %X \n", packet->dst);
    	xil_printf("    -type: %d \n", packet->type);
    	xil_printf("    -crc : %X \n", packet->crc);

		err = OSMutexPost(mtx_nbPacket);
		err_msg("TaskReceivePacket - OSSemPost(sem_nbPacket)", err);

    	// Freeing packet
    	free(packet);
    	packet = NULL;
    }

}
void err_msg(char* entete, INT8U err)
{
	if(err != 0)
	{
		xil_printf(entete);
		xil_printf(": Une erreur est retourn�e : code %d \n",err);
	}
}

// post a packet to verifQ if the packet could not be post in its normal Q (determined with status)
// NOTE that if -1 is return the packet has been freed (no more space in verifQ)
unsigned char post_to_verif(Packet* packet, INT8U status)
{
	if(status != OS_ERR_NONE)
	{
		status = OSQPost(verifQ, packet);
		if(status != OS_ERR_NONE)
		{
			xil_printf("TaskComputing - Packet COMPUTED but verifQ full (packet destroyed) - Type: %d \n", packet->type);	// debug output
			free(packet);
			packet = NULL;
			return -1;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return 0;
	}
}

void forward(Packet* p)
{
	INT8U err;

	// 2 seconds delay (emulate the physical access to table delay...)
	OSTimeDly(TIMER_TICK_FREQUENCY * 2);

	// Broadcast
	if( (p->dst >= INT1_LOW) && (p->dst <= INT1_HIGH) )
	{
		 Packet* packet1 = packetDeepCopy(p);
		 Packet* packet2 = packetDeepCopy(p);

		err = OSMboxPost(Mbox1, packet1);
		err_msg("TaskForwarding - OSMboxPost(Mbox1)", err);
		err = OSMboxPost(Mbox2, packet2);
		err_msg("TaskForwarding - OSMboxPost(Mbox2)", err);
		err = OSMboxPost(Mbox3, p);
		err_msg("TaskForwarding - OSMboxPost(Mbox3)", err);
		return;
	}

	// INT3
	if((p->dst >= INT2_LOW) && (p->dst <= INT2_HIGH))
	{
		err = OSMboxPost(Mbox3, p);
		err_msg("TaskForwarding - OSMboxPost(Mbox3)", err);
		return;
	}

	// INT2
	if((p->dst >= INT3_LOW) && (p->dst <= INT3_HIGH))
	{
		err = OSMboxPost(Mbox2, p);
		err_msg("TaskForwarding - OSMboxPost(Mbox2)", err);
		return;
	}

	// INT1
	if((p->dst >= INT4_LOW) && (p->dst <= INT4_HIGH))
	{
		err = OSMboxPost(Mbox1, p);
		err_msg("TaskForwarding - OSMboxPost(Mbox1)", err);
		return;
	}
}

Packet* packetDeepCopy(Packet* p)
{
	Packet* packet = (Packet*) malloc(sizeof(Packet));
	packet->crc = p->crc;
	packet->src = p->src;
	packet->dst = p->dst;
	packet->type = p->type;
	int i;
	for(i = 0; i < 12; i++)
	{
		packet->data[i] = p->data[i];
	}
	return packet;
}

void incRejectedPacketType(Packet* packet)
{
	INT8U err;
	// 	Inc packet counter based on its priority
	OSMutexPend(mtx_computingValues, 0, &err);
	err_msg("TaskReceivePacket - OSMutexPend(mtx_computingValues)", err);
	switch (packet->type)
	{
	case(VIDEO_PACKET_TYPE) :
		nbPacketHighRejete++;
		break;

	case(AUDIO_PACKET_TYPE) :
		nbPacketMediumRejete++;
		break;

	case(MISC_PACKET_TYPE) :
		nbPacketHighRejete++;
		break;
	}
	err = OSMutexPost(mtx_computingValues);
	err_msg("TaskReceivePacket - OSMutexPost(mtx_computingValues)", err);
}

// utility function that empties all Qs and destroys their packet and destroy all semaphores and mutexes
// Note that if a packet was in the process of being created while TaskStop start, this packet will not be destroy (small possible memory leak [the size of a packet])
void cleaningEverything()
{
	INT8U err;

	// Cleaning Qs
	cleanQ(inputQ);
	cleanQ(verifQ);
	cleanQ(lowQ);
	cleanQ(mediumQ);
	cleanQ(highQ);

	// Cleaning semaphores
	sem_packet_ready = OSSemDel(sem_packet_ready, OS_DEL_ALWAYS, &err);
	sem_packet_computed = OSSemDel(sem_packet_computed, OS_DEL_ALWAYS, &err);
	sem_verif_signal = OSSemDel(sem_verif_signal, OS_DEL_ALWAYS, &err);
	sem_crc_count_check_task_enable = OSSemDel(sem_crc_count_check_task_enable, OS_DEL_ALWAYS, &err);
	sem_enable_stats = OSSemDel(sem_enable_stats, OS_DEL_ALWAYS, &err);

	// Cleaning mutex
	mtx_nbPacket = OSMutexDel(mtx_nbPacket, OS_DEL_ALWAYS, &err);
	mtx_computingValues = OSMutexDel(mtx_computingValues, OS_DEL_ALWAYS, &err);

	// Cleaning MailBox
	cleanMailBox(Mbox1);
	cleanMailBox(Mbox2);
	cleanMailBox(Mbox3);
}

void cleanQ(OS_EVENT* q)
{
	Packet* packet;
	INT8U status = OS_ERR_NONE;
	INT8U err = 0;

	// Cleaning q
	while(status == OS_ERR_NONE)
	{
		packet = OSQAccept(q, &status);
		err_msg("TaskStop - OSQAccept(q)", err);
		if(packet != NULL)
		{
			free(packet);
			packet = NULL;
		}
	}

	// Deleting the Q
	q = OSQDel(q, OS_DEL_ALWAYS, &err);
	err_msg("TaskStop - OSQDel(q)", err);
}

void cleanMailBox(OS_EVENT* m)
{
	Packet* packet = NULL;
	INT8U err;

	// Empty the Mailbox if it is not already
	packet = OSMboxAccept(m);
	if (packet != NULL)
	{
		free(packet);
		packet = NULL;
	}

	// Delete Mailbox
	m = OSMboxDel(m, OS_DEL_ALWAYS, &err);
	err_msg("TaskStop - OSMboxDel(m)", err);
}
