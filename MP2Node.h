/**********************************
 * FILE NAME: MP2Node.h
 *
 * DESCRIPTION: MP2Node class header file
 **********************************/

#ifndef MP2NODE_H_
#define MP2NODE_H_

/**
 * Header files
 */
#include "stdincludes.h"
#include "EmulNet.h"
#include "Node.h"
#include "HashTable.h"
#include "Log.h"
#include "Params.h"
#include "Message.h"
#include "Queue.h"
#include "Trace.h"
#include "CommandTable.h"

/**
 * CLASS NAME: MP2Node
 *
 * DESCRIPTION: This class encapsulates all the key-value store functionality
 * 				including:
 * 				1) Ring
 * 				2) Stabilization Protocol
 * 				3) Server side CRUD APIs
 * 				4) Client side CRUD APIs
 */
class MP2Node {
private:
	// Vector holding the next two neighbors in the ring who have my replicas
	vector<Node> hasMyReplicas;
	// Vector holding the previous two neighbors in the ring whose replicas I have
	vector<Node> haveReplicasOf;
	// Ring
	vector<Node> ring;
	// Hash Table
	HashTable * ht;
	// Member representing this member
	Member *memberNode;
	// Params object
	Params *par;
	// Object of EmulNet
	EmulNet * emulNet;
	// Object of Log
	Log * log;
	// Trace class object
	Trace *trace;
	// Entry Object
	Entry *entry;
	//for logging the command and key transID map
	CommandTable *cmdtb;

public:
	MP2Node(Member *memberNode, Params *par, EmulNet *emulNet, Log *log, Address *addressOfMember);
	Member * getMemberNode() {
		return this->memberNode;
	}

	// ring functionalities
	void updateRing();
	vector<Node> getMembershipList();
	size_t hashFunction(string key);

	// client side CRUD APIs
	void clientCreate(string key, string value);
	void clientRead(string key);
	void clientUpdate(string key, string value);
	void clientDelete(string key);

	// receive messages from Emulnet
	bool recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);

	// handle messages from receiving queue
	void checkMessages();

	// find the addresses of nodes that are responsible for a key
	vector<Node> findNodes(string key);

	// server
	bool createKeyValue(string key, string value, ReplicaType replica);
	string readKey(string key);
	bool updateKeyValue(string key, string value, ReplicaType replica);
	bool deletekey(string key);

	// stabilization protocol - handle multiple failures
	void stabilizationProtocol();

	//operator for the check
	bool compareNode(vector<Node> &node1, vector<Node> &node2);

	void checkTimeout(Message *msg);
	bool check();

	void printNode(vector<Node>&);
	void printAddress(Address *addr);

	~MP2Node();

};



#endif /* MP2NODE_H_ */


// #ifndef COMMANTABLE_H_
// #define COMMANTABLE_H_
// /**
//  * Header files
//  */
// #include "stdincludes.h"
// #include "Params.h"
// #include "Trace.h"
// #include "Message.h"


// /**
//  * CLASS NAME: CommandTable
//  *
//  * DESCRIPTION: This class encapsulates the mapping for the transId
//                 and key for each node:
//  * 				1) mapping of the transId and key
//  * 				2) Each Transaction performed on the node for each transID
//  */
//
// class CommandTable {
// private:
//   Params *par;
// public:
//   map<string, int> transID_key_map;
// 	map<string, MessageType> tran_performed;
//   map<string, int> success_calc;
//   map<string, int> failure_calc;
//   map<string, int> trans_timestamp;
//
//   //public
//   CommandTable(Params *par);
//   /*
//   * get function
//   */
//   int getTransId(string key);
//   string getKeyFromId(int id);
//   MessageType getTrans(string key);
//   int getTransTimestamp(string key);
//   int getsuccessCount(string key);
//   int getFailureCount(string key);
//   /*
//   * set function
//   */
//   bool setTransId(string key, int transID);
//   void setTrans(string key, MessageType type);
//   bool setsuccessCount(string key);
//   bool setFailureCount(string key);
//   /*
//   * remove function
//   */
//   bool removeTransId(string key);
//   bool removeTrans(string key);
//   bool removesuccessCount(string key);
//   bool removeFailureCount(string key);
//   virtual ~CommandTable();
// };
//
//
// #endif /* COMMANTABLE_H_ */
