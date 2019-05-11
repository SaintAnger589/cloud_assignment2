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

	/*
	 * Step 2: Construct the ring
	 */
	// Sort the list based on the hashCode

	sort(curMemList.begin(), curMemList.end());

	//change = (this->ring != curMemList);
	change = !compareNode(this->ring, curMemList);
	//construct the ring
	//update vector "ring" of the node
    if (change){
    	this->ring = curMemList;
    }

	/*
	 * Step 3: Run the stabilization protocol IF REQUIRED
	 */
	// Run stabilization protocol if the hash table size is greater than zero and if there has been a changed in the ring
	if (ht->currentSize() > 0 && change){
	  //this.ring = curMemList;
	  stabilizationProtocol();
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
    //construct the createmessage
    Message *newcreatemsg;
    int msgSize = sizeof(MessageType)
                  + sizeof(ReplicaType)
                  + 2*sizeof(string)
                  + sizeof(Address)
                  + sizeof(int)
                  + sizeof(bool)
                  + sizeof(string);
    newcreatemsg->type     = CREATE;
    newcreatemsg->replica  = SECONDARY;
    newcreatemsg->key      = key;
    newcreatemsg->value    = value;
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
	Message *newcreatemsg;
    int msgSize = sizeof(MessageType)
                  + sizeof(ReplicaType)
                  + 2*sizeof(string)
                  + sizeof(Address)
                  + sizeof(int)
                  + sizeof(bool)
                  + sizeof(string);
    newcreatemsg->type     = UPDATE;
    newcreatemsg->replica  = SECONDARY;
    newcreatemsg->key      = key;
    newcreatemsg->value    = value;
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

	ht->create(key, value);
	//adding it into the entry Object
	entry->value      = value;
	entry->timestamp  = g_transID;
	entry->replica    = replica;


		//tracing this function
	this->trace->funcEntry("createKeyValue");
	cout<<"MOD2 : createKeyValue\n";
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

		string message(data, data + size);

		//coordinator addresses
		Address *coordinatorAddr = new Address;
		coordinatorAddr = message->fromAddr;

		/*
		 * Handle the message types here
		 */
		 bool is_coodinator = (message->fromAddr == memberNode->addr);
		 map <int, int> success_calc; //for transID, count
		 map <int, int> failure_calc; //for transID, count
		 map <int, MessageType> tran_performed;
		 switch(message->type){
			 case CREATE:
			 	//there are no replicas for create. create haveReplicasOf
				//this.hasMyReplicas = findNodes(message->key);

				//add the key to local Table
				bool res = createKeyValue(message->key, message->value, message->replica);
				if (is_coodinator){
					if (res){
					  //clientCreate(message->key, message->value);
						//log the values
							this->log->logCreateSuccess(memberNode->addr, true, g_transID, message->key, message->value);
							g_transID++;
					} else {
						this->log->logCreateFail(memberNode->addr, true, g_transID, message->key, message->value);
					}
				} else {
					if (res){
						//clientCreate(message->key, message->value);
						//log the values
							this->log->logCreateSuccess(memberNode->addr, false, g_transID, message->key, message->value);
							g_transID++;
					} else {
						this->log->logCreateFail(memberNode->addr, false, g_transID, message->key, message->value);
					}
				}

			 break;
			 case READ:
			 	//send message of read to the servers
				//clientRead(message->key);
				//if the read matches and more then 2 wuorum is formed
				string read_message;
				read_message = readKey(message->key);
				tran_performed.insert(message->transID, message->type);
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
						newcreatemsg->replica  = message->replica;
						newcreatemsg->key      = key;
						newcreatemsg->value    = message->value;
						//creating address
						newcreatemsg->fromAddr = this->memberNode->addr;
						newcreatemsg->transID  = g_transID;
						newcreatemsg->success  = 1;
						newcreatemsg->delimiter= "::";
						//sends a REPLY message to the coordinator
						emulNet->ENsend(&memberNode->addr, message->fromAddr, (char *)newcreatemsg, msgSize);
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
							newcreatemsg->replica  = message->replica;
							newcreatemsg->key      = key;
							newcreatemsg->value    = message->value;
							//creating address
							newcreatemsg->fromAddr = this->memberNode->addr;
							newcreatemsg->transID  = g_transID;
							newcreatemsg->success  = 0;
							newcreatemsg->delimiter= "::";
							//sends a REPLY message to the coordinator
							emulNet->ENsend(&memberNode->addr, message->fromAddr, (char *)newcreatemsg, msgSize);
					}
			 break;
			 case UPDATE:
			   //check if the replicas are present
				 //semd reply on receiving update if the given key is present.
				 string read_message;
				 read_message = readKey(message->key);
				 tran_performed.insert(message->transID, message->type);
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
						 newcreatemsg->replica  = message->replica;
						 newcreatemsg->key      = key;
						 newcreatemsg->value    = message->value;
						 //creating address
						 newcreatemsg->fromAddr = this->memberNode->addr;
						 newcreatemsg->transID  = g_transID;
						 newcreatemsg->success  = 1;
						 newcreatemsg->delimiter= "::";
						 //sends a REPLY message to the coordinator
						 emulNet->ENsend(&memberNode->addr, message->fromAddr, (char *)newcreatemsg, msgSize);
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
						 newcreatemsg->replica  = message->replica;
						 newcreatemsg->key      = key;
						 newcreatemsg->value    = message->value;
						 //creating address
						 newcreatemsg->fromAddr = this->memberNode->addr;
						 newcreatemsg->transID  = g_transID;
						 newcreatemsg->success  = 0;
						 newcreatemsg->delimiter= "::";
						 //sends a REPLY message to the coordinator
						 emulNet->ENsend(&memberNode->addr, message->fromAddr, (char *)newcreatemsg, msgSize);
				 }
			 break;
			 case REPLY:
			 //assumption , reply is sent only to coordinatorAddr
			 //update
			 //get quorum_calc
			 if (message->success){
				 int flag = 0;
				 for (std::map<int,int>::iterator it=success_calc.begin(); it!=success_calc.end(); ++it){
					 if (it->first == message->transID){
						 it->second++;
						 flag = 1;
					 }
				 }
				 if (flag == 0){
					 success_calc.insert(message->transID, 1);
				 }
			 } else {
				 int flag = 0;
				 for (std::map<int,int>::iterator it=failure_calc.begin(); it!=failure_calc.end(); ++it){
					 if (it->first == message->transID){
						 it->second++;
						 flag = 1;
					 }
				 }
				 if (flag == 0){
					 failure_calc.insert(message->transID, 1);
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
						 newcreatemsg->replica  = message->replica;
						 newcreatemsg->key      = key;
						 newcreatemsg->value    = message->value;
						 //creating address
						 newcreatemsg->fromAddr = this->memberNode->addr;
						 newcreatemsg->transID  = g_transID;
						 newcreatemsg->success  = 1;
						 newcreatemsg->delimiter= "::";
						 //sends a REPLY message to the coordinator
						 emulNet->ENsend(&memberNode->addr, message->fromAddr, (char *)newcreatemsg, msgSize);

					 //checking the transaction type and logging message

					 switch(tran_performed[message->transID]){
						 case UPDATE:
						 	this->log->logUpdateSuccess(memberNode->addr, true, g_transID, message->key, message->value);
							this->updateKeyValue(message->key, message->value, message->replica);
						 break;
						 case READ:
						 	this->log->logReadSuccess(memberNode->addr, true, g_transID, message->key, message->value);
						 break;
						 case DELETE:
						 	this->log->logDeleteSuccess(memberNode->addr, true, g_transID, message->key, message->value);
							this->deletekey(message->key);
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
						 newcreatemsg->replica  = message->replica;
						 newcreatemsg->key      = key;
						 newcreatemsg->value    = message->value;
						 //creating address
						 newcreatemsg->fromAddr = this->memberNode->addr;
						 newcreatemsg->transID  = g_transID;
						 newcreatemsg->success  = 0;
						 newcreatemsg->delimiter= "::";
						 //sends a REPLY message to the coordinator
						 emulNet->ENsend(&memberNode->addr, message->fromAddr, (char *)newcreatemsg, msgSize);
					 switch(tran_performed[message->transID]){
						 case UPDATE:
							this->log->logUpdateFail(memberNode->addr, true, g_transID, message->key, message->value);
						 break;
						 case READ:
							this->log->logReadFail(memberNode->addr, true, g_transID, message->key, message->value);
						 break;
						 case DELETE:
							this->log->logDeleteFail(memberNode->addr, true, g_transID, message->key, message->value);
						 break;
					 }
				 }
			 }

			 break;
			 case READREPLY:
			 	if (message->success){
					//remove from success_calc and tran_performed
					map<int,int>::iterator it_success;
					it_success = success_calc.find(message->key);
					success_calc.erase(it_success);

					map <int, MessageType>::iterator it_trans;
					it_trans = tran_performed.find(message->key);
					tran_performed.erase(it_trans);

					//log and update according to the transaction tran_performed
					switch(tran_performed[message->transID]){
						case UPDATE:
						 this->log->logUpdateSuccess(memberNode->addr, true, g_transID, message->key, message->value);
						 this->updateKeyValue(message->key, message->value, message->replica);
						break;
						case READ:
						 this->log->logReadSuccess(memberNode->addr, true, g_transID, message->key, message->value);
						break;
						case DELETE:
						 this->log->logDeleteSuccess(memberNode->addr, true, g_transID, message->key, message->value);
						 this->deletekey(message->key);
						break;
					}
				} else{
					//remove from failure_calc and tran_performed
					map<int,int>::iterator it_failure;
					it_failure = failure_calc.find(message->key);
					failure_calc.erase(it_failure);

					map <int, MessageType>::iterator it_trans;
					it_trans = tran_performed.find(message->key);
					tran_performed.erase(it_trans);

					switch(tran_performed[message->transID]){
						case UPDATE:
						 this->log->logUpdateFail(memberNode->addr, true, g_transID, message->key, message->value);
						break;
						case READ:
						 this->log->logReadFail(memberNode->addr, true, g_transID, message->key, message->value);
						break;
						case DELETE:
						 this->log->logDeleteFail(memberNode->addr, true, g_transID, message->key, message->value);
						break;
					}
				}
		 }

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
	 //this is called when the nodelist is different
	 


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
