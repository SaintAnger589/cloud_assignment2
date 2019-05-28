#include "CommandTable.h"

CommandTable::CommandTable(Params *par) {
  this->par = par;
}

CommandTable::~CommandTable() {}


/**
 * FUNCTION NAME: read
 *
 * DESCRIPTION: This function searches for the key in the CommandTable
 *
 * RETURNS:
 * string value if found
 * else it returns a NULL
 */

void setTransTable(int transID, string key, MessageType type){
  transaction_performed *tp;
  tp->key = key;
  tp->transID = transID;
  tp->type = type;
  tp->timestamp = par->getcurrtime();
}
