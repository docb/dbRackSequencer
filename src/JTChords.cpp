#include "chordmgr.hpp"
#include <string>

template<int S>
struct Scale {
  std::string name;
  float values[S]={};
  std::string labels[S];

  Scale(json_t *scaleObj) {
    name=json_string_value(json_object_get(scaleObj,"name"));
    json_t *scale=json_object_get(scaleObj,"scale");
    size_t scaleLen=json_array_size(scale);
    printf("parsing %s len=%zu\n",name.c_str(),scaleLen);
    if(scaleLen!=S)
      throw Exception(string::f("Scale must have exact %d entries",S));
    for(size_t k=0;k<scaleLen;k++) {
      json_t *entry=json_array_get(scale,k);
      size_t elen=json_array_size(entry);
      if(elen!=2)
        throw Exception(string::f("Scale entry must be an array of length 2"));
      unsigned int n=json_integer_value(json_array_get(entry,0));
      unsigned int p=json_integer_value(json_array_get(entry,1));
      labels[k]=std::to_string(n)+"/"+std::to_string(p);
      values[k]=float(n)/float(p);
    }
  }
};

struct JTChords : Module {
  enum ParamIds {
    SCALE_PARAM,NOTE_UP_PARAM,OCT_UP_PARAM,NOTE_DOWN_PARAM,OCT_DOWN_PARAM,CHORD_PARAM,COPY_PARAM,PASTE_PARAM,EDIT_PARAM,PARAMS_LEN
  };
  enum InputIds {
    CHORD_INPUT,NUM_INPUTS
  };
  enum OutputIds {
    VOCT_OUTPUT,GATE_OUTPUT,RTR_OUTPUT,NUM_OUTPUTS
  };
  enum LightIds {
    NUM_LIGHTS
  };
  dsp::PulseGenerator rtrPulse[16];

  ChordManager<156,100> chrMgr;
  bool autoChannels=false;
  bool autoReorder=false;
  std::vector<Scale<31>> scaleVector;

  void parseJson(json_t *rootJ) {
    json_t *data=json_object_get(rootJ,"scales");
    size_t len=json_array_size(data);
    for(unsigned int i=0;i<len;i++) {
      json_t *scaleObj=json_array_get(data,i);
      scaleVector.emplace_back(scaleObj);
    }
  }

  bool load(const std::string &path) {
    INFO("Loading scale file %s",path.c_str());
    FILE *file=fopen(path.c_str(),"r");
    if(!file)
      return false;

    json_error_t error;
    json_t *rootJ=json_loadf(file,0,&error);
    fclose(file);

    if(!rootJ) {
      WARN("%s",string::f("Scales file has invalid JSON at %d:%d %s",error.line,error.column,error.text).c_str());
      return false;
    }
    try {
      parseJson(rootJ);
    } catch(Exception &e) {
      WARN("%s",e.msg.c_str());
      return false;
    }
    json_decref(rootJ);
    return true;
  }

  std::vector<std::string> getLabels() {
    std::vector<std::string> ret;
    for(const auto &scl:scaleVector) {
      ret.push_back(scl.name);
    }
    return ret;
  }

  JTChords() {
    if(!load(asset::plugin(pluginInstance,"res/scales_31.json"))) {
      INFO("user scale file %s does not exist or failed to load. using default_scales ....","res/scales_31.json");
      if(!load(asset::plugin(pluginInstance,"res/default_scales_31.json"))) {
        throw Exception(string::f("Default Scales are damaged, try reinstalling the plugin"));
      }
    }
    config(PARAMS_LEN,NUM_INPUTS,NUM_OUTPUTS,NUM_LIGHTS);
    configSwitch(SCALE_PARAM,0,scaleVector.size()-1,0.f,"Scales",getLabels());
    getParamQuantity(SCALE_PARAM)->snapEnabled=true;
    configSwitch(NOTE_UP_PARAM,0,1,0.f,"Note Up");
    configSwitch(OCT_UP_PARAM,0,1,0,"Octave Up");
    configSwitch(NOTE_DOWN_PARAM,0,1,0.f,"Note Up");
    configSwitch(OCT_DOWN_PARAM,0,1,0,"Octave Up");
    configButton(EDIT_PARAM,"Edit");
    configOutput(GATE_OUTPUT,"Gate");
    configOutput(VOCT_OUTPUT,"V/Oct");
    configInput(CHORD_INPUT,"Chord");
    configParam(CHORD_PARAM,0,99,0,"Chord Selection");
    getParamQuantity(CHORD_PARAM)->snapEnabled=true;
  }

  //void onAdd(const AddEvent &e) override {
   // chrMgr.saveFromGates();
   // chrMgr.lastChord=params[CHORD_PARAM].getValue();
  //}

  void process(const ProcessArgs &args) override {
    chrMgr.adjustMaxChannels();
    int currentScale=params[SCALE_PARAM].getValue();

    if(inputs[CHORD_INPUT].isConnected() && params[EDIT_PARAM].getValue() == 0.f) {
      int c=clamp(inputs[CHORD_INPUT].getVoltage(),0.f,9.99f)*10.f;
      getParamQuantity(CHORD_PARAM)->setValue(c);
    }
    int currentChord=params[CHORD_PARAM].getValue();
    int autoMaxChannels=0;
    for(int k=0;k<chrMgr.maxChannels;k++) {
      if(chrMgr.gates[currentChord][k]) {
        int key=chrMgr.notes[currentChord][k];
        int n=key%31;
        int o=key/31-2;
        float t=scaleVector[currentScale].values[n];
        float lt=log2f(t);
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
    if(autoReorder) chrMgr.reorder(currentChord);
  }

  void switchChord() {
    int newChord=params[CHORD_PARAM].getValue();
    chrMgr.switchChord(newChord);
    for(int k=0;k<16;k++) {
      if(chrMgr.gates[newChord][k]) {
        rtrPulse[k].trigger(0.01f);
      }
    }
    if(autoReorder) chrMgr.reorder(newChord);
  }

  bool getNoteStatus(int key) {
    int chord=int(params[CHORD_PARAM].getValue());
    return chrMgr.getNoteStatus(chord,key);
  }

  std::string getLabel(int key) {
    return scaleVector[(int)params[SCALE_PARAM].getValue()].labels[key%31];
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
  Tooltip *tooltip=nullptr;

  //std::string label;
  NoteButton(M *_module,BtnFormat type,int _key,Vec pos,Vec size) : module(_module),btnType(type),key(_key) {
    box.size=size;
    box.pos=pos;
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
    tooltip->text=module->getLabel(key);
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
  BtnFormat white,gray,blue;
  NVGcolor whiteOn,whiteOff,blackOn,blackOff,whiteBorder,blackBorder;

  NoteDisplay(M *m,Vec _pos) : module(m) {
    box.pos=_pos;
    box.size=Vec(100,10*32);
    white.onColor=nvgRGB(209,255,209);
    white.offColor=nvgRGB(153,153,153);
    white.border=nvgRGB(196,201,194);
    gray.onColor=nvgRGB(118,169,118);
    gray.offColor=nvgRGB(43,43,43);
    gray.border=nvgRGB(196,201,104);
    blue.onColor=nvgRGB(126,166,211);
    blue.offColor=nvgRGB(60,76,113);
    blue.border=nvgRGB(196,201,104);
    for(int j=0;j<5;j++) {
      for(int k=0;k<31;k++) {
        switch(30-k) {
          case 0:
          case 5:
          case 10:
          case 13:
          case 18:
          case 23:
          case 28: {
            addChild(new NoteButton<JTChords>(module,white,j*31+(30-k),Vec(j*20,10+k*10),Vec(20,10)));
            break;
          }
          case 3:
          case 8:
          case 16:
          case 21:
          case 26: {
            addChild(new NoteButton<JTChords>(module,gray,j*31+(30-k),Vec(j*20,10+k*10),Vec(20,10)));
            break;
          }
          default: {
            addChild(new NoteButton<JTChords>(module,blue,j*31+(30-k),Vec(j*20,10+k*10),Vec(20,10)));
            break;
          }
        }
      }
    }
    for(int j=0;j<5;j++) {
      addChild(new NoteButton<JTChords>(module,white,(j+1)*31,Vec(j*20,0),Vec(20,10)));
    }
  }

};

struct JTChordsWidget : ModuleWidget {
  JTChordsWidget(JTChords *module) {
    setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,"res/JTChords.svg")));

    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,0)));
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));
    addChild(new NoteDisplay<JTChords>(module,mm2px(Vec(4.5,MHEIGHT-120))));
    float x=40.8f;
    //auto sknob=createParam<ScaleKnob<JTChords>>(mm2px(Vec(x,MHEIGHT-108-6.2)),module,JTChords::SCALE_PARAM);
    auto sknob=createParam<TrimbotWhite>(mm2px(Vec(x,MHEIGHT-108-6.2)),module,JTChords::SCALE_PARAM);
    sknob->module=module;
    sknob->snap=true;
    addParam(sknob);

    auto noteUpButton=createParam<NoteUpButton<JTChords>>(mm2px(Vec(x,MHEIGHT-102)),module,JTChords::NOTE_UP_PARAM);
    noteUpButton->label="N+";
    noteUpButton->module=module;
    addParam(noteUpButton);
    auto noteDownButton=createParam<NoteDownButton<JTChords>>(mm2px(Vec(x,MHEIGHT-98)),module,JTChords::NOTE_DOWN_PARAM);
    noteDownButton->label="N-";
    noteDownButton->module=module;
    addParam(noteDownButton);
    auto octUpButton=createParam<OctUpButton<JTChords,31>>(mm2px(Vec(x,MHEIGHT-94)),module,JTChords::OCT_UP_PARAM);
    octUpButton->label="O+";
    octUpButton->module=module;
    addParam(octUpButton);
    auto octDownButton=createParam<OctDownButton<JTChords,31>>(mm2px(Vec(x,MHEIGHT-90)),module,JTChords::OCT_DOWN_PARAM);
    octDownButton->label="O-";
    octDownButton->module=module;
    addParam(octDownButton);

    auto copyButton=createParam<CopyButton<JTChords>>(mm2px(Vec(x,MHEIGHT-86)),module,JTChords::COPY_PARAM);
    copyButton->label="Cpy";
    copyButton->module=module;
    addParam(copyButton);
    auto pasteButton=createParam<PasteButton<JTChords>>(mm2px(Vec(x,MHEIGHT-82)),module,JTChords::PASTE_PARAM);
    pasteButton->label="Pst";
    pasteButton->module=module;
    addParam(pasteButton);
    auto editButton=createParam<SmallButtonWithLabel>(mm2px(Vec(x,MHEIGHT-78)),module,JTChords::EDIT_PARAM);
    editButton->setLabel("Edit");
    addParam(editButton);
    //auto chordParam=createParam<MChordKnob<JTChords>>(mm2px(Vec(x,MHEIGHT-58-6.2)),module,JTChords::CHORD_PARAM);
    auto chordParam=createParam<SChord<JTChords>>(mm2px(Vec(x,MHEIGHT-58-6.7)),module,JTChords::CHORD_PARAM);
    chordParam->module=module;
    //chordParam->box.size = Vec(20,60);
    addParam(chordParam);
    addInput(createInput<SmallPort>(mm2px(Vec(x,MHEIGHT-48-6.2)),module,JTChords::CHORD_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,MHEIGHT-36-6.2)),module,JTChords::RTR_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,MHEIGHT-24-6.2)),module,JTChords::GATE_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,MHEIGHT-12-6.2)),module,JTChords::VOCT_OUTPUT));
  }

  void appendContextMenu(Menu *menu) override {
    JTChords *module=dynamic_cast<JTChords *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    auto channelSelect=new IntSelectItem(&module->chrMgr.maxChannels,1,16);
    channelSelect->text="Polyphonic Channels";
    channelSelect->rightText=string::f("%d",module->chrMgr.maxChannels)+"  "+RIGHT_ARROW;
    menu->addChild(channelSelect);
    menu->addChild(createIndexPtrSubmenuItem("Polyphony mode",{"Rotate","Reset","Reuse Note"},&module->chrMgr.mode));

    struct ReorderItem : ui::MenuItem {
      JTChords *module;
      ReorderItem(JTChords *m) : module(m) {}
      void onAction(const ActionEvent& e) override {
        if (!module)
          return;
        module->reorder();
      }
    };
    auto reorderMenu = new ReorderItem(module);
    reorderMenu->text = "Reorder";
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
      JTChords *module;

      InsertItem(JTChords *m) : module(m) {
      }

      void onAction(const ActionEvent &e) override {
        if(!module)
          return;
        module->insert();
      }
    };
    auto insertMenu=new InsertItem(module);
    insertMenu->text="Insert Chord";
    menu->addChild(insertMenu);
    struct DelItem : ui::MenuItem {
      JTChords *module;

      DelItem(JTChords *m) : module(m) {
      }

      void onAction(const ActionEvent &e) override {
        if(!module)
          return;
        module->del();
      }
    };
    auto delMenu=new DelItem(module);
    delMenu->text="Delete Chord";
    menu->addChild(delMenu);
  }
};


Model *modelJTChords=createModel<JTChords,JTChordsWidget>("JTChords");