#include "plugin.hpp"
#include "rnd.h"

#include "Uno.hpp"

extern Model *modelUnoE;

struct Uno : Module {
  enum ParamId {
    CV_PARAMS,PROB_PARAMS=CV_PARAMS+NUM_STEPS,GLIDE_PARAMS=PROB_PARAMS+NUM_STEPS,RST_PARAMS=GLIDE_PARAMS+NUM_STEPS,DIR_PARAM=RST_PARAMS+NUM_STEPS,PARAMS_LEN
  };
  enum InputId {
    CLK_INPUT,RST_INPUT,MASTER_RST_INPUT,POLY_CV_INPUT,POLY_PROB_INPUT,STEP_INPUTS,SEED_INPUT=STEP_INPUTS+NUM_STEPS,INPUTS_LEN
  };
  enum OutputId {
    CV_OUTPUT,GATE_OUTPUT,STEP_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN=8
  };
  int min=-2;
  int max=2;
  dsp::SchmittTrigger setStepTrigger[NUM_STEPS];
  dsp::ClockDivider paramDivider;
  dsp::SchmittTrigger masterReset;
  bool quantize=false;
  int dirty=0;
  RND rnd;
  UnoStrip<Uno> unoStrip;
  UnoExpanderMessage rightMessages[2][1];

  Uno() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    for(int k=0;k<8;k++) {
      configParam(CV_PARAMS+k,min,max,0,"CV "+std::to_string(k+1));
      configParam(PROB_PARAMS+k,0,1,1,"Probability "+std::to_string(k+1)," %",0,100);
      configButton(GLIDE_PARAMS+k,"Glide "+std::to_string(k+1));
      configButton(RST_PARAMS+k,"Reset "+std::to_string(k+1));
      configLight(k,"Step "+std::to_string(k+1));
      configInput(STEP_INPUTS+k,"Set "+std::to_string(k+1));
    }
    configSwitch(DIR_PARAM,0,2,0,"Direction",{"-->","<--","<->"});
    configInput(CLK_INPUT,"Clock");
    configInput(RST_INPUT,"Rst");
    configInput(MASTER_RST_INPUT,"Master Rst");
    configInput(POLY_CV_INPUT,"Poly CV");
    configInput(POLY_PROB_INPUT,"Poly Probability");
    configInput(SEED_INPUT,"Direction");
    configOutput(CV_OUTPUT,"CV");
    configOutput(GATE_OUTPUT,"GATE");
    configOutput(STEP_OUTPUT,"Step");
    //lightDivider.setDivision(32);
    paramDivider.setDivision(32);
    unoStrip.module=this;
    rightExpander.producerMessage=rightMessages[0];
    rightExpander.consumerMessage=rightMessages[1];
  }

  float getCV(int step) {
    float cv=params[CV_PARAMS+step].getValue();
    if(quantize) {
      return roundf(cv*12)/12.f;
    }
    return cv;
  }

  float getProb(int step) {
    return params[PROB_PARAMS+step].getValue();
  }

  bool getGlide(int step) {
    return params[GLIDE_PARAMS+step].getValue()>0.f;
  }

  bool getRst(int step) {
    return params[RST_PARAMS+step].getValue()>0.f;
  }

  void populateExpanderMessage(UnoExpanderMessage *message,bool reset) {
    for(int k=0;k<NUM_STEPS;k++) {
      message->cv[k]=getCV(k);
      message->prob[k]=params[PROB_PARAMS+k].getValue();
      message->glide[k]=params[GLIDE_PARAMS+k].getValue()>0.f;
      message->rst[k]=params[RST_PARAMS+k].getValue()>0.f;
      message->setStep=unoStrip.setStep;
      message->reset=reset;
    }
  }

  void process(const ProcessArgs &args) override {
    if(paramDivider.process()) {
      if(inputs[POLY_CV_INPUT].isConnected()) {
        for(int k=0;k<NUM_STEPS;k++) {
          getParamQuantity(CV_PARAMS+k)->setValue(inputs[POLY_CV_INPUT].getVoltage(k));
        }
      }
      if(inputs[POLY_PROB_INPUT].isConnected()) {
        for(int k=0;k<NUM_STEPS;k++) {
          getParamQuantity(PROB_PARAMS+k)->setValue(inputs[POLY_PROB_INPUT].getVoltage(k));
        }
      }

    }

    for(int k=0;k<NUM_STEPS;k++) {
      if(setStepTrigger[k].process(inputs[STEP_INPUTS+k].getVoltage())) {
        unoStrip.setStep=k;
        break;
      }
    }
    bool reset=false;
    if(masterReset.process(inputs[MASTER_RST_INPUT].getVoltage())) {
      reset=true;
    }
    unoStrip.masterReset = reset;
    unoStrip.process(args.sampleTime);

    if(rightExpander.module) {
      if(rightExpander.module->model==modelUnoE) {
        UnoExpanderMessage *messageToExpander=(UnoExpanderMessage *)(rightExpander.module->leftExpander.producerMessage);
        populateExpanderMessage(messageToExpander,reset);
        rightExpander.module->leftExpander.messageFlipRequested=true;
      }
    }
  }

  void reconfig() {
    for(int nr=0;nr<NUM_STEPS;nr++) {
      float value=getParamQuantity(CV_PARAMS+nr)->getValue();
      if(value>max)
        value=max;
      if(value<min)
        value=min;
      configParam(CV_PARAMS+nr,min,max,0,"CV Step "+std::to_string(nr+1));
      getParamQuantity(CV_PARAMS+nr)->setValue(value);
      dirty=8;// hack
    }
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


struct UnoWidget : ModuleWidget {
  UnoWidget(Uno *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/Uno.svg")));

    float x=3.f;
    float y=TY(80);
    for(int k=0;k<8;k++) {
      addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,Uno::STEP_INPUTS+k));
      auto cvParam=createParam<MKnob<Uno>>(mm2px(Vec(x+10,y)),module,Uno::CV_PARAMS+k);
      cvParam->module=module;
      addParam(cvParam);
      addParam(createParam<TrimbotWhite>(mm2px(Vec(x+20,y)),module,Uno::PROB_PARAMS+k));
      addParam(createParam<MLED>(mm2px(Vec(x+30,y)),module,Uno::GLIDE_PARAMS+k));
      addParam(createParam<MLED>(mm2px(Vec(x+40,y)),module,Uno::RST_PARAMS+k));
      addChild(createLightCentered<LargeLight<RedLight>>(mm2px(Vec(x+52,y+3)),module,k));
      y+=7;
    }
    x=27;
    y=TY(111);
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,Uno::MASTER_RST_INPUT));
    y+=7;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,Uno::POLY_CV_INPUT));
    y+=7;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,Uno::POLY_PROB_INPUT));
    x=52;
    addInput(createInput<SmallPort>(mm2px(Vec(x,TY(118.5))),module,Uno::CLK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x,TY(111.5))),module,Uno::RST_INPUT));

    auto selectParam=createParam<SelectParam>(mm2px(Vec(51.5,TY(107.25-3.5))),module,Uno::DIR_PARAM);
    selectParam->box.size=mm2px(Vec(7,3.2*3));
    selectParam->init({"-->","<--","<->"});
    addParam(selectParam);

    addInput(createInput<SmallPort>(mm2px(Vec(x,TY(92))),module,Uno::SEED_INPUT));

    y=TY(22);
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,Uno::GATE_OUTPUT));
    y+=8;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,Uno::CV_OUTPUT));
    y+=8;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,Uno::STEP_OUTPUT));
  }

  void appendContextMenu(Menu *menu) override {
    Uno *module=dynamic_cast<Uno *>(this->module);
    assert(module);
    menu->addChild(new MenuEntry);
    std::vector <MinMaxRange> ranges={{-3,3},
                                      {-2,2},
                                      {-1,1},
                                      {0, 1},
                                      {0, 2}};
    auto rangeSelectItem=new RangeSelectItem<Uno>(module,ranges);
    rangeSelectItem->text="Range";
    rangeSelectItem->rightText=string::f("%d/%dV",(int)module->min,(int)module->max)+"  "+RIGHT_ARROW;
    menu->addChild(rangeSelectItem);
    menu->addChild(createCheckMenuItem("Quantize","",[=]() {
      return module->quantize;
    },[=]() {
      module->quantize=!module->quantize;
    }));
  }

};


Model *modelUno=createModel<Uno,UnoWidget>("Uno");