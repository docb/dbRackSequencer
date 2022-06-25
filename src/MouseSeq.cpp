#include "plugin.hpp"

#define NUM_CLOCKS 4
#define NUM_SCALES 4
#define NUM_PAT 100
struct RPoint {
  RPoint(unsigned long _pos,uint8_t _y,uint8_t _x,uint8_t _scale,uint8_t _end=0) : pos(_pos),y(_y),x(_x),scale(_scale),end(_end) {
  }
  RPoint(json_t *json) {
    int k=0;
    pos=json_integer_value(json_array_get(json,k++));
    y=json_integer_value(json_array_get(json,k++));
    x=json_integer_value(json_array_get(json,k++));
    scale=json_integer_value(json_array_get(json,k++));
    end=json_integer_value(json_array_get(json,k));
  }
  unsigned long pos=0;
  uint8_t y=0;
  uint8_t x=0;
  uint8_t scale=0;
  uint8_t end=0;
  json_t *toJson() {
    json_t *jList=json_array();
    json_array_append_new(jList,json_integer(pos));
    json_array_append_new(jList,json_integer(y));
    json_array_append_new(jList,json_integer(x));
    json_array_append_new(jList,json_integer(scale));
    json_array_append_new(jList,json_integer(end));
    return jList;
  }

};

struct Scales {
  int scales[NUM_SCALES][12]={{1,0,1,0,1,1,0,1,0,1,0,1},
                              {1,0,1,1,0,1,0,1,0,1,1,0},
                              {1,1,0,0,1,1,0,1,1,0,1,0},
                              {1,0,1,0,1,1,0,1,0,1,0,1}};

  std::vector<int> getScale(int nr) {
    std::vector<int> ret={};
    if(nr>=0&&nr<NUM_SCALES) {
      for(int k=0;k<12;k++) {
        if(scales[nr][k])
          ret.push_back(k);
      }
    }
    return ret;
  }

  void toggle(int nr,int pos) {
    if(nr>=0&&nr<NUM_SCALES&&pos>=0&&pos<12) {
      scales[nr][pos]=!scales[nr][pos];
    }

  }

  void set(int nr,int pos,int v) {
    if(nr>=0&&nr<NUM_SCALES&&pos>=0&&pos<12) {
      scales[nr][pos]=v>0;
    }
  }

  int get(int nr,int pos) {
    if(nr>=0&&nr<NUM_SCALES&&pos>=0&&pos<12) {
      return scales[nr][pos];
    }
    return -1;
  }

  json_t *toJson() {
    json_t *scaleList=json_array();
    for(int k=0;k<NUM_SCALES;k++) {
      json_t *jList=json_array();
      for(int j=0;j<12;j++) {
        json_array_append_new(jList,json_integer(scales[k][j]));
      }
      json_array_append_new(scaleList,jList);
    }
    return scaleList;
  }

  void fromJson(json_t *scaleList) {
    if(!scaleList)
      return;
    for(int k=0;k<NUM_SCALES;k++) {
      json_t *arr=json_array_get(scaleList,k);
      if(arr) {
        for(int j=0;j<12;j++) {
          json_t *on=json_array_get(arr,j);
          scales[k][j]=json_integer_value(on);
        }
      }
    }
  }


};

struct MouseSeq : Module {
  enum ParamId {
    PLAY_PARAM,CUR_SCALE_PARAM,CUR_CLOCK_PARAM,OCT_PARAM,SEMI_PARAM,MODE_PARAM,PAT_PARAM,LOCK_PARAM,PARAMS_LEN
  };
  enum InputId {
    CLOCK_INPUT,SEMI_INPUT=NUM_CLOCKS,OCT_INPUT,X_INPUT,Y_INPUT,GATE_INPUT,PLAY_INPUT,SCALE_INPUT,CLOCK_SELECT_INPUT,PAT_INPUT,INPUTS_LEN
  };
  enum OutputId {
    VOCT_OUTPUT,VEL_OUTPUT,GATE_OUTPUT,RTR_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  Scales scales;
  std::vector <RPoint> points[NUM_PAT]={};
  int size=32;
  int offset=0;
  uint8_t oldY=0;
  int oldPY=0;
  uint8_t playX=0;
  uint8_t playY=0;
  uint8_t y=0;
  uint8_t x=0;
  bool gate=false;
  bool oldGate=false;
  bool play=false;
  int currentPattern=0;
  unsigned long recPos=0;
  unsigned long playPos=0;
  bool playTrig=false;
  int pointPos=0;
  bool extMode=false;
  dsp::SchmittTrigger clockTrigger[NUM_CLOCKS];
  dsp::SchmittTrigger manualPlayTrigger;
  dsp::SchmittTrigger playTrigger;

  dsp::PulseGenerator reTrigPulse;

  MouseSeq() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configParam(SEMI_PARAM,0,11,0,"Semitone Offset");
    getParamQuantity(SEMI_PARAM)->snapEnabled=true;
    configInput(SEMI_INPUT,"Semitone Offset");
    configParam(OCT_PARAM,-4,4,0,"Octave");
    getParamQuantity(OCT_PARAM)->snapEnabled=true;
    configInput(OCT_INPUT,"Octave");
    for(int k=0;k<NUM_CLOCKS;k++) {
      configInput(CLOCK_INPUT+k,"Clock "+std::to_string(k+1));
    }
    configOutput(VOCT_OUTPUT,"V/Oct");
    configOutput(VEL_OUTPUT,"Vel");
    configOutput(RTR_OUTPUT,"ReTrigger");
    configOutput(GATE_OUTPUT,"Gate");
    configButton(PLAY_PARAM,"Replay");
    configParam(CUR_CLOCK_PARAM,0,3,0,"Clock");
    configParam(CUR_SCALE_PARAM,0,3,0,"Scale");
    configInput(X_INPUT,"X");
    configInput(Y_INPUT,"Y");
    configInput(GATE_INPUT,"Gate");
    configInput(PLAY_INPUT,"Play");
    configInput(SCALE_INPUT,"Scale select");
    configInput(CLOCK_SELECT_INPUT,"Clock Select");
    configSwitch(MODE_PARAM,0,1,0,"Mode",{"Mouse","Extern"});
    configParam(PAT_PARAM,0,NUM_PAT-1,0,"Pattern Selection");
    configButton(LOCK_PARAM,"Lock");

  }


  void setPos(uint8_t _y,uint8_t _x) {
    y=_y;
    x=_x;
  }

  float computeVoct(int _y) {
    std::vector<int> scale=scales.getScale(int(params[CUR_SCALE_PARAM].getValue()));
    int octave=(size-_y-1)/scale.size();
    int n=(size-_y-1)%scale.size();
    return params[OCT_PARAM].getValue()+params[SEMI_PARAM].getValue()/12.f+float(octave)+scale[n]/12.f;
  }

  void setCurrentClock(int nr) {
    getParamQuantity(CUR_CLOCK_PARAM)->setValue(nr);
  }

  void setCurrentScale(int nr) {
    getParamQuantity(CUR_SCALE_PARAM)->setValue(nr);
  }

  bool isActive(int nr,int pos) {
    if(nr!=int(params[CUR_SCALE_PARAM].getValue()))
      return false;
    std::vector<int> scale=scales.getScale(params[CUR_SCALE_PARAM].getValue());
    // 0,2,3,5,7,9,10
    if(play) {
      return scale[(size-1-playY)%int(scale.size())]==pos;
    } else {
      return scale[(size-1-y)%int(scale.size())]==pos;
    }

  }

  void process(const ProcessArgs &args) override {
    if(inputs[PAT_INPUT].isConnected() && params[LOCK_PARAM].getValue() == 0) {
      int c=clamp(inputs[PAT_INPUT].getVoltage(),0.f,9.99f)*float(NUM_PAT)/10.f;
      getParamQuantity(PAT_PARAM)->setValue(c);
    }
    extMode=params[MODE_PARAM].getValue()>0;
    if(inputs[OCT_INPUT].isConnected()) {
      getParamQuantity(OCT_PARAM)->setValue(int(inputs[OCT_INPUT].getVoltage()));
    }
    if(inputs[SCALE_INPUT].isConnected()) {
      getParamQuantity(CUR_SCALE_PARAM)->setValue(int(inputs[SCALE_INPUT].getVoltage()*0.4f));
    }
    if(inputs[CLOCK_SELECT_INPUT].isConnected()) {
      getParamQuantity(CUR_CLOCK_PARAM)->setValue(int(inputs[CLOCK_SELECT_INPUT].getVoltage()*0.4f));
    }
    if(inputs[OCT_INPUT].isConnected()) {
      getParamQuantity(OCT_PARAM)->setValue(int(inputs[OCT_INPUT].getVoltage()));
    }
    if(inputs[SEMI_INPUT].isConnected()) {
      getParamQuantity(SEMI_PARAM)->setValue(int(inputs[SEMI_INPUT].getVoltage()*1.2f));
    }

    if(playTrigger.process(inputs[PLAY_INPUT].getVoltage())|manualPlayTrigger.process(params[PLAY_PARAM].getValue())) {
      playTrig=true;
    }

    int currentClock=params[CUR_CLOCK_PARAM].getValue();
    if(clockTrigger[currentClock].process(inputs[CLOCK_INPUT+currentClock].getVoltage())) {
      if(extMode) {
        gate=inputs[GATE_INPUT].getVoltage()>5.f;
        x=clamp(inputs[X_INPUT].getVoltage(),0.f,9.9999f)*3.2f;
        y=size-1-clamp(inputs[Y_INPUT].getVoltage(),0.f,9.9999f)*3.2f;
      }
      if(playTrig) {
        currentPattern=params[PAT_PARAM].getValue();
        if(!points[currentPattern].empty()) {
          play=true;
          pointPos=0;
          playPos=0;
        }
        playTrig=false;
      }
      if(gate&&!oldGate) {
        currentPattern=params[PAT_PARAM].getValue();;
        recPos=0;
        play=false;
        points[currentPattern].clear();
      }
      if(!play) {
        uint8_t curScale=uint8_t(params[CUR_SCALE_PARAM].getValue());
        if(oldY!=y) {
          reTrigPulse.trigger(0.01f);
          if(gate) {
            points[currentPattern].emplace_back(recPos,y,x,curScale);
          }
        }

        if(!gate&&oldGate) {
          points[currentPattern].emplace_back(recPos,y,x,curScale,1);
        }
        float voct=computeVoct(y);
        float vel=x/3.2f;
        outputs[VOCT_OUTPUT].setVoltage(voct);
        outputs[VEL_OUTPUT].setVoltage(vel);
        outputs[GATE_OUTPUT].setVoltage(gate?10.f:0.f);
        oldY=y;
      }
      oldGate=gate;
    }
    if(play) {
      outputs[GATE_OUTPUT].setVoltage(10.f);

      if(playPos==points[currentPattern][pointPos].pos) {
        if(points[currentPattern][pointPos].end) {
          play=false;
          gate=false;
        } else {
          reTrigPulse.trigger(0.01f);
          playY=points[currentPattern][pointPos].y;
          playX=points[currentPattern][pointPos].x;
          getParamQuantity(CUR_SCALE_PARAM)->setValue(points[currentPattern][pointPos].scale);
          float voct=computeVoct(playY);
          float vel=playX/3.2f;
          outputs[VOCT_OUTPUT].setVoltage(voct);
          outputs[VEL_OUTPUT].setVoltage(vel);
        }
        pointPos++;
      }
      playPos++;

    }
    if(gate) {
      recPos++;
    }
    outputs[RTR_OUTPUT].setVoltage(reTrigPulse.process(args.sampleTime)?10.f:0.f);

  }

  json_t *dataToJson() override {
    json_t *data=json_object();
    json_object_set_new(data,"scales",scales.toJson());
    json_t *jPointsList=json_array();
    for(int k=0;k<NUM_PAT;k++) {
      json_t *jPoints=json_array();
      for(auto p:points[k]) {
        json_array_append_new(jPoints,p.toJson());
      }
      json_array_append_new(jPointsList,jPoints);
    }
    json_object_set_new(data,"points",jPointsList);
    return data;
  }
  void dataFromJson(json_t *rootJ) override {
    for(auto p : points) p.clear();
    json_t *data=json_object_get(rootJ,"scales");
    scales.fromJson(data);
    json_t *jPointsList=json_object_get(rootJ,"points");
    for(int k=0;k<NUM_PAT;k++) {
      json_t *jPoints=json_array_get(jPointsList,k);
      int len=json_array_size(jPoints);
      for(int i=0;i<len;i++) {
        points[k].emplace_back(json_array_get(jPoints,i));
      }
    }
  }
  void setCurrentPattern() {

  }
};

struct XYDisplay : OpaqueWidget {
  MouseSeq *module=nullptr;
  int oldC=-1;
  int oldR=-1;
  Vec dragPosition={};

  XYDisplay(MouseSeq *_module,Vec pos,Vec size) : module(_module) {
    box.size=size;
    box.pos=pos;
  }

  void drawLayer(const DrawArgs &args,int layer) override {
    if(layer==1) {
      _draw(args);
    }
    Widget::drawLayer(args,layer);
  }

  void _draw(const DrawArgs &args) {
    if(module==nullptr)
      return;

    int numRows=module->size;
    Vec size=box.size;
    float rowHeight=size.y/numRows;

    float posY=0;
    for(int r=0;r<numRows;r++) {
      NVGcolor dg=r%2==0?nvgRGB(40,64,40):nvgRGB(40,64,64);
      nvgBeginPath(args.vg);
      nvgRect(args.vg,0,posY,box.size.x,rowHeight);
      nvgFillColor(args.vg,dg);
      nvgFill(args.vg);
      posY+=rowHeight;
    }
    if(module->gate) {
      NVGcolor color=nvgRGB(100,64,40);
      nvgBeginPath(args.vg);
      nvgRect(args.vg,module->x*box.size.x/numRows,module->y*box.size.y/numRows,rowHeight,rowHeight);
      nvgFillColor(args.vg,color);
      nvgFill(args.vg);
    }
    if(module->play) {
      NVGcolor pcolor=nvgRGB(0,100,200);
      nvgBeginPath(args.vg);
      nvgRect(args.vg,module->playX*box.size.x/numRows,module->playY*box.size.y/numRows,rowHeight,rowHeight);
      nvgFillColor(args.vg,pcolor);
      nvgFill(args.vg);
    }
  }

  virtual void onButton(const event::Button &e) override {
    if(!module)
      return;
    if(module->extMode)
      return;
    int numRows=module->size;
    if(e.action==GLFW_PRESS&&e.button==GLFW_MOUSE_BUTTON_LEFT) {
      int c=oldC=floor(e.pos.x/(box.size.x/float(numRows)));
      int r=oldR=floor(e.pos.y/(box.size.y/float(numRows)));
      module->y=r;
      module->x=c;
      e.consume(this);
      dragPosition=e.pos;
      module->gate=true;
    }
  }

  void onDragMove(const event::DragMove &e) override {
    if(!module)
      return;
    if(module->extMode)
      return;
    int numRows=module->size;
    dragPosition=dragPosition.plus(e.mouseDelta.div(getAbsoluteZoom()));
    int c=floor(dragPosition.x/(box.size.x/float(numRows)));
    int r=floor(dragPosition.y/(box.size.y/float(numRows)));
    if(c<0)
      c=0;
    if(r<0)
      r=0;
    if(c>=numRows)
      c=numRows-1;
    if(r>=numRows)
      r=numRows-1;
    if(c!=oldC||r!=oldR) {
      if(e.button==GLFW_MOUSE_BUTTON_LEFT) {
        module->y=r;
        module->x=c;
      }

      oldC=c;
      oldR=r;
    }
  }

  void onDragEnd(const event::DragEnd &e) override {
    module->gate=false;
  }

  bool isMouseInDrawArea(Vec position) {
    if(position.x<0)
      return (false);
    if(position.y<0)
      return (false);
    if(position.x>=box.size.x)
      return (false);
    if(position.y>=box.size.y)
      return (false);
    return (true);
  }

  void onSelectText(const SelectTextEvent &e) override {
    switch(e.codepoint) {
      case 122:
        module->setCurrentClock(0);
        break;
      case 120:
        module->setCurrentClock(1);
        break;
      case 99:
        module->setCurrentClock(2);
        break;
      case 118:
        module->setCurrentClock(3);
        break;
      case 97:
        module->setCurrentScale(0);
        break;
      case 115:
        module->setCurrentScale(1);
        break;
      case 100:
        module->setCurrentScale(2);
        break;
      case 102:
        module->setCurrentScale(3);
        break;
      default:
        break;
    }
    e.consume(this);
  }

  void onHover(const HoverEvent &e) override {
    APP->event->setSelectedWidget(this);
  }
};

struct ScaleButton : OpaqueWidget {
  MouseSeq *module;
  int nr;
  int key;
  Tooltip *tooltip=nullptr;
  std::string label;
  NVGcolor onColor=nvgRGB(118,169,118);
  NVGcolor offColor=nvgRGB(55,80,55);
  NVGcolor onColorInactive=nvgRGB(90,120,90);
  NVGcolor border=nvgRGB(196,201,104);
  NVGcolor borderInactive=nvgRGB(150,150,150);
  std::basic_string<char> fontPath;

  ScaleButton(MouseSeq *_module,int _nr,int _key,Vec pos,Vec size) : module(_module),nr(_nr),key(_key) {
    fontPath=asset::plugin(pluginInstance,"res/FreeMonoBold.ttf");
    box.size=size;
    box.pos=pos;
  }

  void onButton(const event::Button &e) override {
    if(!(e.button==GLFW_MOUSE_BUTTON_LEFT&&(e.mods&RACK_MOD_MASK)==0)) {
      return;
    }
    if(e.action==GLFW_PRESS) {
      if(module)
        module->scales.toggle(nr,key);
    }
  }

  void drawLayer(const DrawArgs &args,int layer) override {
    if(layer==1) {
      _draw(args);
    }
    Widget::drawLayer(args,layer);
  }

  void _draw(const DrawArgs &args) {
    std::shared_ptr <Font> font=APP->window->loadFont(fontPath);
    NVGcolor color=offColor;
    NVGcolor borderColor=border;
    if(module) {
      borderColor=border;
      if(module->scales.scales[nr][key]) {
        color=module->isActive(nr,key)?onColor:onColorInactive;
      } else {
        color=offColor;
      }
    }
    nvgBeginPath(args.vg);
    nvgRoundedRect(args.vg,1,1,box.size.x-2,box.size.y-2,2);
    nvgFillColor(args.vg,color);
    nvgStrokeColor(args.vg,borderColor);
    nvgFill(args.vg);
    nvgStroke(args.vg);
    nvgFontSize(args.vg,box.size.y-2);
    nvgFontFaceId(args.vg,font->handle);
    NVGcolor textColor=nvgRGB(0xff,0xff,0xff);
    nvgTextAlign(args.vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
    nvgFillColor(args.vg,textColor);
    nvgText(args.vg,box.size.x/2.f,box.size.y/2.f,std::to_string(key).c_str(),NULL);
  }
};

struct ScaleDisplay : OpaqueWidget {
  MouseSeq *module;
  Vec buttonSize;

  ScaleDisplay(MouseSeq *m,int nr,Vec _pos) : module(m) {
    box.pos=_pos;
    box.size=Vec(20,10*12);
    for(int k=0;k<12;k++) {
      addChild(new ScaleButton(module,nr,k,Vec(0,(11-k)*10),Vec(20,10)));
    }
  }
};
struct MouseSeqPresetSelect : SpinParamWidget {
  MouseSeq *module=nullptr;
  MouseSeqPresetSelect() {
    init();
  }
  void onChange(const ChangeEvent &e) override {
    if(module) {
      module->setCurrentPattern();
    }
  }
};
struct MouseSeqWidget : ModuleWidget {
  MouseSeqWidget(MouseSeq *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/MouseSeq.svg")));

    //addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,0)));
    //addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,0)));
    //addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));
    //addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));

    auto display=new XYDisplay(module,mm2px(Vec(4,4)),mm2px(Vec(70,120)));
    addChild(display);
    float y=18;
    float x=80;
    addInput(createInput<SmallPort>(mm2px(Vec(121,17)),module,MouseSeq::CLOCK_SELECT_INPUT));
    auto selectParam=createParam<SelectParamH>(mm2px(Vec(x-1.5,y+0.75)),module,MouseSeq::CUR_CLOCK_PARAM);
    selectParam->box.size=mm2px(Vec(43,3));
    selectParam->init({"1","2","3","4"},4.5);
    addParam(selectParam);
    for(int k=0;k<NUM_CLOCKS;k++) {
      addInput(createInput<SmallPort>(mm2px(Vec(x+k*10,y+5)),module,MouseSeq::CLOCK_INPUT+k));
    }
    y=38;
    addInput(createInput<SmallPort>(mm2px(Vec(121,37)),module,MouseSeq::SCALE_INPUT));
    auto selectParamS=createParam<SelectParamH>(mm2px(Vec(x-1.5,y)),module,MouseSeq::CUR_SCALE_PARAM);
    selectParamS->box.size=mm2px(Vec(43,3));
    selectParamS->init({"1","2","3","4"},4.5);
    addParam(selectParamS);
    for(int k=0;k<NUM_SCALES;k++) {
      auto scaleWidget=new ScaleDisplay(module,k,mm2px(Vec(x+k*10,y+4)));
      addChild(scaleWidget);
    }
    x=122;
    y=50;
    auto patParam=createParam<MouseSeqPresetSelect>(mm2px(Vec(x,y)),module,MouseSeq::PAT_PARAM);
    patParam->module=module;
    addParam(patParam);
    y+=10;
    auto editButton=createParam<SmallButtonWithLabel>(mm2px(Vec(x,y)),module,MouseSeq::LOCK_PARAM);
    editButton->setLabel("Lock");
    addParam(editButton);
    y+=4;
    addInput(createInput<SmallPort>(mm2px(Vec(x+0.4,y)),module,MouseSeq::PAT_INPUT));
    y=80;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,MouseSeq::VEL_OUTPUT));
    y+=12;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,MouseSeq::RTR_OUTPUT));
    y+=12;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,MouseSeq::GATE_OUTPUT));
    y+=12;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,MouseSeq::VOCT_OUTPUT));
    y=94;
    x=80;
    addParam(createParam<MLED>(mm2px(Vec(x,y)),module,MouseSeq::MODE_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+10,y)),module,MouseSeq::GATE_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x+20,y)),module,MouseSeq::Y_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x+30,y)),module,MouseSeq::X_INPUT));

    y=108;
    addParam(createParam<MLEDM>(mm2px(Vec(80,y)),module,MouseSeq::PLAY_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(88,y)),module,MouseSeq::PLAY_INPUT));
    y=108;
    x=100;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,MouseSeq::OCT_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x,y+8)),module,MouseSeq::OCT_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x+10,y)),module,MouseSeq::SEMI_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+10,y+8)),module,MouseSeq::SEMI_INPUT));
  }
};


Model *modelMouseSeq=createModel<MouseSeq,MouseSeqWidget>("MouseSeq");