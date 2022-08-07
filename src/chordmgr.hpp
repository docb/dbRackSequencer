//
// Created by docb on 1/25/22.
//
#ifndef DBCAEMODULES_CHORDMGR_HPP
#define DBCAEMODULES_CHORDMGR_HPP

#include "plugin.hpp"

template<int NOTES,int NUM_CHORDS>
struct ChordManager {
  enum PolyMode {
    ROTATE,FIRST,REUSE
  };
  int notes[NUM_CHORDS][PORT_MAX_CHANNELS]={};
  int cbNotes[PORT_MAX_CHANNELS]={};
  int cbGates[PORT_MAX_CHANNELS]={};
  bool cbKeys[NOTES];
  bool gates[NUM_CHORDS][PORT_MAX_CHANNELS]={};
  bool chords[NUM_CHORDS][NOTES]={};
  int lastMaxChannels=8;
  int maxChannels=8;
  int pos=0;
  int mode=ROTATE;

  ChordManager() {
    for(int k=0;k<NUM_CHORDS;k++) {
      for(int j=0;j<PORT_MAX_CHANNELS;j++) {
        notes[k][j]=-1;
        gates[k][j]=false;
      }
    }
    for(int k=0;k<NUM_CHORDS;k++) {
      for(int j=0;j<NOTES;j++) {
        chords[k][j]=false;
      }
    }
  }

  void reset() {

    for(int k=0;k<NUM_CHORDS;k++) {
      for(int j=0;j<PORT_MAX_CHANNELS;j++) {
        notes[k][j]=-1;
        gates[k][j]=false;
      }
      for(int j=0;j<NOTES;j++) {
        chords[k][j]=false;
      }
    }
    pos=0;
  }

  json_t *dataToJson() {
    json_t *data=json_object();
    json_t *chordList=json_array();
    for(int k=0;k<NUM_CHORDS;k++) {
      json_t *onList=json_array();
      for(int j=0;j<PORT_MAX_CHANNELS;j++) {
        json_array_append_new(onList,json_integer(gates[k][j]?notes[k][j]:-1));
      }
      json_array_append_new(chordList,onList);
    }
    json_object_set_new(data,"notes",chordList);
    json_object_set_new(data,"mode",json_integer(mode));
    json_object_set_new(data,"channels",json_integer(maxChannels));

    return data;
  }

  void dataFromJson(json_t *rootJ) {
    json_t *data=json_object_get(rootJ,"notes");
    if(!data)
      return;
    reset();
    for(int k=0;k<NUM_CHORDS;k++) {
      json_t *arr=json_array_get(data,k);
      if(arr) {
        for(int j=0;j<PORT_MAX_CHANNELS;j++) {
          json_t *onKey=json_array_get(arr,j);
          notes[k][j]=json_integer_value(onKey);
          if(notes[k][j]>=0) {
            gates[k][j]=true;
            chords[k][notes[k][j]]=true;
          }
        }
      }
    }
    json_t *jMode=json_object_get(rootJ,"mode");
    if(jMode)
      mode=json_integer_value(jMode);
    json_t *jMaxChannels=json_object_get(rootJ,"channels");
    if(jMaxChannels) {
      lastMaxChannels = maxChannels=json_integer_value(jMaxChannels);
    }
  }
  void reorder(int chord) {
    std::vector<int> v;
    for(int k=0;k<PORT_MAX_CHANNELS;k++) {
      if(gates[chord][k]) {
        v.push_back(notes[chord][k]);
      }
    }
    std::sort(v.begin(),v.end());
    unsigned size=v.size();
    unsigned k=0;
    for(;k<size;k++) {
      notes[chord][k]=v[k];
      gates[chord][k]=true;
    }
    for(;k<PORT_MAX_CHANNELS;k++) {
      gates[chord][k]=false;
    }
  }
  void fromGates(int chord) {
    for(int key=0;key<NOTES;key++) {
      chords[chord][key]=false;
      for(int k=0;k<PORT_MAX_CHANNELS;k++) {
        if(gates[chord][k]&&notes[chord][k]==key) {
          chords[chord][key]=true;
        }
      }
    }
  }

  void adjustMaxChannels() {
    if(lastMaxChannels>maxChannels) {
      for(int k=0;k<NUM_CHORDS;k++) {
        int p=0;
        for(int j=0;j<PORT_MAX_CHANNELS;j++) {
          if(gates[k][j]) {
            gates[k][p]=true;
            p++;
          }
          if(p==maxChannels)
            break;
        }
        for(int j=maxChannels;j<PORT_MAX_CHANNELS;j++) {
          gates[k][j]=false;
        }
        fromGates(k);
      }

    }
    lastMaxChannels=maxChannels;
  }

  int getNoteStatus(int chord,int key) {
    return chords[chord][key];
  }

  void switchChord(int newChord) {
    for(int k=0;k<maxChannels;k++) {
      gates[newChord][k]=false;
    }
    for(int k=0;k<NOTES;k++) {
      toGates(newChord,k);
    }
  }

  void insert(int currentChord) {
    for(int k=99;k>currentChord;k--) {
      for(int j=0;j<PORT_MAX_CHANNELS;j++) {
        notes[k][j]=notes[k-1][j];
        gates[k][j]=gates[k-1][j];
      }
      for(int j=0;j<NOTES;j++) {
        chords[k][j]=chords[k-1][j];
      }
    }
    for(int j=0;j<PORT_MAX_CHANNELS;j++) {
      notes[currentChord][j]=-1;
      gates[currentChord][j]=false;
    }
    for(int j=0;j<NOTES;j++) {
      chords[currentChord][j]=false;
    }
  }
  void del(int currentChord) {
    for(int k=currentChord;k<99;k++) {
      for(int j=0;j<PORT_MAX_CHANNELS;j++) {
        notes[k][j]=notes[k+1][j];
        gates[k][j]=gates[k+1][j];
      }
      for(int j=0;j<NOTES;j++) {
        chords[k][j]=chords[k+1][j];
      }
    }
  }

  void copy(int currentChord) {
    for(int k=0;k<PORT_MAX_CHANNELS;k++) {
      cbNotes[k]=notes[currentChord][k];
      cbGates[k]=gates[currentChord][k];
    }
    for(int k=0;k<NOTES;k++) {
      cbKeys[k]=chords[currentChord][k];
    }
  }

  void paste(int currentChord) {

    for(int k=0;k<PORT_MAX_CHANNELS;k++) {
      notes[currentChord][k]=cbNotes[k];
      gates[currentChord][k]=cbGates[k];
    }
    for(int k=0;k<NOTES;k++) {
      chords[currentChord][k]=cbKeys[k];
    }
  }

  void noteMod(int currentChord,int amt) {

    bool canDo=true;
    for(int k=0;k<maxChannels;k++) {
      if(gates[currentChord][k]) {
        int newVal=notes[currentChord][k]+amt;
        if(newVal<0||newVal>=NOTES)
          canDo=false;
      }
    }
    if(canDo) {
      for(int k=0;k<maxChannels;k++) {
        if(gates[currentChord][k]) {
          chords[currentChord][notes[currentChord][k]]=false;
        }
      }
      for(int k=0;k<maxChannels;k++) {
        if(gates[currentChord][k]) {
          notes[currentChord][k]+=amt;
          chords[currentChord][notes[currentChord][k]]=true;
        }
      }
    }
  }

  void saveFromGates() {
    for(int k=0;k<NUM_CHORDS;k++) {
      for(int j=0;j<PORT_MAX_CHANNELS;j++) {
        int key=notes[k][j];
        if(key>=0)
          chords[k][key]=gates[k][j];
      }
    }
  }

  int findInsertPos(int currentChord,int key) {
    if(maxChannels==1)
      return 0;
    if(mode==ROTATE) {
      for(int j=0;j<maxChannels;j++) {
        if(!gates[currentChord][pos])
          return pos;
        pos=(pos+1)%maxChannels;
      }
      return pos;
    }
    if(mode==REUSE) {
      for(int j=0;j<maxChannels;j++) {
        if(!gates[currentChord][j]&&notes[currentChord][j]==key)
          return j;
      }
    }
    for(int j=0;j<maxChannels;j++) {
      if(!gates[currentChord][j])
        return j;
    }
    return pos;
  }

  int insertNote(int currentChord,int key) {
    int iPos=findInsertPos(currentChord,key);

    //INFO("insert note chord=%d key=%d pos=%d",currentChord,key,iPos);
    if(gates[currentChord][iPos]) {
      if(notes[currentChord][iPos]>=0)
        chords[currentChord][notes[currentChord][iPos]]=false;
    }
    notes[currentChord][iPos]=key;
    gates[currentChord][iPos]=true;
    //rtrPulse[iPos].trigger(0.01f);
    if(mode==ROTATE) {
      pos=(pos+1)%maxChannels;
    } else {
      pos=iPos;
    }
    return iPos;
  }

  int toGates(int currentChord,int key) {
    bool keyOn=chords[currentChord][key];
    int index=-1;
    for(int chn=0;chn<maxChannels;chn++) {
      if(gates[currentChord][chn]&&notes[currentChord][chn]==key) {
        index=chn;
        break;
      }
    }
    //INFO("chord=%d key=%d keyon=%d index=%d",currentChord,key,keyOn,index);
    if(keyOn==0) {
      if(index>=0) {
        //INFO("remove key=%d chord=%d index=%d",key,currentChord,index);
        gates[currentChord][index]=false;
        chords[currentChord][key]=false;
      }
    } else {
      return insertNote(currentChord,key);
    }
    return -1;
  }

  int updateKey(int currentChord,int key) {
    //INFO("update key %d",key);
    chords[currentChord][key]=!(chords[currentChord][key]);
    return toGates(currentChord,key);
    //INFO("DONE update key %d",key);
  }

};

template<typename M>
struct MChordKnob : TrimbotWhite {
  M *module;

  void onChange(const event::Change &e) override {
    if(module)
      module->switchChord();
    SvgKnob::onChange(e);
  }

};

template<typename M>
struct NoteUpButton : SmallButtonWithLabel {
  M *module;

  NoteUpButton() : SmallButtonWithLabel() {
    momentary=true;
  }

  void onChange(const ChangeEvent &e) override {
    if(module) {
      if(module->params[M::NOTE_UP_PARAM].getValue()>0)
        module->noteMod(1);
    }
    SvgSwitch::onChange(e);
  }
};

template<typename M>
struct NoteDownButton : SmallButtonWithLabel {
  M *module;

  NoteDownButton() : SmallButtonWithLabel() {
    momentary=true;
  }

  void onChange(const ChangeEvent &e) override {
    if(module) {
      if(module->params[M::NOTE_DOWN_PARAM].getValue()>0)
        module->noteMod(-1);
    }
    SvgSwitch::onChange(e);
  }
};

template<typename M,int OCT>
struct OctUpButton : SmallButtonWithLabel {
  M *module;

  OctUpButton() : SmallButtonWithLabel() {
    momentary=true;
  }

  void onChange(const ChangeEvent &e) override {
    if(module) {
      if(module->params[M::OCT_UP_PARAM].getValue()>0)
        module->noteMod(OCT);
    }
    SvgSwitch::onChange(e);
  }
};

template<typename M,int OCT>
struct OctDownButton : SmallButtonWithLabel {
  M *module;

  OctDownButton() : SmallButtonWithLabel() {
    momentary=true;
  }

  void onChange(const ChangeEvent &e) override {
    if(module) {
      if(module->params[M::OCT_DOWN_PARAM].getValue()>0)
        module->noteMod(-OCT);
    }
    SvgSwitch::onChange(e);
  }
};

struct BtnFormat {
  NVGcolor onColor,offColor,border;
};
template<typename M>
struct SChord : SpinParamWidget {
  M *module;
  SChord() {
    init();
  }
  void onChange(const event::Change &e) override {
    if(module)
      module->switchChord();
  }

};

#endif //DBCAEMODULES_CHORDMGR_HPP
