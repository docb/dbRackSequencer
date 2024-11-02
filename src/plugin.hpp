#pragma once

#include <rack.hpp>
#include <iomanip>
#include <utility>
#include "rnd.h"

using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin *pluginInstance;

struct SelectButton : Widget {
  int _value;
  std::string _label;
  std::basic_string<char> fontPath;
  int fontSize=-1;

  SelectButton(int value,std::string label) : _value(value),_label(std::move(label)) {
    fontPath=asset::plugin(pluginInstance,"res/FreeMonoBold.ttf");
  }

  void draw(const DrawArgs &args) override {
    std::shared_ptr<Font> font=APP->window->loadFont(fontPath);
    int currentValue=0;
    auto paramWidget=getAncestorOfType<ParamWidget>();
    assert(paramWidget);
    engine::ParamQuantity *pq=paramWidget->getParamQuantity();
    if(pq)
      currentValue=std::round(pq->getValue());

    if(currentValue==_value) {
      nvgFillColor(args.vg,nvgRGB(0x7e,0xa6,0xd3));
    } else {
      nvgFillColor(args.vg,nvgRGB(0x3c,0x4c,0x71));
    }
    nvgStrokeColor(args.vg,nvgRGB(0xc4,0xc9,0xc2));
    nvgBeginPath(args.vg);
    nvgRoundedRect(args.vg,0,0,box.size.x,box.size.y,3.f);
    nvgFill(args.vg);
    nvgStroke(args.vg);
    if(fontSize<0)
      nvgFontSize(args.vg,box.size.y-2);
    else
      nvgFontSize(args.vg,fontSize);
    nvgFontFaceId(args.vg,font->handle);
    NVGcolor textColor=nvgRGB(0xff,0xff,0xaa);
    nvgTextAlign(args.vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
    nvgFillColor(args.vg,textColor);
    nvgText(args.vg,box.size.x/2.f+0.4f,box.size.y/2.f+0.4f,_label.c_str(),NULL);
  }

  void onDragHover(const event::DragHover &e) override {
    if(e.button==GLFW_MOUSE_BUTTON_LEFT) {
      e.consume(this);
    }
    Widget::onDragHover(e);
  }

  void onDragEnter(const event::DragEnter &e) override;
};

struct SelectParam : ParamWidget {

  void init(std::vector<std::string> labels) {
    const float margin=0;
    float height=box.size.y-2*margin;
    unsigned int len=labels.size();
    for(unsigned int i=0;i<len;i++) {
      auto selectButton=new SelectButton(i,labels[i]);
      selectButton->box.pos=Vec(0,height/len*i+margin);
      selectButton->box.size=Vec(box.size.x,height/len);
      addChild(selectButton);
    }
  }

  void initWithEmptyLabels(unsigned int len) {
    const float margin=0;
    float height=box.size.y-2*margin;
    for(unsigned int i=0;i<len;i++) {
      auto selectButton=new SelectButton(i,"");
      selectButton->box.pos=Vec(0,(height/len)*i+margin);
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
};

struct SelectButtonH : SelectButton {
  SelectButtonH(int nr,std::string label) : SelectButton(nr,label) {
  }

  void onDragEnter(const event::DragEnter &e) override;
};

struct SelectParamH : ParamWidget {
  void init(std::vector<std::string> labels,float margin=0.f) {

    float width=box.size.x-2*margin;
    unsigned int len=labels.size();
    for(unsigned int i=0;i<len;i++) {
      auto selectButton=new SelectButtonH(i,labels[i]);
      selectButton->fontSize=8;
      selectButton->box.pos=Vec(width/len*i+margin,0);
      selectButton->box.size=Vec(width/len-2*margin,box.size.y);
      addChild(selectButton);
    }
  }

  /*
  void init(std::vector<std::string> labels) {
    const float margin = 0;
    float width = box.size.x - 2 * margin;
    unsigned int len = labels.size();
    for (unsigned int i = 0; i < len; i++) {
      auto selectButton = new SelectButtonH(i,labels[i]);
      selectButton->fontSize=8;
      selectButton->box.pos = Vec(width/len*i+margin,0);
      selectButton->box.size = Vec(width/len,box.size.y);
      addChild(selectButton);
    }
  }
   */

  void draw(const DrawArgs &args) override {
    // Background
    nvgBeginPath(args.vg);
    nvgRect(args.vg,0,0,box.size.x,box.size.y);
    nvgFillColor(args.vg,nvgRGB(0,0,0));
    nvgFill(args.vg);

    ParamWidget::draw(args);
  }
};

struct TrimbotWhite : SvgKnob {
  TrimbotWhite() {
    minAngle=-0.8*M_PI;
    maxAngle=0.8*M_PI;
    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/TrimpotWhite.svg")));
  }
};

struct TrimbotWhite9 : SvgKnob {
  TrimbotWhite9() {
    minAngle=-0.8*M_PI;
    maxAngle=0.8*M_PI;
    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/TrimpotWhite9mm.svg")));
  }
};

struct SmallPort : app::SvgPort {
  SmallPort() {
    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/SmallPort.svg")));
  }
};

struct HiddenPort : app::SvgPort {
  HiddenPort() {
    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/HiddenPort.svg")));
  }
};

struct SmallButton : SvgSwitch {
  SmallButton() {
    momentary=false;
    addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/SmallButton0.svg")));
    addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/SmallButton1.svg")));
    fb->removeChild(shadow);
    delete shadow;
  }
};

template<typename TBase = GrayModuleLightWidget,int R=0,int G=0,int B=0>
struct TLight : TBase {
  TLight() {
    this->addBaseColor(nvgRGB(R,G,B));
  }
};

template<typename TBase>
struct DBSmallLight : TSvgLight<TBase> {
  DBSmallLight() {
    this->setSvg(Svg::load(asset::plugin(pluginInstance,"res/SmallLight.svg")));
  }
};

template<typename TBase>
struct DBLight9px : TSvgLight<TBase> {
  DBLight9px() {
    this->setSvg(Svg::load(asset::plugin(pluginInstance,"res/light_9px_transparent.svg")));
  }
};

template<typename TBase>
struct DBMediumLight : TSvgLight<TBase> {
  DBMediumLight() {
    this->setSvg(Svg::load(asset::plugin(pluginInstance,"res/MediumLight.svg")));
  }
};

struct IntSelectItem : MenuItem {
  int *value;
  int min;
  int max;

  IntSelectItem(int *val,int _min,int _max) : value(val),min(_min),max(_max) {
  }

  Menu *createChildMenu() override {
    Menu *menu=new Menu;
    for(int c=min;c<=max;c++) {
      menu->addChild(createCheckMenuItem(string::f("%d",c),"",[=]() {
        return *value==c;
      },[=]() {
        *value=c;
      }));
    }
    return menu;
  }
};

struct LabelIntSelectItem : MenuItem {
  int *value;
  std::vector<std::string> labels;

  LabelIntSelectItem(int *val,std::vector<std::string> _labels) : value(val),labels(std::move(_labels)) {
  }

  Menu *createChildMenu() override {
    Menu *menu=new Menu;
    for(unsigned k=0;k<labels.size();k++) {
      menu->addChild(createCheckMenuItem(labels[k],"",[=]() {
        return *value==int(k);
      },[=]() {
        *value=k;
      }));
    }
    return menu;
  }
};

struct SmallButtonWithLabel : SvgSwitch {
  std::string label;
  std::basic_string<char> fontPath;

  SmallButtonWithLabel() {
    fontPath=asset::plugin(pluginInstance,"res/FreeMonoBold.ttf");
    momentary=false;
    addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/SmallButton0.svg")));
    addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/SmallButton1.svg")));
    fb->removeChild(shadow);
    delete shadow;
  }

  void setLabel(const std::string &lbl) {
    label=lbl;
  }

  void draw(const DrawArgs &args) override;

};

struct SmallButtonWithLabelV : SvgSwitch {
  std::string label;
  std::basic_string<char> fontPath;

  SmallButtonWithLabelV() {
    fontPath=asset::plugin(pluginInstance,"res/FreeMonoBold.ttf");
    momentary=false;
    addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/SmallButton0V.svg")));
    addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/SmallButton1V.svg")));
    fb->removeChild(shadow);
    delete shadow;
  }

  void setLabel(const std::string &lbl) {
    label=lbl;
  }

  void draw(const DrawArgs &args) override;

};

struct SmallRoundButton : SvgSwitch {
  SmallRoundButton() {
    momentary=false;
    addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/button_9px_off.svg")));
    addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/button_9px_active.svg")));
    fb->removeChild(shadow);
    delete shadow;
  }
};

struct MLED : public SvgSwitch {
  MLED() {
    momentary=false;
    //latch=true;
    addFrame(Svg::load(asset::plugin(pluginInstance,"res/RButton0.svg")));
    addFrame(Svg::load(asset::plugin(pluginInstance,"res/RButton1.svg")));
  }
};

struct MLEDM : public SvgSwitch {
  MLEDM() {
    momentary=true;
    //latch=true;
    addFrame(Svg::load(asset::plugin(pluginInstance,"res/RButton0.svg")));
    addFrame(Svg::load(asset::plugin(pluginInstance,"res/RButton1.svg")));
  }
};

template<typename M>
struct CopyButton : SmallButtonWithLabel {
  M *module;

  CopyButton() : SmallButtonWithLabel() {
    momentary=true;
  }

  void onChange(const ChangeEvent &e) override {
    SvgSwitch::onChange(e);
    if(module) {
      if(module->params[M::COPY_PARAM].getValue()>0)
        module->copy();
    }
  }
};
template<typename M>
struct InsertButton : SmallButtonWithLabel {
  M *module;

  InsertButton() : SmallButtonWithLabel() {
    momentary=true;
  }

  void onChange(const ChangeEvent &e) override {
    SvgSwitch::onChange(e);
    if(module) {
      if(module->params[M::INSERT_PARAM].getValue()>0)
        module->insert();
    }
  }
};
template<typename M>
struct DelButton : SmallButtonWithLabel {
  M *module;

  DelButton() : SmallButtonWithLabel() {
    momentary=true;
  }

  void onChange(const ChangeEvent &e) override {
    SvgSwitch::onChange(e);
    if(module) {
      if(module->params[M::DELETE_PARAM].getValue()>0)
        module->del();
    }
  }
};
template<typename M>
struct PasteButton : SmallButtonWithLabel {
  M *module;

  PasteButton() : SmallButtonWithLabel() {
    momentary=true;
  }

  void onChange(const ChangeEvent &e) override {
    SvgSwitch::onChange(e);
    if(module) {
      if(module->params[M::PASTE_PARAM].getValue()>0)
        module->paste();
    }

  }
};

struct UpButtonWidget : Widget {
  bool pressed=false;

  void draw(const DrawArgs &args) override {
    nvgFillColor(args.vg,pressed?nvgRGB(126,200,211):nvgRGB(126,166,211));
    nvgStrokeColor(args.vg,nvgRGB(196,201,104));
    nvgBeginPath(args.vg);
    nvgMoveTo(args.vg,2,box.size.y);
    nvgLineTo(args.vg,box.size.x/2.f,2);
    nvgLineTo(args.vg,box.size.x-2,box.size.y);
    nvgClosePath(args.vg);
    nvgFill(args.vg);
    nvgStroke(args.vg);
  }

  void onDragHover(const event::DragHover &e) override {
    if(e.button==GLFW_MOUSE_BUTTON_LEFT) {
      e.consume(this);
    }
    Widget::onDragHover(e);
  }

  void onButton(const ButtonEvent &e) override;


};

struct DownButtonWidget : Widget {
  bool pressed=false;

  void draw(const DrawArgs &args) override {

    nvgFillColor(args.vg,pressed?nvgRGB(126,200,211):nvgRGB(126,166,211));
    nvgStrokeColor(args.vg,nvgRGB(196,201,104));
    nvgBeginPath(args.vg);
    nvgMoveTo(args.vg,2,0);
    nvgLineTo(args.vg,box.size.x/2.f,box.size.y-2);
    nvgLineTo(args.vg,box.size.x-2,0);
    nvgClosePath(args.vg);
    nvgFill(args.vg);
    nvgStroke(args.vg);
  }

  void onDragHover(const event::DragHover &e) override {
    if(e.button==GLFW_MOUSE_BUTTON_LEFT) {
      e.consume(this);
    }
    Widget::onDragHover(e);
  }

  void onButton(const ButtonEvent &e) override;
};

struct NumberDisplayWidget : Widget {
  std::basic_string<char> fontPath;
  bool highLight=false;

  NumberDisplayWidget() : Widget() {
    fontPath=asset::plugin(pluginInstance,"res/FreeMonoBold.ttf");
  }

  void drawLayer(const DrawArgs &args,int layer) override {
    if(layer==1) {
      _draw(args);
    }
    Widget::drawLayer(args,layer);
  }

  void onDoubleClick(const DoubleClickEvent &e) override {
    INFO("double click - does not work");
    auto paramWidget=getAncestorOfType<ParamWidget>();
    assert(paramWidget);
    paramWidget->resetAction();
    e.consume(this);
  }

  void onButton(const ButtonEvent &e) override {
    INFO("display on button");
    e.unconsume();
  }

  void _draw(const DrawArgs &args) {
    std::shared_ptr<Font> font=APP->window->loadFont(fontPath);
    auto paramWidget=getAncestorOfType<ParamWidget>();
    assert(paramWidget);
    engine::ParamQuantity *pq=paramWidget->getParamQuantity();
    nvgBeginPath(args.vg);
    nvgRect(args.vg,0,0,box.size.x,box.size.y);
    nvgStrokeColor(args.vg,nvgRGB(128,128,128));
    nvgStroke(args.vg);

    if(pq) {
      std::stringstream stream;
      stream<<std::setfill('0')<<std::setw(2)<<int(pq->getValue());
      nvgFillColor(args.vg,highLight?nvgRGB(255,255,128):nvgRGB(0,255,0));
      nvgFontFaceId(args.vg,font->handle);
      nvgFontSize(args.vg,10);
      nvgTextAlign(args.vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
      nvgText(args.vg,box.size.x/2,box.size.y/2,stream.str().c_str(),NULL);
    }
  }
};

struct SpinParamWidget : ParamWidget {
  bool pressed=false;
  int start;
  float dragY;
  UpButtonWidget *up;
  DownButtonWidget *down;
  NumberDisplayWidget *text;

  void init() {
    box.size.x=20;
    box.size.y=30;
    up=new UpButtonWidget();
    up->box.pos=Vec(0,0);
    up->box.size=Vec(20,10);
    addChild(up);
    text=new NumberDisplayWidget();
    text->box.pos=Vec(0,10);
    text->box.size=Vec(20,10);
    addChild(text);
    down=new DownButtonWidget();
    down->box.pos=Vec(0,20);
    down->box.size=Vec(20,10);
    addChild(down);
  }

  void onButton(const ButtonEvent &e) override {
    ParamWidget::onButton(e);
    if(e.action==GLFW_PRESS&&e.button==GLFW_MOUSE_BUTTON_LEFT) {
      engine::ParamQuantity *pq=getParamQuantity();
      start=pq->getValue();
      pressed=true;
    }
    if(e.action==GLFW_RELEASE&&e.button==GLFW_MOUSE_BUTTON_LEFT) {
      pressed=false;
    }
    e.consume(this);
  }

  void onDoubleClick(const DoubleClickEvent &e) override {

  }

  void onDragStart(const event::DragStart &e) override {
    if(pressed) {
      dragY=APP->scene->rack->getMousePos().y;
      text->highLight=true;
    }
  }

  void onDragEnd(const event::DragEnd &e) override {
    up->pressed=false;
    down->pressed=false;
    text->highLight=false;
  }

  void onDragMove(const event::DragMove &e) override {
    if(pressed) {
      float diff=dragY-APP->scene->rack->getMousePos().y;
      engine::ParamQuantity *pq=getParamQuantity();
      float newValue=float(start)+diff/4.0f;
      if(newValue>pq->getMaxValue())
        pq->setValue(pq->getMaxValue());
      else if(newValue<pq->getMinValue())
        pq->setValue(pq->getMinValue());
      else
        pq->setValue(newValue);
    }
  }

  void draw(const DrawArgs &args) override {
    // Background
    nvgBeginPath(args.vg);
    nvgRect(args.vg,0,0,box.size.x,box.size.y);
    nvgFillColor(args.vg,nvgRGB(0,0,0));
    nvgStrokeColor(args.vg,nvgRGB(128,128,128));
    nvgFill(args.vg);
    nvgStroke(args.vg);

    ParamWidget::draw(args);
  }

};

template<typename T>
struct DensQuantity : Quantity {
  T *module;

  DensQuantity(T *m) : module(m) {
  }

  void setValue(float value) override {
    value=clamp(value,getMinValue(),getMaxValue());
    if(module) {
      module->randomDens=value;
    }
  }

  float getValue() override {
    if(module) {
      return module->randomDens;
    }
    return 0.5f;
  }

  float getMinValue() override {
    return 0.0f;
  }

  float getMaxValue() override {
    return 1.0f;
  }

  float getDefaultValue() override {
    return 0.5f;
  }

  float getDisplayValue() override {
    return getValue()*100.f;
  }

  void setDisplayValue(float displayValue) override {
    setValue(displayValue/100.f);
  }

  std::string getLabel() override {
    return "Random density";
  }

  std::string getUnit() override {
    return "%";
  }
};

template<typename T>
struct DensSlider : ui::Slider {
  DensSlider(T *module) {
    quantity=new DensQuantity<T>(module);
    box.size.x=200.0f;
  }

  virtual ~DensSlider() {
    delete quantity;
  }
};

template<typename T>
struct DensMenuItem : MenuItem {
  T *module;

  DensMenuItem(T *m) : module(m) {
    this->text="Random";
    this->rightText="▸";
  }

  Menu *createChildMenu() override {
    Menu *menu=new Menu;
    menu->addChild(new DensSlider<T>(module));
    return menu;
  }
};

struct MinMaxRange {
  float min;
  float max;
};

template<typename M>
struct RangeSelectItem : MenuItem {
  M *module;
  std::vector<MinMaxRange> ranges;

  RangeSelectItem(M *_module,std::vector<MinMaxRange> _ranges) : module(_module),ranges(std::move(_ranges)) {
  }

  Menu *createChildMenu() override {
    Menu *menu=new Menu;
    for(unsigned int k=0;k<ranges.size();k++) {
      menu->addChild(createCheckMenuItem(string::f("%0.1f/%0.1fV",ranges[k].min,ranges[k].max),"",[=]() {
        return module->min==ranges[k].min&&module->max==ranges[k].max;
      },[=]() {
        module->min=ranges[k].min;
        module->max=ranges[k].max;
        module->reconfig();
      }));
    }
    return menu;
  }
};

template<typename M>
struct MKnob : TrimbotWhite {
  M *module=nullptr;

  void step() override {
    if(module&&module->dirty) {
      ChangeEvent c;
      SvgKnob::onChange(c);
      module->dirty--;
    }
    SvgKnob::step();
  }
};

struct UpdateOnReleaseKnob : TrimbotWhite9 {
  bool *update=nullptr;
  bool contextMenu;

  UpdateOnReleaseKnob() : TrimbotWhite9() {

  }

  void onButton(const ButtonEvent &e) override {
    if(e.action==GLFW_PRESS&&e.button==GLFW_MOUSE_BUTTON_RIGHT&&(e.mods&RACK_MOD_MASK)==0) {
      contextMenu=true;
    } else {
      contextMenu=false;
    }
    Knob::onButton(e);
  }

  void onChange(const ChangeEvent &e) override {
    SvgKnob::onChange(e);
    if(update!=nullptr)
      *update=contextMenu;
  }

  void onDragEnd(const DragEndEvent &e) override {
    SvgKnob::onDragEnd(e);
    if(e.button==GLFW_MOUSE_BUTTON_LEFT) {
      if(update!=nullptr)
        *update=true;
    }

  }
};

template<typename M>
struct SizeSelectItem : MenuItem {
  M *module;
  std::vector<int> sizes;

  SizeSelectItem(M *_module,std::vector<int> _sizes) : module(_module),sizes(std::move(_sizes)) {
  }

  Menu *createChildMenu() override {
    Menu *menu=new Menu;
    for(unsigned int k=0;k<sizes.size();k++) {
      menu->addChild(createCheckMenuItem(string::f("%d",sizes[k]),"",[=]() {
        return module->getSize()==sizes[k];
      },[=]() {
        module->setSize(sizes[k]);
      }));
    }
    return menu;
  }
};

template<typename T,size_t S>
struct ShiftRegister {
  T shiftReg[S];

  ShiftRegister() {
    reset();
  }

  void reset(T value=0) {
    for(unsigned k=0;k<S;k++) {
      shiftReg[k]=value;
    }
  }

  void step(T value) {
    for(int k=S-1;k>0;k--) {
      shiftReg[k]=shiftReg[k-1];
    }
    shiftReg[0]=value;
  }

  T get(unsigned pos) {
    if(pos<S)
      return shiftReg[pos];
    return false;
  }

  void set(unsigned pos,T v) {
    if(pos<S)
      shiftReg[pos]=v;
  }
};

struct SchmittTrigger2 {
  // UNKNOWN is used to represent a stable state when the previous state is not yet set
  enum {
    UNKNOWN,LOW,HIGH
  }state=UNKNOWN;
  float low=0.0;
  float high=1.0;

  void setThresholds(float low,float high) {
    this->low=low;
    this->high=high;
  }

  int process(float in) {
    switch(state) {
      case LOW:
        if(in>=high) {
          state=HIGH;
          return 1;
        }
        break;
      case HIGH:
        if(in<=low) {
          state=LOW;
          return -1;
        }
        break;
      default:
        if(in>=high) {
          state=HIGH;
        } else if(in<=low) {
          state=LOW;
        }
        break;
    }
    return 0;
  }

  void reset() {
    state=UNKNOWN;
  }
};

template<typename TBase = app::ModuleLightWidget>
struct TTransparentLightWidget : TBase {
  TTransparentLightWidget() {
    this->bgColor=nvgRGBA(0x33,0x33,0x33,0x0);
    this->borderColor=nvgRGBA(0,0,0,0);
  }
};

using TransparentLightWidget=TTransparentLightWidget<>;

template<typename TBase = TransparentLightWidget,int R=0,int G=0,int B=0>
struct TDBLight : TBase {
  TDBLight() {
    this->addBaseColor(nvgRGB(R,G,B));
  }
};

using DBRedLight=TDBLight<TransparentLightWidget,0xff,0x0,0x0>;
using DBYellowLight=TDBLight<TransparentLightWidget,0xff,0xff,0x0>;
using DBGreenLight=TDBLight<TransparentLightWidget,0x0,0xff,0x0>;
using DBOrangeLight=TDBLight<TransparentLightWidget,0xff,0x88,0x22>;
using DBTurkLight=TDBLight<TransparentLightWidget,0x0,0xff,0xff>;
using DBPurpleLight=TDBLight<TransparentLightWidget,0xd5,0x2b,0xed>;

struct CellColors {
  NVGcolor selectOnColor=nvgRGB(0xff,0xff,0xff);
  NVGcolor selectOffColor=nvgRGB(0x44,0x44,0xaa);
  NVGcolor chnColors[16]={nvgRGB(255,0,0),nvgRGB(0,255,0),nvgRGB(55,55,255),nvgRGB(255,255,0),nvgRGB(255,0,255),nvgRGB(0,255,255),nvgRGB(128,0,0),nvgRGB(196,85,55),nvgRGB(128,128,80),nvgRGB(255,128,0),nvgRGB(255,0,128),nvgRGB(0,128,255),nvgRGB(128,66,128),nvgRGB(128,255,0),nvgRGB(128,128,255),nvgRGB(128,255,255)};
  std::vector<NVGcolor> palettes[3]={{nvgRGB(0x0,0x00,0x66), nvgRGB(0x0,0x22,0x99), nvgRGB(0x33,0x44,0xAA),nvgRGB(0x00,0x77,0xBB),nvgRGB(0x22,0x77,0xBB),nvgRGB(0x44,0x77,0xBB),nvgRGB(0x55,0x66,0xBB),nvgRGB(0x66,0x44,0xFF),nvgRGB(0x77,0x44,0xFF),nvgRGB(0x88,0x44,0x88),nvgRGB(0x99,0x44,0x55)},
                                     {nvgRGB(0x22,0x22,0x66),nvgRGB(0x44,0xdd,0x44),nvgRGB(0xaa,0xaa,0x44)},
                                     {nvgRGB(0x22,0x88,0x55),nvgRGB(0x9f,0x4b,0x0b),nvgRGB(0x83,0xb8,0x55),nvgRGB(0xdd,0xdd,0x99)}};
};

struct DBTextWidget : Widget {
  std::basic_string<char> fontPath;
  std::string *label;
  DBTextWidget(std::string *_label,Vec _pos,Vec _size) {
    box.pos=_pos;
    box.size=_size;
    label=_label;
    fontPath=asset::plugin(pluginInstance,"res/FreeMonoBold.ttf");
  }

  void drawLayer(const DrawArgs &args,int layer) override {
    if(layer==1) {
      _draw(args);
    }
    Widget::drawLayer(args,layer);
  }


  void _draw(const DrawArgs &args) {
    std::shared_ptr<Font> font=APP->window->loadFont(fontPath);
    nvgFillColor(args.vg,nvgRGB(255,255,128));
    nvgFontFaceId(args.vg,font->handle);
    nvgFontSize(args.vg,10);
    nvgTextAlign(args.vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
    nvgText(args.vg,box.size.x/2,box.size.y/2,label->c_str(),NULL);
  }
};

inline void setImmediateValue(ParamQuantity *pq,float value) {
  if (!pq->module)
    return;
  value = math::clampSafe(value, pq->getMinValue(), pq->getMaxValue());
  if (pq->snapEnabled)
    value = std::round(value);
  APP->engine->setParamValue(pq->module, pq->paramId, value);
}

template<typename M>
struct GateButton : OpaqueWidget {
  M *module;
  int key;
  Tooltip *tooltip=nullptr;
  std::string label;
  NVGcolor onColor=nvgRGB(118,169,118);
  NVGcolor offColor=nvgRGB(55,80,55);
  NVGcolor onColorInactive=nvgRGB(128,128,128);
  NVGcolor offColorInactive=nvgRGB(43,43,43);
  NVGcolor border=nvgRGB(196,201,104);
  NVGcolor borderInactive=nvgRGB(150,150,150);
  std::basic_string<char> fontPath;

  GateButton(M *_module,int _key,Vec pos,Vec size) : module(_module),key(_key) {
    fontPath=asset::plugin(pluginInstance,"res/FreeMonoBold.ttf");
    box.size=size;
    box.pos=pos;
    int g=module?int(!module->gateCountFromZero):1;
    label=string::f("chn %d",key+g);
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
    if(!settings::tooltips)
      return;
    tooltip=new Tooltip;
    tooltip->text=string::f("chn %d",key+int(!module->gateCountFromZero));
    APP->scene->addChild(tooltip);
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

  void drawLayer(const DrawArgs &args,int layer) override {
    if(layer==1) {
      _draw(args);
    }
    Widget::drawLayer(args,layer);
  }

  void _draw(const DrawArgs &args) {
    std::shared_ptr<Font> font=APP->window->loadFont(fontPath);
    NVGcolor color=offColor;
    NVGcolor borderColor=border;
    int g=1;
    if(module) {
      g=int(!module->gateCountFromZero);
      if(key>=module->maxChannels) {
        borderColor=borderInactive;
        if(module->getGateStatus(key)) {
          color=onColorInactive;
        } else {
          color=offColorInactive;
        }
      } else {
        borderColor=border;
        if(module->getGateStatus(key)) {
          color=onColor;
        } else {
          color=offColor;
        }
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
    nvgText(args.vg,box.size.x/2.f,box.size.y/2.f,std::to_string(key+g).c_str(),NULL);
  }
};

template<typename M>
struct GateDisplay : OpaqueWidget {
  M *module;
  Vec buttonSize;

  GateDisplay(M *m,Vec _pos) : module(m) {
    box.pos=_pos;
    box.size=Vec(20,10*16);
    for(int k=0;k<16;k++) {
      addChild(new GateButton<M>(module,k,Vec(0,k*10),Vec(20,10)));
    }
  }
};



#define MHEIGHT 128.5f
#define TY(x) MHEIGHT-(x)-6.237
#define TWOPIF 6.2831853f
