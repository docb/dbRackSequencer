#include "plugin.hpp"

#define NUM_STEPS 16

struct ASEQ : Module {
  enum ParamId {
    DICE_TRIG_PARAM,LEN_PARAM,DENS_PARAM,RANGE_PARAM,HOLD_PARAM,VAR_TRIG_PARAM,ADD_PARAM,REMOVE_PARAM,MOD_TRIG_PARAM,COUNT_PARAM,RST_TRIG_PARAM,CLOCK_TRIG_PARAM,SET_TRIG_PARAM,CV_PARAMS,PARAMS_LEN=CV_PARAMS+16
  };
  enum InputId {
    CLK_INPUT,RST_INPUT,DICE_TRIG_INPUT,MOD_TRIG_INPUT,VAR_TRIG_INPUT,SET_TRIG_INPUT,CV_INPUT,INPUTS_LEN
  };
  enum OutputId {
    GATE_OUTPUT,CV_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  bool gates[NUM_STEPS]={};
  bool gates1[NUM_STEPS]={};
  float cv[NUM_STEPS]={};
  float currentCV=0;
  bool sampleAndHold=true;
  //float cv1[NUM_STEPS]={};
  int pos=0;
  float pmin=-1;
  float pmax=1;
  bool update=false;
  float min=-1;
  float max=1;
  bool quantize=false;
  int dirty=0;
  RND rnd;
  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::SchmittTrigger diceTrigger;
  dsp::SchmittTrigger changeTrigger;
  dsp::SchmittTrigger varyTrigger;
  dsp::SchmittTrigger clockMTrigger;
  dsp::SchmittTrigger rstMTrigger;
  dsp::SchmittTrigger diceMTrigger;
  dsp::SchmittTrigger changeMTrigger;
  dsp::SchmittTrigger varyMTrigger;
  dsp::SchmittTrigger setTrigger;
  dsp::SchmittTrigger setMTrigger;

  dsp::PulseGenerator rstPulse;
  dsp::PulseGenerator dicePulse;

  ASEQ() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    for(int k=0;k<NUM_STEPS;k++) {
      configParam(CV_PARAMS+k,-2,2,0,"CV "+std::to_string(k+1));
    }
    configButton(DICE_TRIG_PARAM,"Dice");
    configButton(VAR_TRIG_PARAM,"Variant");
    configButton(MOD_TRIG_PARAM,"Mod");
    configButton(SET_TRIG_PARAM,"Save");
    configButton(CLOCK_TRIG_PARAM,"Step");
    configButton(RST_TRIG_PARAM,"Reset");
    configParam(LEN_PARAM,2,16,16,"Length");
    getParamQuantity(LEN_PARAM)->snapEnabled=true;
    configParam(DENS_PARAM,2,16,16,"Density");
    getParamQuantity(DENS_PARAM)->snapEnabled=true;
    configParam(RANGE_PARAM,0.1,10,2,"CV Range");
    configParam(HOLD_PARAM,0,1,0,"Hold","%",0,100);
    configParam(ADD_PARAM,0,6,1,"Add Gate");
    getParamQuantity(ADD_PARAM)->snapEnabled=true;
    configParam(REMOVE_PARAM,0,6,1,"Remove Gate");
    getParamQuantity(REMOVE_PARAM)->snapEnabled=true;
    configParam(COUNT_PARAM,0,16,1,"Mod Count");
    getParamQuantity(COUNT_PARAM)->snapEnabled=true;
    configInput(SET_TRIG_INPUT,"Set");
    configInput(CLK_INPUT,"Clock");
    configInput(RST_INPUT,"Rst");
    configInput(VAR_TRIG_INPUT,"Variation");
    configInput(MOD_TRIG_INPUT,"Modulation");
    configInput(DICE_TRIG_INPUT,"Dice");
    configOutput(GATE_OUTPUT,"Gate");
    configOutput(CV_OUTPUT,"CV");
  }

  float getValue(int id) {
    return params[id].getValue();
  }

  int getDens() {
    int dens=0;
    for(int k=0;k<NUM_STEPS;k++) {
      if(gates[k])
        dens++;
    }
    return dens;
  }

  void vary(int len) {
    int dens=getDens();
    int rm=clamp(int(getValue(REMOVE_PARAM)),0,dens);
    int dn=dens;
    for(int k=0;k<rm;k++) {
      int r=rnd.nextRange(0,dn-1);
      int b=0;
      for(int j=0;j<NUM_STEPS;j++) {
        if(gates[j]) {
          if(b==r) {
            gates[j]=false;
            break;
          }
          b++;
        }
      }
      dn--;
      if(dn<0)
        break;
    }
    int add=clamp(int(getValue(ADD_PARAM)),0,len-dens+rm);
    dn=len-(dens-rm);
    for(int k=0;k<add;k++) {
      int r=rnd.nextRange(0,dn-1);
      int b=0;
      for(int j=0;j<NUM_STEPS;j++) {
        if(!gates[j]) {
          if(b==r) {
            gates[j]=true;
            break;
          }
          b++;
        }
      }
      dn--;
      if(dn<0)
        break;
    }
    //INFO("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",gates[0],gates[1],gates[2],gates[3],gates[4],gates[5],gates[6],gates[7],gates[8],gates[9],gates[10],gates[11],gates[12],gates[13],gates[14],gates[15]);
  }

  void changeNote(int len) {
    int dens=getDens();
    int ch=clamp(int(getValue(COUNT_PARAM)),0,dens);
    for(int k=0;k<ch;k++) {
      int r=rnd.nextRange(0,dens);
      int b=0;
      for(int j=0;j<len;j++) {
        if(gates[j]) {
          if(b==r) {
            float rcv=pmin+float(rnd.nextDouble())*(pmax-pmin);
            cv[j]=rcv;
            break;
          }
          b++;
        }
      }
    }
    //INFO("%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f ",cv[0],cv[1],cv[2],cv[3],cv[4],cv[5],cv[6],cv[7],cv[8],cv[9],cv[10],cv[11],cv[12],cv[13],cv[14],cv[15]);

  }

  void dice(int len,int dens) {
    for(bool &g:gates1)
      g=false;
    if(dens==0)
      return;
    int set=0;
    for(int k=0;set<dens;k++) {
      int ps=rnd.nextRange(0,len-1);
      if(!gates1[ps]) {
        set++;
        gates1[ps]=true;
      }
    }
    float hold=getValue(HOLD_PARAM);
    for(int k=0;k<len;k++) {
      gates[k]=gates1[k];
      if(rnd.nextCoin(hold)) {
        float r=pmin+float(rnd.nextDouble())*(pmax-pmin);
        getParamQuantity(CV_PARAMS+k)->setValue(r);
        cv[k]=r;
      }
    }
    update=true;
    //INFO("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",gates[0],gates[1],gates[2],gates[3],gates[4],gates[5],gates[6],gates[7],gates[8],gates[9],gates[10],gates[11],gates[12],gates[13],gates[14],gates[15]);
    //INFO("%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f ",cv[0],cv[1],cv[2],cv[3],cv[4],cv[5],cv[6],cv[7],cv[8],cv[9],cv[10],cv[11],cv[12],cv[13],cv[14],cv[15]);
  }

  void reset(int len) {
    for(int k=0;k<len;k++) {
      gates[k]=gates1[k];
      cv[k]=params[CV_PARAMS+k].getValue();
    }
  }

  void setCV(float v,int k) {
    cv[k]=v;
  }

  void process(const ProcessArgs &args) override {
    int len=int(getValue(LEN_PARAM));
    int dens=clamp(int(getValue(DENS_PARAM)),0,len);
    float range=std::min(params[RANGE_PARAM].getValue(),getParamQuantity(CV_PARAMS)->getMaxValue());
    if(getParamQuantity(CV_PARAMS)->getMinValue()<0) {
      pmin=-range;
      pmax=range;
    } else {
      pmin=0;
      pmax=range;
    }
    bool excl=true;
    if(setTrigger.process(inputs[SET_TRIG_INPUT].getVoltage())|setMTrigger.process(params[SET_TRIG_PARAM].getValue())) {
      for(int k=0;k<NUM_STEPS;k++) {
        setImmediateValue(getParamQuantity(CV_PARAMS+k),cv[k]);
        gates1[k]=gates[k];
      }
      update=true;
    }
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())|rstMTrigger.process(params[RST_TRIG_PARAM].getValue())) {
      rstPulse.trigger(0.001f);
      pos=0;
      reset(len);
    }
    bool resetGate=rstPulse.process(args.sampleTime);
    if((clockTrigger.process(inputs[CLK_INPUT].getVoltage())|clockMTrigger.process(params[CLOCK_TRIG_PARAM].getValue()))&&!resetGate) {
      pos++;
      if(pos>=len) {
        pos=0;
      }
    }
    if((diceTrigger.process(inputs[DICE_TRIG_INPUT].getVoltage())|diceMTrigger.process(params[DICE_TRIG_PARAM].getValue()))&&!resetGate) {
      dicePulse.trigger(0.001f);
      dice(len,dens);
    }
    bool diceGate=dicePulse.process(args.sampleTime)&&excl;
    if((varyTrigger.process(inputs[VAR_TRIG_INPUT].getVoltage())|varyMTrigger.process(params[VAR_TRIG_PARAM].getValue()))&&!resetGate&&!diceGate) {
      vary(len);
    }
    if((changeTrigger.process(inputs[MOD_TRIG_INPUT].getVoltage())|changeMTrigger.process(params[MOD_TRIG_PARAM].getValue()))&&!resetGate&&!diceGate) {
      changeNote(len);
    }
    outputs[GATE_OUTPUT].setVoltage(gates[pos]?inputs[CLK_INPUT].getVoltage():0.f);
    if(sampleAndHold) {
      if(gates[pos])
        currentCV=cv[pos];
    } else {
      currentCV=cv[pos];
    }
    float out=quantize?std::round(currentCV*12.f)/12.f:currentCV;
    outputs[CV_OUTPUT].setVoltage(out);
  }


  void fromJson(json_t *root) override {
    min=-3.f;
    max=3.f;
    reconfig();
    Module::fromJson(root);
  }

  json_t *dataToJson() override {
    json_t *data=json_object();
    json_t *gateList=json_array();
    json_t *gate1List=json_array();
    json_t *cvList=json_array();
    for(int k=0;k<16;k++) {
      json_array_append_new(gateList,json_boolean(gates[k]));
      json_array_append_new(gate1List,json_boolean(gates1[k]));
      json_array_append_new(cvList,json_real(cv[k]));
    }
    json_object_set_new(data,"gates",gateList);
    json_object_set_new(data,"gates1",gate1List);
    json_object_set_new(data,"cv",cvList);
    json_object_set_new(data,"min",json_real(min));
    json_object_set_new(data,"max",json_real(max));
    json_object_set_new(data,"quantize",json_integer(quantize));
    json_object_set_new(data,"sampleAndHold",json_integer(sampleAndHold));

    return data;
  }

  void dataFromJson(json_t *rootJ) override {
    json_t *jGates=json_object_get(rootJ,"gates");
    json_t *jGates1=json_object_get(rootJ,"gates1");
    json_t *jCvList=json_object_get(rootJ,"cv");
    for(int j=0;j<PORT_MAX_CHANNELS;j++) {
      json_t *gate=json_array_get(jGates,j);
      gates[j]=json_boolean_value(gate);
      json_t *gate1=json_array_get(jGates1,j);
      gates1[j]=json_boolean_value(gate1);
      json_t *jcv=json_array_get(jCvList,j);
      cv[j]=json_real_value(jcv);
    }
    json_t *jMin=json_object_get(rootJ,"min");
    if(jMin) {
      min=json_real_value(jMin);
    }

    json_t *jMax=json_object_get(rootJ,"max");
    if(jMax) {
      max=json_real_value(jMax);
    }
    json_t *jQuantize=json_object_get(rootJ,"quantize");
    if(jQuantize) {
      quantize=json_integer_value(jQuantize);
    }
    json_t *jSampleAndHold=json_object_get(rootJ,"sampleAndHold");
    if(jSampleAndHold) {
      sampleAndHold=json_integer_value(jSampleAndHold);
    }
    reconfig();
  }

  void reconfig() {
    for(int nr=0;nr<16;nr++) {
      float value=getParamQuantity(CV_PARAMS+nr)->getValue();
      if(value>max)
        value=max;
      if(value<min)
        value=min;
      configParam(CV_PARAMS+nr,min,max,0,"CV "+std::to_string(nr+1));
      getParamQuantity(CV_PARAMS+nr)->setValue(value);
      dirty=16;
    }
  }
};

struct ASEQColorKnobWidget : Widget {
  ASEQ *module=nullptr;
  int nr;

  void init(ASEQ *_module,int _nr) {
    module=_module;
    nr=_nr;
  }

  void draw(const DrawArgs &args) override {
    NVGcolor color=nvgRGB(0xff,0xff,0xff);
    if(module) {
      if(nr>=int(module->params[ASEQ::LEN_PARAM].getValue())) {
        color=nvgRGB(0x33,0x33,0x33);
      } else {
        if(module->pos==nr) {
          color=nvgRGB(0xbb,0xbb,0x00);
        } else {
          if(module->gates[nr]) {
            if(module->cv[nr]==module->params[ASEQ::CV_PARAMS+nr].getValue())
              color=nvgRGB(0x66,0xaa,0x66);
            else
              color=nvgRGB(0xaa,0x66,0x66);
          } else {
            color=nvgRGB(0x66,0x66,0x66);
          }
        }
      }
    }
    float c=box.size.x*0.5f;

    nvgBeginPath(args.vg);
    nvgCircle(args.vg,c,c,c);
    nvgFillColor(args.vg,nvgRGB(0xbe,0xb5,0xd5));
    nvgFill(args.vg);

    nvgBeginPath(args.vg);
    nvgCircle(args.vg,c,c,c-2);
    nvgFillColor(args.vg,nvgRGB(0xd8,0xd8,0xd8));
    nvgFill(args.vg);
    nvgBeginPath(args.vg);
    nvgCircle(args.vg,c,c,c-2);
    nvgFillColor(args.vg,color);
    nvgFill(args.vg);

    nvgBeginPath(args.vg);
    nvgMoveTo(args.vg,c,0);
    nvgLineTo(args.vg,c,c);
    nvgStrokeWidth(args.vg,2);
    nvgStrokeColor(args.vg,nvgRGB(0x24,0x31,0xa5));
    nvgStroke(args.vg);
  }
};

struct ASEQKnob : Knob {
  FramebufferWidget *fb;
  CircularShadow *shadow;
  TransformWidget *tw;
  ASEQColorKnobWidget *w;
  ASEQ *module=nullptr;
  int nr=0;

  void init(ASEQ *_module,int _nr) {
    module=_module;
    nr=_nr;
    w->init(module,nr);
  }

  ASEQKnob() {
    fb=new widget::FramebufferWidget;
    addChild(fb);
    box.size=mm2px(Vec(6.23f,6.23f));
    fb->box.size=box.size;
    shadow=new CircularShadow;
    fb->addChild(shadow);
    shadow->box.size=box.size;
    shadow->box.pos=math::Vec(0,box.size.y*0.1f);
    tw=new TransformWidget;
    tw->box.size=box.size;
    fb->addChild(tw);

    w=new ASEQColorKnobWidget;
    w->box.size=box.size;
    tw->addChild(w);
    minAngle=-0.83f*M_PI;
    maxAngle=0.83f*M_PI;
  }

  void onChange(const ChangeEvent &e) override {
    // Re-transform the widget::TransformWidget
    engine::ParamQuantity *pq=getParamQuantity();
    if(pq) {
      float value=pq->getValue();
      float angle;
      if(!pq->isBounded()) {
        // Number of rotations equals value for unbounded range
        angle=value*(2*M_PI);
      } else if(pq->getRange()==0.f) {
        // Center angle for zero range
        angle=(minAngle+maxAngle)/2.f;
      } else {
        // Proportional angle for finite range
        angle=math::rescale(value,pq->getMinValue(),pq->getMaxValue(),minAngle,maxAngle);
      }
      angle=std::fmod(angle,2*M_PI);
      if(module) {
        module->setCV(value,nr);
      }
      tw->identity();
      // Rotate SVG
      math::Vec center=w->box.getCenter();
      tw->translate(center);
      tw->rotate(angle);
      tw->translate(center.neg());
      fb->dirty=true;
    }
    Knob::onChange(e);
  }

  void step() override {
    fb->dirty=true;
    if(module&&module->dirty) {
      ChangeEvent c;
      onChange(c);
      module->dirty--;
    }
    Knob::step();
  }

  void onButton(const event::Button &e) override {
    if(!module)
      return;
    if(e.action==GLFW_PRESS&&e.button==GLFW_MOUSE_BUTTON_LEFT&&(e.mods&RACK_MOD_MASK)==GLFW_MOD_SHIFT) {
      module->gates[nr]=!module->gates[nr];
      module->gates1[nr]=!module->gates1[nr];
      e.consume(this);
    }
    Knob::onButton(e);
  }

  void onAction(const ActionEvent &e) override {
    if(module) {
      module->gates[nr]=!module->gates[nr];
      module->gates1[nr]=!module->gates1[nr];
    }
    Knob::onAction(e);
  }

};

struct ASEQWidget : ModuleWidget {
  ASEQWidget(ASEQ *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/ASEQ.svg")));
    float y=11;
    float x=3;
    float x1=14;
    float x2=25;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,ASEQ::DICE_TRIG_INPUT));
    addParam(createParam<MLEDM>(mm2px(Vec(x1,y)),module,ASEQ::DICE_TRIG_PARAM));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,ASEQ::DENS_PARAM));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x1,y)),module,ASEQ::RANGE_PARAM));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,ASEQ::LEN_PARAM));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x1,y)),module,ASEQ::HOLD_PARAM));
    y+=12;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,ASEQ::VAR_TRIG_INPUT));
    addParam(createParam<MLEDM>(mm2px(Vec(x1,y)),module,ASEQ::VAR_TRIG_PARAM));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,ASEQ::ADD_PARAM));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x1,y)),module,ASEQ::REMOVE_PARAM));
    y+=12;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,ASEQ::MOD_TRIG_INPUT));
    addParam(createParam<MLEDM>(mm2px(Vec(x1,y)),module,ASEQ::MOD_TRIG_PARAM));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x+5.5,y+5)),module,ASEQ::COUNT_PARAM));

    y=86;
    addParam(createParam<MLEDM>(mm2px(Vec(x1,y)),module,ASEQ::SET_TRIG_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,ASEQ::SET_TRIG_INPUT));
    y+=9;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,ASEQ::RST_INPUT));
    addParam(createParam<MLEDM>(mm2px(Vec(x1,y)),module,ASEQ::RST_TRIG_PARAM));
    y+=9;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,ASEQ::CLK_INPUT));
    addParam(createParam<MLEDM>(mm2px(Vec(x1,y)),module,ASEQ::CLOCK_TRIG_PARAM));
    y=116;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,ASEQ::GATE_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x1,y)),module,ASEQ::CV_OUTPUT));

    y=11;
    for(int k=0;k<16;k++) {
      auto cvParam=createParam<ASEQKnob>(mm2px(Vec(x2,y)),module,ASEQ::CV_PARAMS+k);
      cvParam->init(module,k);
      addParam(cvParam);
      y+=7;
    }

  }

  void appendContextMenu(Menu *menu) override {
    ASEQ *module=dynamic_cast<ASEQ *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    std::vector<MinMaxRange> ranges={{-10,10},
                                     {-5, 5},
                                     {-4, 4},
                                     {-3, 3},
                                     {-2, 2},
                                     {-1, 1},
                                     {0,  1},
                                     {0,  2},
                                     {0,  3},
                                     {0,  5},
                                     {0,  10}};
    auto rangeSelectItem=new RangeSelectItem<ASEQ>(module,ranges);
    rangeSelectItem->text="Range";
    rangeSelectItem->rightText=string::f("%0.1f/%0.1fV",module->min,module->max)+"  "+RIGHT_ARROW;
    menu->addChild(rangeSelectItem);
    menu->addChild(new MenuSeparator);
    menu->addChild(createBoolPtrMenuItem("Quantize","",&module->quantize));
    menu->addChild(createBoolPtrMenuItem("S&H Mode","",&module->sampleAndHold));
  }

};


Model *modelASEQ=createModel<ASEQ,ASEQWidget>("ASEQ");