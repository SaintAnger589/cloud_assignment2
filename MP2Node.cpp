/**********************************
 * FILE NAME: MP2Node.cpp
 *
 * DESCRIPTION: MP2Node class definition
 **********************************/
#include "MP2Node.h"

/**
 * constructor
 */
MP2Node::MP2Node(Member *memberNode, Params *par, EmulNet * emulNet, Log * log, Address * address) {
	this->memberNode = memberNode;
	this->par = par;
	this->emulNet = emulNet;
	this->log = log;
	ht = new HashTable();
	this->memberNode->addr = *address;
	//trace creation
	this->trace = new Trace();
	this->trace->traceFileCreate();
}

/**
 * Destructor
 */
MP2Node::~MP2Node() {
	delete ht;
	delete memberNode;
	this->trace->traceFileClose();
}

/**
 * FUNCTION NAME: updateRing
 *
 * DESCRIPTION: This function does the following:
 * 				1) Gets the current membership list from the Membership Protocol (MP1Node)
 * 				   The membership list is returned as a vector of Nodes. See Node class in Node.h
 * 				2) Constructs the ring based on the membership list
 * 				3) Calls the Stabilization Protocol
 */
void MP2Node::updateRing() {
	//tracing this function
	this->trace->funcEntry("updateRing");
	/*
	 * Implement this. Parts of it are already implemented
	 */
	vector<Node> curMemList;
	bool change = false;

	/*
	 *  Step 1. Get the current membership list from Membership Protocol / MP1
	 */
	curMemList = getMembershipList();
	//cout<<"updateRing: after curMemList\n";
  //printNode (curMemList);
	/*
	 * Step 2: Construct the ring
	 */
	 sort(curMemList.begin(), curMemList.end());
  bool changed = false;
	 if (!ring.empty()) {
		for (int i = 0; i < curMemList.size(); i++) {
			if (curMemList[i].getHashCode() != ring[i].getHashCode()) {
				changed = true;
				break;
			}
		}
	}

	 this->ring = curMemList;
	 //cout<<"updateRing: after ring creation\n";
	//get the nodes of replicas from the ring from the keys
	//if the number is less than 2 then run stabilization protocol

	if (changed)
	  stabilizationProtocol();
	//finding replicas from the old ring

}

/**
 * FUNCTION NAME: getMemberhipList
 *
 * DESCRIPTION: This function goes through the membership list from the Membership protocol/MP1 and
 * 				i) generates the hash code for each member
 * 				ii) populates the ring member in MP2Node class
 * 				It returns a vector of Nodes. Each element in the vector contain the following fields:
 * 				a) Address of the node
 * 				b) Hash code obtained by consistent hashing of the Address
 */
vector<Node> MP2Node::getMembershipList() {
		//tracing this function
	this->trace->funcEntry("getMembershipList");
	unsigned int i;
	vector<Node> curMemList;
	for ( i = 0 ; i < this->memberNode->memberList.size(); i++ ) {
		Address addressOfThisMember;
		int id = this->memberNode->memberList.at(i).getid();
		short port = this->memberNode->memberList.at(i).getport();
		memcpy(&addressOfThisMember.addr[0], &id, sizeof(int));
		memcpy(&addressOfThisMember.addr[4], &port, sizeof(short));
		curMemList.emplace_back(Node(addressOfThisMember));
	}
	return curMemList;
}

void MP2Node::printNode(vector<Node> &list){
	int len = list.size();
	int i;
	for (i=0;i<len;i++){
		printAddress(&list[i].nodeAddress);
	}
}

void MP2Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;
}
/**
 * FUNCTION NAME: hashFunction
 *
 * DESCRIPTION: This functions hashes the key and returns the position on the ring
 * 				HASH FUNCTION USED FOR CONSISTENT HASHING
 *
 * RETURNS:
 * size_t position on the ring
 */
size_t MP2Node::hashFunction(string key) {
	std::hash<string> hashFunc;
	size_t ret = hashFunc(key);
	return ret%RING_SIZE;
}

int MP2Node::setTransTable(int transID, string key, MessageType type, string value){
	int id = transID;
	auto tp = new transaction_performed{
		type          : type,
    timestamp     : par->getcurrtime(),
		key           : key,
		value         : value,
		success_count : 0,
		failure_count : 0,
		is_logged     : 0
	};
	transID_map.emplace(id, tp);
	g_transID++;
	return id;
}
/**
 * FUNCTION NAME: clientCreate
 *
 * DESCRIPTION: client side CREATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientCreate(string key, string value) {
	/*
	 * Implement this
	 */
		//tracing this function
	this->trace->funcEntry("clientCreate");

	//creating entry in the transactionTable
	int id;
	id = setTransTable(g_transID, key, CREATE, value);

	bool res = createKeyValue(key, value, PRIMARY);
	//logging the value1
	this->log->logCreateSuccess(&memberNode->addr, true, id, key, value);
	hasMyReplicas = findNodes(key);
	int temp_count = 0;
	Address *tempAddr;
	for(Node node: hasMyReplicas){
    Message *newcreatemsg = new Message(id, this->memberNode->addr, CREATE, key, value, ReplicaType(temp_count++));
    tempAddr = &node.nodeAddress;
    emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, sizeof(Message));
  }
}

/**
 * FUNCTION NAME: clientRead
 *
 * DESCRIPTION: client side READ API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientRead(string key){
	/*
	 * Implement this
	 */
	 int id = setTransTable(g_transID, key, READ, "");
   Message *newcreatemsg = new Message(id, this->memberNode->addr, READ, key);
	 //Finds the replicas of the key
   hasMyReplicas = findNodes(key);
	 for(Node node: hasMyReplicas){
     Address *tempAddr = &node.nodeAddress;
     emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, sizeof(Message));
   }
}

/**
 * FUNCTION NAME: clientUpdate
 *
 * DESCRIPTION: client side UPDATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientUpdate(string key, string value){
	/*
	 * Implement this
	 */
	 int id = setTransTable(g_transID, key, UPDATE, value);
	 int replica_count = 0;
	 hasMyReplicas = findNodes(key);
	 for(Node node: hasMyReplicas){
		 Message *newcreatemsg = new Message(id, this->memberNode->addr, UPDATE, key, value, ReplicaType(replica_count++));
	   Address *tempAddr = &node.nodeAddress;
		 emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, sizeof(Message));
	 }
}

/**
 * FUNCTION NAME: clientDelete
 *
 * DESCRIPTION: client side DELETE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientDelete(string key){
	/*
	 * Implement this
	 */

	 //create the transactionTable
	 int id = setTransTable(g_transID, key, DELETE, "");
	 Message *newcreatemsg = new Message(id, this->memberNode->addr, DELETE, key);
   hasMyReplicas = findNodes(key);
   for(Node node: hasMyReplicas){
     Address *tempAddr = &node.nodeAddress;
     emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, sizeof(Message));
  }
}

/**
 * FUNCTION NAME: createKeyValue
 *
 * DESCRIPTION: Server side CREATE API
 * 			   	The function does the following:
 * 			   	1) Inserts key value into the local hash table
 * 			   	2) Return true or false based on success or failure
 */
bool MP2Node::createKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Insert key, value, replicaType into the hash table
	bool res = ht->create(key, value);
	if (res){
	  this->entry = new Entry(value, par->getcurrtime(), replica);
	}
	this->trace->funcEntry("createKeyValue");
	return res;
}

/**
 * FUNCTION NAME: readKey
 *
 * DESCRIPTION: Server side READ API
 * 			    This function does the following:
 * 			    1) Read key from local hash table
 * 			    2) Return value
 */
string MP2Node::readKey(string key) {
	/*
	 * Implement this
	 */
	 string key_read = ht->read(key);
	 this->trace->funcEntry("readKey");
	 return key_read;
}

/**
 * FUNCTION NAME: updateKeyValue
 *
 * DESCRIPTION: Server side UPDATE API
 * 				This function does the following:
 * 				1) Update the key to the new value in the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::updateKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	bool res;
	res = ht->update(key, value);
	//updating the entry for the transaction
	if (res){
		entry->value = value;
		entry->timestamp = par->getcurrtime();
		entry->replica = replica;
	}
	this->trace->funcEntry("updateKeyValue");
	return res;
}

/**
 * FUNCTION NAME: deleteKey
 *
 * DESCRIPTION: Server side DELETE API
 * 				This function does the following:
 * 				1) Delete the key from the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::deletekey(string key) {
	/*
	 * Implement this
	 */
	bool res;
	res = ht->deleteKey(key);
	return res;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: This function is the message handler of this node.
 * 				This function does the following:
 * 				1) Pops messages from the queue
 * 				2) Handles the messages according to message types
 */
void MP2Node::checkMessages() {
	/*
	 * Implement this. Parts of it are already implemented
	 */

	char * data;
	int size;
	this->trace->funcEntry("checkMessages");
	/*
	 * Declare your local variables here
	 */

	// dequeue all messages and handle them
	while ( !memberNode->mp2q.empty() ) {
		/*
		 * Pop a message from the queue
		 */
		data = (char *)memberNode->mp2q.front().elt;
		size = memberNode->mp2q.front().size;
		memberNode->mp2q.pop();

		Message *msg = (Message *) data;
		string message(data, data + size);

		//coordinator addresses
		Address *coordinatorAddr = new Address;
		coordinatorAddr = &msg->fromAddr;
		/*
		 * Handle the message types here
		 */
		bool is_coodinator = ((msg->fromAddr == memberNode->addr));
		//cout<<"This is coordinator = "<<is_coodinator<<"\n";
		string read_message;
		switch(msg->type){
		  case CREATE:{
			  bool res = false;
				Address *tempAddr = &memberNode->addr;
				res = createKeyValue(msg->key, msg->value, msg->replica);
				if (is_coodinator){
				  if (res){
					  this->log->logCreateSuccess(&memberNode->addr, true, msg->transID, msg->key, msg->value);
				  } else {
					  this->log->logCreateFail(&memberNode->addr, true, msg->transID, msg->key, msg->value);
					}
				} else {
				  if (res){
						this->log->logCreateSuccess(&memberNode->addr, false, msg->transID, msg->key, msg->value);
					} else {
						  this->log->logCreateFail(&memberNode->addr, false, msg->transID, msg->key, msg->value);
					}
			  }
		  }
		  break;
			case READ:{
			  string tempstr = msg->toString();
				cout<<"READ: message = "<<tempstr<<"\n";
				read_message = readKey(msg->key);
				cout<<"READ: Read message = "<<read_message<<"\n";
				if (read_message != ""){
					 //send READREPLY
					 Message *newcreatemsg = new Message(msg->transID, memberNode->addr, read_message);
					 cout<<"READ: newcreatemsg = "<<newcreatemsg->toString()<<"\n";
					 newcreatemsg->success = 1;
					 this->log->logReadSuccess(&msg->fromAddr, false, msg->transID, msg->key, msg->value);
					 emulNet->ENsend(&memberNode->addr, &msg->fromAddr, (char *)newcreatemsg, sizeof(Message));
				} else {
				  //send fail ReadReply
					Message *newcreatemsg = new Message(msg->transID, this->memberNode->addr, read_message);
					newcreatemsg->success = 0;
					this->log->logReadFail(&msg->fromAddr, false, msg->transID, msg->key);
					emulNet->ENsend(&memberNode->addr, &msg->fromAddr, (char *)newcreatemsg, sizeof(Message));
				}
		  }
			break;
			case UPDATE:
			{
			  //try updating the key in this node
				int res_update = updateKeyValue(msg->key, msg->value, msg->replica);
				if (res_update){
				  Message *newcreatemsg = new Message(msg->transID, this->memberNode->addr, REPLY, true);
					cout<<"UPDATE: message = "<<newcreatemsg->toString()<<"\n";
					Address *tempAddr = &msg->fromAddr;
					emulNet->ENsend(&memberNode->addr,tempAddr,(char *)newcreatemsg, sizeof(Message));
				 } else{
				   cout<<"UPDATE: message absent, sending Failure REPLY\n";
					 //this->log->logUpdateFail(&memberNode->addr, false, msg->transID, msg->key, msg->value);
					 Message *newcreatemsg = new Message(msg->transID, this->memberNode->addr, REPLY, false);
					 Address *tempAddr = &msg->fromAddr;
				   emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, sizeof(Message));
				 }
			} //case UPDATE
			break;
			case DELETE:
			{
			  cout<<"DELETE: msg->key = "<<msg->key<<"\n";
				//deleting the keys
				bool res_del = this->deletekey(msg->key);
				if (res_del){
				  //sending the reply message
					Message *newcreatemsg = new Message(msg->transID, this->memberNode->addr, REPLY, true);
					Address *tempAddr = &msg->fromAddr;
					emulNet->ENsend(&memberNode->addr,tempAddr,(char *)newcreatemsg, sizeof(Message));
				} else {
				  cout<<"DELETE: message absent, sending Failure REPLY\n";
					Message *newcreatemsg = new Message(msg->transID, this->memberNode->addr, REPLY, false);
					Address *tempAddr = &msg->fromAddr;
					emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, sizeof(Message));
				}
			}
			break;
			case REPLY:
			{
			  cout<<"REPLY: message = "<<msg->toString()<<"\n";
				Address *tempAddr = &this->memberNode->addr;
        cout<<"REPLY: this node address =";
			  printAddress(tempAddr);
				tempAddr = &msg->fromAddr;
				cout<<"REPLY: from Address =";
				printAddress(tempAddr);
				cout<<"\n";

				map<int, transaction_performed*>::iterator search;
				search = transID_map.find(msg->transID);
				if ((par->getcurrtime() - search->second->timestamp) > 10){
					search->second->failure_count++;
				} else {
					if (!msg->success){
						search->second->failure_count++;
					} else {
						if(msg->success){
							search->second->timeoutNode.push_back(msg->fromAddr);
							search->second->success_count++;
						}
					}
				}
				//checking for quorum
				cout<<"Checking the quorum\n";
				if ((search->second->failure_count + search->second->success_count) >=2
				    &&(search->second->success_count >= 2 || search->second->failure_count >= 2)) {
							if (search->second->success_count >= 2){
					  //quorum received
						 switch(search->second->type){
							 case UPDATE:
								 //for each node set log
								 hasMyReplicas = findNodes(search->second->key);
								   for(Node node: hasMyReplicas){
										 //find the timeout node1
 									   this->updateKeyValue(msg->key, msg->value, msg->replica);
										 if (!search->second->is_logged){
											   //server success message
												 if (find(search->second->timeoutNode.begin(), search->second->timeoutNode.end(), node.nodeAddress) != search->second->timeoutNode.end()){
													 this->log->logUpdateSuccess(&node.nodeAddress, false, msg->transID, search->second->key, search->second->value);
												 }
									   }
									 }
									 //coordinator successful
									 if (!search->second->is_logged)
									   this->log->logUpdateSuccess(&memberNode->addr, true, msg->transID, search->second->key, search->second->value);
									 search->second->is_logged = 1;
							 break;
							 case DELETE:
							   cout<<"logging success of the delete\n";
								 hasMyReplicas = findNodes(search->second->key);
								   for(Node node: hasMyReplicas){
										 cout<<"Node Address = ";
										 Address *tempAddr = &node.nodeAddress;
										 printAddress(tempAddr);
									   this->deletekey(search->second->key);
										 if (!search->second->is_logged){
										   //server delete
											 //if (find(search->second->timeoutNode.begin(), search->second->timeoutNode.end(), node.nodeAddress) != search->second->timeoutNode.end()){
												 this->log->logDeleteSuccess(&node.nodeAddress, false, msg->transID, search->second->key);
											 //}
										 }
									 }
									 if (!search->second->is_logged)
									   this->log->logDeleteSuccess(&memberNode->addr, true, msg->transID, search->second->key);
									 search->second->is_logged = 1;
							 break;
						 }
					 } else {
					   //quorum Failed
						 if (search->second->failure_count >= 2){
						 switch(search->second->type){
						   case UPDATE:
							   hasMyReplicas = findNodes(search->second->key);
							     for(Node node: hasMyReplicas){
										 if (!search->second->is_logged){
											 //replica failed logging
											   this->log->logUpdateFail(&node.nodeAddress, false, msg->transID, search->second->key, search->second->value);
									 }
								 }
								 //coordinator successful
								 if (!search->second->is_logged)
								   this->log->logUpdateFail(&memberNode->addr, true, msg->transID, search->second->key, search->second->value);
								 search->second->is_logged = 1;
							 break;
							 case DELETE:
							   hasMyReplicas = findNodes(search->second->key);
								 for(Node node: hasMyReplicas){
									 if (!search->second->is_logged){
										 //replica fail
										   this->log->logDeleteFail(&node.nodeAddress, false, msg->transID, search->second->key);
									 }
								 }
								 if (!search->second->is_logged)
								   this->log->logDeleteFail(&memberNode->addr, true, msg->transID, search->second->key);
								 search->second->is_logged = 1;
							 break;
				     } //switch(search->second->type)
					 } //if (search->second->failure_count >= 2)
				   }
				 }
			 }	//REPLY
			 break;
			 case READREPLY:
	     {
			   cout<<"ReadReply from the coordinator to the REPLY\n";
				 //check QUORUM
				 map<int, transaction_performed*>::iterator search;
				 search = transID_map.find(msg->transID);
				 if (!msg->success || par->getcurrtime() - search->second->timestamp > 10){
 					search->second->failure_count++;
 				} else {
 					if(msg->success)
 					  search->second->success_count++;
 				}
				if ((search->second->failure_count + search->second->success_count) >=2
						|| search->second->success_count >= 2
						|| search->second->failure_count >= 2) {
					if (search->second->success_count >= 2){
					//void logReadSuccess(Address * address, bool isCoordinator, int transID, string key, string value);
						this->log->logReadSuccess(&msg->fromAddr, true, msg->transID, search->second->key, msg->value);
					} else {
						this->log->logReadFail(&msg->fromAddr, true, msg->transID, search->second->key);
					}
				}
	       break;
	     } //ReadReply
	 } //switch

	 this->trace->funcEntry("Leaving checkMessages");
   //cout<<"Leaving checkMessages\n";
	}

	/*
	 * This function should also ensure all READ and UPDATE operation
	 * get QUORUM replies
	 */
}

/**
 * FUNCTION NAME: findNodes
 *
 * DESCRIPTION: Find the replicas of the given keyfunction
 * 				This function is responsible for finding the replicas of a key
 */
vector<Node> MP2Node::findNodes(string key) {
	size_t pos = hashFunction(key);
	vector<Node> addr_vec;
	if (ring.size() >= 3) {
		// if pos <= min || pos > max, the leader is the min
		if (pos <= ring.at(0).getHashCode() || pos > ring.at(ring.size()-1).getHashCode()) {
			addr_vec.emplace_back(ring.at(0));
			addr_vec.emplace_back(ring.at(1));
			addr_vec.emplace_back(ring.at(2));
		}
		else {
			// go through the ring until pos <= node
			for (int i=1; i<ring.size(); i++){
				Node addr = ring.at(i);
				if (pos <= addr.getHashCode()) {
					addr_vec.emplace_back(addr);
					addr_vec.emplace_back(ring.at((i+1)%ring.size()));
					addr_vec.emplace_back(ring.at((i+2)%ring.size()));
					break;
				}
			}
		}
	}
	return addr_vec;
}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: Receive messages from EmulNet and push into the queue (mp2q)
 */
bool MP2Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), this->enqueueWrapper, NULL, 1, &(memberNode->mp2q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue of MP2Node
 */
int MP2Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}
/**
 * FUNCTION NAME: stabilizationProtocol
 *
 * DESCRIPTION: This runs the stabilization protocol in case of Node joins and leaves
 * 				It ensures that there always 3 copies of all keys in the DHT at all times
 * 				The function does the following:
 *				1) Ensures that there are three "CORRECT" replicas of all the keys in spite of failures and joins
 *				Note:- "CORRECT" replicas implies that every key is replicated in its two neighboring nodes in the ring
 */
void MP2Node::stabilizationProtocol() {
	/*
	 * Implement this
	 */
	 cout<<"Inside stabilizationProtocol\n";
	 this->trace->funcEntry("stabilizationProtocol");
   std::map<string, string>::iterator it;
	 vector<Node>num_replicas;
	 for (it = ht->hashTable.begin(); it != ht->hashTable.end();it++){
		 num_replicas = findNodes(it->first);
		 for(auto replica: num_replicas){
			 Message *newcreatemsg = new Message(-1, this->memberNode->addr, CREATE, it->first, it->second);
			 //send create message to the node1
			 emulNet->ENsend(&memberNode->addr, &replica.nodeAddress, (char *)newcreatemsg, sizeof(Message));
		 }
	 } //for each num_replicas
}
