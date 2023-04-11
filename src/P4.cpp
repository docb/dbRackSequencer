#include "plugin.hpp"


struct P4 : Module {
	enum ParamId {
		OFS_PARAM,PERM_PARAM,XY_PARAM,DIV_PARAM,PARAMS_LEN
	};
	enum InputId {
    CLK_INPUT,RST_INPUT,OFS_INPUT,PERM_INPUT,XY_INPUT,INPUTS_LEN
	};
	enum OutputId {
		CV_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

  int patterns[24][4]={
    {1,2,3,4},{1,2,4,3},{1,3,2,4},{1,3,4,2},{1,4,2,3},{1,4,3,2},
    {2,1,3,4},{2,1,4,3},{2,3,1,4},{2,3,4,1},{2,4,1,3},{2,4,3,1},
    {3,2,1,4},{3,2,4,1},{3,1,2,4},{3,1,4,2},{3,4,2,1},{3,4,1,2},
    {4,2,3,1},{4,2,1,3},{4,3,2,1},{4,3,1,2},{4,1,2,3},{4,1,3,2},
  };

  // 1234 2341 3412 4123 4321 3214 2143 1432
  // 1243 2431 4312 3124 3421 4213 2134 1342
  // 1324 3241 2413 4132 4231 2314 3142 1423
  // 4! = 4*2*3

  std::vector<std::string> labels={"1,2,3,4","1,2,4,3","1,3,2,4","1,3,4,2","1,4,2,3","1,4,3,2",
                                   "2,1,3,4","2,1,4,3","2,3,1,4","2,3,4,1","2,4,1,3","2,4,3,1",
                                   "3,2,1,4","3,2,4,1","3,1,2,4","3,1,4,2","3,4,2,1","3,4,1,2",
                                   "4,2,3,1","4,2,1,3","4,3,2,1","4,3,1,2","4,1,2,3","4,1,3,2"};
  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::PulseGenerator rstPulse;
  int stepCounter=0;
  bool resetOnOffset=true;
	P4() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configSwitch(PERM_PARAM,0,23,0,"Permutations",labels);
    configParam(OFS_PARAM,0,31,0,"Offset");
    getParamQuantity(OFS_PARAM)->snapEnabled=true;
    configParam(DIV_PARAM,2,32,16,"Pattern Size");
    getParamQuantity(DIV_PARAM)->snapEnabled=true;
    configSwitch(XY_PARAM,0,1,0,"XY",{"X","Y"});
    configInput(PERM_INPUT,"Permutations");
    configInput(OFS_INPUT,"Offset");
    configInput(XY_INPUT,"XY");
    configOutput(CV_OUTPUT,"CV");
    configInput(CLK_INPUT,"Clock");
    configInput(RST_INPUT,"Reset");
	}

	void process(const ProcessArgs& args) override {
    if(inputs[PERM_INPUT].isConnected()) {
      int c=clamp(inputs[PERM_INPUT].getVoltage(),0.f,9.99f)*2.4f;
      getParamQuantity(PERM_PARAM)->setImmediateValue(c);
    }
    bool changed=false;
    if(inputs[OFS_INPUT].isConnected()) {
      int old=params[OFS_PARAM].getValue();
      int c=clamp(inputs[OFS_INPUT].getVoltage(),0.f,9.99f)*1.6f;
      getParamQuantity(OFS_PARAM)->setImmediateValue(c);
      changed=resetOnOffset && (old!=c);
    }
    if(inputs[XY_INPUT].isConnected()) {
      int c=clamp(inputs[XY_INPUT].getVoltage(),0.f,9.99f)*0.2f;
      getParamQuantity(XY_PARAM)->setImmediateValue(c);
    }

    bool advance=false;
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())||changed) {
      stepCounter=0;
      rstPulse.trigger(0.001f);
      advance=true;
    }
    bool resetGate=rstPulse.process(args.sampleTime);
    if(clockTrigger.process(inputs[CLK_INPUT].getVoltage()) && !resetGate) {
      stepCounter++;
      if(stepCounter==4) stepCounter=0;
      advance=true;
    }
    if(advance) {
      int pat=params[PERM_PARAM].getValue();
      int div=params[DIV_PARAM].getValue();
      int index=int(patterns[pat][stepCounter]-1+params[OFS_PARAM].getValue())%div;
      bool isY=params[XY_PARAM].getValue()>0.f;
      if(isY) {
        index=((index%4)*4+index/4)%div;
      }
      outputs[CV_OUTPUT].setVoltage((float(index)/div)*10.f);
    }
	}
};


struct P4Widget : ModuleWidget {
	P4Widget(P4* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/P4.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    float xpos=1.9f;
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,MHEIGHT-115.f)),module,P4::CLK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,MHEIGHT-103.f)),module,P4::RST_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(xpos,TY(78+6))),module,P4::OFS_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,TY(71+6))),module,P4::OFS_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(xpos,TY(55+6))),module,P4::PERM_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,TY(48+6))),module,P4::PERM_INPUT));

    auto selectParam=createParam<SelectParam>(mm2px(Vec(1.9,MHEIGHT-43-6)),module,P4::XY_PARAM);
    selectParam->box.size=mm2px(Vec(6.4,7));
    selectParam->init({"X","Y"});
    addParam(selectParam);
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,TY(34))),module,P4::XY_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(xpos,TY(22))),module,P4::DIV_PARAM));
    addOutput(createOutput<SmallPort>(mm2px(Vec(xpos,TY(10))),module,P4::CV_OUTPUT));
	}
  void appendContextMenu(Menu *menu) override {
    P4 *module=dynamic_cast<P4 *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    menu->addChild(createCheckMenuItem("Reset on Offset Change", "",
                                       [=]() {return module->resetOnOffset;},
                                       [=]() { module->resetOnOffset=!module->resetOnOffset;}));
  }


};


Model* modelP4 = createModel<P4, P4Widget>("P4");