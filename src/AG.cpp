#include "plugin.hpp"

#define MAX_PATS 100

struct AG : Module {
	enum ParamId {
		PAT_PARAM,EDIT_PARAM,COPY_PARAM,PASTE_PARAM,INSERT_PARAM,DELETE_PARAM,PARAMS_LEN
	};
	enum InputId {
		PAT_CV_INPUT,INPUTS_LEN
	};
	enum OutputId {
		GATE_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
  RND rnd;
  float randomDens=0.5f;
  bool gates[MAX_PATS][PORT_MAX_CHANNELS] = {};
  bool cbGates[PORT_MAX_CHANNELS] = {};
  int maxChannels = 8;
	AG() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configParam(PAT_PARAM,0,MAX_PATS-1,0,"Pattern Selection");
    configButton(COPY_PARAM,"Copy");
    configButton(PASTE_PARAM,"Paste");
    configButton(INSERT_PARAM,"Insert");
    configButton(DELETE_PARAM,"Delete");
    configButton(EDIT_PARAM,"Edit");
    configInput(PAT_CV_INPUT,"Pattern (0.1V/Step)");
    configOutput(GATE_OUTPUT,"Gate");
	}

	void process(const ProcessArgs& args) override {
    if(inputs[PAT_CV_INPUT].isConnected() && params[EDIT_PARAM].getValue() == 0) {
      int c=clamp(inputs[PAT_CV_INPUT].getVoltage(),0.f,9.99f)*float(MAX_PATS)/10.f;
      getParamQuantity(PAT_PARAM)->setValue(c);
    }
    int currentPattern=params[PAT_PARAM].getValue();
    int k=0;
    for(;k<maxChannels;k++) {
      outputs[GATE_OUTPUT].setVoltage(gates[currentPattern][k]?10.f:0.f,k);
    }
    outputs[GATE_OUTPUT].setChannels(maxChannels);
	}

  json_t *dataToJson() override {
    json_t *data=json_object();
    json_t *patternList=json_array();
    for(int k=0;k<MAX_PATS;k++) {
      json_t *onList=json_array();
      for(int j=0;j<PORT_MAX_CHANNELS;j++) {
        json_array_append_new(onList,json_boolean(gates[k][j]));
      }
      json_array_append_new(patternList,onList);
    }
    json_object_set_new(data,"gates",patternList);
    json_object_set_new(data,"channels",json_integer(maxChannels));
    return data;
  }

  void dataFromJson(json_t *rootJ) override {
    json_t *data=json_object_get(rootJ,"gates");
    if(!data)
      return;
    for(int k=0;k<MAX_PATS;k++) {
      json_t *arr=json_array_get(data,k);
      if(arr) {
        for(int j=0;j<PORT_MAX_CHANNELS;j++) {
          json_t *on=json_array_get(arr,j);
          gates[k][j]=json_boolean_value(on);
        }
      }
    }
    json_t *jMaxChannels=json_object_get(rootJ,"channels");
    if(jMaxChannels)
      maxChannels=json_integer_value(jMaxChannels);
  }
  void onReset(const ResetEvent &e) override {
    params[PAT_PARAM].setValue(0);
    for(int k=0;k<MAX_PATS;k++) {
      for(int j=0;j<PORT_MAX_CHANNELS;j++) {
        gates[k][j]=false;
      }
    }
    for(int k=0;k<PORT_MAX_CHANNELS;k++) {
      outputs[GATE_OUTPUT].setVoltage(0,k);
    }
  }

  void insert() {
    int currentPattern=params[PAT_PARAM].getValue();
    for(int k=MAX_PATS-1;k>currentPattern;k--) {
      for(int j=0;j<16;j++) {
        gates[k][j]=gates[k-1][j];
      }
    }
    for(int j=0;j<16;j++) {
      gates[currentPattern][j]=false;
    }
  }

  void del() {
    int currentPattern=params[PAT_PARAM].getValue();
    for(int k=currentPattern;k<MAX_PATS-1;k++) {
      for(int j=0;j<16;j++) {
        gates[k][j]=gates[k+1][j];
      }
    }
  }

  void copy() {
    int currentPattern=params[PAT_PARAM].getValue();
    for(int j=0;j<PORT_MAX_CHANNELS;j++) {
      cbGates[j]=gates[currentPattern][j];
    }
  }

  void paste() {
    int currentPattern=params[PAT_PARAM].getValue();
    for(int j=0;j<PORT_MAX_CHANNELS;j++) {
      gates[currentPattern][j]=cbGates[j];
    }
  }
  void updateKey(int key) {
    int currentPattern=params[PAT_PARAM].getValue();
    gates[currentPattern][key]=!(gates[currentPattern][key]);
  }

  void switchPattern() {
  }
  bool getGateStatus(int key) {
    int pattern = int(params[PAT_PARAM].getValue());
    return gates[pattern][key];
  }
  void randomizePattern(int pattern) {
    for(int k=0;k<maxChannels;k++) {
      gates[pattern][k] = rnd.nextCoin(1-randomDens);
    }
  }
  void onRandomize(const RandomizeEvent& e) override {
    int pattern = int(params[PAT_PARAM].getValue());
    randomizePattern(pattern);
  }
};
template<typename M>
struct GateButton : OpaqueWidget {
  M *module;
  int key;
  Tooltip *tooltip = nullptr;
  std::string label;
  NVGcolor onColor = nvgRGB(118,169,118);
  NVGcolor offColor = nvgRGB(55,80,55);
  NVGcolor onColorInactive = nvgRGB(128,128,128);
  NVGcolor offColorInactive = nvgRGB(43,43,43);
  NVGcolor border = nvgRGB(196,201,104);
  NVGcolor borderInactive = nvgRGB(150,150,150);
  std::basic_string<char> fontPath;
  GateButton(M* _module,int _key,Vec pos, Vec size) : module(_module),key(_key) {
    fontPath = asset::plugin(pluginInstance, "res/FreeMonoBold.ttf");
    box.size = size;
    box.pos = pos;
    label = string::f("chn %d",key);
  }
  void onButton(const event::Button &e) override {
    if(!(e.button==GLFW_MOUSE_BUTTON_LEFT&&(e.mods&RACK_MOD_MASK)==0)) {
      return;
    }
    if(e.action==GLFW_PRESS) {
      if(module) module->updateKey(key);
    }
  }

  void createTooltip() {
    if (!settings::tooltips)
      return;
    tooltip = new Tooltip;
    tooltip->text = label;
    APP->scene->addChild(tooltip);
  }


  void destroyTooltip() {
    if (!tooltip)
      return;
    APP->scene->removeChild(tooltip);
    delete tooltip;
    tooltip = nullptr;
  }


  void onEnter(const EnterEvent& e) override {
    createTooltip();
  }


  void onLeave(const LeaveEvent& e) override {
    destroyTooltip();
  }

  void drawLayer(const DrawArgs &args,int layer) override {
    if(layer==1) {
      _draw(args);
    }
    Widget::drawLayer(args,layer);
  }
  void _draw(const DrawArgs &args) {
    std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
    NVGcolor color = offColor;
    NVGcolor borderColor = border;
    if(module) {
      if(key>=module->maxChannels) {
        borderColor = borderInactive;
        if(module->getGateStatus(key)) {
          color = onColorInactive;
        } else {
          color = offColorInactive;
        }
      } else {
        borderColor = border;
        if(module->getGateStatus(key)) {
          color = onColor;
        } else {
          color = offColor;
        }
      }
    }
    nvgBeginPath(args.vg);
    nvgRoundedRect(args.vg,1,1,box.size.x-2,box.size.y-2,2);
    nvgFillColor(args.vg,color);
    nvgStrokeColor(args.vg,borderColor);
    nvgFill(args.vg);
    nvgStroke(args.vg);
    nvgFontSize(args.vg, box.size.y-2);
    nvgFontFaceId(args.vg, font->handle);
    NVGcolor textColor = nvgRGB(0xff, 0xff, 0xff);
    nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(args.vg, textColor);
    nvgText(args.vg, box.size.x/2.f, box.size.y/2.f, std::to_string(key+1).c_str(), NULL);
  }
};
template<typename M>
struct GateDisplay : OpaqueWidget {
  M *module;
  Vec buttonSize;
  GateDisplay(M *m,Vec _pos) : module(m) {
    box.pos=_pos;
    box.size = Vec(20,10*16);
    for(int k=0;k<16;k++) {
      addChild(new GateButton<AG>(module,k,Vec(0,k*10),Vec(20,10)));
    }
  }
};
template<typename M>
struct PatternSelect : SpinParamWidget {
  M *module;
  PatternSelect() {
    init();
  }
  void onChange(const event::Change &e) override {
    if(module)
      module->switchPattern();
  }
};

struct AGWidget : ModuleWidget {
	AGWidget(AG* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/AG.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    float x = 1.9;
    auto patParam=createParam<PatternSelect<AG>>(mm2px(Vec(1.9f,MHEIGHT-119)),module,AG::PAT_PARAM);
    patParam->module=module;
    addParam(patParam);
    addInput(createInput<SmallPort>(mm2px(Vec(x,MHEIGHT-108)),module,AG::PAT_CV_INPUT));
    auto editButton=createParam<SmallButtonWithLabel>(mm2px(Vec(1.7f,MHEIGHT-100)),module,AG::EDIT_PARAM);
    editButton->setLabel("Edit");
    addParam(editButton);
    auto copyButton=createParam<CopyButton<AG>>(mm2px(Vec(1.7f,MHEIGHT-96)),module,AG::COPY_PARAM);
    copyButton->label="Cpy";
    copyButton->module=module;
    addParam(copyButton);
    auto pasteButton=createParam<PasteButton<AG>>(mm2px(Vec(1.7f,MHEIGHT-92)),module,AG::PASTE_PARAM);
    pasteButton->label="Pst";
    pasteButton->module=module;
    addParam(pasteButton);
    auto insertButton=createParam<InsertButton<AG>>(mm2px(Vec(1.7f,MHEIGHT-88)),module,AG::INSERT_PARAM);
    insertButton->label="+";
    insertButton->module=module;
    addParam(insertButton);
    auto delButton=createParam<DelButton<AG>>(mm2px(Vec(1.7f,MHEIGHT-84)),module,AG::DELETE_PARAM);
    delButton->label="-";
    delButton->module=module;
    addParam(delButton);

    addChild(new GateDisplay<AG>(module,mm2px(Vec(1.7f,MHEIGHT-79))));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,MHEIGHT-19.f)),module,AG::GATE_OUTPUT));
  }
  void appendContextMenu(Menu *menu) override {
    AG *module=dynamic_cast<AG *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    auto channelSelect=new IntSelectItem(&module->maxChannels,1,PORT_MAX_CHANNELS);
    channelSelect->text="Polyphonic Channels";
    channelSelect->rightText=string::f("%d",module->maxChannels)+"  "+RIGHT_ARROW;
    menu->addChild(channelSelect);
    menu->addChild(new DensMenuItem<AG>(module));
  }
};


Model* modelAG = createModel<AG, AGWidget>("AG");