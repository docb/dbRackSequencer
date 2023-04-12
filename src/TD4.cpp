#include "plugin.hpp"


struct TD4 : Module {
	enum ParamId {
		PARAMS_LEN=16
	};
	enum InputId {
		CV_INPUT,GATE_INPUT=CV_INPUT+16,POLY_CV_INPUT=GATE_INPUT+16,POLY_ADDR_INPUT,INPUTS_LEN
	};
	enum OutputId {
		CV_OUTPUT,SINGLE_GATE_OUTPUT=CV_OUTPUT+16,GATE_OUTPUT=SINGLE_GATE_OUTPUT+16,POLY_CV_OUTPUT=GATE_OUTPUT+16,POLY_GATE_OUTPUT,POLY_CV_STATIC_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN=256
	};

  float min=-2;
  float max=2;
  bool quantize=false;
  int dirty=0;
  dsp::ClockDivider divider;
	TD4() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    for(int k=0;k<16;k++) {
      configParam(k,min,max,0,"CV " + std::to_string(k+1));
      configInput(CV_INPUT+k,"CV Addr Track" + std::to_string(k+1));
      configInput(GATE_INPUT+k,"Gate Track "+ std::to_string(k+1));
      configOutput(GATE_OUTPUT+k,"Step Gate Track "+ std::to_string(k+1));
      configOutput(SINGLE_GATE_OUTPUT+k,"Single Step Gate "+ std::to_string(k+1));
      configOutput(CV_OUTPUT+k,"CV Track "+ std::to_string(k+1));
      for(int j=0;j<16;j++) {
        configLight(k*16+j,"Step " + std::to_string(k+1) + " Input " + std::to_string(j+1));
      }
    }
    configInput(POLY_CV_INPUT,"Poly CV");
    configInput(POLY_ADDR_INPUT,"Poly Address CV");
    configOutput(POLY_CV_STATIC_OUTPUT,"Poly Static CV ");
    configOutput(POLY_CV_OUTPUT,"Poly CV ");
    configOutput(POLY_GATE_OUTPUT,"Poly Gate ");
    divider.setDivision(32);
	}

  float getCVParam(int k) {
    float a=params[k].getValue();
    if(quantize) {
      a=roundf(a*12.f)/12.f;
    }
    return a;
  }
  void process(const ProcessArgs &args) override {
    if(divider.process()) {
      _process(args);
    }
  }
	void _process(const ProcessArgs& args) {
    if(inputs[POLY_CV_INPUT].isConnected()) {
      for(int k=0;k<16;k++) {
        setImmediateValue(getParamQuantity(CV_INPUT+k),inputs[POLY_CV_INPUT].getVoltage(k));
      }
    }
    int channels=0;
    for(int k=0;k<16;k++) {
      outputs[GATE_OUTPUT+k].setVoltage(0.f);
      for(int j=0;j<16;j++) {
        lights[j*16+k].setBrightness(0.f);
        outputs[SINGLE_GATE_OUTPUT+j].setVoltage(0.f,k);
        outputs[POLY_CV_STATIC_OUTPUT].setVoltage(getCVParam(k),k);
      }
      int index=-1;
      if(inputs[POLY_ADDR_INPUT].isConnected()) {
        channels = inputs[POLY_ADDR_INPUT].getChannels();
        if(k<channels) {
          index=int(inputs[POLY_ADDR_INPUT].getVoltage(k)*1.6f);
          while(index<0)
            index+=16;
          index%=16;
        }
      } else if(inputs[CV_INPUT+k].isConnected()) {
        index=int(inputs[CV_INPUT+k].getVoltage()*1.6f);
        while(index<0)
          index+=16;
        index%=16;
        channels=k+1;
      }

      if(index>=0) {
        float a=getCVParam(index);
        outputs[CV_OUTPUT+k].setVoltage(a);
        outputs[POLY_CV_OUTPUT].setVoltage(a,k);
        bool on = true;
        if(inputs[GATE_INPUT+k].isConnected()) {
          on=inputs[GATE_INPUT+k].getVoltage(index)>1.f;
        }
        lights[index*16+k].setBrightness(on?1.f:0.f);
        outputs[SINGLE_GATE_OUTPUT+index].setVoltage(10.f,k);
        outputs[GATE_OUTPUT+k].setVoltage(on?10.f:0.f);
        outputs[POLY_GATE_OUTPUT].setVoltage(on?10.f:0.f,k);

      }
    }
    outputs[SINGLE_GATE_OUTPUT].setChannels(16);
    outputs[POLY_CV_STATIC_OUTPUT].setChannels(16);
    outputs[POLY_GATE_OUTPUT].setChannels(channels);
    outputs[POLY_CV_OUTPUT].setChannels(channels);
	}

  void reconfig() {
    for(int nr=0;nr<16;nr++) {
      float value=getParamQuantity(nr)->getImmediateValue();
      if(value>max)
        value=max;
      if(value<min)
        value=min;
      configParam(nr,min,max,0,"CV "+std::to_string(nr+1));
      setImmediateValue(getParamQuantity(nr),value);
      dirty=16;
    }
  }
  void fromJson(json_t *root) override {
    min=-10.f;
    max=10.f;
    reconfig();
    Module::fromJson(root);
  }
  json_t* dataToJson() override {
    json_t *root=json_object();
    json_object_set_new(root,"min",json_real(min));
    json_object_set_new(root,"max",json_real(max));
    json_object_set_new(root,"quantize",json_integer(quantize));
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
    reconfig();
  }
};
using MLIGHT1 = TLight<GrayModuleLightWidget,255,0,0>;
using MLIGHT2 = TLight<GrayModuleLightWidget,0,255,0>;
using MLIGHT3 = TLight<GrayModuleLightWidget,55,55,255>;
using MLIGHT4 = TLight<GrayModuleLightWidget,255,255,0>;
using MLIGHT5 = TLight<GrayModuleLightWidget,255,0,255>;
using MLIGHT6 = TLight<GrayModuleLightWidget,0,255,255>;
using MLIGHT7 = TLight<GrayModuleLightWidget,128,0,0>;
using MLIGHT8 = TLight<GrayModuleLightWidget,196,85,55>;
using MLIGHT9 = TLight<GrayModuleLightWidget,128,128,80>;
using MLIGHT10 = TLight<GrayModuleLightWidget,255,128,0>;
using MLIGHT11 = TLight<GrayModuleLightWidget,255,0,128>;
using MLIGHT12 = TLight<GrayModuleLightWidget,0,128,255>;
using MLIGHT13 = TLight<GrayModuleLightWidget,128,66,128>;
using MLIGHT14 = TLight<GrayModuleLightWidget,128,255,0>;
using MLIGHT15 = TLight<GrayModuleLightWidget,128,128,255>;
using MLIGHT16 = TLight<GrayModuleLightWidget,128,255,255>;

struct TD4Widget : ModuleWidget {
	TD4Widget(TD4* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/TD4.svg")));


    float x=5;
    float y=TY(120);
    for(int k=0;k<16;k++) {
      addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,TD4::CV_INPUT+k));
      x+=8;
      if(k==7) {
        y+=7;x=5;
      }
    }
    x=5;
    y=TY(104);
    for(int k=0;k<16;k++) {
      auto param=createParam<MKnob<TD4>>(mm2px(Vec(x,y)),module,k);
      param->module=module;
      addParam(param);

        int j=0;
        int l=0;
        addChild(createLight<SmallSimpleLight<MLIGHT1>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));
        l++;
        addChild(createLight<SmallSimpleLight<MLIGHT2>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));
        l++;
        addChild(createLight<SmallSimpleLight<MLIGHT3>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));
        l++;
        addChild(createLight<SmallSimpleLight<MLIGHT4>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));
        l++;
        addChild(createLight<SmallSimpleLight<MLIGHT5>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));
        l++;
        addChild(createLight<SmallSimpleLight<MLIGHT6>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));
        l++;
        addChild(createLight<SmallSimpleLight<MLIGHT7>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));
        l++;
        addChild(createLight<SmallSimpleLight<MLIGHT8>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));
        j++;l=0;
        addChild(createLight<SmallSimpleLight<MLIGHT9>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));
        l++;
        addChild(createLight<SmallSimpleLight<MLIGHT10>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));
        l++;
        addChild(createLight<SmallSimpleLight<MLIGHT11>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));
        l++;
        addChild(createLight<SmallSimpleLight<MLIGHT12>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));
        l++;
        addChild(createLight<SmallSimpleLight<MLIGHT13>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));
        l++;
        addChild(createLight<SmallSimpleLight<MLIGHT14>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));
        l++;
        addChild(createLight<SmallSimpleLight<MLIGHT15>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));
        l++;
        addChild(createLight<SmallSimpleLight<MLIGHT16>>(mm2px(Vec(x+l*1.8-0.25,y+7.75+j*1.8)),module,k*16+j*8+l));

      addOutput(createOutput<SmallPort>(mm2px(Vec(x+8,y)),module,TD4::SINGLE_GATE_OUTPUT+k));
      x+=16;
      if(k%4==3) {
        x=5;
        y+=14;
      }
    }
    x=5;
    y=TY(48);
    for(int k=0;k<16;k++) {
      addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,TD4::GATE_INPUT+k));
      addOutput(createOutput<SmallPort>(mm2px(Vec(x,y+16)),module,TD4::GATE_OUTPUT+k));
      addOutput(createOutput<SmallPort>(mm2px(Vec(x,y+32)),module,TD4::CV_OUTPUT+k));
      x+=8;
      if(k==7) {
        y+=7;x=5;
      }
    }
    y=TY(2);
    x=17;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,TD4::POLY_ADDR_INPUT));
    x+=8;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,TD4::POLY_CV_INPUT));
    x+=8;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,TD4::POLY_CV_STATIC_OUTPUT));
    x+=8;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,TD4::POLY_GATE_OUTPUT));
    x+=8;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,TD4::POLY_CV_OUTPUT));
	}

  void appendContextMenu(Menu *menu) override {
    TD4 *module=dynamic_cast<TD4 *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    std::vector <MinMaxRange> ranges={{-10,10},{-5,5},{-3,3},
                                      {-2,2},
                                      {-1,1},
                                      {0,1},
                                      {0,2},{0,3},{0,5},{0,10}};
    auto rangeSelectItem=new RangeSelectItem<TD4>(module,ranges);
    rangeSelectItem->text="Range";
    rangeSelectItem->rightText=string::f("%d/%dV",(int)module->min,(int)module->max)+"  "+RIGHT_ARROW;
    menu->addChild(rangeSelectItem);
    menu->addChild(createCheckMenuItem("Quantize", "",
                                       [=]() {return module->quantize;},
                                       [=]() { module->quantize=!module->quantize;}));
  }
};


Model* modelTD4 = createModel<TD4, TD4Widget>("TD4");