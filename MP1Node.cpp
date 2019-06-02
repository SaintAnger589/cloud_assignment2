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

        msg = new MessageHdr();
        memset(&msg->addr, 0, sizeof(msg->addr));
			  // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy(&msg->addr, &memberNode->addr, sizeof(Address));
#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, sizeof(MessageHdr));

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

if (msg->msgType == JOINREP) {
				int id = *(int*)(&msg->addr.addr);
				short port = *(short*)(&msg->addr.addr[4]);
				//cout<<"recvCallBack: id ="<<id<<"\n";
				//cout<<"recvCallBack: port="<<port<<"\n";
				memberNode->inGroup = true;
				//if the member is not a part of the list add it
				if (findMemberFromId(id, port) != nullptr) {
				} else {
					log->logNodeAdd(&memberNode->addr, &msg->addr);
					MemberListEntry *newMemeb = new MemberListEntry(id, port, 1, par->getcurrtime());
					memberNode->memberList.push_back(*newMemeb);
				}
    } else if (msg->msgType == HEARTBEAT) {
			//cout<<"localMember: Inside heartbeat\n";
				MemberListEntry *mem = findMember(&msg->addr);
				if (mem != nullptr) {
						mem->heartbeat = mem->heartbeat + 1;
						mem->timestamp = par->getcurrtime();
				} else {
						int id = *(int*)(&msg->addr.addr);
						short port = *(short*)(&msg->addr.addr[4]);
						if (findMemberFromId(id, port) != nullptr) {
						} else {
							//this is a new member to the list, add it
							log->logNodeAdd(&memberNode->addr, &msg->addr);
							//cout<<"HEARTBEAT: msg->addr = ";
							//printAddress(msg->addr);
							MemberListEntry *newMemeb = new MemberListEntry(id, port, 1, par->getcurrtime());
							memberNode->memberList.push_back(*newMemeb);
						}
				} //else
				for(int i = 0; i < msg->countMembers; i++) {
						MemberListEntry gossipMember = msg->members[i];
						MemberListEntry *mem = findMemberFromId(gossipMember.id, gossipMember.port);
						if (mem != nullptr) {
								//if the existing member heartbeat is less then the message heartbeat add that
								//cout<<"gossipMember.heartbeat = "<<gossipMember.heartbeat<<"\n";
								//cout<<"mem->heartbeat = "<<mem->heartbeat<<"\n";
								if (gossipMember.heartbeat > mem->heartbeat) {
										mem->heartbeat = gossipMember.heartbeat;
										//change timestamp
										mem->timestamp = par->getcurrtime();
								}
						} else {
							//add the new member to the list
								Address *addr = getAddr(gossipMember);
								if (findMember(addr) != nullptr) {
								} else {
									if (*addr == memberNode->addr) {
										//deleting the address
											delete addr;
									} else {
										if (par->getcurrtime() - gossipMember.timestamp < TREMOVE) {
											  //adding the log message
												log->logNodeAdd(&memberNode->addr, addr);
												MemberListEntry *newMember = new MemberListEntry(gossipMember.id, gossipMember.port, gossipMember.heartbeat, par->getcurrtime());
												//pushing the node to the getMembershipList
												memberNode->memberList.push_back(*newMember);
										}
									}
								}
						}
				}
    } else if (msg->msgType == JOINREQ) {
						int id = *(int*)(&msg->addr.addr);
						short port = *(short*)(&msg->addr.addr[4]);
						//if the member is already present do nothing
						if (findMemberFromId(id, port) != nullptr) {
						} else {
							//add the log message
							log->logNodeAdd(&memberNode->addr, &msg->addr);
							//cout<<"msg->addr = ";
							//printAddress(msg->addr)l
							MemberListEntry *newMemeb = new MemberListEntry(id, port, 1, par->getcurrtime());
							memberNode->memberList.push_back(*newMemeb);
						}
						//create a JOINREP message
						MessageHdr *responseMsg = new MessageHdr();
						responseMsg->countMembers = memberNode->memberList.size();
						if (memberNode->memberList.size() > 0) {
							//if size is greater then 0, add the response message
								responseMsg->members = new MemberListEntry[memberNode->memberList.size()];
								//create the response
								memcpy(responseMsg->members, memberNode->memberList.data(), sizeof(MemberListEntry) * memberNode->memberList.size());
						}
						responseMsg->msgType = JOINREP;
						memcpy(&responseMsg->addr, &memberNode->addr, sizeof(Address));
						//send the JOINREP message
		        emulNet->ENsend(&memberNode->addr, &msg->addr, (char *) responseMsg, sizeof(MessageHdr));
		        delete responseMsg;
		    }

    free(msg);
}

MemberListEntry* MP1Node::findMemberFromId(int id, short port) {
    for(int i = 0; i < memberNode->memberList.size(); i++) {
        MemberListEntry *mem = memberNode->memberList.data() + i;
        if (mem->id == id && mem->port == port) {
            return mem;
        }
    }
    return nullptr;
}

MemberListEntry* MP1Node::findMember(Address *addr) {
    for(int i = 0; i < memberNode->memberList.size(); i++) {
        MemberListEntry *mem = memberNode->memberList.data() + i;
        Address *memberAddr = getAddrFromId(mem->id, mem->port);
        if (*addr == *memberAddr) {
            return mem;
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
	  vector<MemberListEntry> deleteMembers;
		//increase the memberNode heartbeat
    memberNode->heartbeat = memberNode->heartbeat + 1;
		//check for member timestamp is greater then TREMOVE
    for(MemberListEntry mem: memberNode->memberList) {
        if (par->getcurrtime() - mem.timestamp >= TREMOVE ) {
					//push in delete vector
            deleteMembers.push_back(mem);
        }
    }
		//cout<<"nodeLoopOps: deleteMembers.size() = "<<deleteMembers.size()<<"\n";
    for(MemberListEntry delMember: deleteMembers) {
        Address *deleteAddr = getAddr(delMember);
				//add the logNodeRemove message
        log->logNodeRemove(&memberNode->addr, deleteAddr);
				//find the delete Position
          int delPosition;
				for(int i = 0; i < memberNode->memberList.size(); i++) {
						MemberListEntry mem = memberNode->memberList[i];
						if (mem.id == delMember.id && mem.port == delMember.port) {
							  //cout<<"nodeLoopOps: delPosition = "<<delPosition<<"\n";
								//cout<<"nodeLoopOps: delMember:id = "<<delMember.id<<"\n";
								//cout<<"nodeLoopOps: delMember:port = "<<delMember.port<<"\n";
								delPosition = i;
								break;
						}
				}
				//erase the node from the list
        memberNode->memberList.erase(memberNode->memberList.begin() + delPosition);
    }
		MessageHdr *responseMsg = new MessageHdr();
		responseMsg->countMembers = memberNode->memberList.size();
		if (memberNode->memberList.size() > 0) {
				responseMsg->members = new MemberListEntry[memberNode->memberList.size()];
				memcpy(responseMsg->members, memberNode->memberList.data(), sizeof(MemberListEntry) * memberNode->memberList.size());
		}
		responseMsg->msgType = HEARTBEAT;
		memcpy(&responseMsg->addr, &memberNode->addr, sizeof(Address));
    for(MemberListEntry mem: memberNode->memberList) {
        Address *address = getAddr(mem);
				//sending the heartbeat
        emulNet->ENsend(&memberNode->addr, address, (char *) responseMsg, sizeof(MessageHdr));
        delete address;
    }
    delete responseMsg;
}

Address* MP1Node::getAddr(MemberListEntry memList) {
    Address *address = new Address();
    memset(address->addr, 0, sizeof(address->addr));
    memcpy(address->addr, &memList.id, sizeof(int));
    memcpy(address->addr + sizeof(int), &memList.port, sizeof(short));

    return address;
}

Address* MP1Node::getAddrFromId(int id, short port) {
    Address *address = new Address();
    memset(address->addr, 0, sizeof(address->addr));
    memcpy(address->addr, &id, sizeof(int));
    memcpy(address->addr + sizeof(int), &port, sizeof(short));
    return address;
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
