#include <utility>

#include "plugin.hpp"
#include "rnd.h"
#include "C42ExpanderMessage.hpp"

#define MAX_SIZE 32
extern Model *modelC42E;

struct RLEException : public std::exception {
  std::string errString;

  explicit RLEException(std::string _err) : std::exception(),errString(std::move(_err)) {
  }

  const char *what() const noexcept override {
    return errString.c_str();
  }
};

struct MPoint {
  MPoint(int64_t _x,int64_t _y) : x(_x),y(_y) {}
  int64_t x,y;
};

struct RLEParser {
  std::tuple<int64_t,int64_t> get_width_height(const std::string &line) {
    auto x=std::find(line.begin(),line.end(),'x');
    if(x==line.end())
      throw RLEException("Invalid Format: no header found");
    auto x_equals=std::find(x,line.end(),'=');
    if(x_equals==line.end())
      throw RLEException("Invalid Format: header improperly formed");
    auto y=std::find(x_equals,line.end(),'y');
    if(y==line.end())
      throw RLEException("Invalid Format: header improperly formed");
    int64_t width=std::stoull(std::string(x_equals+1,y));
    auto y_equals=std::find(y,line.end(),'=');
    if(y_equals==line.end())
      throw RLEException("Invalid Format: header improperly formed");
    int64_t height=std::stoull(std::string(y_equals+1,line.end()));
    return std::make_tuple(width,height);
  }

  template<typename T>
  T get_nearest_iterator(T it1,T it2,T it3,T it4) {
    if(it1<it2&&it1<it3&&it1<it4)
      return it1;
    else if(it2<it1&&it2<it3&&it2<it4)
      return it2;
    else if(it3<it1&&it3<it2&&it3<it4)
      return it3;
    else
      return it4;
  }

  void add_points_from_line(std::vector<MPoint> &points,int64_t &current_x,int64_t &current_y,std::string line) {
    while(!line.empty()) {
      auto dead=std::find(line.begin(),line.end(),'b');
      auto alive=std::find(line.begin(),line.end(),'o');
      auto end=std::find(line.begin(),line.end(),'$');
      auto stop=std::find(line.begin(),line.end(),'!');
      auto nearest=get_nearest_iterator(dead,alive,end,stop);
      int64_t num_of_cells;
      if(nearest==line.begin())
        num_of_cells=1;
      else
        num_of_cells=std::stoll(std::string(line.begin(),nearest));
      if(nearest==dead) {
        current_x+=num_of_cells;
      } else if(nearest==alive) {
        for(int64_t i=current_x;i<current_x+num_of_cells;i++) {
          points.emplace_back(i,current_y);
        }
        current_x+=num_of_cells;
      } else if(nearest==end) {
        current_x=0;
        current_y+=num_of_cells;
      } else {
        return;
      }
      line=std::string(nearest+1,line.end());
    }
  }


  std::vector<MPoint> get_rle_encoded_points(const std::string &encoded) {
    std::vector<MPoint> points;
    int64_t width,height;
    bool found_width_height=false;
    std::stringstream ss(encoded);
    std::string line;
    int64_t current_y=0;
    int64_t current_x=0;
    while(std::getline(ss,line)) {
      if(line[0]=='#')
        continue;
      if(!found_width_height) {
        std::tie(width,height)=get_width_height(line);
        found_width_height=true;
      } else {
        add_points_from_line(points,current_x,current_y,line);
      }
    }
    if(!found_width_height)
      throw RLEException("File Format Invalid");
    if(points.empty())
      throw RLEException("No Points found!");
    if(width>=MAX_SIZE || height==MAX_SIZE)
      throw RLEException("Pattern to big!");
    return points;
  }
};

struct LifeWorld {
  bool grid[MAX_SIZE][MAX_SIZE]={};
  bool gridSave[MAX_SIZE][MAX_SIZE]={};
  int size=32;
  bool grid2[MAX_SIZE][MAX_SIZE]={};
  std::vector<int> survive={0,0,1,1,0,0,0,0,0};
  std::vector<int> birth={0,0,0,1,0,0,0,0,0};

  std::vector<std::string> ruleLabels={"B3/S23","B34/S34","B234/S","B2/S","B1/S1","B36/S125","B345/S5","B2/S2","B2/S1"};
  RLEParser rleParser;

  void pasteCells(const std::string& rle,int curRow,int curCol) {
    try {
      std::vector<MPoint>points=rleParser.get_rle_encoded_points(rle);
      for(auto p : points) {
        if(p.x+curCol<=MAX_SIZE && p.y+curRow<=MAX_SIZE)
        setCell(p.y+curRow,p.x+curCol,true);
      }
    } catch(RLEException &e) {
      INFO("%s",e.what());
    }
  }

  void setRule(unsigned r) {
    rule=r;
    switch(r) {
      case 1:
        survive={0,0,0,1,1,0,0,0,0};
        birth={0,0,0,1,1,0,0,0,0};
        break;
      case 2:
        survive={0,0,0,0,0,0,0,0,0};
        birth={0,0,1,1,1,0,0,0,0};
        break;
      case 3:
        survive={0,0,0,0,0,0,0,0,0};
        birth={0,0,1,0,0,0,0,0,0};
        break;
      case 4:
        survive={0,1,0,0,0,0,0,0,0};
        birth={0,1,0,0,0,0,0,0,0};
        break;
      case 5:
        survive={0,1,1,0,0,1,0,0,0};
        birth={0,0,0,1,0,0,1,0,0};
        break;
      case 6:
        survive={0,0,0,0,0,1,0,0,0};
        birth={0,0,0,1,1,1,0,0,0};
        break;
      case 7:
        survive={0,0,1,0,0,0,0,0,0};
        birth={0,0,1,0,0,0,0,0,0};
        break;
      case 8:
        survive={0,1,0,0,0,0,0,0,0};
        birth={0,0,1,0,0,0,0,0,0};
        break;
      default:
        survive={0,0,1,1,0,0,0,0,0};
        birth={0,0,0,1,0,0,0,0,0};
    }
  }

  unsigned getRule() {
    return rule;
  }

  void randomize(float dens) {
    for(int x=0;x<size;x++) {
      for(int y=0;y<size;y++) {
        grid[x][y]=rnd.nextCoin(1.0-dens);
        gridSave[x][y]=grid[x][y];
      }
    }
  }

  bool getCell(int row,int col) {
    return grid[row][col];
  }

  int getRowSum(int row,int col) {
    int sum=0;
    for(int k=0;k<size;k++) {
      if(getCell(row,k)&&(k!=col))
        sum++;
    }
    return sum;
  }

  int getLeftRowSum(int row,int col) {
    int sum=0;
    for(int k=0;k<col;k++) {
      if(getCell(row,k))
        sum++;
    }
    return sum;
  }

  int getRightRowSum(int row,int col) {
    int sum=0;
    for(int k=col+1;k<size;k++) {
      if(getCell(row,k))
        sum++;
    }
    return sum;
  }

  int getColSum(int row,int col) {
    int sum=0;
    for(int k=0;k<size;k++) {
      if(getCell(k,col)&&(k!=row))
        sum++;
    }
    return sum;
  }

  int getTopColSum(int row,int col) {
    int sum=0;
    for(int k=0;k<row;k++) {
      if(getCell(k,col))
        sum++;
    }
    return sum;
  }

  int getBottomColSum(int row,int col) {
    int sum=0;
    for(int k=row+1;k<size;k++) {
      if(getCell(k,col))
        sum++;
    }
    return sum;
  }

  int getTopLeftSum(int row,int col) {
    int sum=0;
    for(int k=1;;k++) {
      if(row-k<0||col-k<0)
        break;
      if(getCell(row-k,col-k))
        sum++;
    }
    return sum;
  }

  int getTopRightSum(int row,int col) {
    int sum=0;
    for(int k=1;;k++) {
      if(row-k<0||col+k>=size)
        break;
      if(getCell(row-k,col+k))
        sum++;
    }
    return sum;
  }

  int getBottomRightSum(int row,int col) {
    int sum=0;
    for(int k=1;;k++) {
      if(row+k>=size||col+k>=size)
        break;
      if(getCell(row+k,col+k))
        sum++;
    }
    return sum;
  }

  int getBottomLeftSum(int row,int col) {
    int sum=0;
    for(int k=1;;k++) {
      if(row+k>=size||col-k<0)
        break;
      if(getCell(row+k,col-k))
        sum++;
    }
    return sum;
  }

  void setCell(int row,int col,bool on=true) {
    grid[row][col]=on;
    for(int x=0;x<size;x++)
      for(int y=0;y<size;y++) {
        gridSave[x][y]=grid[x][y];
      }
  }



  void reset() {
    for(int x=0;x<size;x++)
      for(int y=0;y<size;y++) {
        grid[x][y]=gridSave[x][y];
      }
  }

  void clear() {
    for(int x=0;x<MAX_SIZE;x++)
      for(int y=0;y<MAX_SIZE;y++) {
        grid[x][y]=gridSave[x][y]=false;
      }
  }

  void nextGeneration() {

    for(int x=0;x<size;x++)
      for(int y=0;y<size;y++)
        grid2[x][y]=grid[x][y];

    for(int a=0;a<size;a++) {
      for(int b=0;b<size;b++) {
        int life=0;
        int w=size;
        int h=size;
        int top=a==0?h-1:a-1;
        int bottom=a==h-1?0:a+1;
        int left=b==0?w-1:b-1;
        int right=b==w-1?0:b+1;
        if(grid2[top][left])
          life++;
        if(grid2[top][b])
          life++;
        if(grid2[top][right])
          life++;
        if(grid2[a][left])
          life++;
        if(grid2[a][right])
          life++;
        if(grid2[bottom][left])
          life++;
        if(grid2[bottom][b])
          life++;
        if(grid2[bottom][right])
          life++;

        if(grid[a][b])
          grid[a][b]=survive[life];
        else
          grid[a][b]=birth[life];
      }
    }
  }

  void fromJson(json_t *jWorld) {
    json_t *jSize=json_object_get(jWorld,"size");
    if(jSize)
      size=json_integer_value(jSize);
    json_t *jRule=json_object_get(jWorld,"rule");
    unsigned r=0;
    if(jRule)
      r=json_integer_value(jRule);
    setRule(r);
    json_t *jGrid=json_object_get(jWorld,"grid");
    json_t *jGridSave=json_object_get(jWorld,"gridSave");
    for(int k=0;k<size;k++) {
      json_t *row=json_array_get(jGrid,k);
      json_t *rowSave=json_array_get(jGridSave,k);
      for(int j=0;j<size;j++) {
        json_t *col=json_array_get(row,j);
        json_t *colSave=json_array_get(rowSave,j);
        grid[k][j]=json_integer_value(col);
        gridSave[k][j]=json_integer_value(colSave);
      }
    }

  }

  json_t *toJson() {
    json_t *jWorld=json_object();
    json_object_set_new(jWorld,"size",json_integer(size));
    json_object_set_new(jWorld,"rule",json_integer(rule));
    json_t *dataGridSave=json_array();
    json_t *dataGrid=json_array();
    for(int k=0;k<size;k++) {
      json_t *rowSave=json_array();
      json_t *row=json_array();
      for(int j=0;j<size;j++) {
        json_array_append_new(rowSave,json_integer(gridSave[k][j]));
        json_array_append_new(row,json_integer(grid[k][j]));
      }
      json_array_append_new(dataGridSave,rowSave);
      json_array_append_new(dataGrid,row);
    }
    json_object_set_new(jWorld,"gridSave",dataGridSave);
    json_object_set_new(jWorld,"grid",dataGrid);
    return jWorld;
  }

private:
  RND rnd;
  unsigned rule=0;
};

struct C42CellColors {
  NVGcolor offColor=nvgRGB(0x22,0x22,0x22);
  NVGcolor onColor=nvgRGB(0xa2,0xd6,0xc6);
  NVGcolor selectOnColor=nvgRGB(0xff,0xff,0xff);
  NVGcolor selectOffColor=nvgRGB(0x44,0x44,0xaa);
  NVGcolor chnColors[16]={nvgRGB(255,0,0),nvgRGB(0,255,0),nvgRGB(55,55,255),nvgRGB(255,255,0),nvgRGB(255,0,255),nvgRGB(0,255,255),nvgRGB(128,0,0),nvgRGB(196,85,55),nvgRGB(128,128,80),nvgRGB(255,128,0),nvgRGB(255,0,128),nvgRGB(0,128,255),nvgRGB(128,66,128),nvgRGB(128,255,0),nvgRGB(128,128,255),nvgRGB(128,255,255)};
};
using MLIGHT1=TLight<GrayModuleLightWidget,255,0,0>;
using MLIGHT2=TLight<GrayModuleLightWidget,0,255,0>;
using MLIGHT3=TLight<GrayModuleLightWidget,55,55,255>;
using MLIGHT4=TLight<GrayModuleLightWidget,255,255,0>;
using MLIGHT5=TLight<GrayModuleLightWidget,255,0,255>;
using MLIGHT6=TLight<GrayModuleLightWidget,0,255,255>;
using MLIGHT7=TLight<GrayModuleLightWidget,128,0,0>;
using MLIGHT8=TLight<GrayModuleLightWidget,196,85,55>;
using MLIGHT9=TLight<GrayModuleLightWidget,128,128,80>;
using MLIGHT10=TLight<GrayModuleLightWidget,255,128,0>;
using MLIGHT11=TLight<GrayModuleLightWidget,255,0,128>;
using MLIGHT12=TLight<GrayModuleLightWidget,0,128,255>;
using MLIGHT13=TLight<GrayModuleLightWidget,128,66,128>;
using MLIGHT14=TLight<GrayModuleLightWidget,128,255,0>;
using MLIGHT15=TLight<GrayModuleLightWidget,128,128,255>;
using MLIGHT16=TLight<GrayModuleLightWidget,128,255,255>;

template<typename M>
struct C42Display : OpaqueWidget {
  M *module;
  int numRows=MAX_SIZE;
  const int margin=2;
  int oldC=-1;
  int oldR=-1;
  Vec dragPosition={};
  C42CellColors cellColors;
  bool current=false;

  NVGcolor getCellColor(int row,int col) {
    std::vector<int> selected=module->getSelected(row,col);
    switch(selected.size()) {
      case 0:
        if(module->isOn(row,col)) {
          return cellColors.onColor;
        }
        return cellColors.offColor;
        break;
      case 1:
        if(module->isOn(row,col)) {
          return cellColors.chnColors[selected[0]];
        }
        return nvgLerpRGBA(cellColors.offColor,cellColors.chnColors[selected[0]],0.5f);
        break;
      default:
        if(module->isOn(row,col)) {
          return cellColors.selectOnColor;
        }
        return cellColors.selectOffColor;

    }
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
    numRows=module->world.size;

    Vec size=box.size;
    float cellSize=size.x/numRows-margin;

    float posY=1;
    for(int r=0;r<numRows;r++) {
      float posX=1;
      for(int c=0;c<numRows;c++) {
        NVGcolor dg=getCellColor(r,c);
        nvgBeginPath(args.vg);
        nvgRect(args.vg,posX,posY,cellSize,cellSize);
        nvgFillColor(args.vg,dg);
        if(module->isCurrent(r,c)) {
          nvgStrokeColor(args.vg,nvgRGB(255,255,255));
        } else {
          nvgStrokeColor(args.vg,nvgRGB(64,64,64));
        }
        nvgStrokeWidth(args.vg,2);
        nvgStroke(args.vg);
        nvgFill(args.vg);
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
    if(e.action==GLFW_PRESS&&e.button==GLFW_MOUSE_BUTTON_LEFT&&(e.mods&RACK_MOD_MASK)==0) {
      int c=oldC=floor(e.pos.x/(box.size.x/float(numRows)));
      int r=oldR=floor(e.pos.y/(box.size.y/float(numRows)));
      current=module->toggleCell(r,c);
      e.consume(this);
      dragPosition=e.pos;
    } else if(e.action==GLFW_PRESS&&e.button==GLFW_MOUSE_BUTTON_RIGHT) {
      int c=oldC=floor(e.pos.x/(box.size.x/float(numRows)));
      int r=oldR=floor(e.pos.y/(box.size.y/float(numRows)));
      module->setCurrent(r,c);
      e.consume(this);
      dragPosition=e.pos;
    }
  }

  void onDragMove(const event::DragMove &e) override {

    dragPosition=dragPosition.plus(e.mouseDelta.div(getAbsoluteZoom()));
    if(isMouseInDrawArea(dragPosition)) {
      int c=floor(dragPosition.x/(box.size.x/float(numRows)));
      int r=floor(dragPosition.y/(box.size.y/float(numRows)));
      if(c!=oldC||r!=oldR) {
        if(e.button==GLFW_MOUSE_BUTTON_RIGHT)
          module->setCurrent(r,c);
        else
          module->setCell(r,c,current);
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

struct C42 : Module {
  enum ParamId {
    CV_X_PARAM,CV_Y_PARAM,STEP_PARAM,ON_PARAM,RST_PARAM,DENS_PARAM,RND_PARAM,LEVEL_PARAM,CV_ON_X_PARAM,CV_ON_Y_PARAM,PARAMS_LEN
  };
  enum InputId {
    CV_X_INPUT,CV_Y_INPUT,STEP_INPUT,RST_INPUT,ON_INPUT,DENS_INPUT,LEVEL_INPUT,RND_INPUT,CV_ON_X_INPUT,CV_ON_Y_INPUT,INPUTS_LEN
  };
  enum OutputId {
    TRIG_OUTPUT,ROW_LEFT_OUTPUT,ROW_RIGHT_OUTPUT,COL_TOP_OUTPUT,COL_BOTTOM_OUTPUT,TOP_LEFT_OUTPUT,TOP_RIGHT_OUTPUT,BOTTOM_LEFT_OUTPUT,BOTTOM_RIGHT_OUTPUT,MIDDLE_OUTPUT,GATE_OUTPUT,CLOCK_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    GATE_LIGHT,LIGHTS_LEN=GATE_LIGHT+16
  };

  int lastRow[16]={};
  int lastCol[16]={};

  int curRow[16]={};
  int curCol[16]={};
  int dirty=0;
  int channels=0;
  LifeWorld world;
  bool sizeSetFromJson=false;
  dsp::PulseGenerator trigPulse[16];
  dsp::SchmittTrigger stepTrigger;
  dsp::SchmittTrigger manualStepTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::SchmittTrigger rstManualTrigger;
  dsp::PulseGenerator rstPulse;
  dsp::SchmittTrigger rndTrigger;
  dsp::SchmittTrigger manualRndTrigger;
  dsp::ClockDivider divider;

  C42ExpanderMessage rightMessages[2][1];

  C42() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configParam(CV_X_PARAM,0,world.size,world.size/2,"X");
    getParamQuantity(CV_X_PARAM)->snapEnabled=true;
    configParam(CV_Y_PARAM,0,world.size,world.size/2,"Y");
    getParamQuantity(CV_Y_PARAM)->snapEnabled=true;
    configButton(STEP_PARAM,"Step");

    configParam(DENS_PARAM,0,1,0.5,"Random Density");
    configParam(LEVEL_PARAM,0.01,2,1/12.f,"Out Level Factor");
    configButton(RST_PARAM,"Reset");
    configButton(ON_PARAM,"Generation On");
    configButton(CV_ON_X_PARAM,"CV X Input On");
    configButton(CV_ON_Y_PARAM,"CV Y Input On");
    configInput(CV_X_INPUT,"CV X");
    configInput(CV_Y_INPUT,"CV_Y");
    configInput(STEP_INPUT,"Step");
    configInput(RST_INPUT,"Reset");
    configInput(ON_INPUT,"Generation On");
    configInput(CV_ON_X_INPUT,"CV X On");
    configInput(CV_ON_Y_INPUT,"CV Y On");
    configInput(DENS_INPUT,"Random Density");
    configInput(LEVEL_INPUT,"Out Level Factor");
    configInput(ON_INPUT,"On");
    configOutput(GATE_OUTPUT,"Gate");

    configOutput(ROW_LEFT_OUTPUT,"CV Row-Left");
    configOutput(ROW_RIGHT_OUTPUT,"CV Row-Right");
    configOutput(COL_TOP_OUTPUT,"CV Col-Top");
    configOutput(COL_BOTTOM_OUTPUT,"CV Col-Bottom");
    configOutput(TOP_LEFT_OUTPUT,"CV Top-Left");
    configOutput(TOP_RIGHT_OUTPUT,"CV Top-Right");
    configOutput(BOTTOM_LEFT_OUTPUT,"CV Bottom-Left");
    configOutput(BOTTOM_RIGHT_OUTPUT,"CV Bottom-Right");
    configOutput(MIDDLE_OUTPUT,"CV Gate");
    configOutput(TRIG_OUTPUT,"Trigger");
    configOutput(CLOCK_OUTPUT,"Clock");
    divider.setDivision(32);

    rightExpander.producerMessage=rightMessages[0];
    rightExpander.consumerMessage=rightMessages[1];
  }

  void onAdd(const AddEvent &e) override {
    if(!sizeSetFromJson)
      setSize(16);
  }

  void dataFromJson(json_t *root) override {

    json_t *jWorld=json_object_get(root,"world");
    if(jWorld) {
      world.fromJson(jWorld);
      setSize(world.size);
    } else {
      setSize(16);
    }
    sizeSetFromJson=true;
  }

  json_t *dataToJson() override {
    json_t *root=json_object();
    json_object_set_new(root,"world",world.toJson());
    return root;
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

  bool isCurrent(int row,int col) {
    return row==int(params[CV_Y_PARAM].getValue())&&col==int(params[CV_X_PARAM].getValue());
  }

  void setCurrent(int row,int col) {
    getParamQuantity(CV_X_PARAM)->setValue(float(col));
    getParamQuantity(CV_Y_PARAM)->setValue(float(row));
  }


  bool isOn(int row,int col) {
    return world.getCell(row,col);
  }

  void setCell(int row,int col,bool on=true) {
    world.setCell(row,col,on);
  }

  void pasteRLE(const std::string &rle) {
    world.pasteCells(rle,params[CV_Y_PARAM].getValue(),params[CV_X_PARAM].getValue());
  }

  bool toggleCell(int row,int col) {
    if(isOn(row,col)) {
      setCell(row,col,false);
      return false;
    }
    setCell(row,col,true);
    return true;
  }

  void populateOutputs(float level,int chn) {

    outputs[ROW_RIGHT_OUTPUT].setVoltage(world.getRightRowSum(curRow[chn],curCol[chn])*level,chn);
    outputs[ROW_LEFT_OUTPUT].setVoltage(world.getLeftRowSum(curRow[chn],curCol[chn])*level,chn);
    outputs[COL_TOP_OUTPUT].setVoltage(world.getTopColSum(curRow[chn],curCol[chn])*level,chn);
    outputs[COL_BOTTOM_OUTPUT].setVoltage(world.getBottomColSum(curRow[chn],curCol[chn])*level,chn);
    outputs[TOP_LEFT_OUTPUT].setVoltage(world.getTopLeftSum(curRow[chn],curCol[chn])*level,chn);
    outputs[TOP_RIGHT_OUTPUT].setVoltage(world.getTopRightSum(curRow[chn],curCol[chn])*level,chn);
    outputs[BOTTOM_RIGHT_OUTPUT].setVoltage(world.getBottomRightSum(curRow[chn],curCol[chn])*level,chn);
    outputs[BOTTOM_LEFT_OUTPUT].setVoltage(world.getBottomLeftSum(curRow[chn],curCol[chn])*level,chn);

  }

  void populateExpanderChannel(C42ExpanderMessage *message,float level,int chn) {
    message->rowRight[chn]=world.getRightRowSum(curRow[chn],curCol[chn])*level;
    message->rowLeft[chn]=world.getLeftRowSum(curRow[chn],curCol[chn])*level;
    message->colTop[chn]=world.getTopColSum(curRow[chn],curCol[chn])*level;
    message->colBottom[chn]=world.getBottomColSum(curRow[chn],curCol[chn])*level;
    message->topLeft[chn]=world.getTopLeftSum(curRow[chn],curCol[chn])*level;
    message->topRight[chn]=world.getTopRightSum(curRow[chn],curCol[chn])*level;
    message->bottomRight[chn]=world.getBottomRightSum(curRow[chn],curCol[chn])*level;
    message->bottomLeft[chn]=world.getBottomLeftSum(curRow[chn],curCol[chn])*level;
  }

  void process(const ProcessArgs &args) override {

    if(divider.process()) {
      _process(args);
    }
  }

  void _process(const ProcessArgs &args) {

    if(inputs[ON_INPUT].isConnected()) {
      getParamQuantity(ON_PARAM)->setValue(inputs[ON_INPUT].getVoltage()>1.f);
    }
    if(inputs[CV_ON_X_INPUT].isConnected()) {
      getParamQuantity(CV_ON_X_PARAM)->setValue(inputs[CV_ON_X_INPUT].getVoltage()>1.f);
    }
    if(inputs[CV_ON_Y_INPUT].isConnected()) {
      getParamQuantity(CV_ON_Y_PARAM)->setValue(inputs[CV_ON_Y_INPUT].getVoltage()>1.f);
    }
    if(inputs[RND_INPUT].isConnected()) {
      getParamQuantity(RND_PARAM)->setValue(inputs[RND_INPUT].getVoltage()>1.f);
    }
    if(inputs[DENS_INPUT].isConnected()) {
      getParamQuantity(DENS_PARAM)->setValue(inputs[DENS_INPUT].getVoltage()/10.f);
    }
    if(inputs[LEVEL_INPUT].isConnected()) {
      getParamQuantity(LEVEL_PARAM)->setValue(inputs[LEVEL_INPUT].getVoltage()/5.f);
    }
    int channelsX=0;
    if(inputs[CV_X_INPUT].isConnected()&&params[CV_ON_X_PARAM].getValue()>0.f) {
      channelsX=inputs[CV_X_INPUT].getChannels();
      for(int chn=0;chn<16;chn++) {
        int index=int(inputs[CV_X_INPUT].getVoltage(chn)/10.f*float(world.size));
        index+=int(params[CV_X_PARAM].getValue());
        while(index<0)
          index+=world.size;
        index%=world.size;
        curCol[chn]=index;
      }
    } else {
      curCol[0]=(int(params[CV_X_PARAM].getValue())%world.size);
    }
    int channelsY=0;
    if(inputs[CV_Y_INPUT].isConnected()&&params[CV_ON_Y_PARAM].getValue()>0.f) {
      channelsY=inputs[CV_Y_INPUT].getChannels();
      for(int chn=0;chn<16;chn++) {
        int index=int(inputs[CV_Y_INPUT].getVoltage(chn)/10.f*float(world.size));
        index+=params[CV_Y_PARAM].getValue();
        while(index<0)
          index+=world.size;
        index%=world.size;
        curRow[chn]=index;
      }
    } else {
      curRow[0]=(int(params[CV_Y_PARAM].getValue())%world.size);
    }
    channels=std::max(std::max(channelsX,channelsY),1);
    bool on[16]={};
    if(rndTrigger.process(inputs[RND_INPUT].getVoltage())|manualRndTrigger.process(params[RND_PARAM].getValue())) {
      rstPulse.trigger(0.001f);
      world.randomize(params[DENS_PARAM].getValue());
    }
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())|rstManualTrigger.process(params[RST_PARAM].getValue())) {
      rstPulse.trigger(0.001f);
      world.reset();
    }
    bool rstGate=rstPulse.process(args.sampleTime*32);
    bool lastOn[16]={};
    if(((stepTrigger.process(inputs[STEP_INPUT].getVoltage())&&params[ON_PARAM].getValue()>0.f)|manualStepTrigger.process(params[STEP_PARAM].getValue()))&&!rstGate) {
      for(int chn=0;chn<channels;chn++) {
        lastOn[chn]=isOn(curRow[chn],curCol[chn]);
      }
      world.nextGeneration();
      for(int chn=0;chn<channels;chn++) {
        on[chn]=isOn(curRow[chn],curCol[chn])&&!lastOn[chn];
      }
    }

    for(int chn=0;chn<channels;chn++) {
      if(curRow[chn]!=lastRow[chn]||curCol[chn]!=lastCol[chn]) {
        on[chn]=isOn(curRow[chn],curCol[chn]);
      }
      if(on[chn])
        trigPulse[chn].trigger(0.01f);
      if(trigPulse[chn].process(args.sampleTime*32)) {
        outputs[TRIG_OUTPUT].setVoltage(10.f,chn);
        lights[GATE_LIGHT+chn].setSmoothBrightness(1.f,args.sampleTime*32);
      } else {
        outputs[TRIG_OUTPUT].setVoltage(0.f,chn);
        lights[GATE_LIGHT+chn].setSmoothBrightness(0.f,args.sampleTime*32);
      }
      populateOutputs(params[LEVEL_PARAM].getValue(),chn);
      if(isOn(curRow[chn],curCol[chn])) {
        outputs[GATE_OUTPUT].setVoltage(10.f,chn);
        if(params[ON_PARAM].getValue()>0.f)
          outputs[CLOCK_OUTPUT].setVoltage(inputs[STEP_INPUT].getVoltage(),chn);
        else
          outputs[CLOCK_OUTPUT].setVoltage(0.f,chn);
        outputs[MIDDLE_OUTPUT].setVoltage(params[LEVEL_PARAM].getValue(),chn);
      } else {
        outputs[GATE_OUTPUT].setVoltage(0.f,chn);
        outputs[MIDDLE_OUTPUT].setVoltage(0.f,chn);
        outputs[CLOCK_OUTPUT].setVoltage(0.f,chn);
      }
      lastRow[chn]=curRow[chn];
      lastCol[chn]=curCol[chn];
    }

    outputs[ROW_RIGHT_OUTPUT].setChannels(channels);
    outputs[ROW_LEFT_OUTPUT].setChannels(channels);
    outputs[COL_TOP_OUTPUT].setChannels(channels);
    outputs[COL_BOTTOM_OUTPUT].setChannels(channels);
    outputs[TOP_LEFT_OUTPUT].setChannels(channels);
    outputs[TOP_RIGHT_OUTPUT].setChannels(channels);
    outputs[BOTTOM_RIGHT_OUTPUT].setChannels(channels);
    outputs[BOTTOM_LEFT_OUTPUT].setChannels(channels);
    outputs[TRIG_OUTPUT].setChannels(channels);
    outputs[GATE_OUTPUT].setChannels(channels);
    outputs[MIDDLE_OUTPUT].setChannels(channels);
    outputs[CLOCK_OUTPUT].setChannels(channels);

    if(rightExpander.module) {
      if(rightExpander.module->model==modelC42E) {

        auto messageToExpander=(C42ExpanderMessage *)(rightExpander.module->leftExpander.producerMessage);
        messageToExpander->channels=channels;
        for(int k=0;k<channels;k++) {
          populateExpanderChannel(messageToExpander,params[LEVEL_PARAM].getValue(),k);
        }
        rightExpander.module->leftExpander.messageFlipRequested=true;
      }
    }

  }

  void setSize(int size) {
    world.size=size;
    float valueX=getParamQuantity(CV_X_PARAM)->getValue();
    float valueY=getParamQuantity(CV_Y_PARAM)->getValue();
    if(valueX>size)
      valueX=size;
    if(valueY>size)
      valueY=size;
    configParam(CV_X_PARAM,0,world.size,world.size/2,"X");
    getParamQuantity(CV_X_PARAM)->snapEnabled=true;
    configParam(CV_Y_PARAM,0,world.size,world.size/2,"Y");
    getParamQuantity(CV_Y_PARAM)->snapEnabled=true;
    getParamQuantity(CV_X_PARAM)->setValue(valueX);
    getParamQuantity(CV_Y_PARAM)->setValue(valueY);
    dirty=2;
  }

  int getSize() {
    return world.size;
  }
};

struct RuleSelectItem : MenuItem {
  C42 *module;
  std::vector<std::string> labels;

  RuleSelectItem(C42 *_module,std::vector<std::string> _labels) : module(_module),labels(std::move(_labels)) {
  }

  Menu *createChildMenu() override {
    Menu *menu=new Menu;
    for(unsigned k=0;k<labels.size();k++) {
      menu->addChild(createCheckMenuItem(labels[k],"",[=]() {
        return module->world.getRule()==k;
      },[=]() {
        module->world.setRule(k);
      }));
    }
    return menu;
  }
};

struct C42Widget : ModuleWidget {
  explicit C42Widget(C42 *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/C42.svg")));

    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,0)));
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));

    auto display=createWidget<C42Display<C42>>(mm2px(Vec(7,MHEIGHT-9-112)));
    display->box.size=mm2px(Vec(112,112));
    display->module=module;
    addChild(display);
    float x=125.5;
    float y=TY(114);
    addParam(createParam<MLEDM>(mm2px(Vec(x,y)),module,C42::STEP_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,C42::STEP_INPUT));
    addParam(createParam<MLED>(mm2px(Vec(x+18,y)),module,C42::ON_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+25.5f,y)),module,C42::ON_INPUT));

    addInput(createInput<SmallPort>(mm2px(Vec(133.5,TY(103))),module,C42::RST_INPUT));
    addParam(createParam<MLEDM>(mm2px(Vec(143,TY(103))),module,C42::RST_PARAM));

    y=TY(90);
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,C42::DENS_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,C42::DENS_INPUT));
    addParam(createParam<MLEDM>(mm2px(Vec(x+18,y)),module,C42::RND_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+25.5f,y)),module,C42::RND_INPUT));

    y+=13;
    auto param=createParam<MKnob<C42>>(mm2px(Vec(x,y)),module,C42::CV_X_PARAM);
    param->module=module;
    addParam(param);
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,C42::CV_X_INPUT));
    addParam(createParam<MLED>(mm2px(Vec(x+18,y)),module,C42::CV_ON_X_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+25.5f,y)),module,C42::CV_ON_X_INPUT));
    y+=11;

    param=createParam<MKnob<C42>>(mm2px(Vec(x,y)),module,C42::CV_Y_PARAM);
    param->module=module;
    addParam(param);
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,C42::CV_Y_INPUT));
    addParam(createParam<MLED>(mm2px(Vec(x+18,y)),module,C42::CV_ON_Y_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+25.5f,y)),module,C42::CV_ON_Y_INPUT));

    y=TY(53);
    addParam(createParam<TrimbotWhite>(mm2px(Vec(133.5,y)),module,C42::LEVEL_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(143,y)),module,C42::LEVEL_INPUT));


    x=128;
    y=TY(43.5);
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,C42::TOP_LEFT_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+11,y)),module,C42::COL_TOP_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+22,y)),module,C42::TOP_RIGHT_OUTPUT));
    y+=11;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,C42::ROW_LEFT_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+11,y)),module,C42::MIDDLE_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+22,y)),module,C42::ROW_RIGHT_OUTPUT));
    y+=11;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,C42::BOTTOM_LEFT_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+11,y)),module,C42::COL_BOTTOM_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+22,y)),module,C42::BOTTOM_RIGHT_OUTPUT));

    addOutput(createOutput<SmallPort>(mm2px(Vec(128,TY(9))),module,C42::GATE_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(139,TY(9))),module,C42::CLOCK_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(150,TY(9))),module,C42::TRIG_OUTPUT));
    x=125;
    y=MHEIGHT-6-1;
    int k=0;
    addChild(createLight<SmallSimpleLight<MLIGHT1>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT2>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT3>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT4>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT5>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT6>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT7>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT8>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT9>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT10>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT11>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT12>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT13>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT14>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT15>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT16>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
    x+=2;
    k++;
    addChild(createLight<SmallSimpleLight<MLIGHT1>>(mm2px(Vec(x,y)),module,C42::GATE_LIGHT+k));
  }

  void appendContextMenu(Menu *menu) override {
    C42 *module=dynamic_cast<C42 *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    std::vector<int> sizes={8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32};
    auto sizeSelectItem=new SizeSelectItem<C42>(module,sizes);
    sizeSelectItem->text="size";
    sizeSelectItem->rightText=string::f("%d",module->getSize())+"  "+RIGHT_ARROW;
    menu->addChild(sizeSelectItem);
    struct ClearItem : ui::MenuItem {
      C42 *module;

      explicit ClearItem(C42 *m) : module(m) {
      }

      void onAction(const ActionEvent &e) override {
        if(!module)
          return;
        module->world.clear();
      }
    };
    auto clearMenu=new ClearItem(module);
    clearMenu->text="Clear";
    menu->addChild(clearMenu);
    struct PasteItem : ui::MenuItem {
      C42 *module;

      explicit PasteItem(C42 *m) : module(m) {
      }

      void onAction(const ActionEvent &e) override {
        if(!module)
          return;
        const char *pasteText=glfwGetClipboardString(APP->window->win);
        module->pasteRLE(pasteText);
      }
    };
    auto pasteMenu=new PasteItem(module);
    pasteMenu->text="Paste RLE from Clipboard";
    menu->addChild(pasteMenu);

    auto ruleSelectItem=new RuleSelectItem(module,module->world.ruleLabels);
    ruleSelectItem->text="Rule";
    ruleSelectItem->rightText=module->world.ruleLabels[module->world.getRule()]+"  "+ +RIGHT_ARROW;
    menu->addChild(ruleSelectItem);
  }

};


Model *modelC42=createModel<C42,C42Widget>("C42");