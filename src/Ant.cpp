#include "plugin.hpp"

#define CASIZE 32
#define F 2
#define R 1
#define B 0
#define L 3
#define NUM_ANTS 8
struct AntS {
  uint8_t posX=0;
  uint8_t posY=0;
  uint8_t state=0;
  AntS() = default;
  AntS(uint8_t _posX,uint8_t _posY,uint8_t _state) : posX(_posX),posY(_posY),state(_state) {

  }
  AntS(int _posX,int _posY) {
    state = (_posX>=0) | ((_posY>=0) << 1);
    posX=std::abs(_posX);
    posY=std::abs(_posY);
  }

  AntS(json_t *root) {
    state=json_integer_value(json_object_get(root,"state"));
    posX=json_integer_value(json_object_get(root,"posX"));
    posY=json_integer_value(json_object_get(root,"posY"));
  }

  json_t *toJson() {
    json_t *root=json_object();
    json_object_set_new(root,"posX",json_integer(posX));
    json_object_set_new(root,"posY",json_integer(posY));
    json_object_set_new(root,"state",json_integer(state));
    return root;
  }

  void updatePos() {
    switch(state) {
      case L:
        posX=(posX+(CASIZE-1))%CASIZE;
        break;
      case R:
        posX=(posX+1)%CASIZE;
        break;
      case F:
        posY=(posY+1)%CASIZE;
        break;
      case B:
        posY=(posY+(CASIZE-1))%CASIZE;
        break;
    }
  }
  void updateState(uint8_t st) {
    state=(state+st)%4;
  }
};
struct AntMatrix {
  RND rnd;
  uint8_t grid[CASIZE][CASIZE]={};
  uint8_t gridSave[CASIZE][CASIZE]={};
  std::vector<uint8_t> rule;
  AntS startAnts[NUM_ANTS]={};
  AntS ants[NUM_ANTS]={};
  AntMatrix() {
    clear();
  }

  void setAnts(const std::vector<int> &data) {
    int len=data.size()/2;
    for(int k=0;k<len;k++) {
      int x = data[k*2];
      int y = data[k*2+1];
      startAnts[k] = {x,y};
    }
    reset();
  }

  void next(int nr=0) {
    if(rule.empty()) return;
    if(nr>=NUM_ANTS) nr=0;
    ants[nr].updateState(rule[getValue(ants[nr].posY,ants[nr].posX)%rule.size()]);
    setValue(ants[nr].posY,ants[nr].posX,(getValue(ants[nr].posY,ants[nr].posX)+1)%rule.size());
    //grid[ants[nr].posY][ants[nr].posX]=(grid[ants[nr].posY][ants[nr].posX]+1)%rule.size();
    ants[nr].updatePos();
  }

  uint8_t getSize() const {
    return rule.size();
  }

  uint8_t getValue(uint8_t nr) {
    if(nr>=NUM_ANTS) nr=0;
    return getValue(ants[nr].posY,ants[nr].posX);
  }

  uint8_t getValue(uint8_t row,uint8_t col) {
    row&=(CASIZE-1);
    col&=(CASIZE-1);
    return grid[row][col];
  }

  void setValue(uint8_t row,uint8_t col, uint8_t v) {
    row&=(CASIZE-1);
    col&=(CASIZE-1);
    grid[row][col]=v;
    setSave();
  }

  int getAnt(uint8_t row,uint8_t col,uint8_t nr=0) {
    if(nr>=NUM_ANTS) nr=0;
    return (ants[nr].posX==col&&ants[nr].posY==row)?ants[nr].state:-1;
  }

  void clear() {
    for(int k=0;k<CASIZE;k++) {
      for(int j=0;j<CASIZE;j++) {
        gridSave[k][j]=grid[k][j]=0;
      }
    }
    for(int k=0;k<NUM_ANTS;k++) {
      ants[k]=startAnts[k];
    }
  }
  void reset() {
    for(int k=0;k<CASIZE;k++) {
      for(int j=0;j<CASIZE;j++) {
        grid[k][j]=gridSave[k][j];
      }
    }
    for(int k=0;k<NUM_ANTS;k++) {
      ants[k]=startAnts[k];
    }
  }
  void randomize(float dens) {
    for(int k=0;k<CASIZE;k++) {
      for(int j=0;j<CASIZE;j++) {
        gridSave[k][j]=grid[k][j]=rnd.nextCoin(1-dens)?rnd.nextRange(0,getSize()):0;
      }
    }
  }

  void setSave() {
    for(int k=0;k<CASIZE;k++) {
      for(int j=0;j<CASIZE;j++) {
        gridSave[k][j]=grid[k][j];
      }
    }
    for(int k=0;k<NUM_ANTS;k++) {
      startAnts[k]=ants[k];
    }
  }

  void setRule(const std::vector<uint8_t>& r) {
    if(r.size()!=rule.size()) {
      clear();
    }
    rule=r;
    reset();
  }
  void fromJson(json_t *jWorld) {
    json_t *jGrid=json_object_get(jWorld,"grid");
    json_t *jGridSave=json_object_get(jWorld,"gridSave");
    if(!jGrid)
      return;
    for(int k=0;k<CASIZE;k++) {
      json_t *row=json_array_get(jGrid,k);
      json_t *rowSave=json_array_get(jGridSave,k);
      for(int j=0;j<CASIZE;j++) {
        json_t *col=json_array_get(row,j);
        json_t *colSave=json_array_get(rowSave,j);
        grid[k][j]=json_real_value(col);
        gridSave[k][j]=json_real_value(colSave);
      }
    }
    json_t *jRule=json_object_get(jWorld,"rule");
    if(jRule) {
      rule.clear();
      int len=json_array_size(jRule);
      for(int k=0;k<len;k++) {
        rule.push_back(json_integer_value(json_array_get(jRule,k)));
      }
    }
    json_t *jAntList=json_object_get(jWorld,"ants");
    json_t *jStartAntList=json_object_get(jWorld,"startAnts");
    for(int k=0;k<NUM_ANTS;k++) {
      if(jAntList) ants[k] = {json_array_get(jAntList,k)};
      if(jStartAntList) startAnts[k] = {json_array_get(jStartAntList,k)};
    }
  }

  json_t *toJson() {
    json_t *jWorld=json_object();

    json_t *dataGridSave=json_array();
    json_t *dataGrid=json_array();
    for(int k=0;k<CASIZE;k++) {
      json_t *rowSave=json_array();
      json_t *row=json_array();
      for(int j=0;j<CASIZE;j++) {
        json_array_append_new(rowSave,json_real(gridSave[k][j]));
        json_array_append_new(row,json_real(grid[k][j]));
      }
      json_array_append_new(dataGridSave,rowSave);
      json_array_append_new(dataGrid,row);
    }
    json_object_set_new(jWorld,"gridSave",dataGridSave);
    json_object_set_new(jWorld,"grid",dataGrid);
    json_t *jRule=json_array();
    for(auto itr=rule.begin();itr!=rule.end();itr++) {
      json_array_append_new(jRule,json_integer(*itr));
    }
    json_object_set_new(jWorld,"rule",jRule);
    json_t *jAntList = json_array();
    json_t *jStartAntList = json_array();
    for(int k=0;k<NUM_ANTS;k++) {
      json_array_append_new(jAntList,ants[k].toJson());
      json_array_append_new(jStartAntList,startAnts[k].toJson());
    }
    json_object_set_new(jWorld,"ants",jAntList);
    json_object_set_new(jWorld,"startAnts",jStartAntList);

    return jWorld;
  }
};


struct Ant : Module {
  enum ParamId {
    STEP_PARAM,ON_PARAM,RST_PARAM,NUM_STEPS_PARAM,UNUSED_PARAM,RND_PARAM,DENS_PARAM,SCALE_PARAM,OFS_PARAM,SET_PARAM,TRSH_PARAM,PARAMS_LEN
  };
  enum InputId {
    STEP_INPUT,RULE_INPUT,RST_INPUT,RND_INPUT,SCALE_INPUT,OFS_INPUT,SET_INPUT,ANTS_INPUT,INPUTS_LEN
  };
  enum OutputId {
    X_OUTPUT,Y_OUTPUT,CV_OUTPUT,GATE_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  AntMatrix antMatrix;
  dsp::SchmittTrigger stepTrigger[NUM_ANTS];
  dsp::SchmittTrigger manualStepTrigger[NUM_ANTS];
  dsp::SchmittTrigger rstTrigger;
  dsp::SchmittTrigger rstManualTrigger;
  dsp::SchmittTrigger rndTrigger;
  dsp::SchmittTrigger manualRndTrigger;
  dsp::SchmittTrigger manualSetTrigger;
  dsp::SchmittTrigger setTrigger;

  dsp::PulseGenerator rstPulse;
  int colorMode=0;
  int defaultRule=0;
  int numAnts=1;
  bool outputClock=false;
  uint8_t defaultPosX[NUM_ANTS+1]={16,12,20,12,20,8,24,8,24};
  uint8_t defaultPosY[NUM_ANTS+1]={16,12,20,20,12,8,24,24,8};

  std::vector <std::vector<uint8_t>> rules={
    {L,R},
    {L,L,R,R},
    {R,L,L,R},
    {R,R,L,R,R},
    {L,L,R,R,R,L,R,L,R,L,L,R},
    {L,L,R,R,L,L,R,R,L,L,R,R,L,L,R,R,L,L,R,R},
    {R,L,L,R,R,L,L,R,R,L,L,R,R,L,L,R,R,L,L,R}
  };
  std::vector<std::string> names={"LR","LLRR","RLLR","RRLRR","LLRRRLRLRLLR","LLRRLLRRLLRRLLRRLLRR","RLLRRLLRRLLRRLLRRLLR"};
  Ant() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configButton(STEP_PARAM,"Next Step");
    configButton(RST_PARAM,"Reset");
    configInput(RST_INPUT,"Reset");
    configInput(RND_INPUT,"Rnd Trigger");
    configButton(RND_PARAM,"Rnd Trigger");
    configButton(SET_PARAM,"Save Grid");
    configParam(DENS_PARAM,0,1,0.2,"Rnd Density");
    configInput(OFS_INPUT,"Out voltage offset");
    configInput(SCALE_INPUT,"Out scale factor");
    configInput(ANTS_INPUT,"Ants");
    configInput(STEP_INPUT,"Next Step");
    configParam(SCALE_PARAM,0,10,2,"Out Scale Factor");
    configParam(OFS_PARAM,-5,5,-1,"Out Offset Factor");
    configParam(NUM_STEPS_PARAM,1,32,1,"Steps per Clock");
    //configParam(NUM_ANTS_PARAM,1,NUM_ANTS,1,"Number of ants");
    //getParamQuantity(NUM_ANTS_PARAM)->snapEnabled=true;
    getParamQuantity(NUM_STEPS_PARAM)->snapEnabled=true;
    configOutput(X_OUTPUT,"X");
    configOutput(Y_OUTPUT,"Y");
    configOutput(CV_OUTPUT,"CV");
    configOutput(GATE_OUTPUT,"Gate");
    configParam(TRSH_PARAM,0,15,0,"Gate Threshold");
    getParamQuantity(TRSH_PARAM)->snapEnabled=true;
    setAnts(1);
  }

  void onAdd(const AddEvent &addEvent) override {
    //antMatrix.reset();
  }

  float getCellValue(int row,int col) {
    return float(antMatrix.getValue(row,col))/float(antMatrix.getSize());
  }

  void setCellValue(int row,int col, float value) {
    uint8_t v=uint8_t(value*float(antMatrix.getSize()));
    antMatrix.setValue(row,col,v);
  }

  int getAnt(int row,int col) {
    for(int k=0;k<numAnts;k++) {
      int s=antMatrix.getAnt(row,col,k);
      if(s>=0) return s;
    }
    return -1;
  }

  void setAnts(int _numAnts) {
    numAnts=_numAnts;
    std::vector<int> ants={};
    for(int k=0;k<numAnts;k++) {
      ants.push_back(defaultPosX[k]);
      ants.push_back(-defaultPosY[k]);
    }
    antMatrix.setAnts(ants);
  }

  void reset() {
    int antChannels=inputs[ANTS_INPUT].getChannels();
    if(antChannels>1) {
      numAnts=antChannels/2;
      std::vector<int> v;
      for(int k=0;k<numAnts;k++) {
        v.push_back(clamp(inputs[ANTS_INPUT].getVoltage(2*k)*3.2,-31.99f,31.99));
        v.push_back(clamp(inputs[ANTS_INPUT].getVoltage(2*k+1)*3.2,-31.99f,31.99));
      }
      antMatrix.setAnts(v);
    } else {
      setAnts(numAnts);
    }
    int channels=inputs[RULE_INPUT].getChannels();
    if(channels<2) {
      antMatrix.setRule(rules[defaultRule]);
    } else {
      std::vector<uint8_t> rule;
      for(int k=0;k<channels;k++) {
        int r = int(clamp(inputs[RULE_INPUT].getVoltage(k),0.f,3.99f));
        rule.push_back(r);
      }
      antMatrix.setRule(rule);
    }
  }

  void setOutputs(int numAnts) {
    for(int k=0;k<numAnts;k++) {
      int v = antMatrix.getValue(k);
      float val = float(v)/float(antMatrix.getSize());
      float scale=params[SCALE_PARAM].getValue();
      if(inputs[SCALE_INPUT].isConnected()) {
        scale+=inputs[SCALE_INPUT].getPolyVoltage(k);
      }
      float offset=params[OFS_PARAM].getValue();
      if(inputs[OFS_INPUT].isConnected()) {
        offset+=inputs[OFS_INPUT].getPolyVoltage(k);
      }
      outputs[CV_OUTPUT].setVoltage(val*scale+offset,k);
      outputs[X_OUTPUT].setVoltage(10.f*float(antMatrix.ants[k].posX)/float(CASIZE),k);
      outputs[Y_OUTPUT].setVoltage(10.f*float(antMatrix.ants[k].posY)/float(CASIZE),k);
      if(v>=params[TRSH_PARAM].getValue()) {
        outputs[GATE_OUTPUT].setVoltage(outputClock?inputs[STEP_INPUT].getVoltage(k):10.f,k);
      } else {
        outputs[GATE_OUTPUT].setVoltage(0.f,k);
      }
    }
    outputs[CV_OUTPUT].setChannels(numAnts);
    outputs[GATE_OUTPUT].setChannels(numAnts);
    outputs[X_OUTPUT].setChannels(numAnts);
    outputs[Y_OUTPUT].setChannels(numAnts);
  }

  void process(const ProcessArgs &args) override {
    if(setTrigger.process(inputs[SET_INPUT].getVoltage()) | manualSetTrigger.process(params[SET_PARAM].getValue())) {
      antMatrix.setSave();
    }
    if(rndTrigger.process(inputs[RND_INPUT].getVoltage())|manualRndTrigger.process(params[RND_PARAM].getValue())) {
      rstPulse.trigger(0.001f);
      antMatrix.randomize(params[DENS_PARAM].getValue());
    }
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())|rstManualTrigger.process(params[RST_PARAM].getValue())) {
      rstPulse.trigger(0.001f);
      reset();
    }

    bool rstGate=rstPulse.process(args.sampleTime*32);
    int numSteps = params[NUM_STEPS_PARAM].getValue();
    //int numAnts = params[NUM_ANTS_PARAM].getValue();

    for(int j=0;j<numAnts;j++) {
      if(!rstGate&&(stepTrigger[j].process(inputs[STEP_INPUT].getPolyVoltage(j))&&params[ON_PARAM].getValue()>0.f)|manualStepTrigger[j].process(params[STEP_PARAM].getValue())) {
        for(int k=0;k<numSteps;k++) {
          antMatrix.next(j);
        }
      }
    }
    setOutputs(numAnts);
  }
  void dataFromJson(json_t *root) override {
    json_t *jWorld=json_object_get(root,"matrix");
    if(jWorld) {
      antMatrix.fromJson(jWorld);
    }
    json_t *jColorMode=json_object_get(root,"colorMode");
    if(jColorMode) colorMode=json_integer_value(jColorMode);
    json_t *jNumAnts=json_object_get(root,"numAnts");
    if(jNumAnts) numAnts=json_integer_value(jNumAnts);
    json_t *jDefaultRule=json_object_get(root,"defaultRule");
    if(jDefaultRule) defaultRule=json_integer_value(jDefaultRule);
    json_t *jOutputClock=json_object_get(root,"outputClock");
    if(jOutputClock) outputClock=json_integer_value(jOutputClock);
  }

  json_t *dataToJson() override {
    json_t *root=json_object();
    json_object_set_new(root,"matrix",antMatrix.toJson());
    json_object_set_new(root,"colorMode",json_integer(colorMode));
    json_object_set_new(root,"numAnts",json_integer(numAnts));
    json_object_set_new(root,"defaultRule",json_integer(defaultRule));
    json_object_set_new(root,"outputClock",json_integer(outputClock));
    return root;
  }

};

struct AntCellColors {
  NVGcolor selectOnColor=nvgRGB(0xff,0xff,0xff);
  NVGcolor selectOffColor=nvgRGB(0x44,0x44,0xaa);
  NVGcolor chnColors[16]={nvgRGB(255,0,0),nvgRGB(0,255,0),nvgRGB(55,55,255),nvgRGB(255,255,0),nvgRGB(255,0,255),nvgRGB(0,255,255),nvgRGB(128,0,0),nvgRGB(196,85,55),nvgRGB(128,128,80),nvgRGB(255,128,0),nvgRGB(255,0,128),nvgRGB(0,128,255),nvgRGB(128,66,128),nvgRGB(128,255,0),nvgRGB(128,128,255),nvgRGB(128,255,255)};
  std::vector<NVGcolor> palettes[3]={
    {nvgRGB(0x0,0x00,0x66),nvgRGB(0x0,0x22,0x99),nvgRGB(0x33,0x44,0xAA),nvgRGB(0x00,0x77,0xBB),nvgRGB(0x22,0x77,0xBB),nvgRGB(0x44,0x77,0xBB),nvgRGB(0x55,0x66,0xBB),nvgRGB(0x66,0x44,0xFF),nvgRGB(0x77,0x44,0xFF),nvgRGB(0x88,0x44,0x88),nvgRGB(0x99,0x44,0x55)},
    {nvgRGB(0x22,0x22,0x66),nvgRGB(0x44,0xdd,0x44),nvgRGB(0xaa,0xaa,0x44)},
    {nvgRGB(0x22,0x88,0x55),nvgRGB(0x9f,0x4b,0x0b),nvgRGB(0x83,0xb8,0x55),nvgRGB(0xdd,0xdd,0x99)}};
};
struct AntDisplay : OpaqueWidget {
  Ant *module=nullptr;
  int numRows=CASIZE;
  const int margin=2;
  int oldC=-1;
  int oldR=-1;
  float value=0.f;
  const int cellXSize=11;
  const int cellYSize=11;
  Vec dragPosition={};
  AntCellColors cellColors;
  int colorMode=0;
  std::vector<std::string> colorModeLabels={"Grey","Palette 1","Palette 2","Palette 3"};

  AntDisplay(Ant *_module,Vec pos) : module(_module) {
    box.size=Vec(CASIZE*cellXSize+margin*2,CASIZE*cellYSize+margin*2);
    box.pos=pos;
  }

  NVGcolor getPaletteColor(int nr,float v) {
    int size=cellColors.palettes[nr].size()-1;
    int index=floor(v*double(size));
    if(index==size)
      return cellColors.palettes[nr][index];
    return nvgLerpRGBA(cellColors.palettes[nr][index],cellColors.palettes[nr][index+1],(v-(index/float(size)))*float(size));
  }

  NVGcolor getCellColor(int row,int col) {
    float v=module->getCellValue(row,col);

    NVGcolor cellColor;
    if(module->colorMode==0) {
      if(v>0) v=rescale(v,0,1,0.1,1);
      cellColor=nvgRGB(int(v*255.f),int(v*255.f),int(v*255.f));
    } else {
      cellColor=getPaletteColor(module->colorMode-1,v);
    }
    return cellColor;
  }

  void drawLayer(const DrawArgs &args,int layer) override {
    if(layer==1) {
      _draw(args);
    }
    Widget::drawLayer(args,layer);
  }

  void drawAnt(const DrawArgs &args,int posX,int posY, int dir) {
    nvgBeginPath(args.vg);
    int dx1=cellXSize/4;
    int dx2=cellXSize*3/4;
    int dy1=cellYSize/4;
    int dy2=cellYSize*3/4;
    switch(dir) {
      case 2:
        nvgMoveTo(args.vg,posX+dx1,posY);
        nvgLineTo(args.vg,posX+dx2,posY);
        nvgLineTo(args.vg,posX+cellXSize/2,posY+cellYSize);
        nvgClosePath(args.vg);
        break;
      case 1:
        nvgMoveTo(args.vg,posX,posY+dy1);
        nvgLineTo(args.vg,posX,posY+dy2);
        nvgLineTo(args.vg,posX+cellXSize,posY+cellYSize/2);
        nvgClosePath(args.vg);
        break;
      case 0:
        nvgMoveTo(args.vg,posX+dx1,posY+cellYSize);
        nvgLineTo(args.vg,posX+dx2,posY+cellYSize);
        nvgLineTo(args.vg,posX+cellXSize/2,posY);
        nvgClosePath(args.vg);
        break;
      case 3:
        nvgMoveTo(args.vg,posX+cellXSize,posY+dy1);
        nvgLineTo(args.vg,posX+cellXSize,posY+dy2);
        nvgLineTo(args.vg,posX,posY+cellYSize/2);
        nvgClosePath(args.vg);
        break;
      default:break;
    }
    nvgFillColor(args.vg,nvgRGB(255,255,255));
    nvgFill(args.vg);
  }

  void _draw(const DrawArgs &args) {
    if(module==nullptr)
      return;


    Vec size=box.size;
    float cellSize=size.x/numRows-margin;

    float posY=1;
    for(int r=0;r<numRows;r++) {
      float posX=1;
      for(int c=0;c<numRows;c++) {
        NVGcolor dg=getCellColor(r,c);
        int ant=module->getAnt(r,c);
        nvgBeginPath(args.vg);
        nvgRect(args.vg,posX,posY,cellSize,cellSize);
        nvgFillColor(args.vg,dg);
        nvgStrokeColor(args.vg,nvgRGB(64,64,40));
        nvgStrokeWidth(args.vg,2);
        nvgStroke(args.vg);
        nvgFill(args.vg);
        if(ant>=0) {
          drawAnt(args,posX,posY,ant);
        }
        posX+=cellSize+margin;
      }
      posY+=cellSize+margin;
    }
    nvgBeginPath(args.vg);
    float mid=(cellSize+margin)*(numRows/2);
    float end=(cellSize+margin)*(numRows);
    nvgMoveTo(args.vg,0,mid);
    nvgLineTo(args.vg,end,mid);
    nvgMoveTo(args.vg,mid,0);
    nvgLineTo(args.vg,mid,end);
    nvgStrokeColor(args.vg,nvgRGB(80,80,80));
    nvgStrokeWidth(args.vg,2);
    nvgStroke(args.vg);
  }

  virtual void onButton(const event::Button &e) override {

    if(e.action==GLFW_PRESS&&e.button==GLFW_MOUSE_BUTTON_LEFT) {
      int c=oldC=floor(e.pos.x/(box.size.x/float(numRows)));
      int r=oldR=floor(e.pos.y/(box.size.y/float(numRows)));
      if((e.mods&RACK_MOD_MASK)==0) {
        value=0.5f;

      } else if((e.mods&RACK_MOD_MASK)==GLFW_MOD_SHIFT) {
        value=0.f;
      }
      module->setCellValue(r,c,value);
      e.consume(this);
      dragPosition=e.pos;
    } /*else if(e.action==GLFW_PRESS&&e.button==GLFW_MOUSE_BUTTON_RIGHT) {
      int c=oldC=floor(e.pos.x/(box.size.x/float(numRows)));
      int r=oldR=floor(e.pos.y/(box.size.y/float(numRows)));
      module->setCurrent(r,c);
      e.consume(this);
      dragPosition=e.pos;
    } */
  }

  void onDragMove(const event::DragMove &e) override {

    dragPosition=dragPosition.plus(e.mouseDelta.div(getAbsoluteZoom()));
    if(isMouseInDrawArea(dragPosition)) {
      int c=floor(dragPosition.x/(box.size.x/float(numRows)));
      int r=floor(dragPosition.y/(box.size.y/float(numRows)));
      if(c!=oldC||r!=oldR) {
          module->setCellValue(r,c,value);
      }
      oldC=c;
      oldR=r;
    }
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
};

struct AntRuleSelectItem : MenuItem {
  Ant *module;

  AntRuleSelectItem(Ant *_module) : module(_module) {
  }

  Menu *createChildMenu() override {
    Menu *menu=new Menu;
    for(int k=0;k<int(module->names.size());k++) {
      menu->addChild(createCheckMenuItem(module->names[k],"",[=]() {
        return module->defaultRule==k;
      },[=]() {
        module->defaultRule=k;
        module->reset();
      }));
    }
    return menu;
  }
};
struct AntNumSelectItem : MenuItem {
  Ant *module;

  AntNumSelectItem(Ant *_module) : module(_module) {
  }

  Menu *createChildMenu() override {
    Menu *menu=new Menu;
    for(int k=1;k<=NUM_ANTS;k++) {
      menu->addChild(createCheckMenuItem(std::to_string(k),"",[=]() {
        return module->numAnts==k;
      },[=]() {
        module->setAnts(k);
      }));
    }
    return menu;
  }
};
struct AntWidget : ModuleWidget {
  AntDisplay *display=nullptr;
  AntWidget(Ant *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/Ant.svg")));
    display=new AntDisplay(module,mm2px(Vec(4,4)));
    addChild(display);
    float x=127;
    float x2=131.5;
    float y=16;
    addParam(createParam<MLEDM>(mm2px(Vec(x,y)),module,Ant::STEP_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+9,y)),module,Ant::STEP_INPUT));
    addParam(createParam<MLED>(mm2px(Vec(x+18,y)),module,Ant::ON_PARAM));
    y+=14;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,30)),module,Ant::NUM_STEPS_PARAM));
    //addParam(createParam<TrimbotWhite>(mm2px(Vec(x+9,30)),module,Ant::NUM_ANTS_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+9,30)),module,Ant::ANTS_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x+18,30)),module,Ant::RULE_INPUT));

    addParam(createParam<MLEDM>(mm2px(Vec(x2,42)),module,Ant::RST_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x2+9,42)),module,Ant::RST_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,54)),module,Ant::DENS_PARAM));
    addParam(createParam<MLEDM>(mm2px(Vec(x+9,54)),module,Ant::RND_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+18,54)),module,Ant::RND_INPUT));

    addParam(createParam<MLEDM>(mm2px(Vec(x2,66)),module,Ant::SET_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x2+9,66)),module,Ant::SET_INPUT));

    addOutput(createOutput<SmallPort>(mm2px(Vec(x2,78)),module,Ant::X_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x2+9,78)),module,Ant::Y_OUTPUT));

    addParam(createParam<TrimbotWhite>(mm2px(Vec(x2,90)),module,Ant::SCALE_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x2+9,90)),module,Ant::SCALE_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x2,102)),module,Ant::OFS_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x2+9,102)),module,Ant::OFS_INPUT));

    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,114)),module,Ant::TRSH_PARAM));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+9,114)),module,Ant::GATE_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+18,114)),module,Ant::CV_OUTPUT));

  }
  void appendContextMenu(Menu *menu) override {
    Ant *module=dynamic_cast<Ant *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    auto colorSelect=new LabelIntSelectItem(&module->colorMode,display->colorModeLabels);
    colorSelect->text="Color Mode";
    colorSelect->rightText=display->colorModeLabels[module->colorMode]+"  "+RIGHT_ARROW;
    menu->addChild(colorSelect);
    struct ClearItem : ui::MenuItem {
      Ant *module;
      int nr;

      ClearItem(Ant *m) : module(m) {
      }

      void onAction(const ActionEvent &e) override {
        if(!module)
          return;
        module->antMatrix.clear();
      }
    };
    auto clearMenu=new ClearItem(module);
    clearMenu->text="Clear";
    menu->addChild(clearMenu);

    auto ruleSelectItem=new AntRuleSelectItem(module);
    ruleSelectItem->text="Default Rule";
    ruleSelectItem->rightText=module->names[module->defaultRule]+"  "+ +RIGHT_ARROW;
    menu->addChild(ruleSelectItem);
    auto numAntsItem=new AntNumSelectItem(module);
    numAntsItem->text="Num Ants";
    numAntsItem->rightText=string::f("%d",module->numAnts)+"  "+ +RIGHT_ARROW;
    menu->addChild(numAntsItem);
    menu->addChild(createCheckMenuItem("Output Clock","",[=]() {
      return module->outputClock;
    },[=]() {
      module->outputClock=!module->outputClock;
    }));
  }
};


Model *modelAnt=createModel<Ant,AntWidget>("Ant");