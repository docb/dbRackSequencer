#include "plugin.hpp"
#include "rnd.h"

#define MAX_SIZE 32

struct SubMatrix {
  char grid[MAX_SIZE][MAX_SIZE];
  int sizeX;
  int sizeY;

  void clear() {
    for(int k=0;k<MAX_SIZE;k++) {
      for(int j=0;j<MAX_SIZE;j++) {
        grid[k][j]=32;
      }
    }
  }
};

struct Matrix {
  RND rnd;
  char grid[MAX_SIZE][MAX_SIZE];
  SubMatrix clip;

  Matrix() {
    clear();
  }

  void copy(int posY,int posX,int selY,int selX,bool del=false) {
    clip.clear();
    int x0=std::min(posX,selX);
    int x1=std::max(posX,selX);
    int y0=std::min(posY,selY);
    int y1=std::max(posY,selY);
    clip.sizeY=y1-y0+1;
    clip.sizeX=x1-x0+1;
    int x=0;
    int y=0;
    for(int k=y0;k<=y1;k++) {
      for(int j=x0;j<=x1;j++) {
        clip.grid[y][x]=grid[k][j];
        if(del)
          grid[k][j]=32;
        x++;
      }
      y++;
      x=0;
    }
  }

  void cut(int posY,int posX,int selY,int selX) {
    copy(posY,posX,selY,selX,true);
  }

  void paste(int posY,int posX) {
    for(int k=0;k<clip.sizeY;k++) {
      for(int j=0;j<clip.sizeX;j++) {
        if(posY+k<MAX_SIZE&&posX+j<MAX_SIZE)
          grid[posY+k][posX+j]=clip.grid[k][j];
      }
    }
  }

  void set(int k,int j,char c) {
    grid[k][j]=c;
  }

  void inc(int k,int j) {
    if(grid[k][j]+1<127) grid[k][j]++;
  }
  void dec(int k,int j) {
    if(grid[k][j]-1>33) grid[k][j]--;
  }

  char get(int k,int j) {
    return grid[k][j];
  }


  void clear() {
    for(int k=0;k<MAX_SIZE;k++) {
      for(int j=0;j<MAX_SIZE;j++) {
        grid[k][j]=32;
      }
    }
  }

  bool isOn(int row,int col) {
    return grid[row][col]>32;
  }

  std::string toString() {
    std::string ret;
    for(int k=0;k<MAX_SIZE;k++) {
      for(int j=0;j<MAX_SIZE;j++) {
        ret+=get(k,j);
      }
    }
    return ret;
  }

  void fromString(std::string s) {
    clear();
    for(unsigned k=0;k<s.length();k++) {
      set(k/MAX_SIZE,k%MAX_SIZE,s[k]);
    }
  }

  void randomize(float dens,int from,int to) {
    for(int x=0;x<MAX_SIZE;x++) {
      for(int y=0;y<MAX_SIZE;y++) {
        if(rnd.nextCoin(1.0-dens)) {
          grid[x][y]=rnd.nextRange(from,to);
        } else {
          grid[x][y]=32;
        }
      }
    }
  }

};

struct TheMatrix : Module {
  enum ParamId {
    CV_X_PARAM,CV_Y_PARAM,DENS_PARAM,RND_PARAM,LEVEL_PARAM,FROM_PARAM,TO_PARAM,PARAMS_LEN
  };
  enum InputId {
    CV_X_INPUT,CV_Y_INPUT,DENS_INPUT,LEVEL_INPUT,RND_INPUT,TRIG_INPUT,VOCT_INPUT,INPUTS_LEN
  };
  enum OutputId {
    CV_OUTPUT,GATE_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };

  Matrix m;
  int curRow[16]={};
  int curCol[16]={};
  int channels=0;
  dsp::SchmittTrigger rndTrigger;
  dsp::SchmittTrigger extTrigger;
  dsp::SchmittTrigger manualRndTrigger;
  float voct=0.f;
  bool extNote=false;
  bool saveToHistory=false;

  TheMatrix() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configParam(CV_X_PARAM,0,MAX_SIZE-1,0,"X");
    getParamQuantity(CV_X_PARAM)->snapEnabled=true;
    configParam(CV_Y_PARAM,0,MAX_SIZE-1,0,"Y");
    getParamQuantity(CV_Y_PARAM)->snapEnabled=true;
    configParam(DENS_PARAM,0,1,0.9,"Random Density");
    configParam(LEVEL_PARAM,0.01,1,0.1,"Out Level Factor");
    configParam(FROM_PARAM,33,126,65,"Random Range From");
    getParamQuantity(FROM_PARAM)->snapEnabled=true;
    configParam(TO_PARAM,33,126,90,"Random Range To");
    getParamQuantity(TO_PARAM)->snapEnabled=true;
    configInput(CV_X_INPUT,"CV X");
    configInput(CV_Y_INPUT,"CV_Y");
    configInput(TRIG_INPUT,"Trig");
    configInput(VOCT_INPUT,"V/OCT");
    configInput(DENS_INPUT,"Random Density");
    configInput(LEVEL_INPUT,"Out Level Factor");
    configOutput(GATE_OUTPUT,"Gate");
    configOutput(CV_OUTPUT,"CV");
  }

  std::vector<int> getSelected(int row,int col) {
    std::vector<int> ret;
    for(int chn=0;chn<channels;chn++) {
      if(row==curRow[chn]&&col==curCol[chn]) {
        ret.push_back(chn);
      }
    }
    return ret;
  }

  void process(const ProcessArgs &args) override {
    if(inputs[DENS_INPUT].isConnected()) {
      getParamQuantity(DENS_PARAM)->setValue(inputs[DENS_INPUT].getVoltage()/10.f);
    }
    if(inputs[LEVEL_INPUT].isConnected()) {
      getParamQuantity(LEVEL_PARAM)->setValue(inputs[LEVEL_INPUT].getVoltage()/10.f);
    }
    if(extTrigger.process(inputs[TRIG_INPUT].getVoltage())) {
      voct=inputs[VOCT_INPUT].getVoltage();
      extNote=true;
    }
    int channelsX=0;
    if(inputs[CV_X_INPUT].isConnected()) {
      channelsX=inputs[CV_X_INPUT].getChannels();
      for(int chn=0;chn<16;chn++) {
        int index=int(inputs[CV_X_INPUT].getVoltage(chn)/10.f*float(MAX_SIZE));
        index+=params[CV_X_PARAM].getValue();
        while(index<0)
          index+=MAX_SIZE;
        index%=MAX_SIZE;
        curCol[chn]=index;
      }
    } else {
      curCol[0]=(int(params[CV_X_PARAM].getValue())%MAX_SIZE);
      for(int chn=1;chn<16;chn++) curCol[chn]=0;
    }
    int channelsY=0;
    if(inputs[CV_Y_INPUT].isConnected()) {
      channelsY=inputs[CV_Y_INPUT].getChannels();
      for(int chn=0;chn<16;chn++) {
        int index=int(inputs[CV_Y_INPUT].getVoltage(chn)/10.f*float(MAX_SIZE));
        index+=params[CV_Y_PARAM].getValue();
        while(index<0)
          index+=MAX_SIZE;
        index%=MAX_SIZE;
        curRow[chn]=index;
      }
    } else {
      curRow[0]=(int(params[CV_Y_PARAM].getValue())%MAX_SIZE);
      for(int chn=1;chn<16;chn++) curRow[chn]=0;
    }
    channels=std::max(std::max(channelsX,channelsY),1);
    if(rndTrigger.process(inputs[RND_INPUT].getVoltage())) {
      randomize();
    }
    for(int chn=0;chn<channels;chn++) {
      int curVal=m.get(curRow[chn],curCol[chn]);
      if(curVal>32) {
        outputs[CV_OUTPUT].setVoltage((curVal-80)*params[LEVEL_PARAM].getValue(),chn);
        outputs[GATE_OUTPUT].setVoltage(10.f,chn);
      } else {
        outputs[GATE_OUTPUT].setVoltage(0.f,chn);
      }
    }
    outputs[CV_OUTPUT].setChannels(channels);
    outputs[GATE_OUTPUT].setChannels(channels);
  }

  void randomize() {
    m.randomize(params[DENS_PARAM].getValue(),params[FROM_PARAM].getValue(),params[TO_PARAM].getValue());
  }

  json_t *dataToJson() override {
    json_t *data=json_object();
    json_object_set_new(data,"matrix",json_string(m.toString().c_str()));
    return data;
  }

  void dataFromJson(json_t *rootJ) override {
    json_t *jMatrix=json_object_get(rootJ,"matrix");
    if(jMatrix) {
      m.fromString(std::string(json_string_value(jMatrix)));
    }
  }

};

struct MatrixDisplay : OpaqueWidget {
  TheMatrix *theMatrix=nullptr;
  std::basic_string<char> fontPath;
  const int cellXSize=11;
  const int cellYSize=11;
  int posX=0;
  int posY=0;
  int selX=0;
  int selY=0;
  int margin=2;
  NVGcolor chnColors[16]={nvgRGB(200,0,0),nvgRGB(0,160,0),nvgRGB(55,55,200),nvgRGB(200,200,0),nvgRGB(200,0,200),nvgRGB(0,200,200),nvgRGB(128,0,0),nvgRGB(196,85,55),nvgRGB(128,128,80),nvgRGB(255,128,0),nvgRGB(255,0,128),nvgRGB(0,128,255),nvgRGB(128,66,128),nvgRGB(128,255,0),nvgRGB(128,128,255),nvgRGB(128,200,200)};
  json_t *oldModuleJson;

  MatrixDisplay(TheMatrix *module,Vec pos) : theMatrix(module) {
    fontPath=asset::plugin(pluginInstance,"res/FreeMonoBold.ttf");
    box.size=Vec(MAX_SIZE*cellXSize+margin*2,MAX_SIZE*cellYSize+margin*2);
    box.pos=pos;
  }

  void drawLayer(const DrawArgs &args,int layer) override {
    if(layer==1) {
      _draw(args);
    }
    Widget::drawLayer(args,layer);
  }

  void drawCell(const DrawArgs &args,std::shared_ptr <Font> font,char c,int row,int col) {
    char buf[2];
    buf[0]=c;
    buf[1]=0;
    nvgFontSize(args.vg,12);
    nvgFontFaceId(args.vg,font->handle);
    NVGcolor textColor=nvgRGB(0xff,0xff,0xff);
    nvgTextAlign(args.vg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
    nvgFillColor(args.vg,textColor);
    nvgText(args.vg,col*cellXSize+margin+1,row*cellYSize+margin-1,buf,NULL);
  }

  void drawSelection(const DrawArgs &args) {
    if(posX!=selX||posY!=selY) {
      int x0=std::min(posX,selX);
      int x1=std::max(posX,selX);
      int y0=std::min(posY,selY);
      int y1=std::max(posY,selY);
      int h=y1-y0+1;
      int w=x1-x0+1;
      nvgBeginPath(args.vg);
      NVGcolor bg=nvgRGB(0x00,0xaa,0xaa);
      nvgStrokeColor(args.vg,bg);
      nvgRect(args.vg,x0*cellXSize+1,y0*cellYSize+margin,cellXSize*w,cellYSize*h);
      nvgStroke(args.vg);
    }
  }

  void _draw(const DrawArgs &args) {
    if(!theMatrix)
      return;
    if(theMatrix->extNote) {
      save();
      theMatrix->m.set(posY,posX,char(roundf(clamp(theMatrix->voct*12+80.f,33.f,126.f))));
      theMatrix->extNote=false;
      pushHistory();
      goRight();
    }
    std::shared_ptr <Font> font=APP->window->loadFont(fontPath);
    nvgBeginPath(args.vg);
    NVGcolor backColor=nvgRGB(0x20,0x20,0x4c);
    nvgFillColor(args.vg,backColor);
    nvgRect(args.vg,0,0,box.size.x,box.size.y);
    nvgFill(args.vg);

    for(int k=0;k<MAX_SIZE;k++) {
      for(int j=0;j<MAX_SIZE;j++) {
        std::vector<int> selected=theMatrix->getSelected(k,j);
        NVGcolor color;
        if(selected.size()>0) {
          color=chnColors[selected[0]];
          nvgBeginPath(args.vg);
          nvgFillColor(args.vg,color);
          nvgRect(args.vg,j*cellXSize+1,k*cellYSize+margin,cellXSize,cellYSize);
          nvgFill(args.vg);
        }

        drawCell(args,font,theMatrix->m.get(k,j),k,j);
      }
    }
    nvgBeginPath(args.vg);
    NVGcolor bg=nvgRGB(0xaa,0xaa,0xaa);
    nvgStrokeColor(args.vg,bg);
    nvgRect(args.vg,posX*cellXSize+1,posY*cellYSize+margin,cellXSize,cellYSize);
    nvgStroke(args.vg);
    drawSelection(args);
  }

  void goLeft(bool updateSelect=true) {
    if(posX==0) {
      if(posY>0) {
        posX=MAX_SIZE-1;
        posY--;
      }
    } else {
      posX--;
    }
    if(updateSelect) {
      selX=posX;
      selY=posY;
    }
  }

  void goRight(bool updateSelect=true) {
    if(posX==MAX_SIZE-1) {
      if(posY<MAX_SIZE-1) {
        posX=0;
        posY++;
      }
    } else {
      posX++;
    }
    if(updateSelect) {
      selX=posX;
      selY=posY;
    }
  }

  void setPosition(Vec mouse) {
    posX=mouse.x/cellXSize;
    if(posX>=MAX_SIZE)
      posX=MAX_SIZE-1;
    posY=mouse.y/cellYSize;
    if(posY>=MAX_SIZE)
      posY=MAX_SIZE-1;
  }

  void onButton(const ButtonEvent &e) override {
    OpaqueWidget::onButton(e);
    if(e.action==GLFW_PRESS&&e.button==GLFW_MOUSE_BUTTON_LEFT) {
      setPosition(e.pos);
      selX=posX;
      selY=posY;
    }
  }

  void onDragHover(const DragHoverEvent &e) override {
    OpaqueWidget::onDragHover(e);

    if(e.origin==this) {
      setPosition(e.pos);
    }
  }
  void pushHistory() {
    history::ModuleChange *h = new history::ModuleChange;
    h->name = "change matrix";
    h->moduleId = theMatrix->id;
    h->oldModuleJ = oldModuleJson;
    h->newModuleJ = ((ModuleWidget*)getParent())->toJson();
    APP->history->push(h);
  }
  void save() {
    oldModuleJson = ((ModuleWidget*)getParent())->toJson();
  }

  void pasteClipboard() {
    const char *newText=glfwGetClipboardString(APP->window->win);
    const char *ptr=newText;
    while(*ptr) {
      theMatrix->m.set(posY,posX,*ptr);
      goRight();
      ptr++;
    }
  }

  void copy() {
    theMatrix->m.copy(posY,posX,selY,selX);
  }
  void cut() {
    save();
    theMatrix->m.cut(posY,posX,selY,selX);
  }
  void paste() {
    save();
    theMatrix->m.paste(posY,posX);
  }
  void onSelectText(const SelectTextEvent &e) override {
    save();
    if(e.codepoint<128&&e.codepoint>31) {
      theMatrix->m.set(posY,posX,e.codepoint);
      goRight();
    }
    e.consume(this);
    pushHistory();
  }

  void onSelectKey(const SelectKeyEvent &e) override {
    if(e.action==GLFW_PRESS||e.action==GLFW_REPEAT) {
      if(e.key==GLFW_KEY_LEFT) {
        goLeft((e.mods&RACK_MOD_MASK)!=GLFW_MOD_SHIFT);
        e.consume(this);
      }
      if(e.key==GLFW_KEY_RIGHT) {
        goRight((e.mods&RACK_MOD_MASK)!=GLFW_MOD_SHIFT);
        e.consume(this);
      }
      if(e.key==GLFW_KEY_DOWN) {
        if(posY<MAX_SIZE-1) {
          posY++;
          if((e.mods&RACK_MOD_MASK)!=GLFW_MOD_SHIFT) {
            selY=posY;
            selX=posX;
          }
        }
        e.consume(this);
      }
      if(e.key==GLFW_KEY_UP) {
        if(posY>0) {
          posY--;
        }
        if((e.mods&RACK_MOD_MASK)!=GLFW_MOD_SHIFT) {
          selY=posY;
          selX=posX;
        }
        e.consume(this);
      }
      if(e.key==GLFW_KEY_PAGE_DOWN) {
        save();
        theMatrix->m.dec(posY,posX);
        e.consume(this);
        pushHistory();
      }
      if(e.key==GLFW_KEY_PAGE_UP) {
        save();
        theMatrix->m.inc(posY,posX);
        e.consume(this);
        pushHistory();
      }
      if(e.key==GLFW_KEY_BACKSPACE) {
        save();
        theMatrix->m.set(posY,posX,32);
        goLeft();
        e.consume(this);
        pushHistory();
      }
      if(e.key==GLFW_KEY_DELETE) {
        save();
        theMatrix->m.set(posY,posX,32);
        e.consume(this);
        pushHistory();
      }
      if(e.key==GLFW_KEY_HOME) {
        posX=posY=0;
        e.consume(this);
      }
      if(e.key==GLFW_KEY_END) {
        posX=posY=MAX_SIZE-1;
        e.consume(this);
      }
      if(e.keyName=="v"&&(e.mods&RACK_MOD_MASK)==(RACK_MOD_CTRL|GLFW_MOD_SHIFT)) {
        save();
        pasteClipboard();
        e.consume(this);
        pushHistory();
      }
      if(e.keyName=="v"&&(e.mods&RACK_MOD_MASK)==(RACK_MOD_CTRL)) {
        save();
        paste();
        e.consume(this);
        pushHistory();
      }
      if(e.keyName=="c"&&(e.mods&RACK_MOD_MASK)==(RACK_MOD_CTRL)) {
        copy();
        e.consume(this);
      }
      if(e.keyName=="x"&&(e.mods&RACK_MOD_MASK)==(RACK_MOD_CTRL)) {
        save();
        cut();
        e.consume(this);
        pushHistory();
      }
    }
  }

  void onHover(const HoverEvent &e) override {
    APP->event->setSelectedWidget(this);
  }
};
template<typename W>
struct RandomizeButton : MLEDM {
  W *widget;
  bool state=false;
  void onChange(const ChangeEvent &e) override {
    SvgSwitch::onChange(e);
    auto module=(TheMatrix*)widget->getModule();
    if(module) {
      if(module->params[TheMatrix::RND_PARAM].getValue()==0 && state) {
        widget->save();
        module->randomize();
        widget->pushHistory();
        state=false;
      } else {
        state=true;
      }
    }
  }
};
struct TheMatrixWidget : ModuleWidget {
  json_t *oldModuleJson;
  TheMatrixWidget(TheMatrix *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/TheMatrix.svg")));

    auto matrixDisplay=new MatrixDisplay(module,mm2px(Vec(6,4)));
    addChild(matrixDisplay);
    float y=TY(104);
    float x=132;
    auto rndButton=createParam<RandomizeButton<TheMatrixWidget>>(mm2px(Vec(x,y)),module,TheMatrix::RND_PARAM);
    rndButton->widget=this;
    addParam(rndButton);
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,TheMatrix::RND_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,TheMatrix::DENS_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,TheMatrix::DENS_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,TheMatrix::FROM_PARAM));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x+8,y)),module,TheMatrix::TO_PARAM));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,TheMatrix::CV_X_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,TheMatrix::CV_X_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,TheMatrix::CV_Y_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,TheMatrix::CV_Y_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,TheMatrix::LEVEL_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,TheMatrix::LEVEL_INPUT));
    y=TY(26);
    x=130;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,TheMatrix::TRIG_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+12,y)),module,TheMatrix::GATE_OUTPUT));
    y+=12;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,TheMatrix::VOCT_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+12,y)),module,TheMatrix::CV_OUTPUT));

  }

  void pushHistory() {
    history::ModuleChange *h = new history::ModuleChange;
    h->name = "change matrix";
    h->moduleId = module->id;
    h->oldModuleJ = oldModuleJson;
    h->newModuleJ = toJson();
    APP->history->push(h);
  }
  void save() {
    oldModuleJson = toJson();
  }
  void appendContextMenu(Menu *menu) override {
    TheMatrix *module=dynamic_cast<TheMatrix *>(this->module);
    assert(module);
    struct ClearItem : ui::MenuItem {
      TheMatrixWidget *widget;

      ClearItem(TheMatrixWidget *w) : widget(w) {
      }

      void onAction(const ActionEvent &e) override {
        TheMatrix *module=(TheMatrix*)widget->module;
        if(!module)
          return;
        widget->save();
        module->m.clear();
        widget->pushHistory();
      }
    };
    auto clearMenu=new ClearItem(this);
    clearMenu->text="Clear";
    menu->addChild(clearMenu);
  }
};


Model *modelTheMatrix=createModel<TheMatrix,TheMatrixWidget>("TheMatrix");