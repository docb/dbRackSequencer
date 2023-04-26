#include "plugin.hpp"
#include "hexfield.h"
#include "hexutil.h"
#undef NUMSEQ
#define NUMSEQ 16
#define NUMPAT 100

struct HexSeqP2 : Module {
  enum ParamId {
    PAT_PARAM,EDIT_PARAM,COPY_PARAM,PASTE_PARAM,INSERT_PARAM,DELETE_PARAM,MAIN_PROB_PARAM,INV_PROB_PARAM=MAIN_PROB_PARAM+16,ON_PARAM=INV_PROB_PARAM+16,PARAMS_LEN=ON_PARAM+16
  };
  enum InputId {
    CLK_INPUT,RST_INPUT,PAT_CV_INPUT,INPUTS_LEN
  };
  enum OutputId {
    TRG_OUTPUT,GATE_OUTPUT,CLK_OUTPUT,INV_OUTPUT,INV_CLK_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN = NUMSEQ*16
  };
  int currentPattern=0;
  unsigned long songpos = 0;
  std::string hexs[NUMPAT][NUMSEQ]={};
  std::string clipBoard[NUMSEQ];
  bool dirty[NUMSEQ]={};
  bool state[NUMSEQ]={};
  dsp::PulseGenerator gatePulseGenerators[NUMSEQ];
  dsp::PulseGenerator gatePulseInvGenerators[NUMSEQ];
  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::SchmittTrigger patTrigger[NUMSEQ];
  int clockCounter=-1;
  unsigned int delay = 0;
  bool showLights = true;
  float lastClock=0.f;
  float randomDens = 0.3;
  int randomLengthFrom = 8;
  int randomLengthTo = 8;
  RND rnd;
  void setDirty(int nr,bool _dirty) {
    dirty[nr]=_dirty;
  }

  bool isDirty(int nr) {
    return dirty[nr];
  }


  void setHex(int nr,const std::string &hexStr) {
    hexs[currentPattern][nr]=hexStr;
  }

  std::string getHex(int nr) {
    return hexs[currentPattern][nr];
  }

  HexSeqP2() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configParam(PAT_PARAM,0,NUMPAT-1,0,"Pattern Selection");
    getParamQuantity(PAT_PARAM)->snapEnabled=true;
    configButton(COPY_PARAM,"Copy");
    configButton(PASTE_PARAM,"Paste");
    configButton(INSERT_PARAM,"Insert");
    configButton(DELETE_PARAM,"Delete");
    configButton(EDIT_PARAM,"Edit");
    configInput(PAT_CV_INPUT,"Pattern (0.1V/Step)");
    configOutput(TRG_OUTPUT,"Trigger");
    configOutput(GATE_OUTPUT,"Gate");
    configOutput(CLK_OUTPUT,"Clock");
    configOutput(CLK_OUTPUT,"Inverted Clock");
    configOutput(INV_OUTPUT,"Inverted");
    for(int k=0;k<16;k++) {
      configParam(MAIN_PROB_PARAM+k,0,2,1,"Trigger Probability");
      configParam(INV_PROB_PARAM+k,0,1,1,"Inverted Trigger Probability");
      configButton(ON_PARAM+k,"On/Off");
      getParamQuantity(ON_PARAM+k)->setValue(1.f);
    }
  }

  unsigned int hexToInt(const std::string &hex) {
    if(hex == "*") return rnd.nextRange(0,15);
    unsigned int x;
    std::stringstream ss;
    ss<<std::hex<<hex;
    ss>>x;
    return x;
  }

  bool adjOn(bool on,int seq) {
    if(params[ON_PARAM+seq].getValue() == 0.f) return false;
    float prob = params[MAIN_PROB_PARAM+seq].getValue();
    if(prob==1.f) return on;
    if(prob<1) {
      if(on) return rnd.nextCoin(1-prob);
      return false;
    }
    if(!on) return rnd.nextCoin(2-prob);
    return true;
  }

  bool adjOnInv(bool on,int seq) {
    if(params[ON_PARAM+seq].getValue() == 0.f) return false;
    float prob = params[INV_PROB_PARAM+seq].getValue();
    if(prob==1.f) return !on;
    if(!on) return rnd.nextCoin(1-prob);
    return false;
  }

  void next() {
    for(int k=0;k<NUMSEQ;k++) {
      for(int j=0;j<16;j++) {
        lights[k*16+j].setBrightness(0.f);
      }
      unsigned int len=hexs[currentPattern][k].length();
      if(len>0) {
        unsigned spos=songpos%(len*4);
        unsigned charPos=spos/4;
        lights[k*16+charPos].setBrightness(1.f);
        std::string hs=hexs[currentPattern][k].substr(charPos,1);
        unsigned int hex=hexToInt(hs);
        unsigned posInChar=spos%4;
        bool on=hex>>(3-posInChar)&0x01;

        if(adjOn(on,k)) {
          gatePulseGenerators[k].trigger(0.01f);
          state[k] = true;
        } else {
          state[k]=false;
          if(adjOnInv(on,k)) {
            gatePulseInvGenerators[k].trigger(0.01f);
          }
        }
      }
    }
    songpos++;
  }

  void copy() {
    for(int k=0;k<NUMSEQ;k++) {
      clipBoard[k] = hexs[currentPattern][k];
    }
  }

  void paste() {
    for(int k=0;k<NUMSEQ;k++) {
      hexs[currentPattern][k] = clipBoard[k];
      dirty[k]=true;
    }
  }

  void insert() {
    for(int k=NUMPAT-1;k>currentPattern;k--) {
      for(int j=0;j<NUMSEQ;j++) {
        hexs[k][j]=hexs[k-1][j];
      }
    }
    for(int j=0;j<16;j++) {
      hexs[currentPattern][j]="";
      dirty[j]=true;
    }
  }

  void del() {
    for(int k=currentPattern;k<NUMPAT-1;k++) {
      for(int j=0;j<NUMSEQ;j++) {
        hexs[k][j]=hexs[k+1][j];
      }
    }
    for(int j=0;j<16;j++) {
      dirty[j]=true;
    }
  }

  void switchPattern() {
    currentPattern=params[PAT_PARAM].getValue();
    INFO("current pattern: %d",currentPattern);
    for(int k=0;k<NUMSEQ;k++) {
      dirty[k]=true;
    }
  }

  void process(const ProcessArgs &args) override {
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())) {
      for(int k=0;k<NUMSEQ;k++) {
        songpos=0;
      }
    }

    if(inputs[PAT_CV_INPUT].isConnected() && params[EDIT_PARAM].getValue() == 0) {
      int c=clamp(inputs[PAT_CV_INPUT].getVoltage(),0.f,9.99f)*float(NUMPAT)/10.f;
      setImmediateValue(getParamQuantity(PAT_PARAM),c);
    }
    int cp=params[PAT_PARAM].getValue();
    if(cp!=currentPattern) {
      currentPattern=cp;
      songpos=0;
    }

    if(inputs[CLK_INPUT].isConnected()) {
      if(clockTrigger.process(inputs[CLK_INPUT].getVoltage())) {
        clockCounter=delay;
      }
      if(clockCounter>0) {
        clockCounter--;
      }
      if(clockCounter==0) {
        clockCounter--;
        next();
      }
    }
    for(int k=0;k<NUMSEQ;k++) {
      bool trigger=gatePulseGenerators[k].process(1.0/args.sampleRate);
      outputs[TRG_OUTPUT].setVoltage((trigger?10.0f:0.0f),k);
      outputs[GATE_OUTPUT].setVoltage(state[k]?10.0f:0.0f,k);
      outputs[CLK_OUTPUT].setVoltage(state[k]&&lastClock>1.f?10.0f:0.0f,k);
      lastClock = inputs[CLK_INPUT].getVoltage();
      trigger=gatePulseInvGenerators[k].process(1.0/args.sampleRate);
      outputs[INV_OUTPUT].setVoltage((trigger?10.0f:0.0f),k);
      outputs[INV_CLK_OUTPUT].setVoltage(!state[k]&&lastClock>1.f?10.0f:0.0f,k);
    }
    outputs[TRG_OUTPUT].setChannels(16);
    outputs[GATE_OUTPUT].setChannels(16);
    outputs[CLK_OUTPUT].setChannels(16);
    outputs[INV_OUTPUT].setChannels(16);
    outputs[INV_CLK_OUTPUT].setChannels(16);

  }

  json_t *dataToJson() override {
    json_t *data=json_object();
    json_t *patList=json_array();
    for(int j=0;j<NUMPAT;j++) {
      json_t *hexList=json_array();
      for(int k=0;k<NUMSEQ;k++) {
        json_array_append_new(hexList,json_string(hexs[j][k].c_str()));
      }
      json_array_append_new(patList,hexList);
    }
    json_object_set_new(data,"hexStrings",patList);
    json_object_set_new(data,"showLights",json_boolean(showLights));
    json_object_set_new(data,"delay",json_integer(delay));
    json_object_set_new(data,"randomDens",json_real(randomDens));
    json_object_set_new(data,"randomLengthFrom",json_integer(randomLengthFrom));
    json_object_set_new(data,"randomLengthTo",json_integer(randomLengthTo));
    return data;
  }

  void dataFromJson(json_t *rootJ) override {
    json_t *patList=json_object_get(rootJ,"hexStrings");
    for(int k=0;k<NUMPAT;k++) {
      json_t *hexList = json_array_get(patList,k);
      for(int j=0;j<NUMSEQ;j++) {
        json_t *hexStr=json_array_get(hexList,j);
        hexs[k][j]=json_string_value(hexStr);
        dirty[j]=true;
      }
    }
    json_t *jShowLights = json_object_get(rootJ,"showLights");
    if(jShowLights!=nullptr) showLights = json_boolean_value(jShowLights);
    json_t *jDelay =json_object_get(rootJ,"delay");
    if(jDelay!=nullptr) delay = json_integer_value(jDelay);
    json_t *jDens =json_object_get(rootJ,"randomDens");
    if(jDens!=nullptr) randomDens = json_real_value(jDens);
    json_t *jRandomLengthFrom =json_object_get(rootJ,"randomLengthFrom");
    if(jRandomLengthFrom!=nullptr) randomLengthFrom= json_integer_value(jRandomLengthFrom);
    json_t *jRandomLengthTo =json_object_get(rootJ,"randomLengthTo");
    if(jRandomLengthTo!=nullptr) randomLengthTo = json_integer_value(jRandomLengthTo);
  }
  void randomizeField(int j,int k) {
    hexs[j][k]=getRandomHex(rnd,randomDens,randomLengthFrom,randomLengthTo);
    dirty[k]=true;
  }
  void randomizeCurrentPattern() {
    for(int k=0;k<NUMSEQ;k++) {
      randomizeField(currentPattern,k);
    }
  }
  void initializeCurrentPattern() {
    for(int k=0;k<NUMSEQ;k++) {
      hexs[currentPattern][k]="";
      dirty[k]=true;
    }
  }
  void onRandomize(const RandomizeEvent &e) override {
    for(int j=0;j<NUMPAT;j++) {
      for(int k=0;k<NUMSEQ;k++) {
        randomizeField(j,k);
      }
    }
  }

  void onReset(const ResetEvent &e) override {
    for(int j=0;j<NUMPAT;j++) {
      for(int k=0;k<NUMSEQ;k++) {
        hexs[j][k]="";
        dirty[k]=true;
      }
    }
  }
};
struct HexSeqP2Widget;

struct HexFieldP2 : HexField<HexSeqP2,HexSeqP2Widget> {
  HexFieldP2() : HexField<HexSeqP2,HexSeqP2Widget>() {
    font_size=14;
    textOffset=Vec(2.f,1.f);
    box.size=mm2px(Vec(45.5,5.f));
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


struct HexSeqP2Widget : ModuleWidget {
  std::vector<HexFieldP2*> fields;
  std::vector<DBSmallLight<GreenLight>*> lights;


  HexSeqP2Widget(HexSeqP2 *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/HexSeqP2.svg")));

    float y=11;
    float xc=81.5;
    float x1=54.5;
    float x2=63.5;
    float x3=72.5;
    addInput(createInput<SmallPort>(mm2px(Vec(14,2)),module,HexSeqP2::CLK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(32,2)),module,HexSeqP2::RST_INPUT));
    auto patParam=createParam<PatternSelect<HexSeqP2>>(mm2px(Vec(xc,y)),module,HexSeqP2::PAT_PARAM);
    patParam->module=module;
    addParam(patParam);
    y+=11;
    addInput(createInput<SmallPort>(mm2px(Vec(xc,y)),module,HexSeqP2::PAT_CV_INPUT));
    y+=8;
    auto editButton=createParam<SmallButtonWithLabel>(mm2px(Vec(xc,y)),module,HexSeqP2::EDIT_PARAM);
    editButton->setLabel("Edit");
    addParam(editButton);
    y+=4;
    auto copyButton=createParam<CopyButton<HexSeqP2>>(mm2px(Vec(xc,y)),module,HexSeqP2::COPY_PARAM);
    copyButton->label="Cpy";
    copyButton->module=module;
    addParam(copyButton);
    y+=4;
    auto pasteButton=createParam<PasteButton<HexSeqP2>>(mm2px(Vec(xc,y)),module,HexSeqP2::PASTE_PARAM);
    pasteButton->label="Pst";
    pasteButton->module=module;
    addParam(pasteButton);
    y+=4;
    auto insertButton=createParam<InsertButton<HexSeqP2>>(mm2px(Vec(xc,y)),module,HexSeqP2::INSERT_PARAM);
    insertButton->label="+";
    insertButton->module=module;
    addParam(insertButton);
    y+=4;
    auto delButton=createParam<DelButton<HexSeqP2>>(mm2px(Vec(xc,y)),module,HexSeqP2::DELETE_PARAM);
    delButton->label="-";
    delButton->module=module;
    addParam(delButton);
    y=11;

    for(int k=0;k<NUMSEQ;k++) {
      auto *textField=createWidget<HexFieldP2>(mm2px(Vec(3,12+k*7)));
      textField->setModule(module);
      textField->setModuleWidget(this);
      textField->nr=k;
      textField->multiline=false;
      fields.push_back(textField);
      addChild(textField);
      for(int j=0;j<16;j++) {
        auto light = createLight<DBSmallLight<GreenLight>>(mm2px(Vec(3.7f+2.75f*j,10+k*7)), module, k*16+j);
        addChild(light);
        light->setVisible(module!=nullptr&&module->showLights);
        lights.push_back(light);
      }
      addParam(createParam<TrimbotWhite>(mm2px(Vec(x1,y)),module,HexSeqP2::MAIN_PROB_PARAM+k));
      addParam(createParam<TrimbotWhite>(mm2px(Vec(x2,y)),module,HexSeqP2::INV_PROB_PARAM+k));
      addParam(createParam<MLED>(mm2px(Vec(x3,y)),module,HexSeqP2::ON_PARAM+k));
      y+=7;
    }
    y=60;
    addOutput(createOutput<SmallPort>(mm2px(Vec(xc,y)),module,HexSeqP2::TRG_OUTPUT));
    y+=12;
    addOutput(createOutput<SmallPort>(mm2px(Vec(xc,y)),module,HexSeqP2::GATE_OUTPUT));
    y+=12;
    addOutput(createOutput<SmallPort>(mm2px(Vec(xc,y)),module,HexSeqP2::CLK_OUTPUT));
    y=104;
    addOutput(createOutput<SmallPort>(mm2px(Vec(xc,y)),module,HexSeqP2::INV_OUTPUT));
    y+=12;
    addOutput(createOutput<SmallPort>(mm2px(Vec(xc,y)),module,HexSeqP2::INV_CLK_OUTPUT));
  }
  void onHoverKey(const event::HoverKey &e) override {
    if (e.action == GLFW_PRESS) {
      int k=e.key-48;
      if(k>=1&&k<10) {
        fields[k-1]->onWidgetSelect=true;
        APP->event->setSelectedWidget(fields[k-1]);
      }
    }
    ModuleWidget::onHoverKey(e);
  }
  void moveFocusDown(int current) {
    APP->event->setSelectedWidget(fields[(current+1)%NUMSEQ]);
  }
  void moveFocusUp(int current) {
    APP->event->setSelectedWidget(fields[(NUMSEQ+current-1)%NUMSEQ]);
  }
  void toggleLights() {
    HexSeqP2* module = dynamic_cast<HexSeqP2*>(this->module);
    if(!module->showLights) {
      for(auto light: lights) {
        light->setVisible(true);
      }
      module->showLights = true;
    } else {
      for(auto light: lights) {
        light->setVisible(false);
      }
      module->showLights = false;
    }
  }
  void appendContextMenu(Menu* menu) override {
    HexSeqP2* module = dynamic_cast<HexSeqP2*>(this->module);
    assert(module);

    menu->addChild(new MenuSeparator);

    menu->addChild(createCheckMenuItem("ShowLights", "",
                                       [=]() {return module->showLights;},
                                       [=]() { toggleLights();}));
    struct DelayItem : MenuItem {
      HexSeqP2* module;
      Menu* createChildMenu() override {
        Menu* menu = new Menu;
        for (unsigned int c = 0; c <= 10; c++) {
          menu->addChild(createCheckMenuItem(rack::string::f("%d", c), "",
                                             [=]() {return module->delay == c;},
                                             [=]() {module->delay = c;}
          ));
        }
        return menu;
      }
    };
    auto* delayItem = new DelayItem;
    delayItem->text = "Clock In Delay";
    delayItem->rightText = rack::string::f("%d", module->delay) + "  " + RIGHT_ARROW;
    delayItem->module = module;
    menu->addChild(delayItem);
    menu->addChild(new DensMenuItem<HexSeqP2>(module));
    auto* randomLengthFromItem = new IntSelectItem(&module->randomLengthFrom,1,16);
    randomLengthFromItem->text = "Random length from";
    randomLengthFromItem->rightText = rack::string::f("%d", module->randomLengthFrom) + "  " + RIGHT_ARROW;
    menu->addChild(randomLengthFromItem);
    auto* randomLengthToItem = new IntSelectItem(&module->randomLengthTo,1,16);
    randomLengthToItem->text = "Random length to";
    randomLengthToItem->rightText = rack::string::f("%d", module->randomLengthTo) + "  " + RIGHT_ARROW;
    menu->addChild(randomLengthToItem);
    struct RandomizePattern : ui::MenuItem {
      HexSeqP2 *module;
      RandomizePattern(HexSeqP2 *m) : module(m) {}
      void onAction(const ActionEvent& e) override {
        if (!module)
          return;
        module->randomizeCurrentPattern();
      }
    };
    auto rpMenu = new RandomizePattern(module);
    rpMenu->text = "Randomize Pattern";
    menu->addChild(rpMenu);
    struct InitializePattern : ui::MenuItem {
      HexSeqP2 *module;
      InitializePattern(HexSeqP2 *m) : module(m) {}
      void onAction(const ActionEvent& e) override {
        if (!module)
          return;
        module->initializeCurrentPattern();
      }
    };
    auto ipMenu = new InitializePattern(module);
    ipMenu->text = "Initialize Pattern";
    menu->addChild(ipMenu);

  }
};


Model *modelHexSeqP2=createModel<HexSeqP2,HexSeqP2Widget>("HexSeqP2");