#include "plugin.hpp"
#define MAX_PATS 100

struct P16 : Module {
	enum ParamId {
		PAT_PARAM,OFS_PARAM,DIR_PARAM,DIV_PARAM,PARAMS_LEN
	};
	enum InputId {
		CLK_INPUT,RST_INPUT,PAT_CV_INPUT,OFS_INPUT,DIR_INPUT,INPUTS_LEN
	};
	enum OutputId {
		CV_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
  int paths[MAX_PATS][16] = {
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
    {0,1,2,3,4,5,6,7, 15,14,13,12,11,10,9,8},
    {0,8,1,9,2,10,3,11, 4,12,5,13,6,14,7,15},
    {0,1,9,10,2,3,11, 12,4,5,13,14,6,7,15,8},
    {0,9,1,10,2,11,3, 12,4,13,5,14,6,15,7,8},
    {0,7,1,6,2,5,3,4, 8,15,9,14,10,13,11,12},
    {0,4,8,12,13,14,15,11 ,7,3,2,1,5,9,10,6},// spiral
    {0,5,10,15,4,9,14,8,13,12,3,2,7,1,6,11},// md
    {0,4,1,5,2,6,3,7,11,15,8,12,9,13,10,14},// zigzag8
    {0,7,9,14,1,6,10,13,8,15,2,5,11,12,3,4},
    {0,7,3,4,9,14,10,13,1,6,11,12,2,5,8,15},

    {0,3,1,2, 4,7,5,6, 8,11,9,10, 12,15,13,14},//4
    {1,0,2,3, 5,4,6,7 ,9,8,10,11, 13,12,14,15},//4
    {2,0,1,3, 6,4,5,7, 10,8,9,11, 14,12,13,15},//4
    {0,2,1,3 ,4,6,5,7, 8,10,9,11, 12,14,13,15},//4
    {1,2,0,3, 5,6,4,7, 9,10,8,11, 13,14,12,15},//4
    {2,1,0,3, 6,5,4,7, 10,9,8,11, 14,13,12,15},//4
    {3,1,2,0, 7,5,6,4, 11,9,10,8, 15,13,14,12},//4
    {1,3,2,0, 5,7,6,4, 9,11,10,8, 13,15,14,12},
    {2,3,1,0, 6,7,5,4, 10,11,9,8, 14,15,13,12},
    {3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12},
    {1,2,3,0, 5,6,7,4, 9,10,11,8, 13,14,15,12},
    {2,1,3,0, 6,5,7,4, 10,9,11,8, 14,13,15,12},
    {3,0,2,1, 7,4,6,5, 11,8,10,9, 15,12,14,13},
    {0,3,2,1, 4,7,6,5,8, 11,10,9, 12,15,14,13},
    {2,3,0,1, 6,7,4,5, 10,11,8,9, 14,15,12,13},
    {3,2,0,1, 7,6,4,5, 11,10,8,9, 15,14,12,13},
    {0,2,3,1, 4,6,7,5, 8,10,11,9, 12,14,15,13},
    {2,0,3,1, 6,4,7,5, 10,8,11,9, 14,12,15,13},
    {3,0,1,2, 7,4,5,6, 11,8,9,10, 15,12,13,14},
    {0,3,1,2, 4,7,5,6, 8,11,9,10, 12,15,13,14},
    {1,3,0,2, 5,7,4,6, 9,11,8,10, 13,15,12,14},
    {3,1,0,2, 7,5,4,6, 11,9,8,10, 15,13,12,14},
    {0,1,3,2, 4,5,7,6, 8,9,11,10, 12,13,15,14},
    {1,0,3,2, 5,4,7,6, 9,8,11,10, 13,12,15,14},

    {13,12,14,15, 1,0,2,3, 9,8,10,11, 5,4,6,7 },
    {14,12,13,15, 2,0,1,3, 10,8,9,11, 6,4,5,7 },
    {12,14,13,15, 0,2,1,3, 8,10,9,11, 4,6,5,7 },
    {13,14,12,15, 1,2,0,3, 9,10,8,11, 5,6,4,7 },
    {14,13,12,15, 2,1,0,3, 10,9,8,11, 6,5,4,7 },
    {15,13,14,12, 3,1,2,0, 11,9,10,8, 7,5,6,4 },
    {13,15,14,12, 1,3,2,0, 9,11,10,8, 5,7,6,4 },
    {14,15,13,12, 2,3,1,0, 10,11,9,8, 6,7,5,4 },
    {15,14,13,12, 3,2,1,0, 11,10,9,8, 7,6,5,4 },
    {13,14,15,12, 1,2,3,0, 9,10,11,8, 5,6,7,4 },
    {14,13,15,12, 2,1,3,0, 10,9,11,8, 6,5,7,4 },
    {15,12,14,13, 3,0,2,1, 11,8,10,9, 7,4,6,5 },
    {12,15,14,13, 0,3,2,1, 8,11,10,9, 4,7,6,5 },
    {14,15,12,13, 2,3,0,1, 10,11,8,9, 6,7,4,5 },
    {15,14,12,13, 3,2,0,1, 11,10,8,9, 7,6,4,5 },
    {12,14,15,13, 0,2,3,1, 8,10,11,9, 4,6,7,5 },
    {14,12,15,13, 2,0,3,1, 10,8,11,9, 6,4,7,5 },
    {15,12,13,14, 3,0,1,2, 11,8,9,10, 7,4,5,6 },
    {12,15,13,14, 0,3,1,2, 8,11,9,10, 4,7,5,6 },
    {13,15,12,14, 1,3,0,2, 9,11,8,10, 5,7,4,6 },
    {15,13,12,14, 3,1,0,2, 11,9,8,10, 7,5,4,6 },
    {12,13,15,14, 0,1,3,2, 8,9,11,10, 4,5,7,6 },
    {13,12,15,14, 1,0,3,2, 9,8,11,10, 5,4,7,6 },

    {9,8,10,11, 13,12,14,15, 1,0,2,3, 5,4,6,7 },
    {10,8,9,11, 14,12,13,15, 2,0,1,3, 6,4,5,7 },
    {8,10,9,11, 12,14,13,15, 0,2,1,3, 4,6,5,7 },
    {9,10,8,11, 13,14,12,15, 1,2,0,3, 5,6,4,7 },
    {10,9,8,11, 14,13,12,15, 2,1,0,3, 6,5,4,7 },
    {11,9,10,8, 15,13,14,12, 3,1,2,0, 7,5,6,4 },
    {9,11,10,8, 13,15,14,12, 1,3,2,0, 5,7,6,4 },
    {10,11,9,8, 14,15,13,12, 2,3,1,0, 6,7,5,4 },
    {11,10,9,8, 15,14,13,12, 3,2,1,0, 7,6,5,4 },
    {9,10,11,8, 13,14,15,12, 1,2,3,0, 5,6,7,4 },
    {10,9,11,8, 14,13,15,12, 2,1,3,0, 6,5,7,4 },
    {11,8,10,9, 15,12,14,13, 3,0,2,1, 7,4,6,5 },
    {8,11,10,9, 12,15,14,13, 0,3,2,1, 4,7,6,5 },
    {10,11,8,9, 14,15,12,13, 2,3,0,1, 6,7,4,5 },
    {11,10,8,9, 15,14,12,13, 3,2,0,1, 7,6,4,5 },
    {8,10,11,9, 12,14,15,13, 0,2,3,1, 4,6,7,5 },
    {10,8,11,9, 14,12,15,13, 2,0,3,1, 6,4,7,5 },
    {11,8,9,10, 15,12,13,14, 3,0,1,2, 7,4,5,6 },
    {8,11,9,10, 12,15,13,14, 0,3,1,2, 4,7,5,6 },
    {9,11,8,10, 13,15,12,14, 1,3,0,2, 5,7,4,6 },
    {11,9,8,10, 15,13,12,14, 3,1,0,2, 7,5,4,6 },
    {8,9,11,10, 12,13,15,14, 0,1,3,2, 4,5,7,6 },
    {9,8,11,10, 13,12,15,14, 1,0,3,2, 5,4,7,6 },

    {5,4,6,7,9,8,10,11, 13,12,14,15, 1,0,2,3, },
    {6,4,5,7,10,8,9,11, 14,12,13,15, 2,0,1,3,  },
    {4,6,5,7,8,10,9,11, 12,14,13,15, 0,2,1,3,  },
    {5,6,4,7,9,10,8,11, 13,14,12,15, 1,2,0,3,  },
    {6,5,4,7,10,9,8,11, 14,13,12,15, 2,1,0,3,  },
    {7,5,6,4,11,9,10,8, 15,13,14,12, 3,1,2,0,  },
    {5,7,6,4,9,11,10,8, 13,15,14,12, 1,3,2,0,  },
    {6,7,5,4,10,11,9,8, 14,15,13,12, 2,3,1,0,  },
    {7,6,5,4,11,10,9,8, 15,14,13,12, 3,2,1,0,  },
    {5,6,7,4,9,10,11,8, 13,14,15,12, 1,2,3,0,  },
    {6,5,7,4,10,9,11,8, 14,13,15,12, 2,1,3,0,  },
    {7,4,6,5,11,8,10,9, 15,12,14,13, 3,0,2,1,  },
    {4,7,6,5,8,11,10,9, 12,15,14,13, 0,3,2,1,  },
    {6,7,4,5,10,11,8,9, 14,15,12,13, 2,3,0,1,  },
    {7,6,4,5,11,10,8,9, 15,14,12,13, 3,2,0,1,  },
    {4,6,7,5,8,10,11,9, 12,14,15,13, 0,2,3,1,  },
    {6,4,7,5,10,8,11,9, 14,12,15,13, 2,0,3,1,  },
    {0,1,2,3,4,5,6,7, 15,8,9,10,11,12,13,14},//?
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}
  };
  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::PulseGenerator rstPulse;
  dsp::ClockDivider divider;
  int stepCounter=0;
	P16() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configParam(PAT_PARAM,0,MAX_PATS-1,0,"Pattern Selection");
    configParam(OFS_PARAM,0,15,0,"Offset");
    configSwitch(DIR_PARAM,0,1,0,"Direction",{"-->","<--"});
    getParamQuantity(OFS_PARAM)->snapEnabled=true;
    configParam(DIV_PARAM,2,32,16,"Pattern Size");
    getParamQuantity(DIV_PARAM)->snapEnabled=true;
    configInput(CLK_INPUT,"Clock");
    configInput(DIR_INPUT,"Direction");
    configInput(RST_INPUT,"Reset");
    configInput(OFS_INPUT,"Offset");
    configInput(PAT_CV_INPUT,"Pattern Selection");
    configOutput(CV_OUTPUT,"CV");
    divider.setDivision(32);
	}

	void process(const ProcessArgs& args) override {
    bool advance=false;
    bool changed=false;
    if(divider.process()) {
      if(inputs[PAT_CV_INPUT].isConnected()) {
        int c=clamp(inputs[PAT_CV_INPUT].getVoltage(),0.f,9.99f)*float(MAX_PATS)/10.f;
        getParamQuantity(PAT_PARAM)->setValue(c);
      }
      if(inputs[OFS_INPUT].isConnected()) {
        int old=params[OFS_PARAM].getValue();
        int c=clamp(inputs[OFS_INPUT].getVoltage(),0.f,9.99f)*1.6f;
        getParamQuantity(OFS_PARAM)->setValue(c);
        changed=old!=c;
      }
      if(inputs[DIR_INPUT].isConnected()) {
        getParamQuantity(DIR_PARAM)->setValue(inputs[DIR_INPUT].getVoltage()>5.f?1:0);
      }
    }
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage()) || changed) {
      stepCounter=0;
      rstPulse.trigger(0.001f);
      advance=true;
    }
    int size=params[DIV_PARAM].getValue();
    bool resetGate=rstPulse.process(args.sampleTime);
    if(clockTrigger.process(inputs[CLK_INPUT].getVoltage()) && !resetGate) {
      int direction=params[DIR_PARAM].getValue();
      if(direction) {
        stepCounter--;
        if(stepCounter<0) stepCounter=15;
      } else {
        stepCounter++;
        if(stepCounter==16)
          stepCounter=0;
      }
      advance=true;
    }
    if(advance) {
      int pat=params[PAT_PARAM].getValue();
      outputs[CV_OUTPUT].setVoltage(float((paths[pat][stepCounter]+int(params[OFS_PARAM].getValue()))%size)*(10.f/float(size)));
    }
	}
};
//template<typename M>
struct P16PatternSelect : SpinParamWidget {
  //M *module;
  P16PatternSelect() {
    init();
  }
  //void onChange(const event::Change &e) override {
    //if(module)
  //    module->switchPattern();
  //}
};

struct P16Widget : ModuleWidget {
	P16Widget(P16* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/P16.svg")));

    float xpos=1.9f;
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,9)),module,P16::CLK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,21)),module,P16::RST_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(xpos,33)),module,P16::OFS_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,41)),module,P16::OFS_INPUT));

    addParam(createParam<P16PatternSelect>(mm2px(Vec(xpos,55)),module,P16::PAT_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,66)),module,P16::PAT_CV_INPUT));

    auto selectParam=createParam<SelectParam>(mm2px(Vec(1.9,81)),module,P16::DIR_PARAM);
    selectParam->box.size=mm2px(Vec(6.4,7));
    selectParam->init({"-->","<--"});
    addParam(selectParam);
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,90)),module,P16::DIR_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(xpos,104)),module,P16::DIV_PARAM));
    addOutput(createOutput<SmallPort>(mm2px(Vec(xpos,116)),module,P16::CV_OUTPUT));
  }
};


Model* modelP16 = createModel<P16, P16Widget>("P16");