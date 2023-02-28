#include "plugin.hpp"


struct TXVI : Module {
  enum ParamId {
    UNUSED_PARAM,UNUSED2_PARAM,LEN_PARAM,CV_PARAMS,PARAMS_LEN=CV_PARAMS+16
  };
  enum InputId {
    CLK_INPUT,REV_INPUT,RST_INPUT,ADDR_INPUT,PAUSE_INPUT,CV_INPUT,INPUTS_LEN
  };
  enum OutputId {
    SCAN_OUTPUT,TRIG_OUTPUT,CV_OUTPUTS,GATE_OUTPUTS=CV_OUTPUTS+16,STEP_ADR_OUTPUT=GATE_OUTPUTS+16,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN=16
  };

  dsp::SchmittTrigger rstTrigger,clockTrigger;
  dsp::PulseGenerator rstPulse,trigPulse;
  int inMode=0;
  int outMode=0;
  int pos=0;
  int currentPos=0;
  int lastPos=-1;
  float reg[16][16]={};

  TXVI() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    for(int k=0;k<16;k++) {
      std::string strNr=std::to_string(k+1);
      configParam(CV_PARAMS+k,0,1,1,"CV "+strNr);
      configOutput(CV_OUTPUTS+k,"CV "+strNr);
      configOutput(GATE_OUTPUTS+k,"Gate "+strNr);
    }

    configParam(LEN_PARAM,1,16,16,"Length");
    getParamQuantity(LEN_PARAM)->snapEnabled=true;
    configInput(CLK_INPUT,"Clock");
    configInput(REV_INPUT,"Reverse");
    configInput(PAUSE_INPUT,"Pause");
    configInput(RST_INPUT,"Reset");
    configInput(ADDR_INPUT,"Stage Address");
    configInput(CV_INPUT,"CV");
    configOutput(SCAN_OUTPUT,"Scan");
    configOutput(STEP_ADR_OUTPUT,"Step Address");

    configOutput(TRIG_OUTPUT,"Step Trig");
  }

  void processTrigger(const ProcessArgs &args) {
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())) {
      rstPulse.trigger();
      pos=0;
    }
    bool rstGate=rstPulse.process(args.sampleTime);
    bool pause=inputs[PAUSE_INPUT].getVoltage()>1.f;
    bool reverse=inputs[REV_INPUT].getVoltage()>1.f;
    int len=params[LEN_PARAM].getValue();
    if(clockTrigger.process(inputs[CLK_INPUT].getVoltage())&&!rstGate&&!pause) {
      if(reverse) {
        pos--;
        if(pos<0)
          pos=len-1;
      } else {
        pos++;
        if(pos>=len)
          pos=0;
      }
    }
    int inPos=clamp(inputs[ADDR_INPUT].getVoltage(),0.f,10.f)*1.6f;
    currentPos=(pos+inPos)%len;
  }

  void process(const ProcessArgs &args) override {
    bool all=outMode==1;
    bool sh=inMode==1;
    int channels=inputs[CV_INPUT].getChannels();
    bool connected=inputs[CV_INPUT].isConnected();
    processTrigger(args);
    if(currentPos!=lastPos) {
      lastPos=currentPos;
      trigPulse.trigger(0.00005f);
      if(sh) {
        inputs[CV_INPUT].readVoltages(reg[currentPos]);
      }
    }
    for(int k=0;k<16;k++) {
      if(k==currentPos) {
        outputs[GATE_OUTPUTS+k].setVoltage(10.f);
        lights[k].setBrightness(1.f);
        if(!connected) {
          outputs[CV_OUTPUTS+k].setVoltage(10.f*params[CV_PARAMS+k].getValue());
          outputs[SCAN_OUTPUT].setVoltage(10.f*params[CV_PARAMS+k].getValue());
        } else {
          for(int chn=0;chn<channels;chn++) {
            float v=sh?reg[k][chn]:inputs[CV_INPUT].getVoltage(chn);
            outputs[CV_OUTPUTS+k].setVoltage(v*params[CV_PARAMS+k].getValue(),chn);
            outputs[SCAN_OUTPUT].setVoltage(v*params[CV_PARAMS+k].getValue(),chn);
          }
        }
      } else {
        outputs[GATE_OUTPUTS+k].setVoltage(0.f);
        lights[k].setBrightness(0.f);
        if(!all) {
          if(!connected) {
            outputs[CV_OUTPUTS+k].setVoltage(0.f);
          } else {
            for(int chn=0;chn<channels;chn++) {
              outputs[CV_OUTPUTS+k].setVoltage(0.f,chn);
            }
          }
        } else {
          if(!connected) {
            outputs[CV_OUTPUTS+k].setVoltage(10.f*params[CV_PARAMS+k].getValue());
          }
        }
      }
    }
    outputs[TRIG_OUTPUT].setVoltage(trigPulse.process(args.sampleTime)?10.f:0.f);
    outputs[STEP_ADR_OUTPUT].setVoltage(float(currentPos)*(10.f/16.f));
    for(int k=0;k<16;k++) {
      outputs[CV_OUTPUTS+k].setChannels(std::max(channels,1));
    }
    outputs[SCAN_OUTPUT].setChannels(std::max(channels,1));
  }

  json_t *dataToJson() override {
    json_t *data=json_object();
    json_object_set_new(data,"outMode",json_integer(outMode));
    json_object_set_new(data,"inMode",json_integer(inMode));
    return data;
  }

  void dataFromJson(json_t *rootJ) override {
    json_t *jMode = json_object_get(rootJ,"outMode");
    if(jMode!=nullptr) outMode = json_integer_value(jMode);
    jMode = json_object_get(rootJ,"inMode");
    if(jMode!=nullptr) inMode = json_integer_value(jMode);
  }
};


struct TXVIWidget : ModuleWidget {
  std::vector<std::string> inModeLabels={"Track","Sample"};
  std::vector<std::string> outModeLabels={"Solo","All"};

  TXVIWidget(TXVI *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/TXVI.svg")));
    float x1=4;
    float x2=17;
    float x3=29;
    float x4=41;
    float y=11;
    for(int k=0;k<16;k++) {
      addParam(createParam<TrimbotWhite>(mm2px(Vec(x2,y)),module,TXVI::CV_PARAMS+k));
      addOutput(createOutput<SmallPort>(mm2px(Vec(x3,y)),module,TXVI::CV_OUTPUTS+k));
      addOutput(createOutput<SmallPort>(mm2px(Vec(x4,y)),module,TXVI::GATE_OUTPUTS+k));
      addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(38,y+3.5)),module,k));
      y+=7;
    }

    y=11;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x1,y)),module,TXVI::LEN_PARAM));
    y+=11;
    addInput(createInput<SmallPort>(mm2px(Vec(x1,y)),module,TXVI::CLK_INPUT));
    y+=11;
    addInput(createInput<SmallPort>(mm2px(Vec(x1,y)),module,TXVI::REV_INPUT));
    y+=11;
    addInput(createInput<SmallPort>(mm2px(Vec(x1,y)),module,TXVI::PAUSE_INPUT));
    y+=11;
    addInput(createInput<SmallPort>(mm2px(Vec(x1,y)),module,TXVI::RST_INPUT));
    y+=11;
    addInput(createInput<SmallPort>(mm2px(Vec(x1,y)),module,TXVI::ADDR_INPUT));
    y=80;
    addInput(createInput<SmallPort>(mm2px(Vec(x1,y)),module,TXVI::CV_INPUT));
    y=94;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x1,y)),module,TXVI::TRIG_OUTPUT));
    y+=11;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x1,y)),module,TXVI::STEP_ADR_OUTPUT));
    y+=11;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x1,y)),module,TXVI::SCAN_OUTPUT));

  }
  void appendContextMenu(Menu *menu) override {
    TXVI *module=dynamic_cast<TXVI *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    auto inSelect=new LabelIntSelectItem(&module->inMode,inModeLabels);
    inSelect->text="In mode";
    inSelect->rightText=inModeLabels[module->inMode]+"  "+RIGHT_ARROW;
    menu->addChild(inSelect);

    auto outSelect=new LabelIntSelectItem(&module->outMode,outModeLabels);
    outSelect->text="Out mode";
    outSelect->rightText=outModeLabels[module->outMode]+"  "+RIGHT_ARROW;
    menu->addChild(outSelect);

  }
};


Model *modelTXVI=createModel<TXVI,TXVIWidget>("TXVI");