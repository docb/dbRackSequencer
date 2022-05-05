//
// Created by docb on 4/23/22.
//

#ifndef DBCAEMODULES_SEMESSAGE_HPP
#define DBCAEMODULES_SEMESSAGE_HPP
#define NUM_INPUTS 12
struct SEMessage {
  float ins[NUM_INPUTS][16];
  int channels;
};
#endif //DBCAEMODULES_SEMESSAGE_HPP
