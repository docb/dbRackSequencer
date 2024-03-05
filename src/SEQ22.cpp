#include "plugin.hpp"

struct ProbNote {
  uint8_t notes[1600]={};
  unsigned len=0;
  uint8_t noteOccurence[16]={};

  void setNoteOccurence(int note,uint8_t f) {
    noteOccurence[note]=f;
    calculate();
  }

  void calculate() {
    int pos=0;
    for(int k=0;k<16;k++) {
      for(int j=0;j<noteOccurence[k];j++) {
        notes[pos++]=k;
      }
    }
    len=pos;
  }

  int getNote(float rnd) {
    if(len<2)
      return 0;
    return notes[int(rnd*float(len))];
  }

};

struct SEQ22 : Module {
  enum ParamId {
    FRQ_PARAM,REST_PARAM=FRQ_PARAM+16,SEED_PARAM,PARAMS_LEN
  };
  enum InputId {
    CLK_INPUT,RST_INPUT,CV_INPUT,SEED_INPUT,REST_INPUT,POLY_FRQ_INPUT,FRQ_INPUT,INPUTS_LEN=FRQ_INPUT+16
  };
  enum OutputId {
    CV_OUTPUT,GATE_OUTPUT,TRG_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  ProbNote probNote;
  float noteValues[16]={};
  RND rnd;
  dsp::SchmittTrigger clockTrigger[16];;
  dsp::SchmittTrigger rstTrigger;
  dsp::PulseGenerator rstPulse;
  dsp::PulseGenerator trgPulse[16];
  dsp::ClockDivider divider;
  int currentNote[16]={};
  float currentCV[16]={};
  bool gateOn[16]={};
  bool sampleAndHold=true;

  SEQ22() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    for(int k=0;k<16;k++) {
      std::string label=std::to_string(k);
      configParam(FRQ_PARAM+k,0,100,0,"absolute frequency "+label);
      configInput(FRQ_INPUT+k,"absolute frequency "+label);
    }
    configParam(SEED_PARAM,0,1,0,"Random Seed");
    configParam(REST_PARAM,0,1,0,"Rest");
    configInput(CLK_INPUT,"Clock");
    configInput(CV_INPUT,"CV");
    configInput(REST_INPUT,"CV");
    configInput(SEED_INPUT,"Random Seed");
    configInput(RST_INPUT,"Reset");
    configInput(POLY_FRQ_INPUT,"absolute frequency (poly)");
    configOutput(CV_OUTPUT,"CV");
    configOutput(GATE_OUTPUT,"Gate");
    configOutput(TRG_OUTPUT,"Trg");
    divider.setDivision(256);
  }

  void onAdd(const AddEvent &e) override {
    Module::onAdd(e);
    float seedParam=0;
    if(inputs[SEED_INPUT].isConnected())
      seedParam=inputs[SEED_INPUT].getVoltage()/10.f;
    rnd.reset((unsigned long long)(floor((double)seedParam*(double)ULONG_MAX)));
  }

  void process(const ProcessArgs &args) override {
    if(divider.process()) {
      if(inputs[POLY_FRQ_INPUT].isConnected()) {
        for(int c=0;c<inputs[POLY_FRQ_INPUT].getChannels();c++) {
          setImmediateValue(getParamQuantity(FRQ_PARAM+c),inputs[POLY_FRQ_INPUT].getVoltage(c)*10.f);
        }
      }
      for(int k=0;k<16;k++) {
        if(inputs[FRQ_INPUT+k].isConnected()) {
          setImmediateValue(getParamQuantity(FRQ_PARAM+k),inputs[FRQ_INPUT+k].getVoltage()*10.f);
        }
      }
      for(int k=0;k<16;k++) {
        probNote.setNoteOccurence(k,params[k].getValue());
      }
      if(inputs[CV_INPUT].isConnected()) {
        for(int k=0;k<16;k++) {
          noteValues[k]=inputs[CV_INPUT].getVoltage(k);
        }
      } else {
        for(int k=0;k<16;k++) {
          noteValues[k]=float(k)/12.f;
        }
      }
    }
    int channels=inputs[CLK_INPUT].getChannels();
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())) {
      float seedParam;
      if(inputs[SEED_INPUT].isConnected()) {
        seedParam=clamp(inputs[SEED_INPUT].getVoltage()*0.1f);
        setImmediateValue(getParamQuantity(SEED_PARAM),seedParam);
      } else {
        seedParam=params[SEED_PARAM].getValue();
      }
      seedParam=floorf(seedParam*10000)/10000;
      auto seedInput=(unsigned long long)(floor((double)seedParam*(double)ULONG_MAX));
      rnd.reset(seedInput);
    }
    for(int c=0;c<channels;c++) {
      if(clockTrigger[c].process(inputs[CLK_INPUT].getVoltage(c))/*&&!resetGate*/) {
        if(inputs[REST_INPUT].isConnected()) {
          float rest=clamp(inputs[REST_INPUT].getPolyVoltage(c),0.f,10.f)*0.1;
          gateOn[c]=rnd.nextCoin(rest);
        } else {
          gateOn[c]=rnd.nextCoin(params[REST_PARAM].getValue());
        }
        int note=probNote.getNote(float(rnd.nextDouble()));
        if(note!=currentNote[c]) {
          currentNote[c]=note;
          if(gateOn[c])
            trgPulse[c].trigger();
        }
      }
      if(sampleAndHold) {
        if(gateOn[c])
          currentCV[c]=noteValues[currentNote[c]];
      } else {
        currentCV[c]=noteValues[currentNote[c]];
      }
      outputs[CV_OUTPUT].setVoltage(currentCV[c],c);
      outputs[GATE_OUTPUT].setVoltage(gateOn[c]?inputs[CLK_INPUT].getVoltage(c):0.0f,c);
      bool rtr=trgPulse[c].process(args.sampleTime);
      outputs[TRG_OUTPUT].setVoltage(rtr?10.0f:0.0f,c);
    }
    outputs[CV_OUTPUT].setChannels(channels);
    outputs[GATE_OUTPUT].setChannels(channels);
    outputs[TRG_OUTPUT].setChannels(channels);
  }

  json_t *dataToJson() override {
    json_t *data=json_object();
    json_object_set_new(data,"sampleAndHold",json_integer(sampleAndHold));
    return data;
  }

  void dataFromJson(json_t *rootJ) override {
    json_t *jSampleAndHold=json_object_get(rootJ,"sampleAndHold");
    if(jSampleAndHold) {
      sampleAndHold=json_integer_value(jSampleAndHold);
    }
  }
};

struct DBSlider : app::SvgSlider {
  DBSlider() {
    setBackgroundSvg(Svg::load(asset::plugin(pluginInstance,"res/DBSlider.svg")));
    setHandleSvg(Svg::load(asset::plugin(pluginInstance,"res/DBSliderHandle.svg")));
    setHandlePosCentered(math::Vec(19.84260/2,92-11.74218/2),math::Vec(19.84260/2,0.0+11.74218/2));
  }
};

struct SEQ22Widget : ModuleWidget {
  SEQ22Widget(SEQ22 *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/SEQ22.svg")));
    for(int k=0;k<16;k++) {
      if(k<8) {
        addParam(createParam<DBSlider>(mm2px(Vec(3+k*7,10)),module,k));
        addInput(createInput<SmallPort>(mm2px(Vec(3.5+k*7,42)),module,SEQ22::FRQ_INPUT+k));

      } else {
        addParam(createParam<DBSlider>(mm2px(Vec(3+(k-8)*7,53)),module,k));
        addInput(createInput<SmallPort>(mm2px(Vec(3.5+(k-8)*7,84.5)),module,SEQ22::FRQ_INPUT+k));
      }
    }
    addParam(createParam<TrimbotWhite9>(mm2px(Vec(15,99)),module,SEQ22::REST_PARAM));
    addParam(createParam<TrimbotWhite9>(mm2px(Vec(37.5,99)),module,SEQ22::SEED_PARAM));

    addInput(createInput<SmallPort>(mm2px(Vec(4,116)),module,SEQ22::CV_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(4,106)),module,SEQ22::RST_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(4,96)),module,SEQ22::CLK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(39,109.5)),module,SEQ22::SEED_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(16.5,109.5)),module,SEQ22::REST_INPUT));

    addInput(createInput<SmallPort>(mm2px(Vec(28,104.5)),module,SEQ22::POLY_FRQ_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(51,96)),module,SEQ22::TRG_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(51,106)),module,SEQ22::GATE_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(51,116)),module,SEQ22::CV_OUTPUT));
  }

  void appendContextMenu(Menu *menu) override {
    auto *module=dynamic_cast<SEQ22 *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    menu->addChild(createBoolPtrMenuItem("S&H Mode","",&module->sampleAndHold));
  }

};


Model *modelSEQ22=createModel<SEQ22,SEQ22Widget>("SEQ22");