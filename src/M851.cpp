#include "plugin.hpp"
#include "rnd.h"

struct SlewLimiter {
  float delta=0;
  float last;
  float target;

  SlewLimiter() {
  }

  void setParams(float sampleTime,float seconds,float from,float to) {
    delta=(to-from)*sampleTime/seconds;
    last=from;
    target=to;
  }

  float process() {
    if(delta<0)
      return last=std::max(last+delta,target);
    return last=std::min(last+delta,target);
  }
};

struct M851 : Module {
  enum ParamId {
    CV_PARAM,REP_PARAM=CV_PARAM+8,GATE_MODE_PARAM=REP_PARAM+8,GLIDE_STEP_PARAM=GATE_MODE_PARAM+8,ON_OFF_PARAM=GLIDE_STEP_PARAM+8,STEP_COUNT_PARAM=ON_OFF_PARAM+8,GATE_LENGTH_PARAM,GLIDE_PARAM,RUN_MODE_PARAM,PARAMS_LEN=RUN_MODE_PARAM+5
  };
  enum InputId {
    CLK_INPUT,RST_INPUT,CV_INPUT,SEED_INPUT=CV_INPUT+8,POLY_CV_INPUT,INPUTS_LEN
  };
  enum OutputId {
    CV_OUTPUT,GATE_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    STEP_LIGHT,LIGHTS_LEN=STEP_LIGHT+64
  };

  M851() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    for(int k=0;k<8;k++) {
      configParam(CV_PARAM+k,min,max,0,"CV Step "+std::to_string(k+1));
      configInput(CV_INPUT+k,"CV Step"+std::to_string(k+1));
      configParam(REP_PARAM+k,0,7,0,"Repetitions Step"+std::to_string(k+1));
      configSwitch(GATE_MODE_PARAM+k,0,7,1,"Gate Mode",{"-----","*----","*****","*-*-*","*--*-","*---*","?????","_____"});
      configSwitch(GLIDE_STEP_PARAM+k,0,1,0,"Glide "+std::to_string(k+1),{"off","on"});
      configSwitch(ON_OFF_PARAM+k,0,1,1,"On/Off "+std::to_string(k+1),{"off","on"});
    }
    configParam(GATE_LENGTH_PARAM,0.005,1,0.1,"Gate Length");
    configParam(STEP_COUNT_PARAM,1,8,8,"# Steps");
    getParamQuantity(STEP_COUNT_PARAM)->snapEnabled=true;
    configParam(GLIDE_PARAM,0.005,2,0.1,"Portamento");
    configInput(CLK_INPUT,"Clock");
    configInput(SEED_INPUT,"Random Seed");
    configInput(POLY_CV_INPUT,"Poly CV");
    configInput(RST_INPUT,"RST");
    configOutput(GATE_OUTPUT,"Gate");
    configOutput(CV_OUTPUT,"CV");
    configSwitch(RUN_MODE_PARAM,0,4,0,"Run Mode",{"-->","<--","<->","?-?","???"});
  }

  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger rstTrigger;
  //dsp::PulseGenerator gatePulse;
  dsp::PulseGenerator rstPulse;
  RND rnd;
  RND rnd2;
  int stepCounter=0;
  int subStepCounter=0;
  bool reverseDirection=false;
  float lastCV=0.f;
  SlewLimiter sl;
  float min=-2;
  float max=2;
  bool quantize=false;
  bool gate=false;
  int dirty=0; // dirty hack

  bool gateOn() {
    switch((int)params[GATE_MODE_PARAM+stepCounter%8].getValue()) {
      case 0:
        return false;
      case 1:
        return subStepCounter==0;
      case 2:
        return true;
      case 3:
        return subStepCounter%2==0;
      case 4:
        return subStepCounter%3==0;
      case 5:
        return subStepCounter%4==0;
      case 6:
        return rnd.nextCoin(0.5);
      default:
        return false;
    }
  }

  int incStep(int step) {
    for(int k=0;k<8;k++) {
      step++;
      if(step>=params[STEP_COUNT_PARAM].getValue())
        step=0;
      if(params[ON_OFF_PARAM+step%8].getValue()>0)
        break;
    }
    return step;
  }

  int decStep(int step) {
    for(int k=0;k<8;k++) {
      step--;
      if(step<0)
        step=params[STEP_COUNT_PARAM].getValue()-1;
      if(params[ON_OFF_PARAM+step%8].getValue()>0)
        break;
    }
    return step;
  }

  void nextStepCounter() {
    int stepLength=params[STEP_COUNT_PARAM].getValue();
    switch((int)params[RUN_MODE_PARAM].getValue()) {
      case 0:
        stepCounter=incStep(stepCounter);
        break;
      case 1:
        stepCounter=decStep(stepCounter);
        break;
      case 2:
        if(reverseDirection) { //ping pong
          int step=decStep(stepCounter);
          if(step>stepCounter) {
            stepCounter=incStep(stepCounter);
            reverseDirection=false;
          } else {
            stepCounter=step;
          }
        } else {
          int step=incStep(stepCounter);
          if(step<stepCounter) {
            stepCounter=decStep(stepCounter);
            reverseDirection=true;
          } else {
            stepCounter=step;
          }
        }
        break;
      case 3:
        if(rnd.nextCoin(0.5))
          stepCounter=incStep(stepCounter);
        else
          stepCounter=decStep(stepCounter);
        break;
      case 4:
        stepCounter=rnd.nextRange(0,stepLength-1);
        break;
      default:
        break;
    }
  }

  bool next() {
    int currentRepCount=(int)params[REP_PARAM+stepCounter%8].getValue()+1;
    subStepCounter++;
    if(subStepCounter>=currentRepCount) {
      subStepCounter=0;
      nextStepCounter();
      return true;
    }
    return false;
  }

  float getCV(int step) {
    if(inputs[POLY_CV_INPUT].isConnected()) {
      getParamQuantity(CV_PARAM+step)->setValue(inputs[POLY_CV_INPUT].getVoltage(step));
    }
    if(inputs[CV_INPUT+step].isConnected())
      getParamQuantity(CV_PARAM+step)->setValue(inputs[CV_INPUT+step].getVoltage());
    float cv=params[CV_PARAM+step].getValue();
    if(quantize) {
      return roundf(cv*12)/12.f;
    }
    return cv;
  }

  void process(const ProcessArgs &args) override {
    bool advance=false;
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())) {
      if(inputs[SEED_INPUT].isConnected()) {
        float seedParam=0;
        seedParam=inputs[SEED_INPUT].getVoltage();
        seedParam = floorf(seedParam*10000)/10000;
        seedParam/=10.f;
        auto seedInput = (unsigned long long)(floor((double)seedParam*(double)ULONG_MAX));
        rnd.reset(seedInput);
      }
      lastCV=getCV(stepCounter%8);
      rstPulse.trigger(0.001f);
      stepCounter=0;
      subStepCounter=0;
      advance=true;
    }
    bool resetGate=rstPulse.process(args.sampleTime);
    if(clockTrigger.process(inputs[CLK_INPUT].getVoltage()) && !resetGate) {
        lastCV=getCV(stepCounter%8);
        next();
        advance=true;
    }
    if(advance) {
      gate=gateOn();
      float cv=getCV(stepCounter%8);
      if(params[GLIDE_STEP_PARAM+stepCounter%8].getValue()==0) {
        outputs[CV_OUTPUT].setVoltage(cv);
      } else {
        sl.setParams(args.sampleTime,params[GLIDE_PARAM].getValue(),lastCV,cv);
      }
    }
    if(params[GLIDE_STEP_PARAM+stepCounter%8].getValue()>0) {
      outputs[CV_OUTPUT].setVoltage(sl.process());
    }
    if((int)params[GATE_MODE_PARAM+stepCounter%8].getValue()==7) {
      outputs[GATE_OUTPUT].setVoltage(10.f);
    } else {
      if(gate)
        outputs[GATE_OUTPUT].setVoltage(inputs[CLK_INPUT].getVoltage()>1.f?10.0f:0.0f);
    }
    for(int k=0;k<64;k++)
      lights[k].setBrightness(0.f);
    lights[(stepCounter%8)*8].setBrightness(1.f);
    if(subStepCounter>0) {
      lights[(stepCounter%8)*8+subStepCounter].setBrightness(1.f);
    }

  }

  void reconfig() {
    for(int nr=0;nr<8;nr++) {
      float value=getParamQuantity(CV_PARAM+nr)->getValue();
      if(value>max)
        value=max;
      if(value<min)
        value=min;
      configParam(CV_PARAM+nr,min,max,0,"CV Step "+std::to_string(nr+1));
      getParamQuantity(CV_PARAM+nr)->setValue(value);
      dirty=8;
    }

  }

  void randomizeCV() {
    for(int k=0;k<8;k++) {
      getParamQuantity(CV_PARAM+k)->setValue(min+rnd2.nextDouble()*(max-min));
    }
  }

  void randomizeGateMode() {
    for(int k=0;k<8;k++) {
      getParamQuantity(GATE_MODE_PARAM+k)->setValue(rescale(rnd2.nextDouble(),0.f,1.f,0.f,7.9999f));
    }
  }

  void randomizeRepititions() {
    for(int k=0;k<8;k++) {
      getParamQuantity(REP_PARAM+k)->setValue(rescale(rnd2.nextDouble(),0.f,1.f,0.f,7.9999f));
    }
  }
  void fromJson(json_t *root) override {
    min=-3.f;
    max=3.f;
    reconfig();
    Module::fromJson(root);
  }
  json_t *dataToJson() override {
    json_t *root=json_object();
    json_object_set_new(root,"min",json_real(min));
    json_object_set_new(root,"max",json_real(max));
    json_object_set_new(root,"quantize",json_integer(quantize));
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
    reconfig();
  }
};

enum M851RandTarget {
  RNDCV,RNDGATEMODE,RNDREP
};

struct M851Randomize : MenuItem {
  M851 *module;
  M851RandTarget type;

  M851Randomize(M851 *_module,M851RandTarget _type,std::string label) : MenuItem(),module(_module),type(_type) {
    text=label;
  }

  void onAction(const event::Action &e) override {
    switch(type) {
      case RNDCV:
        module->randomizeCV();
        break;
      case RNDGATEMODE:
        module->randomizeGateMode();
        break;
      case RNDREP:
        module->randomizeRepititions();
        break;
      default:
        break;
    }
  }
};

struct M851Widget : ModuleWidget {
  M851Widget(M851 *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/M851.svg")));
    /*
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    */
    float x=4.5;
    for(int k=0;k<8;k++) {
      auto cvParam=createParam<MKnob<M851>>(mm2px(Vec(x+0.363,MHEIGHT-119-6.237)),module,M851::CV_PARAM+k);
      cvParam->module=module;
      addParam(cvParam);
      addInput(createInput<SmallPort>(mm2px(Vec(x+0.363,MHEIGHT-111.5-6.237)),module,M851::CV_INPUT+k));
      auto selectParam=createParam<SelectParam>(mm2px(Vec(x,MHEIGHT-109.4)),module,M851::REP_PARAM+k);
      selectParam->box.size=mm2px(Vec(7,3.2*8));
      selectParam->init({"1","2","3","4","5","6","7","8"});
      addParam(selectParam);
      selectParam=createParam<SelectParam>(mm2px(Vec(x,MHEIGHT-78.4)),module,M851::GATE_MODE_PARAM+k);
      selectParam->box.size=mm2px(Vec(7,3.2*8));
      selectParam->init({"","","","","","","",""});
      addParam(selectParam);
      auto button=createParam<SmallButtonWithLabel>(mm2px(Vec(x,MHEIGHT-50.7)),module,M851::GLIDE_STEP_PARAM+k);
      button->label=std::to_string(k+1);
      addParam(button);
      button=createParam<SmallButtonWithLabel>(mm2px(Vec(x,MHEIGHT-46.2)),module,M851::ON_OFF_PARAM+k);
      button->label=std::to_string(k+1);
      addParam(button);
      addChild(createLightCentered<DBMediumLight<GreenLight>>(mm2px(Vec(x+3.5,MHEIGHT-39.5)),module,k*8));
      for(int j=1;j<8;j++) {
        addChild(createLightCentered<DBSmallLight<BlueLight>>(mm2px(Vec(x+3.5,MHEIGHT-(37-2.5*(j-1)))),module,k*8+j));
      }

      if(k==3) {
        x+=20;
      } else {
        x+=8.5;
      }
    }
    addInput(createInput<SmallPort>(mm2px(Vec(40.5,MHEIGHT-111.5-6.237)),module,M851::POLY_CV_INPUT));
    float y=7;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(26.250,MHEIGHT-y-6.237)),module,M851::STEP_COUNT_PARAM));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(54.500,MHEIGHT-y-6.237)),module,M851::GLIDE_PARAM));
    //addParam(createParam<TrimbotWhite>(mm2px(Vec(26,MHEIGHT-y-6.237)),module,M851::GATE_LENGTH_PARAM));

    auto selectParam=createParam<SelectParam>(mm2px(Vec(40,MHEIGHT-37.3)),module,M851::RUN_MODE_PARAM);
    selectParam->box.size=mm2px(Vec(7,3.2*5));
    selectParam->init({"-->","<--","<->","?-?","???"});
    addParam(selectParam);

    addInput(createInput<SmallPort>(mm2px(Vec(13,MHEIGHT-y-6.237)),module,M851::RST_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(4.5,MHEIGHT-y-6.237)),module,M851::CLK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(40.5,MHEIGHT-y-6.237)),module,M851::SEED_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(67,MHEIGHT-y-6.237)),module,M851::GATE_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(75.5,MHEIGHT-y-6.237)),module,M851::CV_OUTPUT));
  }

  void appendContextMenu(Menu *menu) override {
    M851 *module=dynamic_cast<M851 *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);

    std::vector <MinMaxRange> ranges={{-3,3},
                                      {-2,2},
                                      {-1,1},
                                      {0, 1},
                                      {0, 2}};
    auto rangeSelectItem=new RangeSelectItem<M851>(module,ranges);
    rangeSelectItem->text="Range";
    rangeSelectItem->rightText=string::f("%d/%dV",(int)module->min,(int)module->max)+"  "+RIGHT_ARROW;
    menu->addChild(rangeSelectItem);
    menu->addChild(createCheckMenuItem("Quantize","",[=]() {
      return module->quantize;
    },[=]() {
      module->quantize=!module->quantize;
    }));
    menu->addChild(new M851Randomize(module,RNDCV,"Randomize CV"));
    menu->addChild(new M851Randomize(module,RNDGATEMODE,"Randomize Gate Mode"));
    menu->addChild(new M851Randomize(module,RNDREP,"Randomize Repititions"));
  }


};


Model *modelM851=createModel<M851,M851Widget>("M851");