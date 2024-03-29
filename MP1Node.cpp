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
	//int id = *(int*)(&memberNode->addr.addr);
	//int port = *(short*)(&memberNode->addr.addr[4]);

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
     
   //cout << endl << "initThisNode: id=" << id << ", port=" << port << endl;
	
    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	
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
		
		//cout << "introduceSelfToGroup --> Trying to join... " << &memberNode->addr << " --> heartbeat" << memberNode->heartbeat <<endl;
		//this->debugNode("introduceSelfToGroup");

		
		emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);
	

        // send JOINREQ message to introducer member
        //emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);
	    
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
	Address *srcAddr = (Address *)mpl;

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
		{
			//cout << "JOINREQ: size=" << size <<endl; 
			for (int i = 0; i < msg->MemberEntry; i++) {
				processMembership(mpl->NodeId, mpl->Port, mpl->HeartBeatCntr);
				mpl++;
			}
			spreadGossipMemberList(JOINREP, srcAddr);
			
			break;
		}
		case(JOINREP): 
		{
			//cout << "JOINREP: size=" << size <<endl; 
			memberNode->inGroup = true; 
			//this->debugNode("recvCallBack"); 
			break;
		}
		case(GOSSIP): 
		{
			//cout << "GOSSIP: size=" << size <<endl; 
			for (int i = 0; i < msg->MemberEntry; i++) {
				processMembership(mpl->NodeId, mpl->Port, mpl->HeartBeatCntr);
				mpl++;
			}
			
			//this->debugNode("recvCallBack"); 
			break;
		}
		default: log->LOG(&((Member *)env)->addr, "Recived faulty message type"); return(false);
	}
	return(true);
	
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	memberNode->heartbeat++;

	Address *dstAddr = (Address *)malloc(sizeof(Address));
	std::random_shuffle ( memberNode->memberList.begin(), memberNode->memberList.end() );
	int ls = memberNode->memberList.size();
	int no_of_random_gossip = 0;
	//Send Gossip Out
	while (ls > 0 ) {
		
		*(int *)(&dstAddr->addr[0]) = memberNode->memberList[ls-1].id;
		*(short *)(&dstAddr->addr[4]) = memberNode->memberList[ls-1].port;
		spreadGossipMemberList(GOSSIP, dstAddr);
		
		log->LOG(dstAddr, "Gossip Sent");
		
		if (no_of_random_gossip == 2) break;
		ls--;
		no_of_random_gossip++;
	}

	for (vector<MemberListEntry>::iterator i = memberNode->memberList.begin(); i != memberNode->memberList.end(); ) {
		if (par->getcurrtime() - i->timestamp > memberNode->timeOutCounter ) {
			*(int *)(&dstAddr->addr[0]) = i->id;
			*(short *)(&dstAddr->addr[4]) = i->port;
			log->logNodeRemove(&(memberNode->addr), dstAddr);

			i = memberNode->memberList.erase(i);
		} else {
			++i;
		}
	}
	//this->debugNode("nodeLoopOps");

    return;
}

/* Usage: Send a gossip message */



int MP1Node::spreadGossipMemberList(enum MsgTypes msgType, Address *dstAddr) {

	MessageHdr *msg;
	MessagePayLoad *mpl;
	int n = memberNode->memberList.size();
	size_t msgsize = sizeof(MessageHdr) + n * sizeof(MessagePayLoad); //Form message of size of the table i.e. MessagePayLoad, with header i.e. MessageHdr
	msg = (MessageHdr *) malloc(msgsize);
	mpl = (MessagePayLoad *)(msg + 1);
	
	//cout << "spreadGossipMemberList-->memberList.size=" << memberNode->memberList.size()  << ", value of n=" << n << endl;
 
	msg->msgType = msgType; //Set Message Type
	msg->MemberEntry = n; //Set No of Message in the Payload default atlease 1
	
	for (vector<MemberListEntry>::iterator i = memberNode->memberList.begin(); i != memberNode->memberList.end(); ++i) {

		//cout << "spreadGossipMemberList-->Value of id=" << i->id << ", getcurrtime=" << par->getcurrtime() << ", timestamp=" << i->timestamp << ", pingCounter=" << memberNode->pingCounter << endl;
		
		if (par->getcurrtime() - i->timestamp <= memberNode->pingCounter ) {
			mpl->NodeId = i->id;
			mpl->Port = i->port;
			mpl->HeartBeatCntr = i->heartbeat;
		} else { 
			mpl->NodeId = *(int *)(&memberNode->addr.addr);
			mpl->Port = *(short *)(&memberNode->addr.addr[4]);
			mpl->HeartBeatCntr = memberNode->heartbeat;
		}
		mpl++;
	}
 
	emulNet->ENsend(&memberNode->addr, dstAddr, (char *)msg, msgsize);

	free(msg);
	return 0;	
	
}

/* Usage: processing Join request by introducer */
void MP1Node::processMembership(int id, short port, long HeartBeatCntr) {

		
		bool isnewEntry = true;
		// process the exiting NODE if any
		for (vector<MemberListEntry>::iterator i = memberNode->memberList.begin(); i != memberNode->memberList.end(); ++i) {
			if (i->id == id && i->port == port) {
				if (i->heartbeat < HeartBeatCntr) {
					i->heartbeat = HeartBeatCntr;
					i->timestamp = par->getcurrtime();
				}
				isnewEntry = false;
			}
		}
		if (isnewEntry) { // if it is not exisitng node then add to your list
			MemberListEntry *newMember = new MemberListEntry(id, port, HeartBeatCntr, par->getcurrtime());
			memberNode->memberList.insert(memberNode->memberList.begin(), *newMember);			
		}
		
		Address *srcAddr = (Address *)malloc(sizeof(Address));
		*(int *)(&srcAddr->addr[0]) = id;
		*(short *)(&srcAddr->addr[4]) = port;
		log->logNodeAdd(&(memberNode->addr), srcAddr);
		
		free(srcAddr);
		
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


void MP1Node::debugNode(string whois_calling) {
	cout << endl << "@Time[" << this->par->getcurrtime() << "]in " << whois_calling << " of MP1Node-Addr:" << memberNode->addr.getAddress() << endl;        
    cout << "inGroup=" << memberNode->inGroup << "| " << "heartbeat=" << memberNode->heartbeat << "| " << "pingCounter=" << memberNode->pingCounter << "| ";
	cout << "timeOutCounter=" << memberNode->timeOutCounter << "| " << "nnb=" << memberNode->nnb << "| " "memberList: size=" << memberNode->memberList.size() << endl;
    
    size_t pos = 0;
    for (pos = 0; pos < memberNode->memberList.size(); pos++) {
        cout << "MemberShip Table [@" << this->par->getcurrtime() << "] for " << whois_calling << " of MP1Node:" << memberNode->addr.getAddress();
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
