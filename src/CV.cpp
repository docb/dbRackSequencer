#include "plugin.hpp"


struct CV : Module {
  enum ParamId {
    CV_PARAM,LEVEL_PARAM,PARAMS_LEN
  };
  enum InputId {
    CV_INPUT,INPUTS_LEN
  };
  enum OutputId {
    CV_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };

  //unsigned int sizeIndex=0;
  std::vector<float> sizes={1.f/12.f,0.1,10.f/32.f,0.5,0.625,1,10.f/8.f};
  int currentValue=12;
  dsp::ClockDivider divider;

  CV() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configParam(CV_PARAM,0,24,12,"CV");
    configParam(LEVEL_PARAM,0.01,2.5,1.f/12.f,"Level");
    configInput(CV_INPUT,"CV mod");
    divider.setDivision(64);
  }

  float getVoltage(int value) {
    return (value-12)*params[LEVEL_PARAM].getValue();
  }

  void process(const ProcessArgs &args) override {
    if(inputs[CV_INPUT].isConnected()) {
      float in=inputs[CV_INPUT].getVoltage();
      int param=params[CV_PARAM].getValue();//0-24
      currentValue=float(param)+float(in);
      outputs[CV_PARAM].setVoltage(getVoltage(currentValue));
    } else {
      if(divider.process()) {
        int param=params[CV_PARAM].getValue();//0-24
        currentValue=float(param);
        outputs[CV_PARAM].setVoltage(getVoltage(currentValue));
      }

    }
  }

  void setLevelIndex(int index) {
    getParamQuantity(LEVEL_PARAM)->setValue(sizes[index]);
  }
  unsigned getLevelIndex() {
    float value = params[LEVEL_PARAM].getValue();
    auto itr=std::find(sizes.begin(),sizes.end(),value);
    if(itr==sizes.end()) return -1;
    return itr-sizes.begin();
  }
  float getLevel() {
    return params[LEVEL_PARAM].getValue();
  }
};

struct CVSelectButton : OpaqueWidget {
  int _value;
  std::string _label;
  std::string tooltipLabel;
  CV *_module;
  std::basic_string<char> fontPath;
  Tooltip *tooltip=nullptr;

  CVSelectButton(int value,std::string label,CV *module) : _value(value),_label(std::move(label)),_module(module) {
    fontPath=asset::plugin(pluginInstance,"res/FreeMonoBold.ttf");
  }

  void draw(const DrawArgs &args) override {
    std::shared_ptr <Font> font=APP->window->loadFont(fontPath);
    int currentValue=0;
    int lastValue=0;
    auto paramWidget=getAncestorOfType<ParamWidget>();
    assert(paramWidget);
    engine::ParamQuantity *pq=paramWidget->getParamQuantity();
    if(pq) {
      currentValue=std::round(pq->getValue());
      CV *module=dynamic_cast<CV *>(pq->module);
      if(module) {
        lastValue=module->currentValue;
        tooltipLabel=string::f("%0.4f V",module->getVoltage(_value));
      }
    }
    if(currentValue==_value) {
      nvgFillColor(args.vg,nvgRGB(0x7e,0xa6,0xd3));
    } else if(lastValue==_value) {
      nvgFillColor(args.vg,nvgRGB(0x7e,0x7e,0x7e));
    } else {
      nvgFillColor(args.vg,nvgRGB(0x3c,0x4c,0x71));
    }
    nvgStrokeColor(args.vg,nvgRGB(0xc4,0xc9,0xc2));
    nvgBeginPath(args.vg);
    nvgRoundedRect(args.vg,0,0,box.size.x,box.size.y,3.f);
    nvgFill(args.vg);
    nvgStroke(args.vg);
    nvgFontSize(args.vg,box.size.y-2);
    nvgFontFaceId(args.vg,font->handle);
    NVGcolor textColor=nvgRGB(0xff,0xff,0xaa);
    nvgTextAlign(args.vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
    nvgFillColor(args.vg,textColor);
    nvgText(args.vg,box.size.x/2.f,box.size.y/2.f,_label.c_str(),NULL);
  }

  void onDragHover(const event::DragHover &e) override {
    if(e.button==GLFW_MOUSE_BUTTON_LEFT) {
      e.consume(this);
    }
    Widget::onDragHover(e);
  }

  void onDragEnter(const event::DragEnter &e) override {
    if(e.button==GLFW_MOUSE_BUTTON_LEFT) {
      //auto origin = dynamic_cast<SelectParam*>(e.origin);
      auto origin=dynamic_cast<ParamWidget *>(e.origin);
      if(origin) {
        auto paramWidget=getAncestorOfType<ParamWidget>();
        assert(paramWidget);
        engine::ParamQuantity *pq=paramWidget->getParamQuantity();
        if(pq) {
          pq->setValue(_value);
        }
      }
    }
    createTooltip();
    Widget::onDragEnter(e);
  };

  void createTooltip() {
    if (!settings::tooltips)
      return;
    if(tooltip==nullptr) {
      tooltip=new Tooltip;
      tooltip->text=tooltipLabel;
      APP->scene->addChild(tooltip);
    }
  }


  void destroyTooltip() {
    if(!tooltip)
      return;
    APP->scene->removeChild(tooltip);
    delete tooltip;
    tooltip=nullptr;
  }


  void onEnter(const EnterEvent &e) override {
    createTooltip();
  }

  void onLeave(const LeaveEvent &e) override {
    destroyTooltip();
  }

  void onDragLeave(const DragLeaveEvent &e) override {
    destroyTooltip();
  }
};

struct CVSelectParam : ParamWidget {

  void init(CV *module,std::vector <std::string> labels) {
    const float margin=0;
    float height=box.size.y-2*margin;
    unsigned int len=labels.size();
    for(unsigned int i=0;i<len;i++) {
      auto selectButton=new CVSelectButton(len-i-1,labels[len-i-1],module);
      selectButton->box.pos=Vec(0,height/len*i+margin);
      selectButton->box.size=Vec(box.size.x,height/len);
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

  void onEnter(const EnterEvent &e) override {

  }

  void onLeave(const LeaveEvent &e) override {

  }
};

struct StepSizeSelectItem : MenuItem {
  CV *module;
  std::vector <std::string> labels;

  StepSizeSelectItem(CV *_module,std::vector <std::string> _labels) : module(_module),labels(std::move(_labels)) {
  }

  Menu *createChildMenu() override {
    Menu *menu=new Menu;
    for(unsigned int k=0;k<labels.size();k++) {
      menu->addChild(createCheckMenuItem(labels[k],"",[=]() {
        return module->getLevelIndex()==k;
      },[=]() {
        module->setLevelIndex(k);
      }));
    }
    return menu;
  }
};

struct CVWidget : ModuleWidget {
  CVWidget(CV *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/CV.svg")));


    auto selectParam=createParam<CVSelectParam>(mm2px(Vec(1.7,MHEIGHT-123.f)),module,CV::CV_PARAM);
    selectParam->box.size=Vec(20,250);
    selectParam->init(module,{"-12","-11","-10","-9","-8","-7","-6","-5","-4","-3","-2","-1","0","1","2","3","4","5","6","7","8","9","10","11","12"});
    addParam(selectParam);
    addInput(createInput<SmallPort>(mm2px(Vec(1.9f,TY(30))),module,CV::CV_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(1.9f,TY(18))),module,CV::LEVEL_PARAM));
    addOutput(createOutput<SmallPort>(mm2px(Vec(1.9f,TY(6))),module,CV::CV_OUTPUT));

  }

  void appendContextMenu(Menu *menu) override {
    CV *module=dynamic_cast<CV *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    std::vector <std::string> labels={"Semi","0.1","Pat 32","0.5","Pat 16","1","Pat 8"};
    auto stepSelectItem=new StepSizeSelectItem(module,labels);
    stepSelectItem->text="Step Size";
    stepSelectItem->rightText=string::f("%0.4f V",module->getLevel())+"  "+RIGHT_ARROW;
    menu->addChild(stepSelectItem);
  }

};


Model *modelCV=createModel<CV,CVWidget>("CV");