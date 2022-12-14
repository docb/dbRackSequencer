#include "plugin.hpp"
#include <sstream>

#define NUM_CLOCKS 9

struct Clock {
  double period=0;
  dsp::PulseGenerator clockPulse;

  void reset() {
    clockPulse.reset();
  }

  void setNewLength(double newLength) {
    period=newLength;
  }

  bool process(float pwm,float sampleRate,float swing,uint32_t pos) {
    double sampleTime=1.0/double(sampleRate);
    double time=pos*sampleTime;
    auto beats=uint32_t(time/period);
    bool odd=beats%2;
    if(odd && swing >0.f) {
      uint32_t swi=int32_t(period*swing*sampleRate);
      uint32_t pos1=pos-swi;
      double time1=pos1*sampleTime;
      double rest=fmod(time1,period);
      if(rest<sampleTime) {
        //INFO("trigger oddbeat beats=%d pos1=%d pos=%d period=%f rest=%lf",beats,pos1,pos,period,rest);
        clockPulse.trigger(float(period)*pwm);
      }
    } else {
      double rest=fmod(time,period);
      if(rest<sampleTime) {
        //INFO("trigger beat beats=%d pos=%d period=%f rest=%lf",beats,pos,period,rest);
        clockPulse.trigger(float(period)*pwm);
      }
    }
    return clockPulse.process(1.f/sampleRate);
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
  std::vector<std::string> labels={"16/1 (/64)","8/1 (/32)","4/1 (/16)","2/1 (/8)","1/1 (/4)","3/4 (/3)","1/2 (/2)","3/8 (/1.5)","1/3 (4/3)","1/4 (x1 Beat)","3/16 (x1.5)","1/6","1/8 x2","3/32 x2.5","1/12","1/16 (x4)","3/64","1/24 x6","1/32 (x8)","3/128","1/48 (x12)","1/64 (x16)","1/128 (x32)"};
  float ticks[23]={16*4,8*4,4*4,2*4,4,0.75*4,0.5*4,0.375*4,4/3.f,0.25*4,0.1875*4,4.f/6.f,0.125*4,0.09375*4,1.f/3.f,0.0625*4,0.046875*4,1/6.f,0.03125*4,0.0234375*4,1/12.f,0.015625*4,0.015625*2};

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
  bool bpmVoltageStandard=false;
  bool showTime=true;
  uint32_t pos=0;

  PwmClock() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configParam(BPM_PARAM,30,240,120,"BPM");
    configButton(RUN_PARAM,"Run");
    configButton(RST_PARAM,"Reset");
    for(int k=0;k<NUM_CLOCKS;k++) {
      configSwitch(RATIO_PARAM+k,0,22,15,"Ratio "+std::to_string(k+1),labels);
      getParamQuantity(RATIO_PARAM+k)->snapEnabled=true;
      configParam(PWM_PARAM+k,0,1,0.5,"PW "+std::to_string(k+1));
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

  double getCurrentTime() {
    return double(pos)*APP->engine->getSampleTime();
  }

  void process(const ProcessArgs &args) override {
    if(bpmParamDivider.process()) {
      if(bpmVoltageStandard) {
        if(inputs[BPM_INPUT].isConnected()) {
          float freq=powf(2,clamp(inputs[BPM_INPUT].getVoltage(),-2.f,1.f));
          getParamQuantity(BPM_PARAM)->setValue(freq*120.f);
        }
        updateBpm(args.sampleRate);
        outputs[BPM_OUTPUT].setVoltage(log2f(params[BPM_PARAM].getValue()/120.f));
      } else {
        if(inputs[BPM_INPUT].isConnected()) {
          float freq=powf(2,clamp(inputs[BPM_INPUT].getVoltage(),-1.f,2.f));
          getParamQuantity(BPM_PARAM)->setValue(freq*60.f);
        }
        updateBpm(args.sampleRate);
        outputs[BPM_OUTPUT].setVoltage(log2f(params[BPM_PARAM].getValue()/60.f));
      }
    }
    if(init) {
      updateBpm(args.sampleRate);
      init=false;
    }
    if(resetTrigger.process(params[RST_PARAM].getValue())|resetInputTrigger.process(inputs[RST_INPUT].getVoltage())) {
      sendReset();
      pos=0;
      for(int k=0;k<NUM_CLOCKS;k++) {
        clocks[k].reset();
        changeRatio(k);
      }
    }
    if(inputs[RUN_INPUT].isConnected())
      getParamQuantity(RUN_PARAM)->setValue(float(inputs[RUN_INPUT].getVoltage()>0.f));
    if(onTrigger.process(params[RUN_PARAM].getValue())) {
      sendReset();
      pos=0;
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
        outputs[CLK_OUTPUT+k].setVoltage(clocks[k].process(pwm,args.sampleRate,swing,pos)?10.f:0.f);
      }
    } else {
      for(int k=0;k<NUM_CLOCKS;k++) {
        outputs[CLK_OUTPUT+k].setVoltage(0.f);
      }
    }
    outputs[RUN_OUTPUT].setVoltage(on?10.f:0.f);
    outputs[RST_OUTPUT].setVoltage(rstPulse.process(args.sampleTime)?10.f:0.f);
    if(on)
      pos++;
  }

  json_t *dataToJson() override {
    json_t *root=json_object();
    json_object_set_new(root,"bpmVoltageStandard",json_boolean(bpmVoltageStandard));
    json_object_set_new(root,"showTime",json_boolean(showTime));
    return root;
  }

  void dataFromJson(json_t *root) override {
    json_t *jBpmVoltageStandard=json_object_get(root,"bpmVoltageStandard");
    if(jBpmVoltageStandard) {
      bpmVoltageStandard=json_boolean_value(jBpmVoltageStandard);
    }
    json_t *jShowTime=json_object_get(root,"showTime");
    if(jShowTime) {
      showTime=json_boolean_value(jShowTime);
    }
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
    std::shared_ptr<Font> font=APP->window->loadFont(fontPath);
    nvgFillColor(args.vg,nvgRGB(255,255,128));
    nvgFontFaceId(args.vg,font->handle);
    nvgFontSize(args.vg,20);
    nvgTextAlign(args.vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
    nvgText(args.vg,box.size.x/2,box.size.y/2,text.c_str(),NULL);
  }
};

struct TimeDisplay : OpaqueWidget {
  PwmClock *module;
  std::basic_string<char> fontPath;

  TimeDisplay(PwmClock *_module) : module(_module) {
    fontPath=asset::plugin(pluginInstance,"res/FreeMonoBold.ttf");
  }

  void drawLayer(const DrawArgs &args,int layer) override {
    if(layer==1) {
      _draw(args);
    }
    Widget::drawLayer(args,layer);
  }

  void _draw(const DrawArgs &args) {
    if(!module) return;
    if(!module->showTime) return;
    std::string text="000:00:000";
    auto timems=int32_t(module->getCurrentTime()*1000);
    uint32_t _seconds=timems/1000;
    uint32_t ms=timems%1000;
    uint32_t minutes=_seconds/60;
    uint32_t seconds=_seconds%60;
    std::stringstream ss;
    ss<<std::setfill('0')<<std::setw(3)<<minutes<<":"<<std::setw(2)<<seconds<<":"<<std::setw(3)<<ms;
    text=ss.str();
    std::shared_ptr<Font> font=APP->window->loadFont(fontPath);
    nvgFillColor(args.vg,nvgRGB(255,255,128));
    nvgFontFaceId(args.vg,font->handle);
    nvgFontSize(args.vg,13);
    nvgTextAlign(args.vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
    nvgText(args.vg,0,box.size.y/2,text.c_str(),NULL);

  }

};

struct RatioDisplay : OpaqueWidget {
  PwmClock *module;
  int nr;

  std::basic_string<char> fontPath;
  std::vector<std::string> labels={"16/1","8/1","4/1","2/1","1/1","3/4","1/2","3/8","1/3","1/4","3/16","1/6","1/8","3/32","1/12","1/16","3/64","1/24","1/32","3/128","1/48","1/64","1/128"};

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
    std::shared_ptr<Font> font=APP->window->loadFont(fontPath);
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
    auto bpmParam=createParam<TrimbotWhite9>(mm2px(Vec(12,MHEIGHT-108.2-8.985)),module,PwmClock::BPM_PARAM);
    //if(module) bpmParam->update = &module->update;
    addParam(bpmParam);
    auto bpmDisplay=new BpmDisplay(module);
    bpmDisplay->box.pos=mm2px(Vec(5,MHEIGHT-105));
    bpmDisplay->box.size=mm2px(Vec(23.2,4));
    addChild(bpmDisplay);
    auto timeDisplay=new TimeDisplay(module);
    timeDisplay->box.pos=mm2px(Vec(5,MHEIGHT-96));
    timeDisplay->box.size=mm2px(Vec(27,2.5));
    addChild(timeDisplay);
    addInput(createInput<SmallPort>(mm2px(Vec(3,TY(110))),module,PwmClock::BPM_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(24,TY(110))),module,PwmClock::BPM_OUTPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(35,TY(112))),module,PwmClock::RUN_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(35,TY(100))),module,PwmClock::RST_INPUT));
    addParam(createParam<MLED>(mm2px(Vec(43,TY(112))),module,PwmClock::RUN_PARAM));
    addParam(createParam<MLEDM>(mm2px(Vec(43,TY(100))),module,PwmClock::RST_PARAM));
    addOutput(createOutput<SmallPort>(mm2px(Vec(51,TY(112))),module,PwmClock::RUN_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(51,TY(100))),module,PwmClock::RST_OUTPUT));
    float y=TY(80);
    for(int k=0;k<NUM_CLOCKS;k++) {
      auto ratioKnob=createParam<RatioKnob>(mm2px(Vec(3,y)),module,PwmClock::RATIO_PARAM+k);
      ratioKnob->module=module;
      ratioKnob->nr=k;
      addParam(ratioKnob);
      auto ratioDisplay=new RatioDisplay(module,k);
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

  void appendContextMenu(Menu *menu) override {
    auto *module=dynamic_cast<PwmClock *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    menu->addChild(createBoolPtrMenuItem("BPM Voltage Standard", "", &module->bpmVoltageStandard));
    menu->addChild(createBoolPtrMenuItem("Show Time", "", &module->showTime));

  }

};


Model *modelPwmClock=createModel<PwmClock,PwmClockWidget>("PwmClock");