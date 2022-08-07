#include "chordmgr.hpp"

#define MAX_NOTES 97
#define MAX_CHORDS 100


struct Chords : Module {
  enum ParamId {
    NOTE_UP_PARAM,OCT_UP_PARAM,NOTE_DOWN_PARAM,OCT_DOWN_PARAM,CHORD_PARAM,COPY_PARAM,PASTE_PARAM,EDIT_PARAM,PARAMS_LEN
  };
  enum InputId {
    CHORD_INPUT,INPUTS_LEN
  };
  enum OutputId {
    VOCT_OUTPUT,GATE_OUTPUT,RTR_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  enum PolyMode {
    ROTATE,FIRST,REUSE
  };

  dsp::PulseGenerator rtrPulse[PORT_MAX_CHANNELS];
  ChordManager<MAX_NOTES,MAX_CHORDS> chrMgr;
  bool autoChannels=false;
  bool autoReorder=false;

  Chords() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configSwitch(NOTE_UP_PARAM,0,1,0.f,"Note Up");
    configSwitch(OCT_UP_PARAM,0,1,0,"Octave Up");
    configSwitch(NOTE_DOWN_PARAM,0,1,0.f,"Note Down");
    configSwitch(OCT_DOWN_PARAM,0,1,0,"Octave Down");
    configOutput(GATE_OUTPUT,"Gate");
    configOutput(VOCT_OUTPUT,"V/Oct");
    configInput(CHORD_INPUT,"Chord");
    configParam(CHORD_PARAM,0,MAX_CHORDS-1,0,"Chord Selection");
    getParamQuantity(CHORD_PARAM)->snapEnabled=true;
  }


  void process(const ProcessArgs &args) override {
    chrMgr.adjustMaxChannels();

    if(inputs[CHORD_INPUT].isConnected()&&params[EDIT_PARAM].getValue()==0) {
      int c=round(clamp(inputs[CHORD_INPUT].getVoltage(),0.f,9.99f)*float(MAX_CHORDS)/10.f);
      //if(c!=chrMgr.lastChord) {
      //  chrMgr.lastChord = c;
      getParamQuantity(CHORD_PARAM)->setValue(c);
      //}
    }
    int autoMaxChannels=0;
    int currentChord=params[CHORD_PARAM].getValue();
    for(int k=0;k<chrMgr.maxChannels;k++) {
      if(chrMgr.gates[currentChord][k]) {
        int key=chrMgr.notes[currentChord][k];
        int n=key%12;
        int o=key/12-4;
        float lt=float(n)/12.f;
        float voltage=(float)o+lt;
        outputs[VOCT_OUTPUT].setVoltage(voltage,k);
        outputs[GATE_OUTPUT].setVoltage(10.f,k);
        autoMaxChannels=k;
      } else {
        outputs[GATE_OUTPUT].setVoltage(0.f,k);
      }
      bool trig=rtrPulse[k].process(args.sampleTime);
      outputs[RTR_OUTPUT].setVoltage(trig?10.f:0.f,k);
    }
    outputs[RTR_OUTPUT].setChannels(autoChannels?autoMaxChannels+1:chrMgr.maxChannels);
    outputs[VOCT_OUTPUT].setChannels(autoChannels?autoMaxChannels+1:chrMgr.maxChannels);
    outputs[GATE_OUTPUT].setChannels(autoChannels?autoMaxChannels+1:chrMgr.maxChannels);

  }

  json_t *dataToJson() override {
    json_t *data=chrMgr.dataToJson();
    json_object_set_new(data,"autoReorder",json_integer(autoReorder));
    json_object_set_new(data,"autoChannels",json_integer(autoChannels));
    return data;
  }

  void dataFromJson(json_t *rootJ) override {
    chrMgr.dataFromJson(rootJ);
    json_t *jAutoChannels=json_object_get(rootJ,"autoChannels");
    if(jAutoChannels) {
      autoChannels=json_integer_value(jAutoChannels);
    }
    json_t *jAutoReorder=json_object_get(rootJ,"autoReorder");
    if(jAutoReorder) {
      autoReorder=json_integer_value(jAutoReorder);
    }

  }

  //void onAdd(const AddEvent &e) override {
  //  chrMgr.saveFromGates();
  //}

  void onReset(const ResetEvent &e) override {
    params[CHORD_PARAM].setValue(0);
    chrMgr.reset();
    for(int k=0;k<PORT_MAX_CHANNELS;k++) {
      outputs[VOCT_OUTPUT].setVoltage(0,k);
    }
  }

  void copy() {
    int currentChord=params[CHORD_PARAM].getValue();
    chrMgr.copy(currentChord);
  }

  void paste() {
    int currentChord=params[CHORD_PARAM].getValue();
    chrMgr.paste(currentChord);
  }

  void insert() {
    int currentChord=params[CHORD_PARAM].getValue();
    chrMgr.insert(currentChord);
  }

  void del() {
    int currentChord=params[CHORD_PARAM].getValue();
    chrMgr.del(currentChord);
  }

  void noteMod(int amt) {
    int currentChord=params[CHORD_PARAM].getValue();
    chrMgr.noteMod(currentChord,amt);
  }

  void updateKey(int key) {
    int currentChord=params[CHORD_PARAM].getValue();
    int trg=chrMgr.updateKey(currentChord,key);
    if(trg>=0)
      rtrPulse[trg].trigger(0.01f);
    if(autoReorder)
      chrMgr.reorder(currentChord);
  }

  void switchChord() {
    int newChord=params[CHORD_PARAM].getValue();
    chrMgr.switchChord(newChord);
    for(int k=0;k<16;k++) {
      if(chrMgr.gates[newChord][k]) {
        rtrPulse[k].trigger(0.01f);
      }
    }
    if(autoReorder)
      chrMgr.reorder(newChord);
  }

  bool getNoteStatus(int key) {
    int chord=int(params[CHORD_PARAM].getValue());
    return chrMgr.getNoteStatus(chord,key);
  }

  void reorder() {
    int currentChord=params[CHORD_PARAM].getValue();
    chrMgr.reorder(currentChord);
  }
};

template<typename M>
struct NoteButton : OpaqueWidget {
  M *module;
  BtnFormat btnType;
  int key;
  std::string labels[12]={"C","C#/Db","D","D#/Eb","E","F","F#/Gb","G","G#/Ab","A","A#/Bb","B"};
  Tooltip *tooltip=nullptr;
  std::string label;

  NoteButton(M *_module,BtnFormat type,int _key,Vec pos,Vec size) : module(_module),btnType(type),key(_key) {
    box.size=size;
    box.pos=pos;
    int oct=key/12-4;
    label=string::f("%s%d",labels[key%12].c_str(),oct);
  }

  void onButton(const event::Button &e) override {
    if(!(e.button==GLFW_MOUSE_BUTTON_LEFT&&(e.mods&RACK_MOD_MASK)==0)) {
      return;
    }
    if(e.action==GLFW_PRESS) {
      if(module)
        module->updateKey(key);
    }
  }

  void createTooltip() {
    if(!settings::tooltips)
      return;
    tooltip=new Tooltip;
    tooltip->text=label;
    APP->scene->addChild(tooltip);
  }


  void destroyTooltip() {
    if(!tooltip)
      return;
    APP->scene->removeChild(tooltip);
    delete tooltip;
    tooltip=nullptr;
  }


  void onEnter(const EnterEvent &e) override {
    createTooltip();
  }


  void onLeave(const LeaveEvent &e) override {
    destroyTooltip();
  }

  void drawLayer(const DrawArgs &args,int layer) override {
    if(layer==1) {
      _draw(args);
    }
    Widget::drawLayer(args,layer);
  }

  void _draw(const DrawArgs &args) {
    nvgBeginPath(args.vg);
    nvgRoundedRect(args.vg,1,1,box.size.x-2,box.size.y-2,2);
    nvgFillColor(args.vg,(module&&module->getNoteStatus(key))?btnType.onColor:btnType.offColor);
    nvgStrokeColor(args.vg,btnType.border);
    nvgFill(args.vg);
    nvgStroke(args.vg);
  }
};

template<typename M>
struct NoteDisplay : OpaqueWidget {
  M *module;
  Vec buttonSize;
  BtnFormat white,black;
  NVGcolor whiteOn,whiteOff,blackOn,blackOff,whiteBorder,blackBorder;

  NoteDisplay(M *m,Vec _pos) : module(m) {
    box.pos=_pos;
    box.size=Vec(80,10*25);
    white.onColor=nvgRGB(209,255,209);
    white.offColor=nvgRGB(153,153,153);
    white.border=nvgRGB(196,201,194);
    black.onColor=nvgRGB(118,169,118);
    black.offColor=nvgRGB(43,43,43);
    black.border=nvgRGB(196,201,104);
    for(int j=0;j<4;j++) {
      for(int k=0;k<24;k++) {
        switch((23-k)%12) {
          case 0:
          case 2:
          case 4:
          case 5:
          case 7:
          case 9:
          case 11: {
            addChild(new NoteButton<Chords>(module,white,j*24+(23-k),Vec(j*20,10+k*10),Vec(20,10)));
            break;
          }
          default: {
            addChild(new NoteButton<Chords>(module,black,j*24+(23-k),Vec(j*20,10+k*10),Vec(20,10)));
            break;
          }
        }
      }
    }
    for(int j=0;j<4;j++) {
      addChild(new NoteButton<Chords>(module,white,(j+1)*24,Vec(j*20,0),Vec(20,10)));
    }
  }

};

struct ChordsWidget : ModuleWidget {
  ChordsWidget(Chords *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/Chords.svg")));

    addChild(createWidget<ScrewSilver>(Vec(0,0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-15,0)));
    addChild(createWidget<ScrewSilver>(Vec(0,365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-15,365)));
    addChild(new NoteDisplay<Chords>(module,Vec(5,80)));

    auto copyButton=createParam<CopyButton<Chords>>(mm2px(Vec(12,MHEIGHT-122)),module,Chords::COPY_PARAM);
    copyButton->label="Cpy";
    copyButton->module=module;
    addParam(copyButton);
    auto pasteButton=createParam<PasteButton<Chords>>(mm2px(Vec(12,MHEIGHT-118)),module,Chords::PASTE_PARAM);
    pasteButton->label="Pst";
    pasteButton->module=module;
    addParam(pasteButton);

    auto noteUpButton=createParam<NoteUpButton<Chords>>(mm2px(Vec(21,MHEIGHT-122)),module,Chords::NOTE_UP_PARAM);
    noteUpButton->label="N+";
    noteUpButton->module=module;
    addParam(noteUpButton);
    auto noteDownButton=createParam<NoteDownButton<Chords>>(mm2px(Vec(21,MHEIGHT-118)),module,Chords::NOTE_DOWN_PARAM);
    noteDownButton->label="N-";
    noteDownButton->module=module;
    addParam(noteDownButton);
    auto octUpButton=createParam<OctUpButton<Chords,12>>(mm2px(Vec(21,MHEIGHT-114)),module,Chords::OCT_UP_PARAM);
    octUpButton->label="O+";
    octUpButton->module=module;
    addParam(octUpButton);
    auto octDownButton=createParam<OctDownButton<Chords,12>>(mm2px(Vec(21,MHEIGHT-110)),module,Chords::OCT_DOWN_PARAM);
    octDownButton->label="O-";
    octDownButton->module=module;
    addParam(octDownButton);

    auto chordParam=createParam<SChord<Chords>>(mm2px(Vec(3,MHEIGHT-119)),module,Chords::CHORD_PARAM);
    chordParam->module=module;
    addParam(chordParam);
    auto editButton=createParam<SmallButtonWithLabel>(mm2px(Vec(3,MHEIGHT-108)),module,Chords::EDIT_PARAM);
    editButton->setLabel("Edit");
    addParam(editButton);


    addInput(createInput<SmallPort>(mm2px(Vec(12,MHEIGHT-113)),module,Chords::CHORD_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(3,MHEIGHT-6-6.2)),module,Chords::RTR_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(11.9,MHEIGHT-6-6.2)),module,Chords::GATE_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(20.8,MHEIGHT-6-6.2)),module,Chords::VOCT_OUTPUT));
  }

  void appendContextMenu(Menu *menu) override {
    Chords *module=dynamic_cast<Chords *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    auto channelSelect=new IntSelectItem(&module->chrMgr.maxChannels,1,PORT_MAX_CHANNELS);
    channelSelect->text="Polyphonic Channels";
    channelSelect->rightText=string::f("%d",module->chrMgr.maxChannels)+"  "+RIGHT_ARROW;
    menu->addChild(channelSelect);
    menu->addChild(createIndexPtrSubmenuItem("Polyphony mode",{"Rotate","Reset","Reuse Note"},&module->chrMgr.mode));
    struct ReorderItem : ui::MenuItem {
      Chords *module;

      ReorderItem(Chords *m) : module(m) {
      }

      void onAction(const ActionEvent &e) override {
        if(!module)
          return;
        module->reorder();
      }
    };
    auto reorderMenu=new ReorderItem(module);
    reorderMenu->text="Reorder";
    menu->addChild(reorderMenu);
    menu->addChild(createCheckMenuItem("Auto Channels","",[=]() {
      return module->autoChannels;
    },[=]() {
      module->autoChannels=!module->autoChannels;
    }));
    menu->addChild(createCheckMenuItem("Auto Reorder","",[=]() {
      return module->autoReorder;
    },[=]() {
      module->autoReorder=!module->autoReorder;
    }));

    struct InsertItem : ui::MenuItem {
      Chords *module;

      InsertItem(Chords *m) : module(m) {
      }

      void onAction(const ActionEvent &e) override {
        if(!module)
          return;
        module->insert();
      }
    };
    auto insertMenu=new InsertItem(module);
    insertMenu->text="Insert Pattern";
    menu->addChild(insertMenu);
    struct DelItem : ui::MenuItem {
      Chords *module;

      DelItem(Chords *m) : module(m) {
      }

      void onAction(const ActionEvent &e) override {
        if(!module)
          return;
        module->del();
      }
    };
    auto delMenu=new DelItem(module);
    delMenu->text="Delete Pattern";
    menu->addChild(delMenu);
  }
};


Model *modelChords=createModel<Chords,ChordsWidget>("Chords");
