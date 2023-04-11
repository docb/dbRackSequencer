#include "plugin.hpp"


struct SEQMod : Module {
  enum ParamId {
    POS1_PARAM,POS2_PARAM,POS3_PARAM,CMP1_PARAM,CMP2_PARAM,CMP3_PARAM,READ_POS_PARAM,OUT_POS_PARAM,MODE_PARAM,PARAMS_LEN
  };
  enum InputId {
    CLK_INPUT,RST_INPUT,CV_INPUT,POS1_INPUT,POS2_INPUT,POS3_INPUT,CMP1_INPUT,CMP2_INPUT,CMP3_INPUT,READ_POS_INPUT,OUT_POS_INPUT,MODE_INPUT,INPUTS_LEN
  };
  enum OutputId {
    CV_OUTPUT,SR_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    SEQ_LIGHT,SR_LIGHT,CMP1_LIGHT,CMP2_LIGHT,CMP3_LIGHT,S_LIGHT,POS_LIGHT=S_LIGHT+16,LIGHTS_LEN=POS_LIGHT+80
  };
  ShiftRegister<float,16> sr;
  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger resetTrigger;
  dsp::PulseGenerator resetPulse;
  int counter=0;
  float min=-10;
  float max=10;
  int dirty=0;
  bool inv1=false;
  bool inv2=false;
  bool inv3=false;
  dsp::ClockDivider paramDivider;
  dsp::ClockDivider lightDivider;


  SEQMod() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configParam(POS1_PARAM,0,15,15,"Read Pos 1");
    getParamQuantity(POS1_PARAM)->snapEnabled=true;
    configParam(POS2_PARAM,0,15,14,"Read Pos 2");
    getParamQuantity(POS2_PARAM)->snapEnabled=true;
    configParam(POS3_PARAM,0,15,13,"Read Pos 3");
    getParamQuantity(POS3_PARAM)->snapEnabled=true;
    configParam(CMP1_PARAM,-10,10,0.5,"Cmp 1");
    configParam(CMP2_PARAM,-10,10,0.5,"Cmp 2");
    configParam(CMP3_PARAM,-10,10,0.5,"Cmp 3");
    configParam(READ_POS_PARAM,0,15,7,"Read Pos SR");
    getParamQuantity(READ_POS_PARAM)->snapEnabled=true;
    configParam(OUT_POS_PARAM,0,15,0,"Out Pos");
    getParamQuantity(OUT_POS_PARAM)->snapEnabled=true;
    configSwitch(MODE_PARAM,0,3,0,"Mode",{"Mod","SR","Seq"});
    configInput(POS1_INPUT,"Read Pos 1");
    configInput(POS2_INPUT,"Read Pos 2");
    configInput(POS3_INPUT,"Read Pos 3");
    configInput(MODE_INPUT,"Mode");
    configInput(CMP1_INPUT,"Cmp 1");
    configInput(CMP2_INPUT,"Cmp 2");
    configInput(CMP3_INPUT,"Cmp 3");
    configInput(READ_POS_INPUT,"Read Pos SR");
    configInput(OUT_POS_INPUT,"Out Pos");
    configInput(CV_INPUT,"CV");
    configInput(RST_INPUT,"Reset");
    configInput(CLK_INPUT,"Clock");
    configOutput(CV_OUTPUT,"CV");
    configOutput(SR_OUTPUT,"SR");
    configBypass(CV_INPUT,CV_OUTPUT);
    lightDivider.setDivision(32);
    paramDivider.setDivision(32);
  }

  bool cmp1() {
    bool b1=sr.get(params[POS1_PARAM].getValue())>=params[CMP1_PARAM].getValue();
    if(inv1)
      b1=!b1;
    return b1;
  }

  bool cmp2() {
    bool b2=sr.get(params[POS2_PARAM].getValue())>=params[CMP2_PARAM].getValue();
    if(inv2)
      b2=!b2;
    return b2;
  }

  bool cmp3() {
    bool b3=sr.get(params[POS3_PARAM].getValue())>=params[CMP3_PARAM].getValue();
    if(inv3)
      b3=!b3;
    return b3;
  }
  bool decide() {
    if(params[MODE_PARAM].getValue()==1)
      return false;
    if(params[MODE_PARAM].getValue()==2)
      return true;

    bool b1=cmp1();
    bool b2=cmp2();
    bool b3=cmp3();
    return (!(b1^b2))^b3;
  }

  float nextIn() {
    return decide()?inputs[CV_INPUT].getVoltage():(params[MODE_PARAM].getValue()==1?sr.get(15):sr.get(params[READ_POS_PARAM].getValue()));
  }

  void showLights() {
    lights[SEQ_LIGHT].setBrightness(decide()?1.f:0.f);
    lights[SR_LIGHT].setBrightness(decide()?0.f:1.f);
    lights[CMP1_LIGHT].setBrightness(cmp1()?0.f:1.f);
    lights[CMP2_LIGHT].setBrightness(cmp2()?0.f:1.f);
    lights[CMP3_LIGHT].setBrightness(cmp3()?0.f:1.f);
    float vmin=10;
    float vmax=-10;
    for(int k=0;k<16;k++) {
      if(sr.get(k)<vmin)
        vmin=sr.get(k);
      if(sr.get(k)>vmax)
        vmax=sr.get(k);
    }
    for(int k=0;k<16;k++) {
      lights[S_LIGHT+k].setBrightness(math::rescale(sr.get(k),vmin,vmax,0.f,1.f));
      lights[POS_LIGHT+k].setBrightness(0.f);
      if(params[POS1_PARAM].getValue()!=k)
        lights[POS_LIGHT+k].setBrightness(0.f);
      else
        lights[POS_LIGHT+k].setBrightness(0.5);

      if(params[POS2_PARAM].getValue()!=k)
        lights[POS_LIGHT+k+16].setBrightness(0.f);
      else
        lights[POS_LIGHT+k+16].setBrightness(0.5);

      if(params[POS3_PARAM].getValue()!=k)
        lights[POS_LIGHT+k+32].setBrightness(0.f);
      else
        lights[POS_LIGHT+k+32].setBrightness(0.5);

      if(params[READ_POS_PARAM].getValue()!=k)
        lights[POS_LIGHT+k+48].setBrightness(0.f);
      else
        lights[POS_LIGHT+k+48].setBrightness(0.5);

      if(params[OUT_POS_PARAM].getValue()!=k)
        lights[POS_LIGHT+k+64].setBrightness(0.f);
      else
        lights[POS_LIGHT+k+64].setBrightness(0.5);
    }

  }

  void process(const ProcessArgs &args) override {
    if(lightDivider.process()) showLights();
    if(paramDivider.process()) {
      if(inputs[POS1_INPUT].isConnected()) {
        getParamQuantity(POS1_PARAM)->setValue(inputs[POS1_INPUT].getVoltage()*1.6);
      }
      if(inputs[POS2_INPUT].isConnected()) {
        getParamQuantity(POS2_PARAM)->setValue(inputs[POS2_INPUT].getVoltage()*1.6);
      }
      if(inputs[POS3_INPUT].isConnected()) {
        getParamQuantity(POS3_PARAM)->setValue(inputs[POS3_INPUT].getVoltage()*1.6);
      }
      if(inputs[CMP1_INPUT].isConnected()) {
        getParamQuantity(CMP1_PARAM)->setValue(inputs[CMP1_INPUT].getVoltage());
      }
      if(inputs[CMP2_INPUT].isConnected()) {
        getParamQuantity(CMP2_PARAM)->setValue(inputs[CMP2_INPUT].getVoltage());
      }
      if(inputs[CMP3_INPUT].isConnected()) {
        getParamQuantity(CMP3_PARAM)->setValue(inputs[CMP3_INPUT].getVoltage());
      }
      if(inputs[READ_POS_INPUT].isConnected()) {
        getParamQuantity(READ_POS_PARAM)->setValue(inputs[READ_POS_INPUT].getVoltage());
      }
      if(inputs[OUT_POS_INPUT].isConnected()) {
        getParamQuantity(OUT_POS_PARAM)->setValue(inputs[OUT_POS_INPUT].getVoltage());
      }
    }
    if(resetTrigger.process(inputs[RST_INPUT].getVoltage())) {
      sr.reset();
      //sr.step(nextIn());
      for(unsigned k=0;k<16;k++) {
        outputs[CV_OUTPUT].setVoltage(0.f,k);
      }
      resetPulse.trigger(0.001f);
    }
    bool resetGate=resetPulse.process(args.sampleTime);
    if(clockTrigger.process(inputs[CLK_INPUT].getVoltage())&&!resetGate) {
      counter=10;
    }

    if(counter>=0) {
      if(counter==0) {
        sr.step(nextIn());
      }
      counter--;
    }
    outputs[CV_OUTPUT].setVoltage(sr.get(params[OUT_POS_PARAM].getValue()));
    for(int k=0;k<16;k++) {
      outputs[SR_OUTPUT].setVoltage(sr.get(15-k),k);
    }
    outputs[SR_OUTPUT].setChannels(16);
  }

  void reconfig() {
    for(int k=0;k<3;k++) {
      float value=getParamQuantity(CMP1_PARAM+k)->getImmediateValue();
      if(value>max)
        value=max;
      if(value<min)
        value=min;
      configParam(CMP1_PARAM+k,min,max,0,"CMP "+std::to_string(k+1));
      getParamQuantity(CMP1_PARAM+k)->setValue(value);
      dirty=3;
    }
  }
  void fromJson(json_t *root) override {
    min=-10.f;
    max=10.f;
    reconfig();
    Module::fromJson(root);
  }
  json_t *dataToJson() override {
    json_t *root=json_object();
    json_object_set_new(root,"min",json_real(min));
    json_object_set_new(root,"max",json_real(max));
    json_t *jSR=json_array();
    for(int k=0;k<16;k++) {
      json_array_append_new(jSR,json_real(sr.get(k)));
    }
    json_object_set_new(root,"sr",jSR);
    return root;
  }

  void dataFromJson(json_t *root) override {
    json_t *jMin=json_object_get(root,"min");
    if(jMin) {
      min=json_real_value(jMin);
    }
    json_t *jMax=json_object_get(root,"max");
    if(jMax) {
      max=json_real_value(jMax);
    }
    reconfig();
    json_t *jSR=json_object_get(root,"sr");
    if(jSR) {
      for(int k=0;k<16;k++)
        sr.set(k,json_real_value(json_array_get(jSR,k)));
    }
  }
};


struct SEQModWidget : ModuleWidget {
  SEQModWidget(SEQMod *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/SEQMod.svg")));

    addChild(createWidget<ScrewSilver>(Vec(0,0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-15,0)));
    addChild(createWidget<ScrewSilver>(Vec(0,365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-15,365)));

    float x=3;
    float y=13;
    float c=15;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,SEQMod::POS1_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x,y+7)),module,SEQMod::POS1_INPUT));
    auto cmpParam=createParam<MKnob<SEQMod>>(mm2px(Vec(x+18,y)),module,SEQMod::CMP1_PARAM);
    cmpParam->module=module;
    addParam(cmpParam);
    addInput(createInput<SmallPort>(mm2px(Vec(x+18,y+7)),module,SEQMod::CMP1_INPUT));
    addChild(createLight<TinySimpleLight<GreenLight>>(mm2px(Vec(x+24.5,y+4.5)),module,SEQMod::CMP1_LIGHT));
    y+=18;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,SEQMod::POS2_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x,y+7)),module,SEQMod::POS2_INPUT));
    cmpParam=createParam<MKnob<SEQMod>>(mm2px(Vec(x+18,y)),module,SEQMod::CMP2_PARAM);
    cmpParam->module=module;
    addParam(cmpParam);
    addChild(createLight<TinySimpleLight<GreenLight>>(mm2px(Vec(x+24.5,y+4.5)),module,SEQMod::CMP2_LIGHT));
    addInput(createInput<SmallPort>(mm2px(Vec(x+18,y+7)),module,SEQMod::CMP2_INPUT));
    y+=18;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,SEQMod::POS3_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x,y+7)),module,SEQMod::POS3_INPUT));
    cmpParam=createParam<MKnob<SEQMod>>(mm2px(Vec(x+18,y)),module,SEQMod::CMP3_PARAM);
    cmpParam->module=module;
    addParam(cmpParam);
    addChild(createLight<TinySimpleLight<GreenLight>>(mm2px(Vec(x+24.5,y+4.5)),module,SEQMod::CMP3_LIGHT));
    addInput(createInput<SmallPort>(mm2px(Vec(x+18,y+7)),module,SEQMod::CMP3_INPUT));
    y=70;

    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,SEQMod::CLK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x+18,y)),module,SEQMod::RST_INPUT));

    addInput(createInput<SmallPort>(mm2px(Vec(21,82)),module,SEQMod::MODE_INPUT));

    auto selectParam=createParam<SelectParam>(mm2px(Vec(3,80)),module,SEQMod::MODE_PARAM);
    selectParam->box.size=mm2px(Vec(8,12));
    selectParam->init({"Mod","SR","Seq"});
    addParam(selectParam);
    addChild(createLightCentered<DBSmallLight<GreenLight>>(mm2px(Vec(c,80+6)),module,SEQMod::SR_LIGHT));
    addChild(createLightCentered<DBSmallLight<GreenLight>>(mm2px(Vec(c,80+10)),module,SEQMod::SEQ_LIGHT));
    y=97;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,SEQMod::READ_POS_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x,y+7)),module,SEQMod::READ_POS_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x+18,y)),module,SEQMod::OUT_POS_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+18,y+7)),module,SEQMod::OUT_POS_INPUT));
    y=110;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+9,y)),module,SEQMod::SR_OUTPUT));
    y=116;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,SEQMod::CV_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+18,y)),module,SEQMod::CV_OUTPUT));

    x=13.5;
    y=14.5;
    for(int k=0;k<16;k++) {
      addChild(createLightCentered<DBSmallLight<RedLight>>(mm2px(Vec(x,y)),module,SEQMod::POS_LIGHT+k));
      addChild(createLightCentered<DBSmallLight<BlueLight>>(mm2px(Vec(x,y)),module,SEQMod::POS_LIGHT+k+16));
      addChild(createLightCentered<DBSmallLight<YellowLight>>(mm2px(Vec(x,y)),module,SEQMod::POS_LIGHT+k+32));
      addChild(createLightCentered<DBSmallLight<DBOrangeLight>>(mm2px(Vec(x,y)),module,SEQMod::POS_LIGHT+k+48));
      addChild(createLightCentered<DBSmallLight<DBTurkLight>>(mm2px(Vec(x,y)),module,SEQMod::POS_LIGHT+k+64));
      addChild(createLightCentered<DBSmallLight<GreenLight>>(mm2px(Vec(x+3,y)),module,SEQMod::S_LIGHT+k));
      y+=3;
    }

  }

  void appendContextMenu(Menu *menu) override {
    SEQMod *module=dynamic_cast<SEQMod *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    std::vector <MinMaxRange> ranges={{-10,10},
                                      {-5, 5},
                                      {-3, 3},
                                      {-2, 2},
                                      {-1, 1},
                                      {0,  10},
                                      {0,  5},
                                      {0,  3},
                                      {0,  2},
                                      {0,  1}};
    auto rangeSelectItem=new RangeSelectItem<SEQMod>(module,ranges);
    rangeSelectItem->text="Cmp Range";
    rangeSelectItem->rightText=string::f("%d/%dV",(int)module->min,(int)module->max)+"  "+RIGHT_ARROW;
    menu->addChild(rangeSelectItem);
    menu->addChild(createCheckMenuItem("Invers 1","",[=]() {
      return module->inv1;
    },[=]() {
      module->inv1=!module->inv1;
    }));
    menu->addChild(createCheckMenuItem("Invers 2","",[=]() {
      return module->inv2;
    },[=]() {
      module->inv2=!module->inv2;
    }));
    menu->addChild(createCheckMenuItem("Invers 3","",[=]() {
      return module->inv3;
    },[=]() {
      module->inv3=!module->inv3;
    }));
  }
};


Model *modelSEQMod=createModel<SEQMod,SEQModWidget>("SigMod");