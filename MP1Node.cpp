/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

void MP1Node::printMessage(string callr_fn, Address *sendr, MessagePayLoad *msg, int size)
{
    MsgTypes msgType;
    //char *msg_chr;
    
    cout << "printMessage[" << this->par->getcurrtime() << "] in " << callr_fn << " of MP1Node:" << this->memberNode->addr.getAddress() <<endl;
    memcpy(&msgType, msg, sizeof(MsgTypes));
    //msg_chr = (char *)msg;
    
    switch(msgType) {
        case(JOINREQ): 
            cout << " JOINREQ"; 
            break;
  
        case(JOINREP): 
            cout << " JOINREP"; 
            break;

        case(GOSSIP): 
            cout << " GOSSIP"; 
            break;
        
        default: 
            cout << "UNKNOWN";

    }
    cout << "from=" << sendr->getAddress() <<endl;
    //cout << " to=" << recvr->getAddress();
    cout << endl;
}

void MP1Node::printNodeData(string caller_fn) {
    cout << endl << "@Time[" << this->par->getcurrtime() << "]in " << caller_fn << " of MP1Node.cpp:" << endl;
	cout << "[" << this->par->getcurrtime() << "]in " << caller_fn << " of MP1Node-Addr:" << memberNode->addr.getAddress() << endl;        
    cout << "inGroup=" << memberNode->inGroup << "| " << "heartbeat=" << memberNode->heartbeat << "| " << "pingCounter=" << memberNode->pingCounter << "| ";
	cout << "timeOutCounter=" << memberNode->timeOutCounter << "| " << "nnb=" << memberNode->nnb << "| " "memberList: size=" << memberNode->memberList.size() << endl;
    
    size_t pos = 0;
    for (pos = 0; pos < memberNode->memberList.size(); pos++) {
        cout << "[" << this->par->getcurrtime() << "]in " << caller_fn << " of MP1Node:" << memberNode->addr.getAddress();
        cout << " ";
        cout << "pos=" << pos << "| ";
        cout << "id="           << memberNode->memberList[pos].id << "| ";    
        cout << "port="         << memberNode->memberList[pos].port << "| ";    
        cout << "heartbeat="    << memberNode->memberList[pos].heartbeat << "| ";                
        cout << "timestamp="    << memberNode->memberList[pos].timestamp << "| ";                            
        cout << endl;
    }
	if (this->memberNode->memberList.size() == 0)
		cout << "Nothng in MemberList Table" <<endl<<endl;
}


/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	//memberNode->timeOutCounter = -1;
	memberNode->timeOutCounter = TREMOVE; //Set Actual time out
    initMemberListTable(memberNode);
     
   cout << endl << "initThisNode: id=" << id << ", port=" << port << endl;
	
    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        //size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        //msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        //msg->msgType = JOINREQ;
        //memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        //memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

		MessageHdr *msg;
		MessagePayLoad *mpl;
		int n = 1;
		size_t msgsize = sizeof(MessageHdr) + (n * sizeof(MessagePayLoad));

		msg = (MessageHdr *) malloc(msgsize);
		msg->msgType = JOINREQ;
		msg->MemberEntry = 1;
		mpl = (MessagePayLoad *)(msg + 1);
		mpl->NodeId = *(int *)(&memberNode->addr.addr);
		mpl->Port = *(short *)(&memberNode->addr.addr[4]);
		mpl->HeartBeatCntr = memberNode->heartbeat;
		
		cout << "introduceSelfToGroup --> Trying to join... " << &memberNode->addr << " --> heartbeat" << memberNode->heartbeat <<endl;
		this->printNodeData("introduceSelfToGroup");

		
		emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);
	

        // send JOINREQ message to introducer member
        //emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);
	//this->printMessage("introduceSelfToGroup", &this->memberNode->addr, msg, sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1);
	    
        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
	return 0;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();
	
    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	
	Member *node = (Member *) env;
	MessageHdr *msg = (MessageHdr *)data;
	//char *MessagePayLoad = (char *)(msg + 1);
	MessagePayLoad *mpl = (MessagePayLoad *)(msg + 1);

	if ( (unsigned)size < sizeof(MessageHdr) ) {
		#ifdef DEBUGLOG
			log->LOG(&node->addr, "Faulty packet received - ignoring");
			return false;
		#endif
	}

	#ifdef DEBUGLOG
		log->LOG(&((Member *)env)->addr, "Received message type %d with %d Byte", msg->msgType, size - sizeof(MessageHdr));
	#endif

	switch(msg->msgType) {
		case(JOINREQ): 
			cout << "JOINREQ: size=" << size; 
			Address *srcAddr = (Address *)mpl;
			for (int i = 0; i < msg->MemberEntry; i++) {
				processJoinReq(mpl->id, mpl->port, mpl->heartbeat);
				mpl++;
			}
			this->printNodeData("recvCallBack"); 
		break;
		case(JOINREP): cout << "JOINREP: size=" << size; memberNode->inGroup = true; this->printNodeData("recvCallBack"); break;
		case(GOSSIP): cout << "GOSSIP: size=" << size; this->printNodeData("recvCallBack"); break;
		default: cout << "WrongMessageType: size=" << size; return(false);
	}
	
	
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */
	

    return;
}

/* Usage: Send a gossip message */

int MP1Node::spreadGossipMemberList(enum MsgTypes msgType, Address *dstAddr) {

	MessageHdr *msg;
	MessagePayLoad *mpl;
	int n;
	n += memberNode->memberList.size();
	size_t msgsize = sizeof(MessageHdr) + n * sizeof(MessagePayLoad);
	
	msg = (MessageHdr *) malloc(msgsize);
	msg->msgType = msgType;
	msg->MemberEntry = n;
	mpl = (MessagePayLoad *)(msg + 1);
	mpl->NodeId = *(int *)(&memberNode->addr.addr);
	mpl->Port = *(short *)(&memberNode->addr.addr[4]);
	mpl->HeartBeatCntr = memberNode->heartbeat;
	n += memberNode->memberList.size();
	
	for (vector<MemberListEntry>::iterator m = memberNode->memberList.begin(); m != memberNode->memberList.end(); ++m) {
		mpl++;
		if (par->getcurrtime() - m->timestamp <= memberNode->pingCounter ) {
			mpl->NodeId = m->id;
			mpl->Port = m->port;
			mpl->HeartBeatCntr = m->heartbeat;
		} else { 
			mpl->NodeId = *(int *)(&memberNode->addr.addr);
			mpl->Port = *(short *)(&memberNode->addr.addr[4]);
			mpl->HeartBeatCntr = memberNode->heartbeat;
		}
	}

	emulNet->ENsend(&memberNode->addr, dstAddr, (char *)msg, msgsize);
	
	free(msg);
	return 0;
}

/* Usage: processing Join request by introducer */
void MP1Node::processJoinReq(int id, short port, long HeartBeatCntr) {

		// process the new NODE
		MemberListEntry *newMember = new MemberListEntry(id, port, HeartBeatCntr, par->getcurrtime());
		memberNode->memberList.insert(memberNode->memberList.begin(), *newMember);

		Address *srcAddr = (Address *)malloc(sizeof(Address));
		*(int *)(&srcAddr->addr[0]) = id;
		*(short *)(&srcAddr->addr[4]) = port;
		log->logNodeAdd(&(memberNode->addr), srcAddr);
		
		spreadGossipMemberList(JOINREP, srcAddr);
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}
