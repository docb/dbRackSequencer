#include "plugin.hpp"

#define CYCLEN 32
#define NUM_TRACK 6

template<typename M>
struct Track {
  M *module;
  int nr;
  RND rnd;
  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::PulseGenerator rstPulse;
  dsp::ClockDivider divider;
  int stepCounter=0;
  bool reverseDirection=false;
  int startId;
  int startInputId;
  int strideId;
  int strideInputId;
  int lengthId;
  int lengthInputId;
  int runModeId;
  int trackOnId;
  int onOffId;
  int trackOnInputId;
  //int runModeInputBaseId;
  int lightBaseId;
  bool sampleAndHold=false;
  float currentCV=0;


  Track(M *_module,int _nr) : module(_module),nr(_nr) {
    startId=M::OFFSET_PARAM+nr;
    startInputId=M::OFFSET_INPUT+nr;
    strideId=M::STRIDE_PARAM+nr;
    strideInputId=M::STRIDE_INPUT+nr;
    lengthId=M::LENGTH_PARAM+nr;
    lengthInputId=M::LENGTH_INPUT+nr;
    runModeId=M::RUN_MODE_PARAM+nr;
    //runModeInputBaseId=M::RUN_MODE_INPUT+nr;
    lightBaseId=nr*CYCLEN;
    trackOnId=M::TRACK_ON_PARAM+nr;
    trackOnInputId=M::TRACK_ON_INPUT+nr;
    onOffId=M::ON_OFF_PARAM+nr*CYCLEN;
    divider.setDivision(64);
    //INFO("init %d %d %d %d %d",nr,startBaseId,lengthBaseId,strideBaseId,lightBaseId);
  }

  int incStep(int step,int length,int amount) {
    return (step+amount)%length;
  }

  int decStep(int step,int length,int amount) {
    return (step-amount+length)%length;
  }

  int getLength() {
    if(module->inputs[lengthInputId].isConnected())
      setImmediateValue(module->getParamQuantity(lengthId),module->inputs[lengthInputId].getVoltage()*3.2f);
    return module->params[lengthId].getValue();
  }

  int getStart() {
    if(module->inputs[startInputId].isConnected())
      module->getParamQuantity(startId)->setValue(module->inputs[startInputId].getVoltage()*3.2f);
    return module->params[startId].getValue();
  }

  int getStride() {
    if(module->inputs[strideInputId].isConnected())
      module->getParamQuantity(strideId)->setValue(module->inputs[strideInputId].getVoltage()*0.8f);
    return module->params[strideId].getValue();
  }

  void reset() {
    switch((int)module->params[runModeId].getValue()) {
      case 0:
      case 2:
      case 3:
        stepCounter=0;
        break;
      case 1:
        stepCounter=getLength()-1;
        break;
      default:
        break;
    }
  }

  void nextStepCounter() {
    int stepLength=getLength();
    int stepAmount=getStride();
    switch((int)module->params[runModeId].getValue()) {
      case 0:
        stepCounter=incStep(stepCounter,stepLength,stepAmount);
        break;
      case 1:
        stepCounter=decStep(stepCounter,stepLength,stepAmount);
        break;
      case 2:
        if(reverseDirection) { //ping pong
          int step=decStep(stepCounter,stepLength,stepAmount);
          if(step>stepCounter) {
            stepCounter=incStep(stepCounter,stepLength,stepAmount);
            reverseDirection=false;
          } else {
            stepCounter=step;
          }
        } else {
          int step=incStep(stepCounter,stepLength,stepAmount);
          if(step<stepCounter) {
            stepCounter=decStep(stepCounter,stepLength,stepAmount);
            reverseDirection=true;
          } else {
            stepCounter=step;
          }
        }
        break;
      case 3:
        if(rnd.nextCoin(0.5))
          stepCounter=incStep(stepCounter,stepLength,stepAmount);
        else
          stepCounter=decStep(stepCounter,stepLength,stepAmount);
        break;
      case 4:
        stepCounter=rnd.nextRange(0,stepLength-1);
        break;
      default:
        break;
    }
  }

  int getCurrentStep() {
    int start=getStart();
    return (start+stepCounter)%CYCLEN;
  }

  bool gateOn(int step) {
    return module->params[onOffId+step].getValue()>0;
  }

  float getCV(int step) {
    float val=module->params[M::CV_PARAM+step].getValue();
    if(module->quantize) {
      val=roundf(val*12.f)/12.f;
    }
    return val;
  }

  void process(const Module::ProcessArgs &args) {
    if(module->inputs[trackOnInputId].isConnected()) {
      module->getParamQuantity(trackOnId)->setValue(module->inputs[trackOnInputId].getVoltage()>1.f);
    }
    if(module->params[trackOnId].getValue()>0) {
      bool advance=false;
      if(rstTrigger.process(module->inputs[M::RST_INPUT+nr].getVoltage())) {
        rstPulse.trigger(1e-3f);
        float seedParam=0;
        if(module->inputs[M::SEED_INPUT].isConnected()) {
          seedParam=module->inputs[M::SEED_INPUT].getVoltage(nr);
          seedParam = floorf(seedParam*10000)/10000;
          seedParam/=10.f;
        }
        auto seedInput = (unsigned long long)(floor((double)seedParam*(double)ULONG_MAX));
        rnd.reset(seedInput);
        reset();
        advance=true;
      }
      bool rstGate=rstPulse.process(args.sampleTime);

      if(clockTrigger.process(module->inputs[M::CLK_INPUT+nr].getVoltage())&&!rstGate) {
        nextStepCounter();
        advance=true;
      }
      int currentStep=getCurrentStep();
      module->outputs[M::GATE_OUTPUT+nr].setVoltage(gateOn(currentStep)?module->inputs[M::CLK_INPUT+nr].getVoltage():0.0f);
      if(advance) {
        if(sampleAndHold) {
          if(gateOn(currentStep)) {
            currentCV=getCV(currentStep);
          }
        } else {
          currentCV=getCV(currentStep);
        }
        module->outputs[M::CV_OUTPUT+nr].setVoltage(currentCV);
      }
      if(divider.process())
        show();
    } else {
      module->outputs[M::GATE_OUTPUT+nr].setVoltage(0.f);
      module->outputs[M::CV_OUTPUT+nr].setVoltage(0.f);
      if(divider.process())
        show(false);
    }
  }

  bool inside(int step,int start,int len) {
    int end=(start+len)%CYCLEN;
    if(start<end) {
      return (step>=start)&&(step<end);
    } else {
      return (step>=start)||(step<end);
    }

  }

  void show(bool on=true) {
    getStride();
    int len=getLength();
    int start=getStart();
    for(int k=0;k<CYCLEN;k++) {
      if(on) {
        if(k==getCurrentStep()) {
          if(gateOn(k))
            module->lights[lightBaseId+k].setBrightness(1.f);
          else
            module->lights[lightBaseId+k].setBrightness(0.1f);
        } else if(inside(k,start,len)) {
          if(gateOn(k))
            module->lights[lightBaseId+k%CYCLEN].setBrightness(0.2f);
          else
            module->lights[lightBaseId+k%CYCLEN].setBrightness(0.04f);
        } else {
          module->lights[lightBaseId+k%CYCLEN].setBrightness(0.f);
        }
      } else {
        module->lights[lightBaseId+k%CYCLEN].setBrightness(0.f);
      }
    }
  }
};

struct CYC : Module {
  enum ParamId {
    RUN_MODE_PARAM,OFFSET_PARAM=RUN_MODE_PARAM+NUM_TRACK,LENGTH_PARAM=OFFSET_PARAM+NUM_TRACK,STRIDE_PARAM=LENGTH_PARAM+NUM_TRACK,TRACK_ON_PARAM=STRIDE_PARAM+NUM_TRACK,ON_OFF_PARAM=TRACK_ON_PARAM+NUM_TRACK,CV_PARAM=ON_OFF_PARAM+NUM_TRACK*32,PARAMS_LEN=CV_PARAM+32
  };
  enum InputId {
    CLK_INPUT,RST_INPUT=CLK_INPUT+NUM_TRACK,OFFSET_INPUT=RST_INPUT+NUM_TRACK,LENGTH_INPUT=OFFSET_INPUT+NUM_TRACK,STRIDE_INPUT=LENGTH_INPUT+NUM_TRACK,CV_INPUT=STRIDE_INPUT+NUM_TRACK,CV_POLY_0_15_INPUT=CV_INPUT+32,CV_POLY_16_31_INPUT,SEED_INPUT,TRACK_ON_INPUT,INPUTS_LEN=TRACK_ON_INPUT+NUM_TRACK
  };
  enum OutputId {
    CV_OUTPUT,GATE_OUTPUT=CV_OUTPUT+NUM_TRACK,OUTPUTS_LEN=GATE_OUTPUT+NUM_TRACK
  };
  enum LightId {
    LIGHTS_LEN=32*NUM_TRACK
  };
  Track<CYC> tracks[NUM_TRACK]={{this,0},
                                {this,1},
                                {this,2},
                                {this,3},
                                {this,4},
                                {this,5}};
  RND rnd;
  float min=-2;
  float max=2;
  bool quantize=false;
  int dirty=0; // dirty hack
  dsp::ClockDivider paramDivider;
  CYC() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    for(int k=0;k<32;k++) {
      configParam(CV_PARAM+k,min,2,min,"CV "+std::to_string(k+1));

      configInput(CV_INPUT+k,"CV "+std::to_string(k+1));
      for(int j=0;j<NUM_TRACK;j++) {
        configSwitch(ON_OFF_PARAM+j*32+k,0,1,1,"Track "+std::to_string(j+1)+" Step "+std::to_string(k+1),{"Off","On"});
        //configLight(j*32+k,"Track "+std::to_string(j+1)+" Step "+std::to_string(k+1));
      }
    }
    for(int k=0;k<NUM_TRACK;k++) {
      configSwitch(RUN_MODE_PARAM+k,0,4,0,"Run Mode "+std::to_string(k+1),{"-->","<--","<->","?-?","???"});
      configParam(OFFSET_PARAM+k,0,31,0,"Offset "+std::to_string(k+1));
      getParamQuantity(OFFSET_PARAM+k)->snapEnabled=true;
      configInput(OFFSET_INPUT+k,"Offset "+std::to_string(k+1));
      configParam(LENGTH_PARAM+k,1,32,8,"Length "+std::to_string(k+1));
      getParamQuantity(LENGTH_PARAM+k)->snapEnabled=true;
      configInput(LENGTH_INPUT+k,"Length "+std::to_string(k+1));
      configParam(STRIDE_PARAM+k,1,8,1,"Stride "+std::to_string(k+1));
      getParamQuantity(STRIDE_PARAM+k)->snapEnabled=true;
      configInput(STRIDE_INPUT+k,"Stride "+std::to_string(k+1));
      configSwitch(TRACK_ON_PARAM+k,0,1,k==0?1:0,"Track "+std::to_string(k+1),{"Off","On"});
      configInput(TRACK_ON_INPUT+k,"On/Off "+std::to_string(k+1));
    }
    configInput(CV_POLY_0_15_INPUT,"Poly CV 0-15");
    configInput(CV_POLY_16_31_INPUT,"Poly CV 0-15");
    paramDivider.setDivision(32);
  }

  void process(const ProcessArgs &args) override {
    if(paramDivider.process()) {
      for(int step=0;step<32;step++) {
        if(step<16&&inputs[CV_POLY_0_15_INPUT].isConnected()) {
          float v=inputs[CV_POLY_0_15_INPUT].getVoltage(step);
          getParamQuantity(CV_PARAM+step)->setValue(v);
        }
        if(step>=16&&inputs[CV_POLY_16_31_INPUT].isConnected()) {
          float v=inputs[CV_POLY_16_31_INPUT].getVoltage(step-16);
          getParamQuantity(CV_PARAM+step)->setValue(v);
        }
        if(inputs[CV_INPUT+step].isConnected()) {
          float v=inputs[CV_INPUT+step].getVoltage();
          getParamQuantity(CV_PARAM+step)->setValue(v);
        }
      }
    }
    for(int k=0;k<NUM_TRACK;k++) {
      tracks[k].process(args);
    }
  }

  void randomizeCV() {
    for(int k=0;k<CYCLEN;k++) {
      getParamQuantity(CV_PARAM+k)->setValue((rnd.nextDouble()-0.5)*4);
    }
  }

  void reconfig() {
    for(int nr=0;nr<32;nr++) {
      float value=getParamQuantity(CV_PARAM+nr)->getValue();
      if(value>max)
        value=max;
      if(value<min)
        value=min;
      configParam(CV_PARAM+nr,min,max,0,"CV "+std::to_string(nr+1));
      getParamQuantity(CV_PARAM+nr)->setValue(value);
      dirty=32;
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
    json_t *shArray=json_array();
    for(int k=0;k<NUM_TRACK;k++) {
      json_array_append_new(shArray,json_integer(tracks[k].sampleAndHold));
    }
    json_object_set_new(root,"sh",shArray);
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
    json_t *jSHArray=json_object_get(root,"sh");
    if(jSHArray) {
      for(int k=0;k<NUM_TRACK;k++) {
        json_t *sh=json_array_get(jSHArray,k);
        tracks[k].sampleAndHold=json_integer_value(sh);
      }
    }
    reconfig();
  }
};

struct SmallGrayRoundButton : SvgSwitch {
  SmallGrayRoundButton() {
    momentary = false;
    addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/button_9px_gray_off.svg")));
    addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/button_9px_gray_active.svg")));
    fb->removeChild(shadow);
    delete shadow;
  }
};
enum CYCRandTarget {
  RNDCV,RNDGATE,RNDLEN,RNDOFS,RNDMODE
};

struct CYCRandomize : MenuItem {
  CYC *module;
  CYCRandTarget type;

  CYCRandomize(CYC *_module,CYCRandTarget _type,std::string label) : MenuItem(),module(_module),type(_type) {
    text=label;
  }

  void onAction(const event::Action &e) override {
    switch(type) {
      case RNDCV:
        module->randomizeCV();
        break;
      default:
        break;
    }
  }
};

struct SmallRoundTransparentButton : SvgSwitch {
  SmallRoundTransparentButton() {
    momentary=false;
    addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/button_9px_transparent.svg")));
    addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/button_9px_transparent.svg")));
    //fb->removeChild(shadow);
    //delete shadow;
  }
};

template<typename TLight>
struct MLightButton : SmallRoundButton {
  app::ModuleLightWidget *light;

  MLightButton() : SmallRoundButton() {
    light=new TLight;
    // Move center of light to center of box
    light->box.pos=box.size.div(2).minus(light->box.size.div(2));
    addChild(light);
  }

  app::ModuleLightWidget *getLight() {
    return light;
  }
};


struct CYCWidget : ModuleWidget {
  CYCWidget(CYC *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/CYC.svg")));

    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,0)));
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));
    float cx=56;
    float cy=64;
    float start=M_PI;
    int r=35;
    float d=3.5;
    for(int k=0;k<CYCLEN;k++) {
      float alpha=M_PI*((k+1)*360.f/float(CYCLEN))/180.f+start;
      float x=cosf(alpha);
      float y=-sinf(alpha);
      addInput(createInputCentered<SmallPort>(mm2px(Vec(x*47+cx,y*47+cy)),module,CYC::CV_INPUT+(31-k)));
      auto cvKnob=createParamCentered<MKnob<CYC>>(mm2px(Vec(x*40+cx,y*40+cy)),module,CYC::CV_PARAM+(31-k));
      cvKnob->module=module;
      addParam(cvKnob);
      int j=0;
      addParam(createParamCentered<SmallGrayRoundButton>(mm2px(Vec(x*(r-(d*j))+cx,y*(r-(d*j))+cy)),module,CYC::ON_OFF_PARAM+j*32+(31-k)));
      j++;
      addParam(createParamCentered<SmallGrayRoundButton>(mm2px(Vec(x*(r-(d*j))+cx,y*(r-(d*j))+cy)),module,CYC::ON_OFF_PARAM+j*32+(31-k)));
      j++;
      addParam(createParamCentered<SmallGrayRoundButton>(mm2px(Vec(x*(r-(d*j))+cx,y*(r-(d*j))+cy)),module,CYC::ON_OFF_PARAM+j*32+(31-k)));
      j++;
      addParam(createParamCentered<SmallGrayRoundButton>(mm2px(Vec(x*(r-(d*j))+cx,y*(r-(d*j))+cy)),module,CYC::ON_OFF_PARAM+j*32+(31-k)));
      j++;
      addParam(createParamCentered<SmallGrayRoundButton>(mm2px(Vec(x*(r-(d*j))+cx,y*(r-(d*j))+cy)),module,CYC::ON_OFF_PARAM+j*32+(31-k)));
      j++;
      addParam(createParamCentered<SmallGrayRoundButton>(mm2px(Vec(x*(r-(d*j))+cx,y*(r-(d*j))+cy)),module,CYC::ON_OFF_PARAM+j*32+(31-k)));
      j=0;
      addChild(createLightCentered<DBLight9px<DBRedLight>>(mm2px(Vec(x*(r-(d*j))+cx,y*(r-(d*j))+cy)),module,j*32+(31-k)));
      j++;
      addChild(createLightCentered<DBLight9px<DBGreenLight>>(mm2px(Vec(x*(r-(d*j))+cx,y*(r-(d*j))+cy)),module,j*32+(31-k)));
      j++;
      addChild(createLightCentered<DBLight9px<DBYellowLight>>(mm2px(Vec(x*(r-(d*j))+cx,y*(r-(d*j))+cy)),module,j*32+(31-k)));
      j++;
      addChild(createLightCentered<DBLight9px<DBPurpleLight>>(mm2px(Vec(x*(r-(d*j))+cx,y*(r-(d*j))+cy)),module,j*32+(31-k)));
      j++;
      addChild(createLightCentered<DBLight9px<DBOrangeLight>>(mm2px(Vec(x*(r-(d*j))+cx,y*(r-(d*j))+cy)),module,j*32+(31-k)));
      j++;
      addChild(createLightCentered<DBLight9px<DBTurkLight>>(mm2px(Vec(x*(r-(d*j))+cx,y*(r-(d*j))+cy)),module,j*32+(31-k)));

    }
    float y=MHEIGHT-(109+6.237);
    for(int k=0;k<NUM_TRACK;k++) {
      addInput(createInput<SmallPort>(mm2px(Vec(119,y)),module,CYC::CLK_INPUT+k));
      addInput(createInput<SmallPort>(mm2px(Vec(119,y+8)),module,CYC::RST_INPUT+k));

      auto selectParam=createParam<SelectParam>(mm2px(Vec(129,y-0.5)),module,CYC::RUN_MODE_PARAM+k);
      selectParam->box.size=mm2px(Vec(7,3.2*5));
      selectParam->init({"-->","<--","<->","?-?","???"});
      addParam(selectParam);

      addParam(createParam<TrimbotWhite>(mm2px(Vec(141,y)),module,CYC::LENGTH_PARAM+k));
      addInput(createInput<SmallPort>(mm2px(Vec(141,y+8)),module,CYC::LENGTH_INPUT+k));
      addParam(createParam<TrimbotWhite>(mm2px(Vec(153,y)),module,CYC::OFFSET_PARAM+k));
      addInput(createInput<SmallPort>(mm2px(Vec(153,y+8)),module,CYC::OFFSET_INPUT+k));

      addParam(createParam<TrimbotWhite>(mm2px(Vec(165,y)),module,CYC::STRIDE_PARAM+k));
      addInput(createInput<SmallPort>(mm2px(Vec(165,y+8)),module,CYC::STRIDE_INPUT+k));

      addParam(createParam<MLED>(mm2px(Vec(175,y)),module,CYC::TRACK_ON_PARAM+k));
      addInput(createInput<SmallPort>(mm2px(Vec(175,y+8)),module,CYC::TRACK_ON_INPUT+k));
      //addInput(createInput<SmallPort>(mm2px(Vec(175,y+8)),module,CYC::ON_OFF_INPUT+k));
      addOutput(createOutput<SmallPort>(mm2px(Vec(191.25,y)),module,CYC::CV_OUTPUT+k));
      addOutput(createOutput<SmallPort>(mm2px(Vec(191.25,y+8)),module,CYC::GATE_OUTPUT+k));
      y+=18;
    }
    addInput(createInput<SmallPort>(mm2px(Vec(4,TY(114))),module,CYC::CV_POLY_0_15_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(4,TY(8))),module,CYC::CV_POLY_16_31_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(100,TY(114))),module,CYC::SEED_INPUT));
  }

  void appendContextMenu(Menu *menu) override {
    CYC *module=dynamic_cast<CYC *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    std::vector <MinMaxRange> ranges={{-3,3},
                                      {-2,2},
                                      {-1,1},
                                      {0, 1},
                                      {0, 2}};
    auto rangeSelectItem=new RangeSelectItem<CYC>(module,ranges);
    rangeSelectItem->text="Range";
    rangeSelectItem->rightText=string::f("%d/%dV",(int)module->min,(int)module->max)+"  "+RIGHT_ARROW;
    menu->addChild(rangeSelectItem);
    menu->addChild(createBoolPtrMenuItem("Quantize","",&module->quantize));
    for(int k=0;k<NUM_TRACK;k++) {
      menu->addChild(createBoolPtrMenuItem("Hold CV on Rest - Track " + std::to_string(k+1),"",&module->tracks[k].sampleAndHold));
    }
    menu->addChild(new CYCRandomize(module,RNDCV,"Randomize CV"));
  }
};


Model *modelCYC=createModel<CYC,CYCWidget>("CYC");