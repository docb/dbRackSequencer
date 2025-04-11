#include "plugin.hpp"
#include "rnd.h"

#define LVL_MIN   (-10.0f)
#define LVL_MAX   (10.0f)


struct Klee : Module {
  enum ParamId {
    CV_PARAM,PATTERN_PARAM=16,BUS_PARAM=32,LOAD_PARAM=48,BUS_LOAD_PARAM,STEP_PARAM,LENGTH_SWITCH_PARAM,RANDOM_SWITCH_PARAM,INVERT_B_SWITCH_PARAM,RANDOM_THRESHOLD_PARAM,BUS_2_MODE_SWITCH_PARAM,TABS_ON_PARAM,TAB_PARAMS,PARAMS_LEN=TAB_PARAMS+16
  };
  enum InputId {
    CV_INPUT,CLK_INPUT=CV_INPUT+16,LOAD_INPUT,EXT_INPUT,POLY_PATTERN_INPUT,POLY_BUS_INPUT,RANDOM_ON_INPUT,POLY_CV_INPUT,TAB_INPUT,TABS_ON_INPUT,INPUTS_LEN
  };
  enum OutputId {
    A_OUTPUT,B_OUTPUT,A_PLUS_B_OUTPUT,A_MINUS_B_OUTPUT,B_MINUS_A_OUTPUT,POLY_GATE_OUTPUT,POLY_TRG_OUTPUT,POLY_CLK_OUTPUT,POLY_CV_OUTPUT,BUS_GATE_OUTPUT,BUS_CLK_OUTPUT=BUS_GATE_OUTPUT+3,BUS_TRG_OUTPUT=BUS_CLK_OUTPUT+3,OUTPUTS_LEN=BUS_TRG_OUTPUT+3
  };
  enum LightId {
    STEP_LIGHT,BUS_LIGHT=STEP_LIGHT+16,MAIN_LIGHT=BUS_LIGHT+3,LIGHTS_LEN
  };
  RND rnd;
  dsp::SchmittTrigger loadTrigger;
  dsp::SchmittTrigger busLoadTrigger;
  dsp::SchmittTrigger manualLoadTrigger;
  dsp::SchmittTrigger clockTrigger;
  dsp::PulseGenerator triggers[3];
  dsp::PulseGenerator stepTriggers[16];
  dsp::PulseGenerator loadPulse;
  bool instant = false;
  bool extAsGate = false;
  union {
    struct {
      bool A[8];
      bool B[8];
    };
    bool P[16]={};
  }shiftRegister;

  bool bus_active[3]={};

  float min=-1;
  float max=1;
  bool quantize=false;
  int dirty=0;
  dsp::ClockDivider divider;
  dsp::ClockDivider paramDivider;

  Klee() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    for(int k=0;k<16;k++) {
      configParam(CV_PARAM+k,min,max,0,"CV "+std::to_string(k+1));
      configInput(CV_INPUT+k,"CV "+std::to_string(k+1));
      configParam(PATTERN_PARAM+k,0,1,0,"Bit "+std::to_string(k+1));
      configParam(TAB_PARAMS+k,0,1,0,"Tab "+std::to_string(k+1));
      configSwitch(BUS_PARAM+k,0,2,1,"Bus Select "+std::to_string(k+1),{"Bus 1","Bus 2","Bus 3"});
    }
    configButton(LOAD_PARAM,"Load");
    configButton(BUS_LOAD_PARAM,"Load from Bus 1");
    configSwitch(LENGTH_SWITCH_PARAM,0,1,1,"2x8/1x16",{"2x8","1x16"});
    configSwitch(RANDOM_SWITCH_PARAM,0,1,0,"Random",{"Pattern","Random"});
    configSwitch(INVERT_B_SWITCH_PARAM,0,1,1,"B Invert",{"B_Invert","Normal"});
    configParam(RANDOM_THRESHOLD_PARAM,0,1,0.5,"Random Threshold");
    configSwitch(BUS_2_MODE_SWITCH_PARAM,0,1,0,"Bus 2 Mode",{"NOR","AND"});
    for(int k=0;k<3;k++) {
      configOutput(BUS_TRG_OUTPUT+k,"Trigger Bus "+std::to_string(k+1));
      configOutput(BUS_CLK_OUTPUT+k,"Clock Bus "+std::to_string(k+1));
      configOutput(BUS_GATE_OUTPUT+k,"Gate Bus "+std::to_string(k+1));
    }

    configInput(CLK_INPUT,"Clock");
    configInput(LOAD_INPUT,"Load Pattern");
    configInput(EXT_INPUT,"Ext Random");
    configInput(POLY_PATTERN_INPUT,"Poly Pattern");
    configInput(POLY_BUS_INPUT,"Poly Bus");
    configInput(POLY_CV_INPUT,"Poly CV");
    configOutput(A_OUTPUT,"A");
    configOutput(B_OUTPUT,"B");
    configOutput(A_PLUS_B_OUTPUT,"A+B");
    configOutput(A_MINUS_B_OUTPUT,"A-B");
    configOutput(B_MINUS_A_OUTPUT,"B-A");
    configOutput(POLY_GATE_OUTPUT,"Poly Gate");
    configOutput(POLY_TRG_OUTPUT,"Poly Trig");
    configOutput(POLY_CLK_OUTPUT,"Poly Clock");
    configOutput(POLY_CV_OUTPUT,"Poly CV");
    configInput(TAB_INPUT,"Tabs Poly");
    configInput(TABS_ON_INPUT,"Tabs On");
    configButton(TABS_ON_PARAM,"Tabs On");
    divider.setDivision(32);
    paramDivider.setDivision(32);
  }

  bool getCoin() {
    float threshold=params[RANDOM_THRESHOLD_PARAM].getValue();
    if(inputs[EXT_INPUT].isConnected()) {
      if(extAsGate) return inputs[EXT_INPUT].getVoltage()>1.f;
      return rescale(inputs[EXT_INPUT].getVoltage(),-5.f,5.f,0.f,1.f)<threshold;
    } else {
      return rnd.nextCoin(1-threshold);
    }
  }

  bool computeTabs16(bool lastBit) {
    bool ret=lastBit;
    for(int k=1;k<16;k++) {
      if(params[TAB_PARAMS+k].getValue()>0.f) {
        ret = ret ^ shiftRegister.P[k];
      }
    }
    return ret;
  }
  bool computeTabs8(bool lastBit) {
    bool ret=lastBit;
    for(int k=1;k<8;k++) {
      if(params[TAB_PARAMS+k].getValue()>0.f) {
      //if(inputs[TAB_INPUT].getVoltage(k)>1.f) {
        ret = ret ^ shiftRegister.A[k];
      }
    }
    return ret;
  }
  void rotateSR() {

    if(params[LENGTH_SWITCH_PARAM].getValue()>0)  // mode 1 x 16
    {
      int fl=shiftRegister.P[15];
      for(int k=15;k>0;k--) {
        shiftRegister.P[k]=shiftRegister.P[k-1];
      }
      if(params[RANDOM_SWITCH_PARAM].getValue()==1) {
        shiftRegister.P[0]=getCoin();
      }
      else if(params[TABS_ON_PARAM].getValue()>0) {
        shiftRegister.P[0]=computeTabs16(fl);
      }
      else
        shiftRegister.P[0]=fl;
    } else {
      int fla=shiftRegister.A[7];
      int flb=shiftRegister.B[7];
      for(int k=7;k>0;k--) {
        shiftRegister.A[k]=shiftRegister.A[k-1];
        shiftRegister.B[k]=shiftRegister.B[k-1];
      }
      if(params[RANDOM_SWITCH_PARAM].getValue()==1) {
        shiftRegister.A[0]=getCoin();
      }
      else if(params[TABS_ON_PARAM].getValue()>0) {
        shiftRegister.A[0]=computeTabs8(fla);
      } else
        shiftRegister.A[0]=fla;
      shiftRegister.B[0]=params[INVERT_B_SWITCH_PARAM].getValue()==0?!flb:flb;
    }
  }

  void processBus() {
    for(int k=0;k<3;k++)
      bus_active[k]=false;

    for(int k=0;k<16;k++) {
      if(shiftRegister.P[k]) {
        bus_active[(int)params[BUS_PARAM+k].getValue()]=true;
        outputs[POLY_GATE_OUTPUT].setVoltage(10.f,k);
        stepTriggers[k].trigger(0.001f);
      } else {
        outputs[POLY_GATE_OUTPUT].setVoltage(0.f,k);
      }
    }
    if(params[BUS_2_MODE_SWITCH_PARAM].getValue()>0)
      bus_active[1]=bus_active[0]&&bus_active[2];
    else
      bus_active[1]&=!(bus_active[0]||bus_active[2]);  //BUS 2: NOR 0 , 3

    for(int k=0;k<3;k++) {
      if(bus_active[k]) {
        triggers[k].trigger(0.001f);
        outputs[BUS_GATE_OUTPUT+k].setVoltage(10.f);
      } else {
        outputs[BUS_GATE_OUTPUT+k].setVoltage(0.f);
      }
    }

  }

  void populateOutputs() {
    float a=0,b=0;

    for(int k=0;k<8;k++) {
      if(shiftRegister.A[k]) {
        a+=params[CV_PARAM+k].getValue();
      }

      if(shiftRegister.B[k]) {
        b+=params[CV_PARAM+8+k].getValue();
      }
    }
    if(quantize) {
      a=roundf(a*12.f)/12.f;
      b=roundf(b*12.f)/12.f;
    }
    outputs[A_OUTPUT].setVoltage(clamp(a,LVL_MIN,LVL_MAX));
    outputs[B_OUTPUT].setVoltage(clamp(b,LVL_MIN,LVL_MAX));
    outputs[A_PLUS_B_OUTPUT].setVoltage(clamp(a+b,LVL_MIN,LVL_MAX));
    outputs[A_MINUS_B_OUTPUT].setVoltage(clamp(a-b,LVL_MIN,LVL_MAX));
    outputs[B_MINUS_A_OUTPUT].setVoltage(clamp(b-a,LVL_MIN,LVL_MAX));
    for(int k=0;k<16;k++) {
      outputs[POLY_CV_OUTPUT].setVoltage(params[CV_PARAM+k].getValue(),k);
    }
  }

  void showValues() {
    for(int k=0;k<16;k++) {
      lights[STEP_LIGHT+k].setBrightness(shiftRegister.P[k]?1.f:0.f);
    }

    for(int k=0;k<3;k++) {
      lights[BUS_LIGHT+k].setBrightness(outputs[BUS_GATE_OUTPUT+k].getVoltage()/10.f);
    }
  }

  void load() {
    if(inputs[POLY_PATTERN_INPUT].isConnected()) {
      for(int k=0;k<16;k++) {
        getParamQuantity(PATTERN_PARAM+k)->setValue(inputs[POLY_PATTERN_INPUT].getVoltage(k)>0?1.f:0.f);
      }
    }
    for(int k=0;k<16;k++) {
      shiftRegister.P[k]=params[PATTERN_PARAM+k].getValue();
    }

  }

  void process(const ProcessArgs &args) override {
    if(paramDivider.process()) {
      int polyCVChannels=inputs[POLY_CV_INPUT].getChannels();
      for(int k=0;k<16;k++) {
        if(inputs[CV_INPUT+k].isConnected()) {
          getParamQuantity(CV_PARAM+k)->setValue(inputs[CV_INPUT+k].getVoltage());
        } else if(inputs[POLY_CV_INPUT].isConnected()) {
          if(k<polyCVChannels)
            getParamQuantity(CV_PARAM+k)->setValue(inputs[POLY_CV_INPUT].getVoltage(k));
        }
        if(inputs[TAB_INPUT].isConnected()) {
          getParamQuantity(TAB_PARAMS+k)->setValue(inputs[TAB_INPUT].getVoltage(k)>1.f);
        }
        if(inputs[POLY_BUS_INPUT].isConnected()) {
          getParamQuantity(BUS_PARAM+k)->setValue(std::floor(rescale(inputs[POLY_BUS_INPUT].getVoltage(k),0.f,10.f,0.f,2.99999f)));
        }
        if(inputs[RANDOM_ON_INPUT].isConnected()) {
          getParamQuantity(RANDOM_SWITCH_PARAM)->setValue(inputs[RANDOM_ON_INPUT].getVoltage()>5.f?1.f:0.f);
        }
        if(inputs[TABS_ON_INPUT].isConnected()) {
          getParamQuantity(TABS_ON_PARAM)->setValue(inputs[TABS_ON_INPUT].getVoltage()>5.f?1.f:0.f);
        }
      }
    }
    bool advance=false;
    if(manualLoadTrigger.process(params[LOAD_PARAM].getValue()) |
       loadTrigger.process(inputs[LOAD_INPUT].getVoltage())|
       (params[BUS_LOAD_PARAM].getValue()>0&&busLoadTrigger.process(bus_active[0]))) {
      loadPulse.trigger(1e-3f);
      advance=true;
      load();
    }
    bool loadGate = loadPulse.process(args.sampleTime);

    if(clockTrigger.process(inputs[CLK_INPUT].getVoltage()+params[STEP_PARAM].value) && !loadGate) {
      advance=true;
      rotateSR();
    }

    if(advance || instant) {
      processBus();
      populateOutputs();
    }
    if(divider.process()) {
      for(int k=0;k<16;k++) {
        bool trigger=stepTriggers[k].process(args.sampleTime*32.f);
        outputs[POLY_TRG_OUTPUT].setVoltage(trigger?10.f:0.f,k);
        outputs[POLY_CLK_OUTPUT].setVoltage((outputs[POLY_GATE_OUTPUT].getVoltage(k)==10.f?inputs[CLK_INPUT].getVoltage():0.0f),k);
      }
      for(int k=0;k<3;k++) {
        bool trigger=triggers[k].process(args.sampleTime*32.f);
        outputs[BUS_TRG_OUTPUT+k].setVoltage((trigger?10.0f:0.0f));
        outputs[BUS_CLK_OUTPUT+k].setVoltage((outputs[BUS_GATE_OUTPUT+k].getVoltage()==10.f)?inputs[CLK_INPUT].getVoltage():0.0f);
      }
      showValues();
    }
    outputs[POLY_GATE_OUTPUT].setChannels(16);
    outputs[POLY_TRG_OUTPUT].setChannels(16);
    outputs[POLY_CLK_OUTPUT].setChannels(16);
    outputs[POLY_CV_OUTPUT].setChannels(16);
  }

  void randomizeCV() {
    for(int k=0;k<16;k++) {
      getParamQuantity(CV_PARAM+k)->setValue(rescale(rnd.nextDouble(),0.f,1.f,min,max));
    }
  }
  void randomizeLoad() {
    for(int k=0;k<16;k++) {
      getParamQuantity(PATTERN_PARAM+k)->setValue(rnd.nextCoin()?1.f:0.f);
    }
  }
  void randomizeBus() {
    for(int k=0;k<16;k++) {
      getParamQuantity(BUS_PARAM+k)->setValue(std::floor(rescale(rnd.nextDouble(),0.f,1.f,0.f,2.99999f)));
    }
  }
  void reconfig() {
    for(int nr=0;nr<16;nr++) {
      float value=getParamQuantity(CV_PARAM+nr)->getValue();
      if(value>max)
        value=max;
      if(value<min)
        value=min;
      configParam(CV_PARAM+nr,min,max,0,"CV "+std::to_string(nr+1));
      getParamQuantity(CV_PARAM+nr)->setValue(value);
      dirty=16;
    }
  }
  void fromJson(json_t *root) override {
    min=-3.f;
    max=3.f;
    reconfig();
    Module::fromJson(root);
  }
  json_t* dataToJson() override {
    json_t *root=json_object();
    json_object_set_new(root,"min",json_real(min));
    json_object_set_new(root,"max",json_real(max));
    json_object_set_new(root,"quantize",json_integer(quantize));
    json_object_set_new(root,"extAsGate",json_boolean(extAsGate));
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
    json_t *jQuantize=json_object_get(root,"quantize");
    if(jQuantize) {
      quantize=json_integer_value(jQuantize);
    }
    json_t *jExtAsGate=json_object_get(root,"extAsGate");
    if(jExtAsGate) {
      extAsGate=json_boolean_value(jExtAsGate);
    }
    reconfig();
  }
};

struct MButton : public SvgSwitch {
  MButton() {
    momentary=true;
    addFrame(Svg::load(asset::plugin(pluginInstance,"res/RButton0.svg")));
    addFrame(Svg::load(asset::plugin(pluginInstance,"res/RButton1.svg")));
  }
};

enum RandTarget { RNDCV,RNDBUS,RNDLOAD};
struct RandomizeItem : MenuItem {
  Klee *module;
  RandTarget type;
  RandomizeItem(Klee *_module, RandTarget _type,std::string label) : MenuItem(),module(_module),type(_type) {
     text = label;
  }
  void onAction(const event::Action &e) override {
    switch(type) {
      case RNDCV: module->randomizeCV();break;
      case RNDBUS: module->randomizeBus();break;
      case RNDLOAD: module->randomizeLoad();break;
      default: break;
    }
  }
};

struct KleeWidget : ModuleWidget {
  KleeWidget(Klee *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/Klee.svg")));

    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,0)));
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));

    for(int k=0;k<16;k++) {
      auto param=createParam<SmallButtonWithLabel>(mm2px(Vec(k*8+6,MHEIGHT-114-3.2)),module,Klee::PATTERN_PARAM+k);
      param->label=std::to_string(k%8+1);
      if(k>7)
        param->label+=("/"+std::to_string(k+1));
      addParam(param);
      auto selectParam=createParam<SelectParam>(mm2px(Vec(k*8+6,MHEIGHT-20)),module,Klee::BUS_PARAM+k);
      selectParam->box.size=mm2px(Vec(7,9.6));
      selectParam->init({"1","2","3"});
      addParam(selectParam);
    }
    float cx=70;
    float cy=MHEIGHT-69;
    float ratio=0.6;
    float start=180-11.25;

    for(int k=0;k<16;k++) {
      float alpha=M_PI*((k+1)*360.f/float(16)+start)/180.f;
      float x=cosf(alpha);
      float y=ratio*sinf(alpha);
      auto cvParam=createParamCentered<MKnob<Klee>>(mm2px(Vec(x*62+cx,y*62+cy)),module,Klee::CV_PARAM+k);
      cvParam->module=module;
      addParam(cvParam);
      addInput(createInputCentered<SmallPort>(mm2px(Vec(x*50+cx,y*50+cy)),module,Klee::CV_INPUT+k));
      addChild(createLightCentered<DBSmallLight<GreenLight>>(mm2px(Vec(x*40+cx,y*40+cy)),module,k));
    }
    start=180-4.25;
    for(int k=0;k<16;k++) {
      float alpha=M_PI*((k)*360.f/float(16)+start)/180.f;
      float x=cosf(alpha);
      float y=ratio*sinf(alpha);
      if(k>0)
      addParam(createParamCentered<SmallRoundButton>(mm2px(Vec(x*40+cx,y*40+cy)),module,Klee::TAB_PARAMS+k));
    }
    float x=1;
    float y=0;

    auto selectParam=createParam<SelectParam>(mm2px(Vec(x+46.0,MHEIGHT-65.500-8)),module,Klee::LENGTH_SWITCH_PARAM);
    selectParam->box.size=mm2px(Vec(14,8));
    selectParam->init({"2x8","1x16"});
    addParam(selectParam);
    selectParam=createParam<SelectParam>(mm2px(Vec(x+62.5,MHEIGHT-65.500-8)),module,Klee::RANDOM_SWITCH_PARAM);
    selectParam->box.size=mm2px(Vec(14,8));
    selectParam->init({"Pat","Random"});
    addParam(selectParam);
    selectParam=createParam<SelectParam>(mm2px(Vec(x+79,MHEIGHT-65.500-8)),module,Klee::INVERT_B_SWITCH_PARAM);
    selectParam->box.size=mm2px(Vec(14,8));
    selectParam->init({"B Inv","Normal"});
    addParam(selectParam);

    selectParam=createParam<SelectParam>(mm2px(Vec(141.875,MHEIGHT-16.526-6.4)),module,Klee::BUS_2_MODE_SWITCH_PARAM);
    selectParam->box.size=mm2px(Vec(7,6.4));
    selectParam->init({"NOR","AND"});
    addParam(selectParam);

    addInput(createInput<SmallPort>(mm2px(Vec(6,MHEIGHT-24-6.237)),module,Klee::POLY_BUS_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(126.250,MHEIGHT-106-6.237)),module,Klee::POLY_PATTERN_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(6,MHEIGHT-106-6.237)),module,Klee::POLY_CV_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(150,MHEIGHT-51.503-6.237)),module,Klee::TAB_INPUT));
    addParam(createParam<MLED>(mm2px(Vec(160.695,MHEIGHT-51.503-6.237)),module,Klee::TABS_ON_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(172,MHEIGHT-51.503-6.237)),module,Klee::TABS_ON_INPUT));
    addParam(createParam<MButton>(mm2px(Vec(150.2,MHEIGHT-106.5-6.237)),module,Klee::LOAD_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(161.13,MHEIGHT-111-6.237)),module,Klee::LOAD_INPUT));
    auto param=createParam<SmallButtonWithLabel>(mm2px(Vec(161.737,MHEIGHT-102.417-3.2)),module,Klee::BUS_LOAD_PARAM);
    param->label="Bus1";
    addParam(param);

    addInput(createInput<SmallPort>(mm2px(Vec(150.06,MHEIGHT-88.253-6.237)),module,Klee::RANDOM_ON_INPUT));
    addParam(createParam<MLED>(mm2px(Vec(160.695,MHEIGHT-88.253-6.237)),module,Klee::RANDOM_SWITCH_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(150.06,MHEIGHT-76.634-6.237)),module,Klee::EXT_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(160.695,MHEIGHT-76.550-6.237)),module,Klee::RANDOM_THRESHOLD_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(150.06,MHEIGHT-63.384-6.237)),module,Klee::CLK_INPUT));
    addParam(createParam<MButton>(mm2px(Vec(160.61,MHEIGHT-63.384-6.237)),module,Klee::STEP_PARAM));

    x=182.f;
    y=MHEIGHT-108-6.27;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,Klee::A_OUTPUT));
    y+=12;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,Klee::B_OUTPUT));
    y+=12;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,Klee::A_PLUS_B_OUTPUT));
    y+=12;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,Klee::A_MINUS_B_OUTPUT));
    y+=12;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,Klee::B_MINUS_A_OUTPUT));
    y+=12;

    y=MHEIGHT-24.5-6.237;
    float y1=MHEIGHT-(24.5+4.3f);
    for(int k=0;k<3;k++) {
      addOutput(createOutput<SmallPort>(mm2px(Vec(158,y)),module,Klee::BUS_TRG_OUTPUT+k));
      addOutput(createOutput<SmallPort>(mm2px(Vec(170,y)),module,Klee::BUS_CLK_OUTPUT+k));
      addOutput(createOutput<SmallPort>(mm2px(Vec(182,y)),module,Klee::BUS_GATE_OUTPUT+k));
      addChild(createLight<DBSmallLight<RedLight>>(mm2px(Vec(152.500,y1)),module,Klee::BUS_LIGHT+k));
      y+=8;y1+=8;
    }
    y=MHEIGHT-36.093-6.237;
    addOutput(createOutput<SmallPort>(mm2px(Vec(146,y)),module,Klee::POLY_CV_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(158,y)),module,Klee::POLY_TRG_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(170,y)),module,Klee::POLY_CLK_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(182,y)),module,Klee::POLY_GATE_OUTPUT));
  }

  void appendContextMenu(Menu *menu) override {
    Klee *module=dynamic_cast<Klee *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    std::vector <MinMaxRange> ranges={{-3,3},
                                      {-2,2},
                                      {-1,1},
                                      {-0.5,0.5},
                                      {-0.2,0.2},
                                      {0,1},
                                      {0,2}};
    auto rangeSelectItem=new RangeSelectItem<Klee>(module,ranges);
    rangeSelectItem->text="Range";
    rangeSelectItem->rightText=string::f("%0.1f/%0.1fV",module->min,module->max)+"  "+RIGHT_ARROW;
    menu->addChild(rangeSelectItem);
    menu->addChild(new MenuSeparator);
    menu->addChild(createBoolPtrMenuItem("Quantize", "", &module->quantize));
    menu->addChild(createBoolPtrMenuItem("Instant mode", "", &module->instant));
    menu->addChild(createBoolPtrMenuItem("Ext as Gate", "", &module->extAsGate));
    menu->addChild(new MenuSeparator);
    menu->addChild(new RandomizeItem(module,RNDCV,"Randomize CV"));
    menu->addChild(new RandomizeItem(module,RNDBUS,"Randomize Bus"));
    menu->addChild(new RandomizeItem(module,RNDLOAD,"Randomize Load"));
  }
};


Model *modelKlee=createModel<Klee,KleeWidget>("Klee");