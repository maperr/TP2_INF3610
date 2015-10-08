#include "cpu1_uc.h"

///////////////////////////////////////////////////////////////////////////////////////
//								Routines d'interruptions
///////////////////////////////////////////////////////////////////////////////////////

void irq_gen_0_isr(void* data) {
	/* � compl�ter */
	INT8U err;
	xil_printf("ISR gen0 \n");

	// Enable ReceivePacket task
	err = OSSemPost(sem_packet_ready);
	err_msg("irq_gen_0_isr - OSSemPost(sem_packet_ready)", err);

	// Cleaning interrupt flags registers
	fpga_core* coreData = (fpga_core*) data;
	Xil_Out32(coreData->Config.BaseAddress, 0);
	XIntc_Acknowledge(&m_axi_intc, 1);
}

void irq_gen_1_isr(void* data) {
	/* � compl�ter */
	xil_printf("ISR gen1 \n");
}
void timer_isr(void* not_valid) {
	if (private_timer_irq_triggered()) {
		private_timer_clear_irq();
		OSTimeTick();
	}                           
}

void fit_timer_1s_isr(void *not_valid) {
	/* � compl�ter */
	xil_printf("ISR timer_1s \n");
}
void fit_timer_5s_isr(void *not_valid) {
	/* � compl�ter */
	xil_printf("ISR timer_5s \n");
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
		xil_printf("Error %d while creating tasks\n", error);

	error = create_events();
	if (error != 0)
		xil_printf("Error %d while creating events\n", error);
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

    return 0;
}

int create_events() {
	/*CREATION DES FILES*/
	inputQ = OSQCreate(&inputMsg[0], 16);
	lowQ = OSQCreate(&lowMsg[0], 4);
	mediumQ = OSQCreate(&mediumMsg[0], 4);
	highQ = OSQCreate(&highMsg[0], 4);
	verifQ = OSQCreate(&verifMsg[0], 10);

	/*CREATION DES MAILBOXES*/


	/*ALLOCATION ET DEFINITIONS DES STRUCTURES PRINT_PARAM*/

	/*CREATION DES SEMAPHORES*/
	sem_packet_ready = OSSemCreate(0);
	sem_packet_computed = OSSemCreate(0);

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
    	OSSemPend(sem_packet_ready, 0, &err);
    	err_msg("TaskReceivePacket - OSSemPend(sem_packet_ready)", err);
    	/* � compl�ter : R�ception des paquets de Linux */
    	uint32_t *ll;
		ll = (uint32_t *) (packet);

		for(k = 0; k < 16; k++)
		{
			while(COMM_RX_FLAG == 0); //wait for Linux to produce the value
			*ll = COMM_RX_DATA; // read current part of packet
			ll++;	// point to next part of packet
			COMM_RX_FLAG = 0;
		}

		xil_printf("RECEIVE : ********Reception du Paquet # %d ******** \n", nbPacketSent++);
		xil_printf("ADD %x \n", packet);
		xil_printf("    ** src : %x \n", packet->src);
		xil_printf("    ** dst : %x \n", packet->dst);
		xil_printf("    ** type : %d \n", packet->type);
		xil_printf("    ** crc : %x \n", packet->crc);

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
	INT8U err;
	Packet *packet = NULL;
	while (1) {
		/* � compl�ter */
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
	int compteur = 0;
	while (1) {
		// Unqueue a packet form the queue
		packet = OSQPend(inputQ, 0, &err);

		// Debug output...
		xil_printf("COMPUTE : ********Traitement du Paquet # %d ******** \n", compteur++);
		xil_printf("ADD %x \n", packet);
		xil_printf("    ** src : %x \n", packet->src);
		xil_printf("    ** dst : %x \n", packet->dst);
		xil_printf("    ** type : %d \n", packet->type);
		xil_printf("    ** crc : %x \n", packet->crc);

		err_msg("TaskComputing - OSQPend(inputQ)", err);

		// Validate the source and reject unknown packet
		if (packet->src >= REJECT_LOW1 && packet->src <= REJECT_HIGH1 ||
			packet->src >= REJECT_LOW2 && packet->src <= REJECT_HIGH2 ||
			packet->src >= REJECT_LOW3 && packet->src <= REJECT_HIGH3 ||
			packet->src >= REJECT_LOW4 && packet->src <= REJECT_HIGH4  )
		{
			xil_printf("PacketSourceRejected - SourceAddress: %x \n", packet->src);
			nbPacketSourceRejete++;
			free(packet);
			packet = NULL;
			continue;
		}

		// Validate checksum and delete corrupted packet
		unsigned int checksum = computeCRC(packet, 64);
		if (checksum != 0)
		{
			xil_printf("PacketChecksumCorrupted - ChecksumValue: %x \n", checksum);
			free(packet);
			packet = NULL;
			continue;
		}

		// Transfer to the right type queue
		switch (packet->type)
		{
		case(VIDEO_PACKET_TYPE) :
			xil_printf("VideoPacketTransfered \n");
			err = OSQPost(highQ, packet);
			if(post_to_verif(packet, err) == 0)
			{
				err = OSSemPost(sem_packet_computed);
				err_msg("TaskComputing - OSSemPost(sem_packet_computed)", err);
			}
			break;

		case(AUDIO_PACKET_TYPE) :
			xil_printf("AudioPacketTransfered \n");
			err = OSQPost(mediumQ, packet);
			if(post_to_verif(packet, err) == 0)
			{
				err = OSSemPost(sem_packet_computed);
				err_msg("TaskComputing - OSSemPost(sem_packet_computed)", err);
			}
			break;

		case(MISC_PACKET_TYPE) :
			xil_printf("MiscPacketTransfered \n");
			err = OSQPost(lowQ, packet);
			xil_printf("Post return %d \n", err);
			if(post_to_verif(packet, err) == 0)
			{
				err = OSSemPost(sem_packet_computed);
				err_msg("TaskComputing - OSSemPost(sem_packet_computed)", err);
			}
			break;

		default:
			// Type is invalid
			xil_printf("InvalidType \n");
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
    int compteur = 0;

    while(1){
        /* � compl�ter */
    	OSSemPend(sem_packet_computed, 0, &err);
    	err_msg("TaskForwarding - OSSemPend(sem_packet_computed)", err);
    	xil_printf("Packet forwarding packet: %d \n", compteur++);
    	OSQAccept(highQ, &err);
    	if(err == 0)
    	{
    		// TODO: Forward...
    		packet = OSQPend(highQ, 0, &err);
    		xil_printf("VideoPacketForwarded \n");
    	}
    	else if(err == OS_ERR_Q_EMPTY)
    	{
    		OSQAccept(mediumQ, &err);
			if(err == 0)
			{
				// TODO: Forward...
				xil_printf("AudioPacketForwarded \n");
				packet = OSQPend(mediumQ, 0, &err);
			}
			else if(err == OS_ERR_Q_EMPTY)
			{
				OSQAccept(lowQ, &err);
				if(err == 0)
				{
					// TODO: Forward...
					packet = OSQPend(lowQ, 0, &err);
					xil_printf("MiscPacketForwarded \n");
				}
				else if(err == OS_ERR_Q_EMPTY)
				{
					xil_printf("SEMAPHORE FREE WHILE NO PACKET TO FORWARD!!!! \n");
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

unsigned char post_to_verif(Packet* packet, INT8U status)
{
	if(status == OS_ERR_Q_FULL)
	{
		status = OSQPost(verifQ, packet);
		if(status == OS_ERR_Q_FULL)
		{
			xil_printf("No more place in verification queue \n");
			free(packet);
			packet = NULL;
			return -1;
		}
		else
			return 1;
	}
	else
		return 0;
}

