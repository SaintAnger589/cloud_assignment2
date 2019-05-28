
#ifndef COMMANTABLE_H_
#define COMMANTABLE_H_
/**
 * Header files
 */
#include "stdincludes.h"
#include "Params.h"
#include "Trace.h"
#include "Message.h"


/**
 * CLASS NAME: CommandTable
 *
 * DESCRIPTION: This class encapsulates the mapping for the transId
                and key for each node:
 * 				1) mapping of the transId and key
 * 				2) Each Transaction performed on the node for each transID
 */

class CommandTable {
private:
  Params *par;
public:
  map<string, int> transID_key_map;
	map<string, MessageType> tran_performed;
  map<string, int> success_calc;
  map<string, int> failure_calc;
  map<string, int> trans_timestamp;

  //public
  CommandTable(Params *par);
  /*
  * get function
  */
  int getTransId(string key);
  string getKeyFromId(int id);
  MessageType getTrans(string key);
  int getTransTimestamp(string key);
  int getsuccessCount(string key);
  int getFailureCount(string key);
  /*
  * set function
  */
  bool setTransId(string key, int transID);
  void setTrans(string key, MessageType type);
  bool setsuccessCount(string key);
  bool setFailureCount(string key);
  /*
  * remove function
  */
  bool removeTransId(string key);
  bool removeTrans(string key);
  bool removesuccessCount(string key);
  bool removeFailureCount(string key);
  virtual ~CommandTable();
};


#endif /* COMMANTABLE_H_ */
