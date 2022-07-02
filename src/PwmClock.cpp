#include "plugin.hpp"

#define NUM_CLOCKS 9

struct Clock {
  double period=0;
  double step=0;
  bool oddBeat=false;
  dsp::PulseGenerator clockPulse;
  void reset() {
    step=-1;
    clockPulse.reset();
    oddBeat=false;
  }
  void setNewLength(double newLength) {
    if(step>=0) step *= (newLength/period);
    period = newLength;
  }
  bool process(float pwm,float sampleTime,float swing) {
    bool ret=false;
    double dur=period;
    if(oddBeat) dur+=dur*swing;
    if(step<0) step=dur; else step+=sampleTime;
    if(step>=dur) {
      step-=dur;
      clockPulse.trigger(period*pwm);
      ret=true;
      oddBeat=!oddBeat;
    } else {
      ret=clockPulse.process(sampleTime);
    }
    return ret;
  }
};


struct PwmClock : Module {
  enum ParamId {
    BPM_PARAM,RUN_PARAM,RST_PARAM,RATIO_PARAM,SWING_PARAM=RATIO_PARAM+NUM_CLOCKS,PWM_PARAM=SWING_PARAM+NUM_CLOCKS,PARAMS_LEN=PWM_PARAM+NUM_CLOCKS
  };
  enum InputId {
    RUN_INPUT,RST_INPUT,BPM_INPUT,PWM_INPUT,INPUTS_LEN=PWM_INPUT+NUM_CLOCKS
  };
  enum OutputId {
    RUN_OUTPUT,RST_OUTPUT,CLK_OUTPUT,BPM_OUTPUT=CLK_OUTPUT+NUM_CLOCKS,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  std::vector <std::string> labels={"16/1 (/64)","8/1 (/32)","4/1 (/16)","2/1 (/8)","1/1 (/4)","3/4 (/3)","1/2 (/2)","3/8 (/1.5)","1/3 (4/3)","1/4 (x1 Beat)","3/16 (x1.5)","1/6","1/8 x2","3/32 x2.5","1/12","1/16 (x4)","3/64","1/24 x6","1/32 (x8)","3/128","1/48 (x12)","1/64 (x16)"};
  float ticks[22]={16*4,8*4,4*4,2*4,4,0.75*4,0.5*4,0.375*4,4/3.f,0.25*4,0.1875*4,4.f/6.f,0.125*4,0.09375*4,1.f/3.f,0.0625*4,0.046875*4,1/6.f,0.03125*4,0.0234375*4,1/12.f,0.015625};

  Clock clocks[NUM_CLOCKS];
  dsp::SchmittTrigger resetTrigger;
  dsp::SchmittTrigger onTrigger;
  dsp::SchmittTrigger onInputTrigger;
  dsp::SchmittTrigger resetInputTrigger;
  dsp::SchmittTrigger pulseTrigger;
  dsp::PulseGenerator rstPulse;
  dsp::ClockDivider bpmParamDivider;
  float sampleRate=0;
  float bpm=0.f;
  bool init=true;

  PwmClock() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configParam(BPM_PARAM,30,240,120,"BPM");
    configButton(RUN_PARAM,"Run");
    configButton(RST_PARAM,"Reset");
    for(int k=0;k<NUM_CLOCKS;k++) {
      configSwitch(RATIO_PARAM+k,0,21,15,"Ratio "+std::to_string(k+1),labels);
      getParamQuantity(RATIO_PARAM+k)->snapEnabled=true;
      configParam(PWM_PARAM+k,0,1,0.5,"PW " + std::to_string(k+1));
      configParam(SWING_PARAM+k,0,0.5,0,"Swing "+std::to_string(k+1),"%",0,100);
      configInput(PWM_INPUT+k,"Pw "+std::to_string(k+1));
      configOutput(CLK_OUTPUT+k,"Clk "+std::to_string(k+1));
    }
    configInput(RUN_INPUT,"Run");
    configInput(RST_INPUT,"Reset");
    configOutput(RUN_OUTPUT,"Run");
    configOutput(BPM_OUTPUT,"BPM (V/Oct)");
    configInput(BPM_INPUT,"BPM (V/Oct)");
    configOutput(RST_OUTPUT,"Reset");
    bpmParamDivider.setDivision(64);
  }

  float getBpm() {
    return params[BPM_PARAM].getValue();
  }

  int getRatioIndex(int k) {
    return int(params[RATIO_PARAM+k].getValue());
  }

  void updateBpm(float sr) {
    float new_bpm=params[BPM_PARAM].getValue();
    if(bpm!=new_bpm||sr!=sampleRate) {
      bpm=new_bpm;
      sampleRate=sr;
      for(int k=0;k<NUM_CLOCKS;k++)
        changeRatio(k);
    }
  }

  void updateRatio(int k) {
    for(int k=0;k<NUM_CLOCKS;k++) {
      clocks[k].reset();
    }
    changeRatio(k);
  }

  void changeRatio(int k) {
    int idx=int(params[RATIO_PARAM+k].getValue());
    double length=(ticks[idx]*60.f)/bpm;
    clocks[k].setNewLength(length);
  }
  float getPwm(int k) {
    if(inputs[PWM_INPUT+k].isConnected()) {
      getParamQuantity(PWM_PARAM+k)->setValue(inputs[PWM_INPUT+k].getVoltage()/10.f);
    }
    return params[PWM_PARAM+k].getValue();
  };

  void sendReset() {
    rstPulse.trigger(0.001f);
  }



  void process(const ProcessArgs &args) override {
    if(bpmParamDivider.process()) {
      if(inputs[BPM_INPUT].isConnected()) {
        float freq=powf(2,clamp(inputs[BPM_INPUT].getVoltage(),-1.f,2.f));
        getParamQuantity(BPM_PARAM)->setValue(freq*60.f);
      }
      updateBpm(args.sampleRate);
      outputs[BPM_OUTPUT].setVoltage(log2f(params[BPM_PARAM].getValue()/60.f));
    }
    if(init) {
      updateBpm(args.sampleRate);
      init=false;
    }
    if(resetTrigger.process(params[RST_PARAM].getValue())|resetInputTrigger.process(inputs[RST_INPUT].getVoltage())) {
      sendReset();
      for(int k=0;k<NUM_CLOCKS;k++) {
        clocks[k].reset();
        changeRatio(k);
      }
    }
    if(inputs[RUN_INPUT].isConnected())
      getParamQuantity(RUN_PARAM)->setValue(float(inputs[RUN_INPUT].getVoltage()>0.f));
    if(onTrigger.process(params[RUN_PARAM].getValue())) {
      sendReset();
      for(int k=0;k<NUM_CLOCKS;k++) {
        clocks[k].reset();
        changeRatio(k);
      }
    }
    bool on=params[RUN_PARAM].getValue()>0.f||inputs[RUN_INPUT].getVoltage()>0.f;

    if(on) {
      for(int k=0;k<NUM_CLOCKS;k++) {
        float pwm=getPwm(k);
        float swing=params[SWING_PARAM+k].getValue();
        outputs[CLK_OUTPUT+k].setVoltage(clocks[k].process(pwm,args.sampleTime,swing)?10.f:0.f);
      }
    } else {
      for(int k=0;k<NUM_CLOCKS;k++) {
        outputs[CLK_OUTPUT+k].setVoltage(0.f);
      }
    }
    outputs[RUN_OUTPUT].setVoltage(on?10.f:0.f);
    outputs[RST_OUTPUT].setVoltage(rstPulse.process(args.sampleTime)?10.f:0.f);
  }
};

struct RatioKnob : TrimbotWhite {
  PwmClock *module;
  int nr;

  void onChange(const ChangeEvent &e) override {
    SVGKnob::onChange(e);
    if(module)
      module->updateRatio(nr);
  }
};

struct BpmDisplay : OpaqueWidget {
  std::basic_string<char> fontPath;
  PwmClock *module;

  BpmDisplay(PwmClock *_module) : OpaqueWidget(),module(_module) {
    fontPath=asset::plugin(pluginInstance,"res/FreeMonoBold.ttf");
  }

  void drawLayer(const DrawArgs &args,int layer) override {
    if(layer==1) {
      _draw(args);
    }
    Widget::drawLayer(args,layer);
  }

  void _draw(const DrawArgs &args) {
    std::string text="120.00";
    if(module) {
      text=rack::string::f("%3.2f",module->getBpm());
    }
    std::shared_ptr <Font> font=APP->window->loadFont(fontPath);
    nvgFillColor(args.vg,nvgRGB(255,255,128));
    nvgFontFaceId(args.vg,font->handle);
    nvgFontSize(args.vg,20);
    nvgTextAlign(args.vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
    nvgText(args.vg,box.size.x/2,box.size.y/2,text.c_str(),NULL);
  }
};
struct RatioDisplay : OpaqueWidget {
  PwmClock *module;
  int nr;

  std::basic_string<char> fontPath;
  std::vector <std::string> labels={"16/1","8/1","4/1","2/1","1/1","3/4","1/2","3/8","1/3","1/4","3/16","1/6","1/8","3/32","1/12","1/16","3/64","1/24","1/32","3/128","1/48","1/64"};

  RatioDisplay(PwmClock *_module,int _nr) : OpaqueWidget(),module(_module),nr(_nr) {
    fontPath=asset::plugin(pluginInstance,"res/FreeMonoBold.ttf");
  }

  void drawLayer(const DrawArgs &args,int layer) override {
    if(layer==1) {
      _draw(args);
    }
    Widget::drawLayer(args,layer);
  }

  void _draw(const DrawArgs &args) {
    std::string text="1/16";
    if(module) {
      text=labels[module->getRatioIndex(nr)];
    }
    std::shared_ptr <Font> font=APP->window->loadFont(fontPath);
    nvgFillColor(args.vg,nvgRGB(255,255,128));
    nvgFontFaceId(args.vg,font->handle);
    nvgFontSize(args.vg,9);
    nvgTextAlign(args.vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
    nvgText(args.vg,0,box.size.y/2,text.c_str(),NULL);
  }
};
struct PwmClockWidget : ModuleWidget {
  PwmClockWidget(PwmClock *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/PwmClock.svg")));

    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,0)));
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));
    auto bpmParam = createParam<TrimbotWhite9>(mm2px(Vec(12,MHEIGHT-108.2-8.985)),module,PwmClock::BPM_PARAM);
    //if(module) bpmParam->update = &module->update;
    addParam(bpmParam);
    auto bpmDisplay = new BpmDisplay(module);
    bpmDisplay->box.pos=mm2px(Vec(5,MHEIGHT-105));
    bpmDisplay->box.size=mm2px(Vec(23.2,4));
    addChild(bpmDisplay);
    addInput(createInput<SmallPort>(mm2px(Vec(3,TY(110))),module,PwmClock::BPM_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(24,TY(110))),module,PwmClock::BPM_OUTPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(35,TY(112))),module,PwmClock::RUN_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(35,TY(100))),module,PwmClock::RST_INPUT));
    addParam(createParam<MLED>(mm2px(Vec(43,TY(112))),module,PwmClock::RUN_PARAM));
    addParam(createParam<MLEDM>(mm2px(Vec(43,TY(100))),module,PwmClock::RST_PARAM));
    addOutput(createOutput<SmallPort>(mm2px(Vec(51,TY(100))),module,PwmClock::RUN_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(51,TY(100))),module,PwmClock::RST_OUTPUT));
    float y=TY(80);
    for(int k=0;k<NUM_CLOCKS;k++) {
      auto ratioKnob=createParam<RatioKnob>(mm2px(Vec(3,y)),module,PwmClock::RATIO_PARAM+k);
      ratioKnob->module=module;
      ratioKnob->nr=k;
      addParam(ratioKnob);
      auto ratioDisplay = new RatioDisplay(module,k);
      ratioDisplay->box.pos=mm2px(Vec(10.5,y+2.5));
      ratioDisplay->box.size=mm2px(Vec(10,2));
      addChild(ratioDisplay);

      addParam(createParam<TrimbotWhite>(mm2px(Vec(22,y)),module,PwmClock::SWING_PARAM+k));
      addParam(createParam<TrimbotWhite>(mm2px(Vec(40,y)),module,PwmClock::PWM_PARAM+k));
      addInput(createInput<SmallPort>(mm2px(Vec(32,y)),module,PwmClock::PWM_INPUT+k));
      addOutput(createOutput<SmallPort>(mm2px(Vec(51,y)),module,PwmClock::CLK_OUTPUT+k));

      y+=9;
    }
  }
};


Model *modelPwmClock=createModel<PwmClock,PwmClockWidget>("PwmClock");