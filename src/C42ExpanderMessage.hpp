
#ifndef DBCAEMODULES_C42EXPANDERMESSAGE_HPP
#define DBCAEMODULES_C42EXPANDERMESSAGE_HPP

struct C42ExpanderMessage {
  float topLeft[16]={};
  float colTop[16]={};
  float topRight[16]={};
  float rowLeft[16]={};
  float rowRight[16]={};
  float bottomLeft[16]={};
  float colBottom[16]={};
  float bottomRight[16]={};
  int channels=0;
};


#endif //DBCAEMODULES_C42EXPANDERMESSAGE_HPP
