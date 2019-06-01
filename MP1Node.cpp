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
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

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
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

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
 		//initMemberListTable(memberNode);
    //delete memberNode;
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

    MessageHdr *msg = (MessageHdr *) data;

    if (msg->msgType == JOINREQ) {
			int id = *(int*)(&m->addr.addr);
			short port = *(short*)(&m->addr.addr[4]);

			if (findMember(id, port) != nullptr) {
					return;
			}

			log->logNodeAdd(&memberNode->addr, &m->addr);

			MemberListEntry *newMemList = new MemberListEntry(id, port, 1, par->getcurrtime());
			memberNode->memberList.push_back(*newMemList);

      MessageHdr *repMsg = createMessage(JOINREP);
			//creating JOINREP
			MessageHdr *responseMsg = new MessageHdr();
			responseMsg->countMembers = memberNode->memberList.size();

			if (memberNode->memberList.size() > 0) {
					responseMsg->members = new MemberListEntry[memberNode->memberList.size()];
					memcpy(responseMsg->members, memberNode->memberList.data(), sizeof(MemberListEntry) * memberNode->memberList.size());
			}

			responseMsg->msgType = JOINREP;
			memcpy(&responseMsg->addr, &memberNode->addr, sizeof(Address));


      emulNet->ENsend(&memberNode->addr, &msg->addr, (char *) repMsg, sizeof(MessageHdr));
      delete repMsg;
    } else if (msg->msgType == JOINREP) {
        memberNode->inGroup = true;
				int id = *(int*)(&m->addr.addr);
				short port = *(short*)(&m->addr.addr[4]);

				if (findMember(id, port) != nullptr) {
						return;
				}

				log->logNodeAdd(&memberNode->addr, &m->addr);

				MemberListEntry *newMemList = new MemberListEntry(id, port, 1, par->getcurrtime());
				memberNode->memberList.push_back(*newMemList);

    } else if (msg->msgType == HEARTBEAT) {
        heartBeat(msg);
    }

    free(msg);
}

void MP1Node::heartBeat(MessageHdr *m) {
    // update current node counter
    MemberListEntry *localMember = findMember(&m->addr);

    if (localMember != nullptr) {
        localMember->heartbeat += 1;
        localMember->timestamp = par->getcurrtime();
    } else {
			int id = *(int*)(&m->addr.addr);
			short port = *(short*)(&m->addr.addr[4]);

			if (findMember(id, port) != nullptr) {
					return;
			}
			log->logNodeAdd(&memberNode->addr, &m->addr);
			MemberListEntry *newMemList = new MemberListEntry(id, port, 1, par->getcurrtime());
			memberNode->memberList.push_back(*newMemList);
    }

    // update other node counters
    for(int i = 0; i < m->countMembers; i++) {
        MemberListEntry gossipMember = m->members[i];
        MemberListEntry *clusterMember = findMember(gossipMember.id, gossipMember.port);

        if (clusterMember != nullptr) {
            if (gossipMember.heartbeat > clusterMember->heartbeat) {
                clusterMember->heartbeat = gossipMember.heartbeat;
                clusterMember->timestamp = par->getcurrtime();
            }
        } else {
					Address *addr = getAddr(*e);

					if (findMember(addr) != nullptr) {
							return;
					}
					if (*addr == memberNode->addr) {
							delete addr;
							return;
					}
					if (par->getcurrtime() - e->timestamp < TREMOVE) {
							log->logNodeAdd(&memberNode->addr, addr);
							MemberListEntry *newMember = new MemberListEntry(e->id, e->port, e->heartbeat, par->getcurrtime());
							memberNode->memberList.push_back(*newMember);
					}
        }
    }
}

int MP1Node::getMemberPosition(MemberListEntry *e) {
    for(int i = 0; i < memberNode->memberList.size(); i++) {
        MemberListEntry clusterMemb = memberNode->memberList[i];

        if (clusterMemb.id == e->id && clusterMemb.port == e->port) {
            return i;
        }
    }
}


MemberListEntry* MP1Node::findMember(int id, short port) {
    for(int i = 0; i < memberNode->memberList.size(); i++) {
        MemberListEntry *clusterMemb = memberNode->memberList.data() + i;

        if (clusterMemb->id == id && clusterMemb->port == port) {
            return clusterMemb;
        }
    }

    return nullptr;
}

MemberListEntry* MP1Node::findMember(Address *addr) {
    for(int i = 0; i < memberNode->memberList.size(); i++) {
        MemberListEntry *clusterMemb = memberNode->memberList.data() + i;
        Address *memberAddr = getNodeAddress(clusterMemb->id, clusterMemb->port);

        if (*addr == *memberAddr) {
            return clusterMemb;
        }

        delete memberAddr;
    }

    return nullptr;
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {
    memberNode->heartbeat += 1;

    vector<MemberListEntry> deleteMembers;

    // check local members status
    for(MemberListEntry mem: memberNode->memberList) {
        if (par->getcurrtime() - mem.timestamp >= TREMOVE ) {
            deleteMembers.push_back(mem);
        }
    }

    for(MemberListEntry delMember: deleteMembers) {
        Address *deleteAddr = getAddr(delMember);
        log->logNodeRemove(&memberNode->addr, deleteAddr);
        int delPos = getMemberPosition(&delMember);
        memberNode->memberList.erase(memberNode->memberList.begin() + delPos);
    }

		//cout<<"Sending heartbeat"
    MessageHdr * message = createMessage(HEARTBEAT);
    for(MemberListEntry mem: memberNode->memberList) {
        Address *address = getAddr(mem);
        emulNet->ENsend(&memberNode->addr, address, (char *) message, sizeof(MessageHdr));
        delete address;
    }
    delete message;
}

Address* MP1Node::getAddr(MemberListEntry e) {
    Address *address = new Address();
    memset(address->addr, 0, sizeof(address->addr));
    memcpy(address->addr, &e.id, sizeof(int));
    memcpy(address->addr + sizeof(int), &e.port, sizeof(short));
    return address;
}

Address* MP1Node::getNodeAddress(int id, short port) {
    Address *address = new Address();
    memset(address->addr, 0, sizeof(address->addr));
    memcpy(address->addr, &id, sizeof(int));
    memcpy(address->addr + sizeof(int), &port, sizeof(short));

    return address;
}

MessageHdr* MP1Node::createMessage(MsgTypes t) {
    MessageHdr *responseMsg = new MessageHdr();
    responseMsg->countMembers = memberNode->memberList.size();

    if (memberNode->memberList.size() > 0) {
        responseMsg->members = new MemberListEntry[memberNode->memberList.size()];
        memcpy(responseMsg->members, memberNode->memberList.data(), sizeof(MemberListEntry) * memberNode->memberList.size());
    }

    responseMsg->msgType = t;
    memcpy(&responseMsg->addr, &memberNode->addr, sizeof(Address));

    return responseMsg;
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
