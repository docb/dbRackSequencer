#include "plugin.hpp"


struct PXY : Module {
  enum ParamId {
    START_X_PARAM,START_Y_PARAM,SIZE_X_PARAM,SIZE_Y_PARAM,PARAMS_LEN
  };
  enum InputId {
    RST_INPUT,LEFT_INPUT,RIGHT_INPUT,DOWN_INPUT,UP_INPUT,START_X_INPUT,START_Y_INPUT,INPUTS_LEN
  };
  enum OutputId {
    CV_OUTPUT,CV_X_OUTPUT,CV_Y_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  dsp::SchmittTrigger leftTrigger;
  dsp::SchmittTrigger rightTrigger;
  dsp::SchmittTrigger downTrigger;
  dsp::SchmittTrigger upTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::PulseGenerator rstPulse;
  int stepX=0;
  int stepY=0;
  int sizeX=4;
  int sizeY=4;
  int gridSize=16;

  PXY() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configParam(START_X_PARAM,0,31,0,"Start X");
    configParam(START_Y_PARAM,0,31,0,"Start Y");
    configParam(SIZE_X_PARAM,2,32,4,"Size X");
    configParam(SIZE_Y_PARAM,2,32,4,"Size Y");
    getParamQuantity(START_X_PARAM)->snapEnabled=true;
    getParamQuantity(START_Y_PARAM)->snapEnabled=true;
    getParamQuantity(SIZE_X_PARAM)->snapEnabled=true;
    getParamQuantity(SIZE_Y_PARAM)->snapEnabled=true;
    configInput(LEFT_INPUT,"Step left");
    configInput(RIGHT_INPUT,"Step right");
    configInput(DOWN_INPUT,"Step down");
    configInput(UP_INPUT,"Step up");
    configInput(START_X_INPUT,"Start X");
    configInput(START_Y_INPUT,"Start Y");
    configOutput(CV_OUTPUT,"CV");
    configOutput(CV_X_OUTPUT,"CV X");
    configOutput(CV_Y_OUTPUT,"CV Y");
  }

  void process(const ProcessArgs &args) override {
    sizeX=params[SIZE_X_PARAM].getValue();
    sizeY=params[SIZE_Y_PARAM].getValue();
    if(inputs[START_X_INPUT].isConnected()) {
      int c=(clamp(inputs[START_X_INPUT].getVoltage(),0.f,9.99f)/10.f)*float(sizeX);
      getParamQuantity(START_X_PARAM)->setValue(c);
    }
    if(inputs[START_Y_INPUT].isConnected()) {
      int c=(clamp(inputs[START_Y_INPUT].getVoltage(),0.f,9.99f)/10.f)*float(sizeY);
      getParamQuantity(START_Y_PARAM)->setValue(c);
    }

    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())) {
      stepX=0;
      stepY=0;
      rstPulse.trigger(0.001f);
    }
    bool resetGate=rstPulse.process(args.sampleTime);
    if(rightTrigger.process(inputs[RIGHT_INPUT].getVoltage())&&!resetGate) {
      stepX++;
      if(stepX==sizeX)
        stepX=0;
    }
    if(leftTrigger.process(inputs[LEFT_INPUT].getVoltage())&&!resetGate) {
      stepX--;
      if(stepX<0)
        stepX=sizeX-1;
    }
    if(downTrigger.process(inputs[DOWN_INPUT].getVoltage())&&!resetGate) {
      stepY++;
      if(stepY==sizeY)
        stepY=0;
    }
    if(upTrigger.process(inputs[UP_INPUT].getVoltage())&&!resetGate) {
      stepY--;
      if(stepY<0)
        stepY=sizeY-1;
    }

    int sx=stepX+params[START_X_PARAM].getValue();
    int sy=stepY+params[START_Y_PARAM].getValue();
    outputs[CV_X_OUTPUT].setVoltage(float(sx)/(gridSize/10.f));
    outputs[CV_Y_OUTPUT].setVoltage(float(sy)/(gridSize/10.f));
    outputs[CV_OUTPUT].setVoltage(float(sy*sizeY+sx)/(gridSize/10.f));
  }

  void setSize(int size) {
    gridSize=size;
  }

  int getSize() {
    return gridSize;
  }

  void dataFromJson(json_t *root) override {
    json_t *jSize=json_object_get(root,"size");
    if(jSize) {
      gridSize=json_integer_value(jSize);
    }
  }

  json_t *dataToJson() override {
    json_t *root=json_object();
    json_object_set_new(root,"size",json_integer(gridSize));
    return root;
  }

};


struct PXYWidget : ModuleWidget {
  PXYWidget(PXY *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/PXY.svg")));

    addChild(createWidget<ScrewSilver>(Vec(0,0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-15,0)));
    addChild(createWidget<ScrewSilver>(Vec(0,365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-15,365)));

    addInput(createInput<SmallPort>(mm2px(Vec(2,TY(105.5))),module,PXY::LEFT_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(17,TY(105.5))),module,PXY::RIGHT_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(9.4,TY(98))),module,PXY::DOWN_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(9.4,TY(113))),module,PXY::UP_INPUT));

    addInput(createInput<SmallPort>(mm2px(Vec(4,TY(83))),module,PXY::START_X_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(15,TY(83))),module,PXY::START_Y_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(4,TY(75))),module,PXY::START_X_PARAM));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(15,TY(75))),module,PXY::START_Y_PARAM));

    addParam(createParam<TrimbotWhite>(mm2px(Vec(4,TY(58))),module,PXY::SIZE_X_PARAM));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(15,TY(58))),module,PXY::SIZE_Y_PARAM));

    addInput(createInput<SmallPort>(mm2px(Vec(9.4,TY(44))),module,PXY::RST_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(4,TY(28))),module,PXY::CV_X_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(15,TY(28))),module,PXY::CV_Y_OUTPUT));

    addOutput(createOutput<SmallPort>(mm2px(Vec(9.4,TY(11))),module,PXY::CV_OUTPUT));
  }

  void appendContextMenu(Menu *menu) override {
    PXY *module=dynamic_cast<PXY *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    std::vector<int> sizes={8,16,32};
    auto sizeSelectItem=new SizeSelectItem<PXY>(module,sizes);
    sizeSelectItem->text="size";
    sizeSelectItem->rightText=string::f("%d",module->getSize())+"  "+RIGHT_ARROW;
    menu->addChild(sizeSelectItem);
  }

};


Model *modelPXY=createModel<PXY,PXYWidget>("PXY");