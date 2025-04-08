#include "plugin.hpp"
#define MAX_PATS 100

struct P16A : Module {
	enum ParamId {
		ADDR_PARAMS,RND_PARAM=ADDR_PARAMS+16,LENGTH_PARAM,OFS_PARAM,SIZE_PARAM,PAT_PARAM,COPY_PARAM,PASTE_PARAM,EDIT_PARAM,HOLD_PARAMS,REVERSE_PARAM=HOLD_PARAMS+16,PARAMS_LEN
	};
	enum InputId {
		CLK_INPUT,UNUSED_INPUT,RESET_INPUT,PAT_INPUT,RND_INPUT,OFS_INPUT,INPUTS_LEN
	};
	enum OutputId {
		CV_OUTPUT,POLY_CV_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN=16
	};

  RND rnd;

  const int initPatterns[MAX_PATS][16] = {
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
  int patterns[MAX_PATS][16]={};
  int cb[16]={};
  int stepCounter=0;
  bool divBy10=false;
  int rndMin=0;
  int rndMax=15;
  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::PulseGenerator rstPulse;
  dsp::SchmittTrigger rndTrigger;
  dsp::SchmittTrigger manualRndTrigger;
  dsp::ClockDivider paramDivider;
  dsp::ClockDivider lightDivider;

	P16A() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    for(int k=0;k<16;k++) {
      configParam(ADDR_PARAMS+k,0,15,k,std::to_string(k+1));
      configButton(HOLD_PARAMS+k,"Hold " + std::to_string(k+1));
    }
    configParam(OFS_PARAM,0,15,0,"Offset");
    configButton(RND_PARAM,"Randomize Pattern");
    configButton(COPY_PARAM,"Copy Pattern");
    configButton(PASTE_PARAM,"Paste Pattern");
    configButton(REVERSE_PARAM,"Reverse Pattern");
    configButton(EDIT_PARAM,"LOCK");
    getParamQuantity(OFS_PARAM)->snapEnabled=true;
    configParam(LENGTH_PARAM,2,16,16,"Length");
    getParamQuantity(LENGTH_PARAM)->snapEnabled=true;
    configParam(SIZE_PARAM,2,32,16,"Pattern Size");
    getParamQuantity(SIZE_PARAM)->snapEnabled=true;
    configInput(CLK_INPUT,"Step +");
    configInput(RESET_INPUT,"Reset");
    configInput(PAT_INPUT,"Pattern Select");
    configInput(OFS_INPUT,"Offset");
    configInput(RND_INPUT,"Randomize Pattern");
    configOutput(CV_OUTPUT,"CV");
    configOutput(POLY_CV_OUTPUT,"Poly CV (static)");

    configParam(PAT_PARAM,0,MAX_PATS-1,0,"Pattern Selection");
    getParamQuantity(PAT_PARAM)->snapEnabled=true;
    init();
    paramDivider.setDivision(32);
    lightDivider.setDivision(32);
	}

  void init() {
    for(int k=0;k<MAX_PATS;k++) {
      for(int j=0;j<16;j++) {
        patterns[k][j]=initPatterns[k][j];
      }
    }
  }

  void onRandomize(const RandomizeEvent &e) override {
    for(int k=0;k<MAX_PATS;k++) {
      for(int j=0;j<16;j++) {
        patterns[k][j]=rnd.nextRange(rndMin,rndMax);
      }
    }
    setCurrentPattern();
  }

  void onReset(const ResetEvent &e) override {
    init();
    setCurrentPattern();
    Module::onReset(e);
  }

  json_t *dataToJson() override {
    json_t *data=json_object();
    json_t *patternList=json_array();
    for(int k=0;k<MAX_PATS;k++) {
      json_t *onList=json_array();
      for(int j=0;j<PORT_MAX_CHANNELS;j++) {
        json_array_append_new(onList,json_integer(patterns[k][j]));
      }
      json_array_append_new(patternList,onList);
    }
    json_object_set_new(data,"patterns",patternList);
    json_object_set_new(data,"rndMin",json_integer(rndMin));
    json_object_set_new(data,"rndMax",json_integer(rndMax));
    json_object_set_new(data,"divBy10",json_boolean(divBy10));
    return data;
  }

  void dataFromJson(json_t *rootJ) override {
    json_t *data=json_object_get(rootJ,"patterns");
    if(!data)
      return;
    for(int k=0;k<MAX_PATS;k++) {
      json_t *arr=json_array_get(data,k);
      if(arr) {
        for(int j=0;j<PORT_MAX_CHANNELS;j++) {
          json_t *pattern=json_array_get(arr,j);
          patterns[k][j]=json_integer_value(pattern);
        }
      }
    }
    json_t *jRndMin=json_object_get(rootJ,"rndMin");
    if(jRndMin) {
      rndMin=json_integer_value(jRndMin);
    }
    json_t *jRndMax=json_object_get(rootJ,"rndMax");
    if(jRndMax) {
      rndMax=json_integer_value(jRndMax);
    }
    json_t *jDivBy10=json_object_get(rootJ,"divBy10");
    if(jDivBy10) {
      divBy10=json_boolean_value(jDivBy10);
    }
  }

  void setCurrentPattern() {
    int pat=params[PAT_PARAM].getValue();
    for(int k=0;k<16;k++) {
      getParamQuantity(ADDR_PARAMS+k)->setValue(patterns[pat][k]);
    }
  }

  void onAdd(const AddEvent &e) override {
    setCurrentPattern();
  }

  void setValue(int nr,int value) {
    int pat=params[PAT_PARAM].getValue();
    patterns[pat][nr]=value;
  }

  void insert() {
    int pat=params[PAT_PARAM].getValue();
    for(int k=99;k>pat;k--) {
      for(int j=0;j<16;j++) {
        patterns[k][j]=patterns[k-1][j];
      }
    }
    for(int j=0;j<16;j++) {
      patterns[pat][j]=0;
    }
    setCurrentPattern();
  }

  void del() {
    int pat=params[PAT_PARAM].getValue();
    for(int k=pat;k<99;k++) {
      for(int j=0;j<16;j++) {
        patterns[k][j]=patterns[k+1][j];
      }
    }
    setCurrentPattern();
  }

  void copy() {
    int pat=params[PAT_PARAM].getValue();
    for(int k=0;k<16;k++) {
      cb[k]=patterns[pat][k];
    }
  }

  void paste() {
    int pat=params[PAT_PARAM].getValue();
    for(int k=0;k<16;k++) {
      patterns[pat][k]=cb[k];
    }
    setCurrentPattern();
  }

  void randomize() {
    int pat=params[PAT_PARAM].getValue();
    for(int k=0;k<16;k++) {
      if(params[HOLD_PARAMS+k].getValue()==0.f)
        patterns[pat][k]=rnd.nextRange(rndMin,rndMax);
    }
    setCurrentPattern();
  }
  void reverse() {
    int pat=params[PAT_PARAM].getValue();
    copy();
    for(int k=0;k<16;k++) {
      patterns[pat][k]=cb[15-k];
    }
    setCurrentPattern();
  }

	void process(const ProcessArgs& args) override {
    if(paramDivider.process()) {
      if(inputs[PAT_INPUT].isConnected()&&params[EDIT_PARAM].getValue()==0) {
        int c=clamp(inputs[PAT_INPUT].getVoltage(),0.f,9.99f)*float(MAX_PATS)/10.f;
        getParamQuantity(PAT_PARAM)->setValue(c);
      }
      if(inputs[OFS_INPUT].isConnected()) {
        int c=inputs[OFS_INPUT].getVoltage()*1.6f;
        getParamQuantity(OFS_PARAM)->setValue(c);
      }
    }
    int pat=params[PAT_PARAM].getValue();
    int len=params[LENGTH_PARAM].getValue();
    int ofs=int(params[OFS_PARAM].getValue());
    if(rndTrigger.process(inputs[RND_INPUT].getVoltage())|manualRndTrigger.process(params[RND_PARAM].getValue())) {
      randomize();
    }
    if(rstTrigger.process(inputs[RESET_INPUT].getVoltage())) {
      stepCounter=0;
      rstPulse.trigger(0.001f);
    }
    bool resetGate=rstPulse.process(args.sampleTime);
    if(clockTrigger.process(inputs[CLK_INPUT].getVoltage())&&!resetGate) {
      stepCounter=(stepCounter+1)%len;
      if(stepCounter==16) stepCounter=0;
    }
    int pos=(ofs+stepCounter)%16;
    outputs[CV_OUTPUT].setVoltage(float(patterns[pat][pos]%16)*(divBy10?1.f:10.000001f)/params[SIZE_PARAM].getValue());

    if(lightDivider.process()) {
      for(int k=0;k<16;k++) {
        lights[k].setBrightness(k==pos?1.f:0.f);
        outputs[POLY_CV_OUTPUT].setVoltage(float(patterns[pat][k]%16)*(divBy10?1.f:10.f)/params[SIZE_PARAM].getValue(),k);
      }
      outputs[POLY_CV_OUTPUT].setChannels(len);
    }
	}
};
template<typename P>
struct PSwitchButton : OpaqueWidget {
  P16A *module;
  int nr;
  int value;
  PSwitchButton(P16A *_module,int _nr,int _value) : module(_module),nr(_nr),value(_value) {}
  void draw(const DrawArgs &args) override {
    int currentValue=0;
    //int lastValue=0;
    auto paramWidget=getAncestorOfType<ParamWidget>();
    assert(paramWidget);
    engine::ParamQuantity *pq=paramWidget->getParamQuantity();
    if(pq) {
      currentValue=(int)floor(pq->getValue());
    }
    if(currentValue==value) {
      nvgFillColor(args.vg,nvgRGB(0x7e,0xa6,0xd3));
    } else {
      nvgFillColor(args.vg,nvgRGB(0x3c,0x4c,0x71));
    }
    nvgStrokeColor(args.vg,nvgRGB(0xc4,0xc9,0xc2));
    nvgBeginPath(args.vg);
    nvgEllipse(args.vg,box.size.x/2,box.size.y/2,box.size.x/2,box.size.x/2);
    nvgFill(args.vg);
    nvgStroke(args.vg);
  }

  void onDragHover(const event::DragHover &e) override {
    if(e.button==GLFW_MOUSE_BUTTON_LEFT) {
      e.consume(this);
    }
    Widget::onDragHover(e);
  }

  void onDragEnter(const event::DragEnter &e) override {
    if(e.button==GLFW_MOUSE_BUTTON_LEFT) {
      auto origin=dynamic_cast<P *>(e.origin);
      if(origin) {
        auto paramWidget=getAncestorOfType<ParamWidget>();
        assert(paramWidget);
        engine::ParamQuantity *pq=paramWidget->getParamQuantity();
        if(pq) {
          pq->setValue(value);
          module->setValue(nr,value);
        }
      }
    }
    Widget::onDragEnter(e);
  };

};

struct AddrParam : ParamWidget {
  P16A *module=nullptr;
  int nr=0;
  void init(int _nr) {
    nr=_nr;
    const float margin=0;
    float height=box.size.y-2*margin;
    for(unsigned int i=0;i<16;i++) {
      auto selectButton=new PSwitchButton<AddrParam>(module,nr,i);
      selectButton->box.pos=Vec(0,(height/16)*(15-i)+margin);
      selectButton->box.size=Vec(box.size.x,height/16);
      addChild(selectButton);
    }
  }

  void draw(const DrawArgs &args) override {
    // Background
    nvgBeginPath(args.vg);
    nvgRect(args.vg,0,0,box.size.x,box.size.y);
    nvgFillColor(args.vg,nvgRGB(0,0,0));
    nvgFill(args.vg);

    ParamWidget::draw(args);
  }
  void onChange(const ChangeEvent &e) override {
    if(module) {
      //module->setValue(nr,getParamQuantity()->getValue());
    }
    ParamWidget::onChange(e);
  }
};
struct P16APatternSelect : SpinParamWidget {
  P16A *module=nullptr;
  P16APatternSelect() {
    init();
  }
  void onChange(const ChangeEvent &e) override {
    if(module) {
      module->setCurrentPattern();
    }
  }
};

template<typename M>
struct ReverseButton : SmallButtonWithLabel {
  P16A *module;

  ReverseButton() : SmallButtonWithLabel() {
    momentary=true;
  }

  void onChange(const ChangeEvent &e) override {
    SvgSwitch::onChange(e);
    if(module) {
      if(module->params[P16A::REVERSE_PARAM].getValue()>0)
        module->reverse();
    }
  }
};

struct P16AWidget : ModuleWidget {
	P16AWidget(P16A* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/P16A.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    for(int k=0;k<16;k++) {
      auto addrParam=createParam<AddrParam>(mm2px(Vec(8.5+4.5*k,11)),module,P16A::ADDR_PARAMS+k);
      addrParam->box.size=mm2px(Vec(3.4,72));
      addrParam->module=module;
      addrParam->init(k);
      addParam(addrParam);
      addChild(createLightCentered<DBMediumLight<GreenLight>>(mm2px(Vec(10+4.5*k,85)),module,k));
      addParam(createParam<SmallRoundButton>(mm2px(Vec(8.5+4.5*k,89)),module,P16A::HOLD_PARAMS+k));
    }
    float x=18;
    auto patParam=createParam<P16APatternSelect>(mm2px(Vec(x,100.5)),module,P16A::PAT_PARAM);
    patParam->module=module;
    addParam(patParam);
    addInput(createInput<SmallPort>(mm2px(Vec(x,114)),module,P16A::PAT_INPUT));
    auto copyButton=createParam<CopyButton<P16A>>(mm2px(Vec(x+8,100)),module,P16A::COPY_PARAM);
    copyButton->label="Cpy";
    copyButton->module=module;
    addParam(copyButton);
    auto pasteButton=createParam<PasteButton<P16A>>(mm2px(Vec(x+8,104)),module,P16A::PASTE_PARAM);
    pasteButton->label="Pst";
    pasteButton->module=module;
    addParam(pasteButton);
    auto reverseButton=createParam<ReverseButton<P16A>>(mm2px(Vec(x+8,108)),module,P16A::REVERSE_PARAM);
    reverseButton->label="Rev";
    reverseButton->module=module;
    addParam(reverseButton);
    auto editButton=createParam<SmallButtonWithLabel>(mm2px(Vec(x+8,116)),module,P16A::EDIT_PARAM);
    editButton->setLabel("Lock");
    addParam(editButton);

    addInput(createInput<SmallPort>(mm2px(Vec(6,100)),module,P16A::CLK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(6,114)),module,P16A::RESET_INPUT));
    addParam(createParam<MLEDM>(mm2px(Vec(42,100)),module,P16A::RND_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(50,100)),module,P16A::RND_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(42,114)),module,P16A::OFS_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(50,114)),module,P16A::OFS_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(62,100)),module,P16A::SIZE_PARAM));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(62,114)),module,P16A::LENGTH_PARAM));

    addOutput(createOutput<SmallPort>(mm2px(Vec(74,100)),module,P16A::POLY_CV_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(74,114)),module,P16A::CV_OUTPUT));
	}

  void appendContextMenu(Menu *menu) override {
    P16A *module=dynamic_cast<P16A *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    menu->addChild(createCheckMenuItem("DivBy10","",[=]() {
      return module->divBy10;
    },[=]() {
      module->divBy10=!module->divBy10;
    }));
    struct InsertItem : ui::MenuItem {
      P16A *module;

      InsertItem(P16A *m) : module(m) {
      }

      void onAction(const ActionEvent &e) override {
        if(!module)
          return;
        module->insert();
      }
    };
    auto insertMenu=new InsertItem(module);
    insertMenu->text="Insert Pattern";
    menu->addChild(insertMenu);
    struct DelItem : ui::MenuItem {
      P16A *module;

      DelItem(P16A *m) : module(m) {
      }

      void onAction(const ActionEvent &e) override {
        if(!module)
          return;
        module->del();
      }
    };
    auto delMenu=new DelItem(module);
    delMenu->text="Delete Pattern";
    menu->addChild(delMenu);

    auto rndMinSelect=new IntSelectItem(&module->rndMin,0,15);
    rndMinSelect->text="Rnd Min";
    rndMinSelect->rightText=string::f("%d",module->rndMin)+"  "+RIGHT_ARROW;
    menu->addChild(rndMinSelect);

    auto rndMaxSelect=new IntSelectItem(&module->rndMax,0,15);
    rndMaxSelect->text="Rnd Max";
    rndMaxSelect->rightText=string::f("%d",module->rndMax)+"  "+RIGHT_ARROW;
    menu->addChild(rndMaxSelect);

  }
};


Model* modelP16A = createModel<P16A, P16AWidget>("P16A");