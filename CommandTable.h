
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
  map<int, transaction_performed*> transID_map;

  typedef struct transaction_performed{
    MessageType type;
    int timestamp;
    string key;
  } transaction_performed;

  //public
  CommandTable(Params *par);
  /*
  * get function
  */
  transaction_performed* getTrans(int transID);
  /*
  * set function
  */
   void setTransTable(int transID, string key, MessageType type);
  /*
  * remove function
  */

  virtual ~CommandTable();
};


#endif /* COMMANTABLE_H_ */
