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
	 this->ring = curMemList;
	 //cout<<"updateRing: after ring creation\n";
	//get the nodes of replicas from the ring from the keys
	//if the number is less than 2 then run stabilization protocol

	vector<Node>num_replicas;
	//finding replicas from the old ring

	for(std::map<string,string>::iterator it = this->ht->hashTable.begin(); it != this->ht->hashTable.end();++it){
		num_replicas = findNodes(it->first);
		//cout<<"updateRing: after num_replicas\n";

 	  if (num_replicas.size() < 2){
 		  //run stabilization
			cout<<"Running stabilization protocol\n";
 		  stabilizationProtocol();
 		  //this will update the ring and replicas of the
 	  }
	}
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
	cout<<"g_transID = "<<g_transID<<"\n";

	hasMyReplicas = findNodes(key);
	if (DEBUGLOG){
		cout<<"clientCreate: replicas of the keys\n";
		printNode (hasMyReplicas);
	}
    //construct the createmessage
		int temp_count = 0;
		for(Node node: hasMyReplicas){
		cout<<"Starting sending create message\n";
    Message *newcreatemsg = new Message(g_transID, this->memberNode->addr, CREATE, key, value, ReplicaType(temp_count++));
		g_transID++;
		int msgSize = sizeof(MessageType)
                  + sizeof(ReplicaType)
                  + 2*sizeof(string)
                  + sizeof(Address)
                  + sizeof(int)
                  + sizeof(string);

    //Finds the replicas of the key
    //vector<Node> repNode;

    //sends a message to the replicas
    //memberNode->dispatchMessages(*newcreatemsg);

    	Address *tempAddr = &node.nodeAddress;
			if (DEBUGLOG){
				cout<<"clientCreate: sendint to address\n";
				printAddress (tempAddr);
			}

      emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, msgSize);
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
    Message *newcreatemsg;
    int msgSize = sizeof(MessageType)
                  + sizeof(ReplicaType)
                  + 2*sizeof(string)
                  + sizeof(Address)
                  + sizeof(int)
                  + sizeof(bool)
                  + sizeof(string);
    newcreatemsg->type     = READ;
    newcreatemsg->replica  = SECONDARY;
    newcreatemsg->key      = key;
    newcreatemsg->value    = "0";
    //creating address
    newcreatemsg->fromAddr = this->memberNode->addr;
    newcreatemsg->transID  = g_transID;
    newcreatemsg->success  = 1;
    newcreatemsg->delimiter= "::";
    //Finds the replicas of the key
    //vector<Node> repNode;
    hasMyReplicas = findNodes(key);
    //sends a message to the replicas
    //memberNode->dispatchMessages(*newcreatemsg);
    for(Node node: hasMyReplicas){
    	Address *tempAddr = &node.nodeAddress;
      emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, msgSize);
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

	  Message *newcreatemsg = new Message(g_transID, this->memberNode->addr, UPDATE, key, value);
		int msgSize = sizeof(MessageType) //message type = UPDATE
                  + 2*sizeof(string)  //key, value
                  + sizeof(Address)   //address
                  + sizeof(int);      //transID
    //Finds the replicas of the key
    //vector<Node> repNode;
    hasMyReplicas = findNodes(key);
    //sends a message to the replicas
    //memberNode->dispatchMessages(*newcreatemsg);
    for(Node node: hasMyReplicas){
    	Address *tempAddr = &node.nodeAddress;
      emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, msgSize);
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
	Message *newcreatemsg;
    int msgSize = sizeof(MessageType)
                  + sizeof(ReplicaType)
                  + 2*sizeof(string)
                  + sizeof(Address)
                  + sizeof(int)
                  + sizeof(bool)
                  + sizeof(string);
    newcreatemsg->type     = DELETE;
    newcreatemsg->replica  = this->entry->replica;
    newcreatemsg->key      = key;
    newcreatemsg->value    = "0";
    //creating address
    newcreatemsg->fromAddr = this->memberNode->addr;
    newcreatemsg->transID  = g_transID;
    newcreatemsg->success  = 1;
    newcreatemsg->delimiter= "::";
    //Finds the replicas of the key
    //vector<Node> repNode;
    hasMyReplicas = findNodes(key);
    //sends a message to the replicas
    //memberNode->dispatchMessages(*newcreatemsg);
    for(Node node: hasMyReplicas){
    	Address *tempAddr = &node.nodeAddress;
      emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, msgSize);
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
  this->trace->funcEntry("createKeyValue");
	bool res = ht->create(key, value);
	cout<<"Created local entry in the node\n";
	//adding it into the entry Object
	//Creating a new entry Object
	if (res){
	  this->entry = new Entry(value, par->getcurrtime(), replica);
	}
	//entering in its own haveReplicasOf
	//this->haveReplicasOf.push_back()


		//tracing this function
	this->trace->funcEntry("createKeyValue");
	cout<<"MOD2 : createKeyValue\n";
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
	// Read key from local hash table and return value

	 string key_read = ht->read(key);
		//tracing this function
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
	// Update key in local hash table and return true or false
		//tracing this function
	bool res;
	res = ht->update(key, value);
	//updating the entry for the transaction
	if (res){
		entry->value = value;
		entry->timestamp = g_transID;
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
	// Delete the key from the local hash table
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
		 bool is_coodinator = ((msg->fromAddr == memberNode->addr) && (msg->replica == PRIMARY));
		 //cout<<"This is coordinator = "<<is_coodinator<<"\n";
		 map <int, int> success_calc; //for transID, count
		 map <int, int> failure_calc; //for transID, count
		 map <int, MessageType> tran_performed;
		 string read_message;
		 switch(msg->type){
			 case CREATE:{
			 	//there are no replicas for create. create haveReplicasOf
				//this.hasMyReplicas = findNodes(message->key);

				//add the key to local Table

				//cout<<"is_coodinator = "<<is_coodinator<<"\n";
				bool res = false;
				if (!(this->getMemberNode()->bFailed)){
				  res = createKeyValue(msg->key, msg->value, msg->replica);
				}
				if (is_coodinator){

					if (res){
					  //clientCreate(message->key, message->value);
						//log the values
						//cout<<"Logging coordinator success message\n";
							this->log->logCreateSuccess(&memberNode->addr, true, msg->transID, msg->key, msg->value);
							//g_transID++;
					} else {
						//cout<<"Logging coordinator failure message\n";
						this->log->logCreateFail(&memberNode->addr, true, msg->transID, msg->key, msg->value);
					}
				} else {
					//not a coordinator
					//make a message and send reply that the node is ready to accept
					if (res){
						//cout<<"Logging copies success message\n";
						//clientCreate(message->key, message->value);
						//log the values
							this->log->logCreateSuccess(&memberNode->addr, false, msg->transID, msg->key, msg->value);
							//g_transID++;
					} else {
						//cout<<"Logging copies failure message\n";
						this->log->logCreateFail(&memberNode->addr, false, msg->transID, msg->key, msg->value);
					}
				}


		   }
		   break;
			 case READ:{
			 	//send message of read to the servers
				//clientRead(message->key);
				//if the read matches and more then 2 wuorum is formed
				read_message = readKey(msg->key);
				tran_performed.insert(std::pair<int, MessageType>(msg->transID, msg->type));
				//send REPLY if the transaction is found
				if (read_message != ""){
					//key present
					//send the REPLY
					Message *newcreatemsg;
					int msgSize = sizeof(MessageType)
													+ sizeof(ReplicaType)
													+ 2*sizeof(string)
													+ sizeof(Address)
													+ sizeof(int)
													+ sizeof(bool)
													+ sizeof(string);
						newcreatemsg->type     = REPLY;
						newcreatemsg->replica  = msg->replica;
						newcreatemsg->key      = msg->key;
						newcreatemsg->value    = msg->value;
						//creating address
						newcreatemsg->fromAddr = this->memberNode->addr;
						newcreatemsg->transID  = g_transID;
						newcreatemsg->success  = 1;
						newcreatemsg->delimiter= "::";
						//sends a REPLY message to the coordinator
						emulNet->ENsend(&memberNode->addr, &msg->fromAddr, (char *)newcreatemsg, msgSize);
					} else {
						Message *newcreatemsg;
						int msgSize = sizeof(MessageType)
														+ sizeof(ReplicaType)
														+ 2*sizeof(string)
														+ sizeof(Address)
														+ sizeof(int)
														+ sizeof(bool)
														+ sizeof(string);
							newcreatemsg->type     = REPLY;
							newcreatemsg->replica  = msg->replica;
							newcreatemsg->key      = msg->key;
							newcreatemsg->value    = msg->value;
							//creating address
							newcreatemsg->fromAddr = this->memberNode->addr;
							newcreatemsg->transID  = g_transID;
							newcreatemsg->success  = 0;
							newcreatemsg->delimiter= "::";
							//sends a REPLY message to the coordinator
							emulNet->ENsend(&memberNode->addr, &msg->fromAddr, (char *)newcreatemsg, msgSize);
					}

		   }
			 break;
			 case UPDATE:
			   //check if the replicas are present
				 //semd reply on receiving update if the given key is present.

				 read_message = readKey(msg->key);
				 tran_performed.insert(std::pair<int, MessageType>(msg->transID, msg->type));
				 if (read_message != ""){

					 Message *newcreatemsg;
					 int msgSize = sizeof(MessageType)
													 + sizeof(ReplicaType)
													 + 2*sizeof(string)
													 + sizeof(Address)
													 + sizeof(int)
													 + sizeof(bool)
													 + sizeof(string);
						 newcreatemsg->type     = REPLY;
						 newcreatemsg->replica  = msg->replica;
						 newcreatemsg->key      = msg->key;
						 newcreatemsg->value    = msg->value;
						 //creating address
						 newcreatemsg->fromAddr = this->memberNode->addr;
						 newcreatemsg->transID  = g_transID;
						 newcreatemsg->success  = 1;
						 newcreatemsg->delimiter= "::";
						 //sends a REPLY message to the coordinator
						 emulNet->ENsend(&memberNode->addr, &msg->fromAddr, (char *)newcreatemsg, msgSize);
				 } else{
					 Message *newcreatemsg;
					 int msgSize = sizeof(MessageType)
													 + sizeof(ReplicaType)
													 + 2*sizeof(string)
													 + sizeof(Address)
													 + sizeof(int)
													 + sizeof(bool)
													 + sizeof(string);
						 newcreatemsg->type     = REPLY;
						 newcreatemsg->replica  = msg->replica;
						 newcreatemsg->key      = msg->key;
						 newcreatemsg->value    = msg->value;
						 //creating address
						 newcreatemsg->fromAddr = this->memberNode->addr;
						 newcreatemsg->transID  = g_transID;
						 newcreatemsg->success  = 0;
						 newcreatemsg->delimiter= "::";
						 //sends a REPLY message to the coordinator
						 emulNet->ENsend(&memberNode->addr, &msg->fromAddr, (char *)newcreatemsg, msgSize);

				 }
			 break;
			 case REPLY:
			 //assumption , reply is sent only to coordinatorAddr
			 //update
			 //get quorum_calc
			 cout<<"Reply message to the Coordinator\n";
			 if (msg->success){
				 int flag = 0;
				 for (std::map<int,int>::iterator it=success_calc.begin(); it!=success_calc.end(); ++it){
					 if (it->first == msg->transID){
						 it->second++;
						 flag = 1;
					 }
				 }
				 if (flag == 0){
					 success_calc.insert(std::pair<int, int>(msg->transID, 1));
				 }
			 } else {
				 int flag = 0;
				 for (std::map<int,int>::iterator it=failure_calc.begin(); it!=failure_calc.end(); ++it){
					 if (it->first == msg->transID){
						 it->second++;
						 flag = 1;
					 }
				 }
				 if (flag == 0){
					 failure_calc.insert(std::pair<int, int>(msg->transID, 1));
				 }
			 }

			 //checking for quorum
			 if ((failure_calc.size() + success_calc.size()) == this->hasMyReplicas.size()){
				 //all messages received
				 if (success_calc.size() >= 2){
					 //quorum received
					 //sending READREPLY as success
					 Message *newcreatemsg;
					 int msgSize = sizeof(MessageType)
													 + sizeof(ReplicaType)
													 + 2*sizeof(string)
													 + sizeof(Address)
													 + sizeof(int)
													 + sizeof(bool)
													 + sizeof(string);
						 newcreatemsg->type     = READREPLY;
						 newcreatemsg->replica  = msg->replica;
						 newcreatemsg->key      = msg->key;
						 newcreatemsg->value    = msg->value;
						 //creating address
						 newcreatemsg->fromAddr = this->memberNode->addr;
						 newcreatemsg->transID  = g_transID;
						 newcreatemsg->success  = 1;
						 newcreatemsg->delimiter= "::";
						 //sends a REPLY message to the coordinator
						 emulNet->ENsend(&memberNode->addr, &msg->fromAddr, (char *)newcreatemsg, msgSize);


					 //checking the transaction type and logging message

					 switch(tran_performed[msg->transID]){
						 case UPDATE:
						 	this->log->logUpdateSuccess(&memberNode->addr, true, g_transID, msg->key, msg->value);
							this->updateKeyValue(msg->key, msg->value, msg->replica);
						 break;
						 case READ:
						 	this->log->logReadSuccess(&memberNode->addr, true, g_transID, msg->key, msg->value);
						 break;
						 case DELETE:
						 	this->log->logDeleteSuccess(&memberNode->addr, true, g_transID, msg->key);
							this->deletekey(msg->key);
						 break;
					 }
				 } else {
					 //quorum Failed
					 //sending failed READREPLY
					 Message *newcreatemsg;
					 int msgSize = sizeof(MessageType)
													 + sizeof(ReplicaType)
													 + 2*sizeof(string)
													 + sizeof(Address)
													 + sizeof(int)
													 + sizeof(bool)
													 + sizeof(string);
						 newcreatemsg->type     = READREPLY;
						 newcreatemsg->replica  = msg->replica;
						 newcreatemsg->key      = msg->key;
						 newcreatemsg->value    = msg->value;
						 //creating address
						 newcreatemsg->fromAddr = this->memberNode->addr;
						 newcreatemsg->transID  = g_transID;
						 newcreatemsg->success  = 0;
						 newcreatemsg->delimiter= "::";
						 //sends a REPLY message to the coordinator
						 emulNet->ENsend(&memberNode->addr, &msg->fromAddr, (char *)newcreatemsg, msgSize);
					 switch(tran_performed[msg->transID]){
						 case UPDATE:
							this->log->logUpdateFail(&memberNode->addr, true, g_transID, msg->key, msg->value);
						 break;
						 case READ:
							this->log->logReadFail(&memberNode->addr, true, g_transID, msg->key);
						 break;
						 case DELETE:
							this->log->logDeleteFail(&memberNode->addr, true, g_transID, msg->key);
						 break;
					 }
				 }
			 }

			 break;
			 case READREPLY:
			 cout<<"ReadReply from the coordinator to the REPLY\n";
			 	if (msg->success){
					//remove from success_calc and tran_performed
					map<int,int>::iterator it_success;
					it_success = success_calc.find(msg->transID);
					success_calc.erase(it_success);

					map <int, MessageType>::iterator it_trans;
					it_trans = tran_performed.find(msg->transID);


					//log and update according to the transaction tran_performed
					switch(tran_performed[msg->transID]){
						case UPDATE:
						 this->log->logUpdateSuccess(&memberNode->addr, true, g_transID, msg->key, msg->value);
						 this->updateKeyValue(msg->key, msg->value, msg->replica);
						break;
						case READ:
						 this->log->logReadSuccess(&memberNode->addr, true, g_transID, msg->key, msg->value);
						break;
						case DELETE:
						 this->log->logDeleteSuccess(&memberNode->addr, true, g_transID, msg->key);
						 this->deletekey(msg->key);
						break;
					}
					tran_performed.erase(it_trans);
				} else{
					//remove from failure_calc and tran_performed
					map<int,int>::iterator it_failure;
					it_failure = failure_calc.find(msg->transID);
					failure_calc.erase(it_failure);

					map <int, MessageType>::iterator it_trans;
					it_trans = tran_performed.find(msg->transID);


					switch(tran_performed[msg->transID]){
						case UPDATE:
						 this->log->logUpdateFail(&memberNode->addr, true, g_transID, msg->key, msg->value);
						break;
						case READ:
						 this->log->logReadFail(&memberNode->addr, true, g_transID, msg->key);
						break;
						case DELETE:
						 this->log->logDeleteFail(&memberNode->addr, true, g_transID, msg->key);
						break;
					}
					tran_performed.erase(it_trans);
				}
				break;
		 }
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
	 this->trace->funcEntry("stabilizationProtocol");
   std::map<string, string>::iterator it;
	 vector<Node>num_replicas;
	 for (it = ht->hashTable.begin(); it != ht->hashTable.end();it++){
		 num_replicas = findNodes(it->first);
		 int count_replica_type = 0;
		 //std::vector<Node>::iterator replica;
		 for(auto replica: num_replicas){
			 if (replica.nodeAddress == memberNode->addr){
			 } else {
			 //if (replica.nodeAddress != memberNode->addr){
				 //create a createmessage
				 Message *newcreatemsg;
				 int msgSize = sizeof(MessageType)
												 + sizeof(ReplicaType)
												 + 2*sizeof(string)
												 + sizeof(Address)
												 + sizeof(int)
												 + sizeof(bool)
												 + sizeof(string);
					 newcreatemsg->type     = CREATE;
					 newcreatemsg->replica  = ReplicaType(count_replica_type++);
					 newcreatemsg->key      = it->first;
					 newcreatemsg->value    = it->second;
					 //creating address
					 newcreatemsg->fromAddr = this->memberNode->addr;
					 newcreatemsg->transID  = g_transID;
					 newcreatemsg->success  = 1;
					 newcreatemsg->delimiter= "::";
					 //send create message to the node1
					 emulNet->ENsend(&memberNode->addr, &replica.nodeAddress, (char *)newcreatemsg, msgSize);
					 //emulNet->ENsend(&memberNode->addr, message->fromAddr, (char *)newcreatemsg, msgSize);
					 //assign replicas vector
					 this->hasMyReplicas.push_back(replica);
			 }
		 }
	 }


}

bool MP2Node::compareNode(vector<Node> &node1, vector<Node> &node2){
	int len = node1.size();
	int i;
	for(i=0;i<len;i++){
		if (node1[i].nodeAddress == node2[i].nodeAddress){

		} else{
			return false;
		}
	}
	return true;
}

bool MP2Node::check(){
	return true;
}
