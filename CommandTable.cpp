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
int CommandTable::getTransId(string key){
  map<string, int>::iterator search;

  search = transID_key_map.find(key);
  if ( search != transID_key_map.end() ) {
    // Value found
    return search->second;
  }
  else {
    // Value not found
    return -1;
  }
}

string CommandTable::getKeyFromId(int id){
  //cout<<"getKeyFromId: id = "<<id<<"\n";
  auto it = transID_key_map.begin();
  //cout<<"after finding in the key map\n";
  while (it != transID_key_map.end()){
    //cout<<"it->second = "<<it->second<<"\n";
    //if (it->second == id){
      return it->first;
    //}
  }
  cout<<"getKeyFromId: key not found\n";
  return "";
}

int CommandTable::getsuccessCount(string key){
  map<string, int>::iterator search;

  search = success_calc.find(key);
  if ( search != success_calc.end() ) {
    // Value found
    return search->second;
  }
  else {
    // Value not found
    return -1;
  }
}

int CommandTable::getFailureCount(string key){
  map<string, int>::iterator search;

  search = failure_calc.find(key);
  if ( search != failure_calc.end() ) {
    // Value found
    return search->second;
  }
  else {
    // Value not found
    return -1;
  }
}

MessageType CommandTable::getTrans(string key){
  map<string, MessageType>::iterator search;

  search = tran_performed.find(key);
  if ( search != tran_performed.end() ) {
    // Value found
    return search->second;
  }
  else {
    // Value not found
    return UPDATE;
  }
}

int CommandTable::getTransTimestamp(string key){
  map<string, int>::iterator search;
  //cout<<"getTransTimestamp: key = "<<key<<"\n";
  search = trans_timestamp.find(key);
  if ( search != trans_timestamp.end() ) {
    // Value found
    return search->second;
  }
  else {
    // Value not found
    return -1;
  }
}

bool CommandTable::setTransId(string key, int value){
  //cout<<"Adding the TransId\n";
  transID_key_map.emplace(key, value);
  //cout<<"TransId added\n";
  return true;
}

bool CommandTable::setsuccessCount(string key){
  //cout<<"Adding the TransId\n";
  map<string, int>::iterator search;

  search = success_calc.find(key);
  if ( search != success_calc.end() ) {
    // Value found
    search->second = search->second + 1;
  }
  else {
    // Value not found
    success_calc.emplace(key, 1);
  }
  //cout<<"TransId added\n";
  return true;
}

bool CommandTable::setFailureCount(string key){
  //cout<<"Adding the TransId\n";
  map<string, int>::iterator search;

  search = failure_calc.find(key);
  if ( search != failure_calc.end() ) {
    // Value found
    search->second = search->second + 1;
  }
  else {
    // Value not found
    failure_calc.emplace(key, 1);
  }
  //cout<<"TransId added\n";
  return true;
}

void CommandTable::setTrans(string key, MessageType value){
  tran_performed.emplace(key, value);
  //cout<<"setTrans: key = "<<key<<"\n";
  //cout<<"setTrans: value = "<<value<<"\n";
  //cout<<"setTrans: current_time = "<<par->getcurrtime()<<"\n";
  trans_timestamp.emplace(key, par->getcurrtime());
}

bool CommandTable::removeTransId(string key){
  uint eraseCount = 0;
  if (getTransId(key) == NULL) {
    // Key not found
    return false;
  }
  eraseCount = transID_key_map.erase(key);
  if ( eraseCount < 1 ) {
    // Could not erase
    return false;
  }
  // Delete was successful

  return true;
}

bool CommandTable::removesuccessCount(string key){
  uint eraseCount = 0;
  if (getTransId(key) == NULL) {
    // Key not found
    return false;
  }
  eraseCount = success_calc.erase(key);
  if ( eraseCount < 1 ) {
    // Could not erase
    return false;
  }
  // Delete was successful
  return true;
}

bool CommandTable::removeFailureCount(string key){
  uint eraseCount = 0;
  if (getTransId(key) == NULL) {
    // Key not found
    return false;
  }
  eraseCount = failure_calc.erase(key);
  if ( eraseCount < 1 ) {
    // Could not erase
    return false;
  }
  // Delete was successful
  return true;
}


bool CommandTable::removeTrans(string key){
  uint eraseCount = 0;
  if (getTrans(key) == NULL) {
    // Key not found
    return false;
  }
  eraseCount = tran_performed.erase(key);
  if ( eraseCount < 1 ) {
    // Could not erase
    return false;
  }
  // Delete was successful
  eraseCount = trans_timestamp.erase(key);
  return true;
}
