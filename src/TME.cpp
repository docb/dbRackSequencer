#include "plugin.hpp"



struct Triadex {
  ShiftRegister<bool,31> shiftRegister;
  uint8_t interval[4]={};
  uint8_t theme[4]={};
  uint8_t counter=0;
  uint8_t counter6=0;

  bool calculateBit(uint8_t slot) {
    if(slot>8) {
      return shiftRegister.get(slot-9);
    }
    switch(slot) {
      case 1:
        return true;  // ON
      case 2:
        return counter&0x01; // C1/2
      case 3:
        return counter&0x02; // C1
      case 4:
        return counter&0x04; // C2
      case 5:
        return counter&0x08; // C4
      case 6:
        return counter&0x10; // C8
      case 7:
        return ((counter6)/3)%2; // C3
      case 8:
        return ((counter6)/6)%2; // C6
      default:
        return false; // OFF
    }
  }

  uint8_t currentCVIndex() {
    uint8_t index=0;
    for(int i=0,j=1;i<4;i++,j<<=1)
      index|=calculateBit(interval[i])?j:0;
    return index;
  }


  bool nextBit() {
    bool bit=1;
    for(int i=0;i<4;i++)
      bit^=calculateBit(theme[i]);
    return bit;
  }

  void step() {
    counter++;
    counter&=0x1f; // mod 32
    if(counter&0x1) // only relevant for C1/2, ignored in the shift register
      return;
    counter6++;
    if(counter6==24)
      counter6=0;
    bool bit=nextBit();
    shiftRegister.step(bit);
  }

  void reset() {
    counter=0;
    counter6=0;
    shiftRegister.reset();
  }

};

struct TME : Module {
  enum ParamId {
    A_PARAM,B_PARAM,C_PARAM,D_PARAM,W_PARAM,X_PARAM,Y_PARAM,Z_PARAM,RST_PARAM,STEP_PARAM,LVL_PARAM,PARAMS_LEN
  };
  enum InputId {
    CLK_INPUT,RST_INPUT,SCL_INPUT,A_INPUT,B_INPUT,C_INPUT,D_INPUT,W_INPUT,X_INPUT,Y_INPUT,Z_INPUT,INPUTS_LEN
  };
  enum OutputId {
    NOTE_OUTPUT,TRG_OUTPUT,CV_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN=40
  };
  dsp::PulseGenerator resetPulse;
  dsp::PulseGenerator trigPulse;
  dsp::SchmittTrigger resetTrigger;
  dsp::SchmittTrigger manualResetTrigger;
  dsp::SchmittTrigger clockTrigger;
  SchmittTrigger2 schmittTrigger2;
  dsp::SchmittTrigger manualClockTrigger;
  dsp::ClockDivider divider;
  dsp::ClockDivider paramDivider;
  int defaultScale[8]={0,2,4,5,7,9,11,12};
  int scale[8]={0,2,4,5,7,9,11,12};
  Triadex triadex;
  uint8_t cvIndex;
  bool usePWMClock=false;
  int counter=-1;
  bool clock=false;

  uint8_t lastIndex=255;
  std::vector <std::string> labels={"OFF","ON","C1/2","C1","C2","C4","C8","C3","C6","B1","B2","B3","B4","B5","B6","B7","B8","B9","B10","B11","B12","B13","B14","B15","B16","B17","B18","B19","B20","B21","B22","B23","B24","B25","B26","B27","B28","B29","B30","B31"};

  TME() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configSwitch(A_PARAM,0,39,39,"A",labels);
    getParamQuantity(A_PARAM)->snapEnabled=true;
    configSwitch(B_PARAM,0,39,39,"B",labels);
    getParamQuantity(B_PARAM)->snapEnabled=true;
    configSwitch(C_PARAM,0,39,39,"C",labels);
    getParamQuantity(C_PARAM)->snapEnabled=true;
    configSwitch(D_PARAM,0,39,39,"D",labels);
    getParamQuantity(D_PARAM)->snapEnabled=true;
    configSwitch(W_PARAM,0,39,39,"W",labels);
    getParamQuantity(W_PARAM)->snapEnabled=true;
    configSwitch(X_PARAM,0,39,39,"X",labels);
    getParamQuantity(X_PARAM)->snapEnabled=true;
    configSwitch(Y_PARAM,0,39,39,"Y",labels);
    getParamQuantity(Y_PARAM)->snapEnabled=true;
    configSwitch(Z_PARAM,0,39,39,"Z",labels);
    getParamQuantity(Z_PARAM)->snapEnabled=true;
    configParam(LVL_PARAM,0,1,0.1,"CV Output scale");
    configOutput(CV_OUTPUT,"CV");
    configButton(STEP_PARAM,"Step");
    configButton(RST_PARAM,"Reset");
    configInput(CLK_INPUT,"Clock");
    configInput(RST_INPUT,"Reset");
    configInput(SCL_INPUT,"Scale");
    configInput(A_INPUT,"A");
    configInput(B_INPUT,"B");
    configInput(C_INPUT,"C");
    configInput(D_INPUT,"D");
    configInput(W_INPUT,"W");
    configInput(X_INPUT,"Y");
    configInput(Y_INPUT,"X");
    configInput(Z_INPUT,"Z");
    configOutput(NOTE_OUTPUT,"Note");
    configOutput(TRG_OUTPUT,"Trig");
    divider.setDivision(32);
    paramDivider.setDivision(8);
    schmittTrigger2.setThresholds(0.1,1);
  }

  void onReset() override {
    triadex.reset();
  }

  void process(const ProcessArgs &args) override {
    if(paramDivider.process()) {
      if(inputs[SCL_INPUT].isConnected()) {
        for(int i=0;i<8;i++) {
          scale[i]=int(clamp(inputs[SCL_INPUT].getVoltage(i),0.f,1.f)*12.f);
        }
      } else {
        for(int i=0;i<8;i++) {
          scale[i]=defaultScale[i];
        }
      }

      if(inputs[A_INPUT].isConnected()) {
        getParamQuantity(A_PARAM)->setValue(inputs[A_INPUT].getVoltage()*4);
      }
      if(inputs[B_INPUT].isConnected()) {
        getParamQuantity(B_PARAM)->setValue(inputs[B_INPUT].getVoltage()*4);
      }
      if(inputs[C_INPUT].isConnected()) {
        getParamQuantity(C_PARAM)->setValue(inputs[C_INPUT].getVoltage()*4);
      }
      if(inputs[D_INPUT].isConnected()) {
        getParamQuantity(D_PARAM)->setValue(inputs[D_INPUT].getVoltage()*4);
      }
      if(inputs[W_INPUT].isConnected()) {
        getParamQuantity(W_PARAM)->setValue(inputs[W_INPUT].getVoltage()*4);
      }
      if(inputs[X_INPUT].isConnected()) {
        getParamQuantity(X_PARAM)->setValue(inputs[X_INPUT].getVoltage()*4);
      }
      if(inputs[Y_INPUT].isConnected()) {
        getParamQuantity(Y_PARAM)->setValue(inputs[Y_INPUT].getVoltage()*4);
      }
      if(inputs[Z_INPUT].isConnected()) {
        getParamQuantity(Z_PARAM)->setValue(inputs[Z_INPUT].getVoltage()*4);
      }

      for(int i=0;i<4;i++) {
        triadex.theme[i]=params[7-i].getValue()+0.05;
        triadex.interval[i]=params[i].getValue()+0.05;
      }
    }
    if(manualResetTrigger.process(params[RST_PARAM].getValue())|resetTrigger.process(inputs[RST_INPUT].getVoltage())) {
      triadex.reset();
      resetPulse.trigger(0.001f);
      cvIndex=triadex.currentCVIndex();
    }

    bool rstGate=resetPulse.process(args.sampleTime);
    if(usePWMClock) {
      int b=schmittTrigger2.process(inputs[CLK_INPUT].getVoltage());
      if((b==1 && !rstGate)||b==-1) {
        counter = 12;
      }
    } else {
      if((clockTrigger.process(inputs[CLK_INPUT].getVoltage())&&!rstGate)|manualClockTrigger.process(params[STEP_PARAM].getValue())) {
        counter = 12;
      }
    }
    if(counter>=0) {
      if(counter==0) {
        triadex.step();
        cvIndex=triadex.currentCVIndex();
      }
      counter--;
    }
    if(cvIndex!=lastIndex) {
      trigPulse.trigger(0.001f);
    }
    lastIndex=cvIndex;
    outputs[TRG_OUTPUT].setVoltage(trigPulse.process(args.sampleTime)?10.0f:0.0);
    outputs[NOTE_OUTPUT].setVoltage((scale[cvIndex&0x7]+((cvIndex&0x8)?12:0))/12.0);
    outputs[CV_OUTPUT].setVoltage(cvIndex*params[LVL_PARAM].getValue());
    if(divider.process()) {
      for(int i=0;i<39;i++)
        lights[i].setBrightness(triadex.calculateBit(39-i));
    }
  }

  json_t *dataToJson() override {
    json_t *data=json_object();
    json_object_set_new(data,"usePWMClock",json_integer(usePWMClock));
    return data;
  }

  void dataFromJson(json_t *data) override {
    json_t *jUsePWMClock=json_object_get(data,"usePWMClock");
    if(jUsePWMClock) {
      usePWMClock=json_integer_value(jUsePWMClock);
    }

  }

};

struct TMEWidget : ModuleWidget {
  TMEWidget(TME *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/TME.svg")));

    float x=67.5;
    float y=114.5;
    float cellHeight=2.7053;
    for(int i=0;i<31;i++)
      addChild(createLightCentered<DBSmallLight<BlueLight>>(mm2px(Vec(x,y-cellHeight*i)),module,i));

    for(int i=31;i<33;i++)
      addChild(createLightCentered<DBSmallLight<YellowLight>>(mm2px(Vec(x,y-cellHeight*i)),module,i));

    for(int i=33;i<38;i++)
      addChild(createLightCentered<DBSmallLight<GreenLight>>(mm2px(Vec(x,y-cellHeight*i)),module,i));

    for(int i=38;i<40;i++)
      addChild(createLightCentered<DBSmallLight<RedLight>>(mm2px(Vec(x,y-cellHeight*i)),module,i));

    x=3.5;
    y=8.220;
    auto selectParam=createParam<SelectParam>(mm2px(Vec(x,y)),module,TME::A_PARAM);
    selectParam->box.size=mm2px(Vec(6,cellHeight*40));
    selectParam->initWithEmptyLabels(40);
    addParam(selectParam);
    x+=7;
    selectParam=createParam<SelectParam>(mm2px(Vec(x,y)),module,TME::B_PARAM);
    selectParam->box.size=mm2px(Vec(6,cellHeight*40));
    selectParam->initWithEmptyLabels(40);
    addParam(selectParam);
    x+=7;
    selectParam=createParam<SelectParam>(mm2px(Vec(x,y)),module,TME::C_PARAM);
    selectParam->box.size=mm2px(Vec(6,cellHeight*40));
    selectParam->initWithEmptyLabels(40);
    addParam(selectParam);
    x+=7;
    selectParam=createParam<SelectParam>(mm2px(Vec(x,y)),module,TME::D_PARAM);
    selectParam->box.size=mm2px(Vec(6,cellHeight*40));
    selectParam->initWithEmptyLabels(40);
    addParam(selectParam);
    x+=14;
    selectParam=createParam<SelectParam>(mm2px(Vec(x,y)),module,TME::W_PARAM);
    selectParam->box.size=mm2px(Vec(6,cellHeight*40));
    selectParam->initWithEmptyLabels(40);
    addParam(selectParam);
    x+=7;
    selectParam=createParam<SelectParam>(mm2px(Vec(x,y)),module,TME::X_PARAM);
    selectParam->box.size=mm2px(Vec(6,cellHeight*40));
    selectParam->initWithEmptyLabels(40);
    addParam(selectParam);
    x+=7;
    selectParam=createParam<SelectParam>(mm2px(Vec(x,y)),module,TME::Y_PARAM);
    selectParam->box.size=mm2px(Vec(6,cellHeight*40));
    selectParam->initWithEmptyLabels(40);
    addParam(selectParam);
    x+=7;
    selectParam=createParam<SelectParam>(mm2px(Vec(x,y)),module,TME::Z_PARAM);
    selectParam->box.size=mm2px(Vec(6,cellHeight*40));
    selectParam->initWithEmptyLabels(40);
    addParam(selectParam);

    x=72;
    addInput(createInput<SmallPort>(mm2px(Vec(x,14)),module,TME::CLK_INPUT));
    addParam(createParam<MLEDM>(mm2px(Vec(x,22)),module,TME::STEP_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x,35)),module,TME::RST_INPUT));
    addParam(createParam<MLEDM>(mm2px(Vec(x,44)),module,TME::RST_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x,57)),module,TME::SCL_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,74)),module,TME::NOTE_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,86)),module,TME::TRG_OUTPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,98)),module,TME::LVL_PARAM));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,110)),module,TME::CV_OUTPUT));
    x=3;
    addInput(createInput<SmallPort>(mm2px(Vec(x,118)),module,TME::A_INPUT));
    x+=7;
    addInput(createInput<SmallPort>(mm2px(Vec(x,118)),module,TME::B_INPUT));
    x+=7;
    addInput(createInput<SmallPort>(mm2px(Vec(x,118)),module,TME::C_INPUT));
    x+=7;
    addInput(createInput<SmallPort>(mm2px(Vec(x,118)),module,TME::D_INPUT));
    x+=14;
    addInput(createInput<SmallPort>(mm2px(Vec(x,118)),module,TME::W_INPUT));
    x+=7;
    addInput(createInput<SmallPort>(mm2px(Vec(x,118)),module,TME::X_INPUT));
    x+=7;
    addInput(createInput<SmallPort>(mm2px(Vec(x,118)),module,TME::Y_INPUT));
    x+=7;
    addInput(createInput<SmallPort>(mm2px(Vec(x,118)),module,TME::Z_INPUT));
  }
  void appendContextMenu(Menu *menu) override {
    TME *module=dynamic_cast<TME *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    menu->addChild(createCheckMenuItem("PWMClock", "",
                                       [=]() {return module->usePWMClock;},
                                       [=]() { module->usePWMClock=!module->usePWMClock;}));
  }
};


Model *modelTME=createModel<TME,TMEWidget>("TME");