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
	this->cmdtb = new CommandTable(par);
	int dbg_cnt = 0;
}

/**
 * Destructor
 */
MP2Node::~MP2Node() {
	delete ht;
	delete memberNode;
	delete cmdtb;
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

	cmdtb->setTransId(key, g_transID);

	bool res = createKeyValue(key, value, PRIMARY);
	//logging the value1
	this->log->logCreateSuccess(&memberNode->addr, true, g_transID, key, value);
	cout<<"clientCreate: setting trans_d = "<<cmdtb->getTransId(key)<<" to key = "<<key<<"\n";

	Address *tempAddr = &this->memberNode->addr;
	cout<<"clientCreate: This Node = ";
	printAddress(tempAddr);

	hasMyReplicas = findNodes(key);
    //construct the createmessage
		int temp_count = 0;

		for(Node node: hasMyReplicas){
		  cout<<"clientCreate: Replica in vector = ";
			tempAddr = &node.nodeAddress;
			printAddress(tempAddr);
    Message *newcreatemsg = new Message(g_transID, this->memberNode->addr, CREATE, key, value, ReplicaType(temp_count++));
		//g_transID++;
    	tempAddr = &node.nodeAddress;
      emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, sizeof(Message));
    }
		g_transID++;
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
	 cout<<"thisNode = ";
	 Address *tempAddr = &memberNode->addr;
	 printAddress(tempAddr);
	 cout<<"inside clientRead\n";
	 cout<<"clientRead: key = "<<key<<"\n";
	 int transaction_id = cmdtb->getTransId(key);
	 cout<<"clientRead: transaction_id = "<<transaction_id<<"\n";
	 if ( transaction_id != -1 ){
		 cout<<"Sending the READ message\n";
    Message *newcreatemsg = new Message(transaction_id, this->memberNode->addr, READ, key);
		cmdtb->setTrans(key, READ);
		//Finds the replicas of the key
    hasMyReplicas = findNodes(key);
		//printNode(hasMyReplicas);
    //sends a message to the replicas
    for(Node node: hasMyReplicas){
    	Address *tempAddr = &node.nodeAddress;
      emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, sizeof(Message));
    }
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
	   cout<<"thisNode = ";
		 Address *tempAddr = &memberNode->addr;
		 printAddress(tempAddr);
	   cout<<"clientUpdate: coordinator client\n";
     int transaction_id = cmdtb->getTransId(key);
		 cout<<"clientUpdate: transID to search = "<<transaction_id<<"\n";
		 cout<<"clientUpdate: key to search = "<<key<<"\n";

			if ( transaction_id != -1 ){

				Message *newcreatemsg = new Message(transaction_id, this->memberNode->addr, UPDATE, key, value);
				cmdtb->setTrans(key, UPDATE);

				int msgSize = sizeof(MessageType) //message type = UPDATE
		                  + 2*sizeof(string)  //key, value
		                  + sizeof(Address)   //address
		                  + sizeof(int);      //transID
		    //Finds the replicas of the key
		    //vector<Node> repNode;
		    hasMyReplicas = findNodes(key);
		    for(Node node: hasMyReplicas){
		    	Address *tempAddr = &node.nodeAddress;
		      emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, msgSize);
		    }
			} else {
				//key not found
				this->log->logUpdateFail(&memberNode->addr, true, transaction_id, key, value);
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
	 cout<<"thisNode = ";
	 Address *tempAddr = &memberNode->addr;
	 printAddress(tempAddr);
	 int transaction_id = cmdtb->getTransId(key);
	 cout<<"clientDelete: transID to search = "<<transaction_id<<"\n";
	 cout<<"clientDelete: key to search = "<<key<<"\n";
	 cout<<"\n";
	 //delete the key from the coordinatorAddr
	 //int dbg_cnt = 0;
	 if ( transaction_id != -1 ){
		 //dbg_cnt++;
		 //this->log->logDeleteSuccess(&memberNode->addr, false, transaction_id, key);
		 //this->deletekey(key);
	   Message *newcreatemsg = new Message(transaction_id, this->memberNode->addr, DELETE, key);
		 cmdtb->setTrans(key, DELETE);
     hasMyReplicas = findNodes(key);
     for(Node node: hasMyReplicas){
    	 Address *tempAddr = &node.nodeAddress;
       emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, sizeof(Message));
     }
	 } else { // if ( transaction_id != -1 )
		 //this->log->logDeleteFail()
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
  //this->trace->funcEntry("createKeyValue");
	//cout<<"creating entry for node =";
	//Address *tempAddr = &this->memberNode->addr;
	//printAddress(tempAddr);
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
	map <int, int> success_calc; //for transID, count
	map <int, int> failure_calc; //for transID, count

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
				//if (!(this->getMemberNode()->bFailed)){
					//cout<<"memberNode = ";
					Address *tempAddr = &memberNode->addr;
					//printAddress(tempAddr);
					//cout<<"CREATE: msg->key = "<<msg->key<<" trans_id = "<<msg->transID<<"\n";
					cmdtb->setTransId(msg->key, msg->transID);
				  res = createKeyValue(msg->key, msg->value, msg->replica);
				//}
				if (is_coodinator){
					if (res){
						//this->hasMyReplicas = findNodes(msg->key);
					  this->log->logCreateSuccess(&memberNode->addr, true, msg->transID, msg->key, msg->value);
					} else {
						this->log->logCreateFail(&memberNode->addr, true, msg->transID, msg->key, msg->value);
					}
				} else {
					if (res){
						  //this->haveReplicasOf = findNodes(msg->key);
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
					  Message *newcreatemsg = new Message(msg->transID, memberNode->addr, REPLY, true);
					  cout<<"READ: newcreatemsg = "<<newcreatemsg->toString()<<"\n";
						emulNet->ENsend(&memberNode->addr, &msg->fromAddr, (char *)newcreatemsg, sizeof(Message));
					} else {
						Message *newcreatemsg = new Message(msg->transID, this->memberNode->addr, REPLY, false);
						emulNet->ENsend(&memberNode->addr, &msg->fromAddr, (char *)newcreatemsg, sizeof(Message));
					}

		   }
			 break;
			 case UPDATE:
			 {
			   cout<<"UPDATE: msg->key = "<<msg->key<<"\n";
				 int transaction_id = cmdtb->getTransId(msg->key);

				 cout<<"UPDATE: transaction_id = "<<transaction_id<<"\n";
				 //cout<<"size of haveReplicasOf = "<<haveReplicasOf.size()<<"\n";
				 if (transaction_id != -1){
					 cmdtb->setTrans(msg->key, UPDATE);
					 cout<<"UPDATE: message present, sending Success REPLY\n";
					 Message *newcreatemsg = new Message(transaction_id, this->memberNode->addr, REPLY, true);
					 cout<<"UPDATE: message = "<<newcreatemsg->toString()<<"\n";
					 //cout<<"UPDATE: msgSize = "<<msgSize<<"\n";
					 cout<<"Sending REPLY to address = ";
					 Address *tempAddr = &msg->fromAddr;
					 printAddress(tempAddr);
					 cout<<"UPDATE: sending success as = "<<newcreatemsg->success<<"\n";
					 emulNet->ENsend(&memberNode->addr,tempAddr,(char *)newcreatemsg, sizeof(Message));
				 } else{
					 cout<<"UPDATE: message absent, sending Failure REPLY\n";
					 Message *newcreatemsg = new Message(transaction_id, this->memberNode->addr, REPLY, false);
					 Address *tempAddr = &msg->fromAddr;
					 emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, sizeof(Message));
				}
			 } //case UPDATE
			 break;
			 case DELETE:
			 {
				 cout<<"DELETE: msg->key = "<<msg->key<<"\n";

				 int transaction_id = cmdtb->getTransId(msg->key);
				 //cout<<"DELETE: transaction_id = "<<transaction_id<<"\n";

				 if (transaction_id != -1){
					 //deleting the keys
					 this->deletekey(msg->key);
					 cmdtb->setTrans(msg->key, DELETE);
					 //this->log->logDeleteSuccess(&memberNode->addr, false, transaction_id, msg->key);
					 //sending the reply message
					 Message *newcreatemsg = new Message(transaction_id, this->memberNode->addr, REPLY, true);
					 Address *tempAddr = &msg->fromAddr;
					 cout<<"sending REPLY to=";
					 printAddress(tempAddr);
					 cout<<"\n";
					 //cout<<"DELETE: sending success as = "<<newcreatemsg->success<<"\n";
					 emulNet->ENsend(&memberNode->addr,tempAddr,(char *)newcreatemsg, sizeof(Message));
				 } else {
					 cout<<"DELETE: message absent, sending Failure REPLY\n";
					 //this->log->logDeleteFail(&memberNode->addr, false, transaction_id, msg->key);
					 Message *newcreatemsg = new Message(msg->transID, this->memberNode->addr, REPLY, false);
					 Address *tempAddr = &msg->fromAddr;
					 emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, sizeof(Message));
				 }
			 }
			 break;
			 case REPLY:
			 {
				 //check timeOutCounter at the coordinatorAddr
				 //this->checkTimeout(msg);
				 //cout<<"REPLY: dbg_cnt = "<<dbg_cnt<<"\n";
				 cout<<"REPLY: message = "<<msg->toString()<<"\n";
				 cout<<"msg->transID = "<<msg->transID<<"\n";
				 Address *tempAddr = &this->memberNode->addr;
         cout<<"REPLY: this node address =";
			   printAddress(tempAddr);
				 tempAddr = &msg->fromAddr;
				 cout<<"REPLY: from Address =";
				 printAddress(tempAddr);
				 string temp_key = cmdtb->getKeyFromId(msg->transID);
				 cout<<"REPLY: temp_key = "<<temp_key<<"\n";

			   cout<<"Reply message to the Coordinator\n";
			   cout<<"REPLY: msg->success = "<<(msg->success)<<"\n";


				 cout<<"REPLY: cmdtb->getTrans(temp_key) = "<<cmdtb->getTrans(temp_key)<<"\n";
				 cout<<"\n";
			   if (cmdtb->getTrans(temp_key) == UPDATE
				   || cmdtb->getTrans(temp_key) == DELETE
					 || cmdtb->getTrans(temp_key) == READ)
			   {

					 //no quorum for deleteTest
					 // if (cmdtb->getTrans(temp_key) == DELETE){
						//  cout<<"logging success of the delete\n";
						// 	this->log->logDeleteSuccess(&memberNode->addr, true, msg->transID, temp_key);
						// 	this->deletekey(temp_key);
					 // }

				   cout<<"par->getcurrtime() = "<<par->getcurrtime()<<"\n";
				   cout<<"cmdtb->getTransTimestamp(temp_key) = "<<cmdtb->getTransTimestamp(temp_key)<<"\n";
				   if (!msg->success || (par->getcurrtime() - cmdtb->getTransTimestamp(temp_key)) > 10){
					 cmdtb->setFailureCount(temp_key);
				 } else {
					 cmdtb->setsuccessCount(temp_key);
				 }

				 //checking for quorum
				 cout<<"Checking the quorum\n";
				 int temp_failure = cmdtb->getFailureCount(temp_key);
				 int temp_success = cmdtb->getsuccessCount(temp_key);
				 cout<<"REPLY: temp_failure = "<<temp_failure<<"\n";
				 cout<<"REPLY: temp_success = "<<temp_success<<"\n";

				//no quorum check for deleteTest
				if (cmdtb->getTrans(temp_key) == DELETE){
					this->log->logDeleteSuccess(&memberNode->addr, true, msg->transID, temp_key);
					this->deletekey(temp_key);
					//client delete
					this->log->logDeleteSuccess(&msg->fromAddr, false, msg->transID, temp_key);
				}

				 if ((temp_failure + temp_success) >=2 || temp_success >= 2 || temp_failure >= 2){
					 if (temp_success >= 2){
						 //quorum received
						 //sending READREPLY as success
						 Message *newcreatemsg = new Message(msg->transID, this->memberNode->addr, msg->value);
						 int msgSize =   sizeof(Address)
														 + sizeof(int)
														 + sizeof(string);

						 //checking the transaction type and logging message
						 cout<<"REPLY: cmdtb->getTrans(temp_key) = "<<cmdtb->getTrans(temp_key)<<"\n";
						 switch(cmdtb->getTrans(temp_key)){
							 case UPDATE:
								this->log->logUpdateSuccess(&memberNode->addr, true, msg->transID, temp_key, msg->value);
								//client Fail
								this->log->logUpdateSuccess(&msg->fromAddr, false, msg->transID, temp_key, msg->value);
								this->updateKeyValue(temp_key, msg->value, msg->replica);
								//sends a REPLY message to the replica
							 emulNet->ENsend(&memberNode->addr, &msg->fromAddr, (char *)newcreatemsg, sizeof(Message));
							 break;
							 case READ:
								 //no need to send the ReadReply. just log the success
								 //coordinator logReadSuccess
								 this->log->logReadSuccess(&memberNode->addr, true, msg->transID, temp_key, msg->value);
								 //replica logReadSuccess
								 this->log->logReadSuccess(&msg->fromAddr, false, msg->transID, temp_key, msg->value);
							 break;
							 case DELETE:
							 cout<<"logging success of the delete\n";
								// this->log->logDeleteSuccess(&memberNode->addr, true, msg->transID, temp_key);
								// this->deletekey(temp_key);
								// //client delete
								// this->log->logDeleteSuccess(&msg->fromAddr, false, msg->transID, temp_key);
								//sends a REPLY message to the coordinator
							 //emulNet->ENsend(&memberNode->addr, &msg->fromAddr, (char *)newcreatemsg, sizeof(Message));
							 break;

						 }
					 } else {
						 //quorum Failed
						 //no need to send message. just log the failures

						 switch(cmdtb->getTrans(temp_key)){
							 case UPDATE:
								 //coordinator fail
								 this->log->logUpdateFail(&memberNode->addr, true, msg->transID, temp_key, msg->value);
								 //replica failed logging
								 this->log->logUpdateFail(&msg->fromAddr, false, msg->transID, temp_key, msg->value);
							 break;
							 case READ:
								 //coordinator failed
								 this->log->logReadFail(&memberNode->addr, true, msg->transID, temp_key);
								 //replica failed
								 this->log->logReadFail(&msg->fromAddr, false, msg->transID, temp_key);
							 break;

							 case DELETE:
								//coordinator fail
								this->log->logDeleteFail(&memberNode->addr, true, msg->transID, temp_key);
								//replica fail
								this->log->logDeleteFail(&msg->fromAddr, false, msg->transID, temp_key);
							 break;

							 }
						 }
					 } //if ((failure_calc.size() + success_calc.size()) == this->hasMyReplicas.size())
			   }	//		 if (tran_performed[msg->transID] == UPDATE || tran_performed[msg->transID] == DELETE|| tran_performed[msg->transID] == READ)
		   } //READ
			 break;
			 case READREPLY:
		   {
			 cout<<"ReadReply from the coordinator to the REPLY\n";
			 	if (msg->success){
					//log and update according to the transaction tran_performed
					switch(cmdtb->getTrans(msg->key)){
						case UPDATE:
						 this->log->logUpdateSuccess(&memberNode->addr, false, msg->transID, msg->key, msg->value);
						 //void logUpdateSuccess(Address * address, bool isCoordinator, int transID, string key, string newValue);
						 this->updateKeyValue(msg->key, msg->value, msg->replica);
						break;
						case READ:
						 //this->log->logReadSuccess(&memberNode->addr, true, msg->transID, msg->key, msg->value);
						break;
						case DELETE:
						 //this->log->logDeleteSuccess(&memberNode->addr, true, msg->transID, msg->key);
						 this->deletekey(msg->key);
						break;
					}
				} else{
				}
				cmdtb->removeTrans(msg->key);
				cmdtb->removesuccessCount(msg->key);
				cmdtb->removeFailureCount(msg->key);
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

void MP2Node::checkTimeout(Message *msg){
	//check for timeouts
	int tran_logged = cmdtb->getTransTimestamp(msg->key);
	cout<<"checkMessages: tran_logged = "<<tran_logged<<"\n";
	int current_time = par->getcurrtime();
	if ((current_time - tran_logged) > 10 || tran_logged == -1){
		int transaction_id = cmdtb->getTransId(msg->key);
		//send REPLY to the coordinatorAddr
		Message *newcreatemsg = new Message(transaction_id, this->memberNode->addr, REPLY, false);
		Address *tempAddr = &msg->fromAddr;
		emulNet->ENsend(&memberNode->addr, tempAddr, (char *)newcreatemsg, sizeof(Message));
	}
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
		 //get transID
		 //put g_transID to 0 and begin again
		 g_transID = 0;

		 //int temp_transId = cmdtb->getTransId(it->first);
		 for(auto replica: num_replicas){
			 if (replica.nodeAddress == memberNode->addr){
			 } else {
				 //create a createmessage
				 Message *newcreatemsg = new Message(g_transID, this->memberNode->addr, CREATE, it->first, it->second, this->entry->replica);
					 //send create message to the node1
					 emulNet->ENsend(&memberNode->addr, &replica.nodeAddress, (char *)newcreatemsg, sizeof(Message));
			 }
		 } //for each num_replicas
		 //this->hasMyReplicas = num_replicas;
		 g_transID++;
	 } //each entry in hashtable


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
