#include "plugin.hpp"

#define MAX_PATS 100

struct AG : Module {
  enum ParamId {
    PAT_PARAM,EDIT_PARAM,COPY_PARAM,PASTE_PARAM,INSERT_PARAM,DELETE_PARAM,PARAMS_LEN
  };
  enum InputId {
    PAT_CV_INPUT,INPUTS_LEN
  };
  enum OutputId {
    GATE_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  RND rnd;
  bool gateCountFromZero=false;
  float randomDens=0.5f;
  bool gates[MAX_PATS][PORT_MAX_CHANNELS]={};
  bool cbGates[PORT_MAX_CHANNELS]={};
  int maxChannels=8;

  AG() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configParam(PAT_PARAM,0,MAX_PATS-1,0,"Pattern Selection");
    configButton(COPY_PARAM,"Copy");
    configButton(PASTE_PARAM,"Paste");
    configButton(INSERT_PARAM,"Insert");
    configButton(DELETE_PARAM,"Delete");
    configButton(EDIT_PARAM,"Edit");
    configInput(PAT_CV_INPUT,"Pattern (0.1V/Step)");
    configOutput(GATE_OUTPUT,"Gate");
  }

  void process(const ProcessArgs &args) override {
    if(inputs[PAT_CV_INPUT].isConnected()&&params[EDIT_PARAM].getValue()==0) {
      int c=clamp(inputs[PAT_CV_INPUT].getVoltage(),0.f,9.99f)*float(MAX_PATS)/10.f;
      setImmediateValue(getParamQuantity(PAT_PARAM),c);
    }
    int currentPattern=params[PAT_PARAM].getValue();
    int k=0;
    for(;k<maxChannels;k++) {
      outputs[GATE_OUTPUT].setVoltage(gates[currentPattern][k]?10.f:0.f,k);
    }
    outputs[GATE_OUTPUT].setChannels(maxChannels);
  }

  json_t *dataToJson() override {
    json_t *data=json_object();
    json_t *patternList=json_array();
    for(int k=0;k<MAX_PATS;k++) {
      json_t *onList=json_array();
      for(int j=0;j<PORT_MAX_CHANNELS;j++) {
        json_array_append_new(onList,json_boolean(gates[k][j]));
      }
      json_array_append_new(patternList,onList);
    }
    json_object_set_new(data,"gates",patternList);
    json_object_set_new(data,"channels",json_integer(maxChannels));
    json_object_set_new(data,"gateCountFromZero",json_boolean(gateCountFromZero));
    return data;
  }

  void dataFromJson(json_t *rootJ) override {
    json_t *data=json_object_get(rootJ,"gates");
    if(!data)
      return;
    for(int k=0;k<MAX_PATS;k++) {
      json_t *arr=json_array_get(data,k);
      if(arr) {
        for(int j=0;j<PORT_MAX_CHANNELS;j++) {
          json_t *on=json_array_get(arr,j);
          gates[k][j]=json_boolean_value(on);
        }
      }
    }
    json_t *jMaxChannels=json_object_get(rootJ,"channels");
    if(jMaxChannels)
      maxChannels=int(json_integer_value(jMaxChannels));
    json_t *jGateCountFromZero=json_object_get(rootJ,"gateCountFromZero");
    if(jGateCountFromZero)
      gateCountFromZero=json_boolean_value(jGateCountFromZero);
  }

  void onReset(const ResetEvent &e) override {
    params[PAT_PARAM].setValue(0);
    for(int k=0;k<MAX_PATS;k++) {
      for(int j=0;j<PORT_MAX_CHANNELS;j++) {
        gates[k][j]=false;
      }
    }
    for(int k=0;k<PORT_MAX_CHANNELS;k++) {
      outputs[GATE_OUTPUT].setVoltage(0,k);
    }
  }

  void insert() {
    int currentPattern=params[PAT_PARAM].getValue();
    for(int k=MAX_PATS-1;k>currentPattern;k--) {
      for(int j=0;j<16;j++) {
        gates[k][j]=gates[k-1][j];
      }
    }
    for(int j=0;j<16;j++) {
      gates[currentPattern][j]=false;
    }
  }

  void del() {
    int currentPattern=params[PAT_PARAM].getValue();
    for(int k=currentPattern;k<MAX_PATS-1;k++) {
      for(int j=0;j<16;j++) {
        gates[k][j]=gates[k+1][j];
      }
    }
  }

  void copy() {
    int currentPattern=params[PAT_PARAM].getValue();
    for(int j=0;j<PORT_MAX_CHANNELS;j++) {
      cbGates[j]=gates[currentPattern][j];
    }
  }

  void paste() {
    int currentPattern=params[PAT_PARAM].getValue();
    for(int j=0;j<PORT_MAX_CHANNELS;j++) {
      gates[currentPattern][j]=cbGates[j];
    }
  }

  void updateKey(int key) {
    int currentPattern=params[PAT_PARAM].getValue();
    gates[currentPattern][key]=!(gates[currentPattern][key]);
  }

  void switchPattern() {
  }

  bool getGateStatus(int key) {
    int pattern=int(params[PAT_PARAM].getValue());
    return gates[pattern][key];
  }

  void randomizePattern(int pattern) {
    for(int k=0;k<maxChannels;k++) {
      gates[pattern][k]=rnd.nextCoin(1-randomDens);
    }
  }

  void onRandomize(const RandomizeEvent &e) override {
    int pattern=int(params[PAT_PARAM].getValue());
    randomizePattern(pattern);
  }
};


template<typename M>
struct PatternSelect : SpinParamWidget {
  M *module;

  PatternSelect() {
    init();
  }

  void onChange(const event::Change &e) override {
    if(module)
      module->switchPattern();
  }
};

struct AGWidget : ModuleWidget {
  AGWidget(AG *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/AG.svg")));

    float x=1.9;
    auto patParam=createParam<PatternSelect<AG>>(mm2px(Vec(1.9f,11)),module,AG::PAT_PARAM);
    patParam->module=module;
    addParam(patParam);
    addInput(createInput<SmallPort>(mm2px(Vec(x,22)),module,AG::PAT_CV_INPUT));
    auto editButton=createParam<SmallButtonWithLabel>(mm2px(Vec(1.7f,30)),module,AG::EDIT_PARAM);
    editButton->setLabel("Edit");
    addParam(editButton);
    auto copyButton=createParam<CopyButton<AG>>(mm2px(Vec(1.7f,34)),module,AG::COPY_PARAM);
    copyButton->label="Cpy";
    copyButton->module=module;
    addParam(copyButton);
    auto pasteButton=createParam<PasteButton<AG>>(mm2px(Vec(1.7f,38)),module,AG::PASTE_PARAM);
    pasteButton->label="Pst";
    pasteButton->module=module;
    addParam(pasteButton);
    auto insertButton=createParam<InsertButton<AG>>(mm2px(Vec(1.7f,42)),module,AG::INSERT_PARAM);
    insertButton->label="+";
    insertButton->module=module;
    addParam(insertButton);
    auto delButton=createParam<DelButton<AG>>(mm2px(Vec(1.7f,46)),module,AG::DELETE_PARAM);
    delButton->label="-";
    delButton->module=module;
    addParam(delButton);

    addChild(new GateDisplay<AG>(module,mm2px(Vec(1.7f,54))));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,116)),module,AG::GATE_OUTPUT));
  }

  void appendContextMenu(Menu *menu) override {
    AG *module=dynamic_cast<AG *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    auto channelSelect=new IntSelectItem(&module->maxChannels,1,PORT_MAX_CHANNELS);
    channelSelect->text="Polyphonic Channels";
    channelSelect->rightText=string::f("%d",module->maxChannels)+"  "+RIGHT_ARROW;
    menu->addChild(channelSelect);
    menu->addChild(new DensMenuItem<AG>(module));
    menu->addChild(createBoolPtrMenuItem("Count from zero","",&module->gateCountFromZero));

  }
};


Model *modelAG=createModel<AG,AGWidget>("AG");
