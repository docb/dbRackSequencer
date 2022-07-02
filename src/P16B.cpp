#include "plugin.hpp"
#include "rnd.h"

struct P16B : Module {
	enum ParamId {
    A_PARAM,B_PARAM,C_PARAM,D_PARAM,SIZE_PARAM,UNUSED_PARAM,OFS_PARAM,NOT_A_PARAM,NOT_B_PARAM,NOT_C_PARAM,NOT_D_PARAM,GATE_INV_PARAM,PARAMS_LEN
	};
	enum InputId {
    CLK_INPUT,RST_INPUT,OFS_INPUT,A_INPUT,B_INPUT,C_INPUT,D_INPUT,INPUTS_LEN
	};
	enum OutputId {
		CV_OUTPUT,TRG_OUTPUT,GATE_A_OUTPUT,GATE_B_OUTPUT,GATE_C_OUTPUT,GATE_D_OUTPUT,GATE_DIV_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		GATE_LIGHTS=18,LIGHTS_LEN=GATE_LIGHTS+4
	};
  dsp::PulseGenerator resetPulse;
  dsp::SchmittTrigger resetTrigger;
  dsp::SchmittTrigger clockTrigger;
  dsp::ClockDivider divider;
  dsp::ClockDivider paramDivider;
  dsp::PulseGenerator trigPulse;
  std::vector <std::string> labels={"OFF","ON","C1","C2","C3","C4","C5","C6","C7","C8","C9","C10","C11","C12","C13","C14","C15","C16"};
  unsigned long counter;
  bool state[16]={};
  float out=0;
  float lastOut=-1;
  RND rnd;
  P16B() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configSwitch(A_PARAM,0,17,2,"A",labels);
    getParamQuantity(A_PARAM)->snapEnabled=true;
    configSwitch(B_PARAM,0,17,3,"B",labels);
    getParamQuantity(B_PARAM)->snapEnabled=true;
    configSwitch(C_PARAM,0,17,5,"C",labels);
    getParamQuantity(C_PARAM)->snapEnabled=true;
    configSwitch(D_PARAM,0,17,9,"D",labels);
    configButton(NOT_A_PARAM,"Not A");
    configButton(NOT_B_PARAM,"Not B");
    configButton(NOT_C_PARAM,"Not C");
    configButton(NOT_D_PARAM,"Not D");
    configButton(GATE_INV_PARAM,"Gate inv");
    getParamQuantity(D_PARAM)->snapEnabled=true;
    configParam(SIZE_PARAM,4,32,16,"Size");
    configParam(OFS_PARAM,0,32,0,"Offset");
    getParamQuantity(OFS_PARAM)->snapEnabled=true;
    configOutput(CV_OUTPUT,"CV");
    configOutput(GATE_A_OUTPUT,"Gate A");
    configOutput(GATE_B_OUTPUT,"Gate_B");
    configOutput(GATE_C_OUTPUT,"Gate C");
    configOutput(GATE_D_OUTPUT,"Gate D");
    configOutput(GATE_DIV_OUTPUT,"Divider Gate");
    configInput(CLK_INPUT,"Clock");
    configInput(RST_INPUT,"Reset");
    configInput(OFS_INPUT,"Offset");
    configInput(A_INPUT,"A");
    configInput(B_INPUT,"B");
    configInput(C_INPUT,"C");
    configInput(D_INPUT,"D");
    divider.setDivision(32);
    paramDivider.setDivision(32);
	}

  void onRandomize(const RandomizeEvent &e) override {
    getParamQuantity(A_PARAM)->setValue(rnd.nextRange(0,17));
    getParamQuantity(B_PARAM)->setValue(rnd.nextRange(0,17));
    getParamQuantity(C_PARAM)->setValue(rnd.nextRange(0,17));
    getParamQuantity(D_PARAM)->setValue(rnd.nextRange(0,17));
    getParamQuantity(NOT_A_PARAM)->setValue(rnd.nextCoin()?1.f:0.f);
    getParamQuantity(NOT_B_PARAM)->setValue(rnd.nextCoin()?1.f:0.f);
    getParamQuantity(NOT_C_PARAM)->setValue(rnd.nextCoin()?1.f:0.f);
    getParamQuantity(NOT_D_PARAM)->setValue(rnd.nextCoin()?1.f:0.f);
  }

  void updateState() {
    for(unsigned k=0;k<16;k++) {
      state[k]=(counter/(k+1))%2;
    }
  }
  bool getBit(unsigned param) {
    if(param==0) return false;
    if(param==1) return true;
    return state[param-2];
  }
  uint8_t currentCVIndex() {
    uint8_t index=0;
    for(int i=0,j=1;i<4;i++,j<<=1) {
      bool nt=params[i+NOT_A_PARAM].getValue()>0;
      bool bit=getBit(params[i].getValue());
      if(nt) bit=!bit;
      index|= bit?j:0;
    }
    return index;
  }

	void process(const ProcessArgs& args) override {
    if(paramDivider.process()) {
      for(int k=0;k<4;k++) {
        if(inputs[A_INPUT+k].isConnected()) {
          float v=inputs[A_INPUT+k].getVoltage()*4;
          if(v<0) {
            getParamQuantity(NOT_A_PARAM+k)->setValue(1.0);
            v=-v;
          } else {
            getParamQuantity(NOT_A_PARAM+k)->setValue(0);
          }
          getParamQuantity(A_PARAM+k)->setValue(v);
        }
      }
    }

    if(resetTrigger.process(inputs[RST_INPUT].getVoltage())) {
      counter=params[OFS_PARAM].getValue();
      lastOut=-1;
      updateState();
      resetPulse.trigger(0.001f);
      out=currentCVIndex()*10.f/params[SIZE_PARAM].getValue();
    }
    bool rstGate=resetPulse.process(args.sampleTime);
    if(clockTrigger.process(inputs[CLK_INPUT].getVoltage())&&!rstGate) {
      counter++;
      updateState();
      out=currentCVIndex()*10.f/params[SIZE_PARAM].getValue();
    }
    if(out!=lastOut) {
      trigPulse.trigger(0.001f);
    }
    outputs[CV_OUTPUT].setVoltage(out);
    outputs[TRG_OUTPUT].setVoltage(trigPulse.process(args.sampleTime));
    if(divider.process()) {
      int gateInv=params[GATE_INV_PARAM].getValue()>0?1:0;
      for(int i=0;i<18;i++) {
        lights[i].setBrightness(getBit(i));
        if(i>1) outputs[GATE_DIV_OUTPUT].setVoltage(getBit(i)^gateInv?10.f:0.f,i-2);
      }
      for(int k=0;k<4;k++) {
        bool bit=getBit(params[A_PARAM+k].getValue());
        outputs[GATE_A_OUTPUT+k].setVoltage(bit?10.f:0.f);
        lights[GATE_LIGHTS+k].setBrightness(bit?1.f:0.f);
      }
      outputs[GATE_DIV_OUTPUT].setChannels(16);
    }
    lastOut=out;
	}
};


struct P16BWidget : ModuleWidget {
	P16BWidget(P16B* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/P16B.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    float x=2.5;
    float y=13;
    float cellHeight=2.7053;

    auto selectParam=createParam<SelectParam>(mm2px(Vec(x,y)),module,P16B::A_PARAM);
    selectParam->box.size=mm2px(Vec(6,cellHeight*18));
    selectParam->initWithEmptyLabels(18);
    addParam(selectParam);
    auto notParam=createParam<SmallButtonWithLabel>(mm2px(Vec(x-0.5,63)),module,P16B::NOT_A_PARAM);
    notParam->label="!A";
    addParam(notParam);
    x+=7;
    selectParam=createParam<SelectParam>(mm2px(Vec(x,y)),module,P16B::B_PARAM);
    selectParam->box.size=mm2px(Vec(6,cellHeight*18));
    selectParam->initWithEmptyLabels(18);
    addParam(selectParam);
    notParam=createParam<SmallButtonWithLabel>(mm2px(Vec(x-0.5,63)),module,P16B::NOT_B_PARAM);
    notParam->label="!B";
    addParam(notParam);
    x+=8;
    auto gateInvParam=createParam<SmallButtonWithLabel>(mm2px(Vec(x-0.5,63)),module,P16B::GATE_INV_PARAM);
    gateInvParam->label="!G";
    addParam(gateInvParam);
    x+=8;
    selectParam=createParam<SelectParam>(mm2px(Vec(x,y)),module,P16B::C_PARAM);
    selectParam->box.size=mm2px(Vec(6,cellHeight*18));
    selectParam->initWithEmptyLabels(18);
    addParam(selectParam);
    notParam=createParam<SmallButtonWithLabel>(mm2px(Vec(x-0.5,63)),module,P16B::NOT_C_PARAM);
    notParam->label="!C";
    addParam(notParam);
    x+=7;
    selectParam=createParam<SelectParam>(mm2px(Vec(x,y)),module,P16B::D_PARAM);
    selectParam->box.size=mm2px(Vec(6,cellHeight*18));
    selectParam->initWithEmptyLabels(18);
    addParam(selectParam);
    notParam=createParam<SmallButtonWithLabel>(mm2px(Vec(x-0.5,63)),module,P16B::NOT_D_PARAM);
    notParam->label="!D";
    addParam(notParam);
    x=23;
    y=14;
    for(int i=0;i<18;i++)
      addChild(createLightCentered<DBSmallLight<GreenLight>>(mm2px(Vec(x,y+cellHeight*i)),module,i));

    x=3;
    y=67;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,P16B::A_INPUT));
    x+=7;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,P16B::B_INPUT));
    x+=15;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,P16B::C_INPUT));
    x+=7;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,P16B::D_INPUT));

    y=80;
    x=3;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,P16B::GATE_A_OUTPUT));
    addChild(createLightCentered<DBSmallLight<RedLight>>(mm2px(Vec(x+3.1,y-2)),module,P16B::GATE_LIGHTS));
    x+=7;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,P16B::GATE_B_OUTPUT));
    addChild(createLightCentered<DBSmallLight<RedLight>>(mm2px(Vec(x+3.1,y-2)),module,P16B::GATE_LIGHTS+1));
    x+=7.5;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,P16B::GATE_DIV_OUTPUT));
    x+=7.5;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,P16B::GATE_C_OUTPUT));
    addChild(createLightCentered<DBSmallLight<RedLight>>(mm2px(Vec(x+3.1,y-2)),module,P16B::GATE_LIGHTS+2));
    x+=7;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,P16B::GATE_D_OUTPUT));
    addChild(createLightCentered<DBSmallLight<RedLight>>(mm2px(Vec(x+3.1,y-2)),module,P16B::GATE_LIGHTS+3));

    y=92;
    addInput(createInput<SmallPort>(mm2px(Vec(6.5,y+12)),module,P16B::CLK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(6.5,y+24)),module,P16B::RST_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(17,y+12)),module,P16B::OFS_PARAM));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(28,y)),module,P16B::SIZE_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(17,y+24)),module,P16B::OFS_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(28,y+12)),module,P16B::TRG_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(28,y+24)),module,P16B::CV_OUTPUT));
  }
};


Model* modelP16B = createModel<P16B, P16BWidget>("P16B");