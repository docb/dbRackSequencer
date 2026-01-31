#include <filesystem>

#include "plugin.hpp"
#include <regex>
#include <thread>
#include <utility>

struct Dir {
  std::string path;
  std::string name;
  std::vector<std::string> presetNames = {};
  std::vector<std::string> presetPaths = {};
  std::vector<json_t *> presetData = {};
  std::vector<Dir *> childs = {};
  Dir *parent = nullptr;
  // root dir
  Dir(Dir *dir1, Dir *dir2) {
    childs.push_back(dir1);
    childs.push_back(dir2);
  }

  std::string getPath() const {
    std::string ret = name;
    Dir *p = parent;
    while(p != nullptr && !p->name.empty()) {
      ret = p->name + "/" + ret;
      p = p->parent;
    }
    return "/" + ret;
  }

  static std::string trimName(const std::string &name) {
    if(name.length() < 12) return name;
    return name.substr(0, 9) + "..";
  }

  Dir(std::string _path, std::string _name, Dir *p = nullptr) : path(std::move(_path)), name(std::move(_name)), parent(p) {
    INFO("Dir construct %s %s", path.c_str(), name.c_str());
    std::vector<std::string> entries = system::getEntries(path);
    std::sort(entries.begin(), entries.end());
    for(const std::string& entry:entries) {
      std::string entry_name = system::getStem(entry);
      // Remove "1_", "42_", "001_", etc at the beginning of preset filenames
      std::regex r("^\\d+_");
      entry_name = std::regex_replace(entry_name, r, "");
      if(system::isDirectory(entry)) {
        INFO("scanning %s %s", entry.c_str(), entry_name.c_str());
        childs.push_back(new Dir(entry, trimName(entry_name), this));
        INFO("scanned %s %s", entry.c_str(), entry_name.c_str());
      } else if(system::getExtension(entry) == ".vcvm") {
        FILE *file = std::fopen(entry.c_str(), "r");
        if(!file)
          throw Exception("Could not load patch file %s", entry.c_str());
        DEFER({std::fclose(file);});

        INFO("Loading preset %s", entry.c_str());

        json_error_t error;
        json_t *moduleJ = json_loadf(file, 0, &error);
        if(!moduleJ)
          throw Exception("File is not a valid patch file. JSON parsing error at %s %d:%d %s", error.source, error.line,
                          error.column, error.text);

        presetData.push_back(moduleJ);
        presetNames.push_back(entry_name);
        presetPaths.push_back(entry);
      }
    }
    INFO("Dir constructed %s %s", path.c_str(), name.c_str());
  }

  ~Dir() {
    INFO("Dir destruct %s %s", path.c_str(), name.c_str());
    for(auto itr = presetData.begin();itr != presetData.end();itr++) {
      json_t *moduleJ = *itr;
      json_decref(moduleJ);
    }
    presetData.clear();
    presetNames.clear();
    presetPaths.clear();
    for(auto itr = childs.begin();itr != childs.end();itr++) {
      delete *itr;
    }
    childs.clear();
  }

  Dir *getChild(int k) {
    return childs[k];
  }

  Dir *getParent(int k) {
    return parent;
  }

  std::vector<std::string> getDirNames() {
    std::vector<std::string> ret = {};
    for(Dir *child:childs) {
      ret.push_back(child->name);
    }
    return ret;
  }
};

struct Preset : Module {
  enum ParamId {
    REFRESH_PARAM, PARAMS_LEN
  };

  enum InputId {
    CV_INPUT, TRIG_INPUT, TRIG_SAVE_INPUT, INPUTS_LEN
  };

  enum OutputId {
    OUTPUTS_LEN
  };

  enum LightId {
    LIGHTS_LEN
  };

  bool expanderChanged = false;
  bool presetChanged = false;
  bool naviChanged = false;
  int currentCV = -1;
  int currentNr = -1;
  bool overwrite=true;
  bool cvConnected=false;
  Preset() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
  }

  dsp::SchmittTrigger trig;

  Dir *rootDir = nullptr;
  Module *leftModule = nullptr;
  Dir *currentDir = nullptr;
  dsp::SchmittTrigger saveTrigger;

  void onRemove(const RemoveEvent &e) override {
    if(rootDir) {
      delete rootDir;
    }
    rootDir = nullptr;
    currentDir = nullptr;
  }

  void updatePresets() {
    if(leftModule) {
      std::string presetUserDir = leftModule->model->getUserPresetDirectory();
      Dir *userDir = new Dir(presetUserDir, "user");
      std::string presetFactoryDir = leftModule->model->getFactoryPresetDirectory();
      Dir *factoryDir = new Dir(presetFactoryDir, "factory");
      currentDir = rootDir = new Dir(userDir, factoryDir);
      userDir->parent = rootDir;
      factoryDir->parent = rootDir;
    } else {
      delete rootDir;
      rootDir = nullptr;
      currentDir = nullptr;
    }
  }

  void navigate(int pos) {
    naviChanged = true;
    if(!currentDir) return;
    if(pos < 0) {
      if(currentDir->parent)
        currentDir = currentDir->parent;
    } else {
      currentDir = currentDir->getChild(pos);
    }
  }

  void onExpanderChange(const ExpanderChangeEvent &e) override {
    if(e.side == false) {
      expanderChanged = true;
    }
  }

  void applyPreset(unsigned int nr) {
    if(!currentDir) return;
    if(nr < currentDir->presetData.size()) {
      currentNr = static_cast<int>(nr);
      if(leftModule) {
        json_t *moduleJ = currentDir->presetData[nr];
        leftModule->fromJson(moduleJ);
      }
    }
  }

  void savePreset() {
    if(currentDir == nullptr || currentNr == -1) return;
    std::string path = currentDir->presetPaths[currentNr];
    if(!overwrite) {
      std::string dir=currentDir->path;
      for(int k=1;;k++) {
        std::string stem=system::getDirectory(path) + "/" +  system::getStem(path) + "_" + std::to_string(k);
        if(!system::exists(stem+".vcvm")) {
          path=stem+".vcvm";
          break;
        }
      }
    }
    INFO("save preset %s",path.c_str());
    currentDir->presetData[currentNr] = leftModule->toJson();
    std::thread t([this,path]() {
      json_dump_file(currentDir->presetData[currentNr], path.c_str(), 0);
    });
    t.detach();
  }


  void process(const ProcessArgs &args) override {
    cvConnected=inputs[CV_INPUT].isConnected();
    if(leftExpander.module) {
      leftModule = leftExpander.module;
    } else {
      leftModule = nullptr;
    }
    if(cvConnected && currentDir) {
      int pos = clamp(inputs[CV_INPUT].getVoltage() * 10.f, -1.f, float(currentDir->presetData.size() - 1));
      if(pos != currentCV) {
        currentCV = pos;
        applyPreset(currentCV);
      }
    } else {
      currentCV = -1;
    }
    if(saveTrigger.process(inputs[TRIG_SAVE_INPUT].getVoltage())) {
      if(leftModule) savePreset();
    }
  }

  int getCurrentPreset() const {
    return currentNr;
  }
};

struct PresetButton : OpaqueWidget {
  Preset *module = nullptr;
  int nr;
  std::string label;
  std::basic_string<char> fontPath;
  bool pressed = false;

  PresetButton(Preset *m, const int _nr, std::string _label, const Vec pos, const Vec size) : module(m), nr(_nr), label(std::move(_label)) {
    fontPath = asset::plugin(pluginInstance, "res/FreeMonoBold.ttf");
    box.size = size;
    box.pos = pos;
  }

  void onButton(const ButtonEvent &e) override {
    if(e.button == GLFW_MOUSE_BUTTON_LEFT) {
      if(e.action == GLFW_PRESS) {
        if(!module->cvConnected) {
          module->applyPreset(nr);
          pressed = true;
        }
      } else {
        pressed = false;
      }
    }
  }

  void draw(const DrawArgs &args) override {
    bool selected = false;
    if(module) {
      selected = module->getCurrentPreset()==nr;
    }
    if(pressed) {
      nvgFillColor(args.vg, nvgRGB(0x7e, 0xa6, 0xd3));
    } else {
      if(selected)
        nvgFillColor(args.vg, nvgRGB(0x3c, 0x71, 0x4c));
      else
        nvgFillColor(args.vg, nvgRGB(0x3c, 0x4c, 0x71));
    }
    nvgStrokeColor(args.vg, nvgRGB(0xc4, 0xc9, 0xc2));
    nvgBeginPath(args.vg);
    nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3.f);
    nvgFill(args.vg);
    nvgStroke(args.vg);
    std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
    nvgFontSize(args.vg, box.size.y - 2);
    nvgFontFaceId(args.vg, font->handle);
    NVGcolor textColor = nvgRGB(0xff, 0xff, 0xaa);
    nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(args.vg, textColor);
    nvgText(args.vg, 2, box.size.y / 2 + 1.5, label.c_str(), NULL);
  }
};

struct DirButton : OpaqueWidget {
  Preset *module = nullptr;
  int nr;
  std::string label;
  std::basic_string<char> fontPath;
  bool pressed = false;

  DirButton(Preset *m, int _nr, std::string _label, Vec pos, Vec size) : module(m), nr(_nr), label(std::move(_label)) {
    fontPath = asset::plugin(pluginInstance, "res/FreeMonoBold.ttf");
    box.size = size;
    box.pos = pos;
  }

  void onButton(const ButtonEvent &e) override {
    if(e.button == GLFW_MOUSE_BUTTON_LEFT) {
      if(e.action == GLFW_PRESS) {
        pressed = true;
      } else {
        pressed = false;
        module->navigate(nr);
      }
    }
  }

  void draw(const DrawArgs &args) override {
    if(pressed) {
      nvgFillColor(args.vg, nvgRGB(0x7e, 0xa6, 0xd3));
    } else {
      nvgFillColor(args.vg, nvgRGB(0x3c, 0x4c, 0x71));
    }
    nvgStrokeColor(args.vg, nvgRGB(0xc4, 0xc9, 0xc2));
    nvgBeginPath(args.vg);
    nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3.f);
    nvgFill(args.vg);
    nvgStroke(args.vg);
    std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
    nvgFontSize(args.vg, box.size.y - 2);
    nvgFontFaceId(args.vg, font->handle);
    NVGcolor textColor = nvgRGB(0xff, 0xff, 0xaa);
    nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(args.vg, textColor);
    nvgText(args.vg, 2, box.size.y / 2 + 1.5, label.c_str(), nullptr);
  }
};

static unsigned int getMaxLength(const std::vector<std::string> &list) {
  unsigned int max = 0;
  for(const std::string &s:list) {
    if(s.length() > max) max = s.length();
  }
  return max;
}

struct DirWidget : OpaqueWidget {
  Preset *module = nullptr;

  explicit DirWidget(Preset *p) : module(p) {
    box.pos = Vec(0, 0);
  }

  void init(const std::vector<std::string> &names) {
    clearChildren();
    if(!module) return;
    unsigned int max = getMaxLength(names);
    if(max < 8) max = 8;
    auto max_f = static_cast<float>(max);
    box.size = Vec(max_f * 6.f, (static_cast<float>(names.size()) + 1.f) * 11.f);
    float y = 0;
    addChild(new DirButton(module, -1, "..", Vec(0, y), Vec(max_f * 5.5f, 11)));
    int k = 0;
    for(const std::string &name:names) {
      y += 11;
      addChild(new DirButton(module, k, name, Vec(0, y), Vec(max_f * 5.5f, 11)));
      k++;
    }
  }

  void clear() {
    clearChildren();
    box.size = Vec(100, 11);
  }
};

struct PresetItemWidget : OpaqueWidget {
  Preset *module = nullptr;

  explicit PresetItemWidget(Preset *p) : module(p) {
    box.pos = Vec(0, 0);
  }

  void init(std::vector<std::string> names) {
    unsigned int max = getMaxLength(names);
    if(max < 8) max = 8;
    box.size = Vec(static_cast<float>(max) * 6, (static_cast<float>(names.size()) + 1) * 12);
    clearChildren();
    int k = 0;
    float y = 0;
    for(auto itr = names.begin();itr != names.end();++itr, k++, y += 12) {
      const std::string &l = *itr;
      auto *p = new PresetButton(module, k, l, Vec(0, y), Vec(static_cast<float>(max) * 6, 11));
      addChild(p);
    }
  }

  void clear() {
    clearChildren();
    box.size = Vec(100, 11);
  }
};

struct PathWidget : OpaqueWidget {
  Preset *module = nullptr;
  std::string fontPath;
  Tooltip *tooltip = nullptr;

  PathWidget(Preset *p, Vec pos, Vec size) : OpaqueWidget(), module(p) {
    box.pos = pos;
    box.size = size;
    fontPath = asset::plugin(pluginInstance, "res/FreeMonoBold.ttf");
  }

  void draw(const DrawArgs &args) override {
    if(!module) return;
    if(!module->currentDir) return;
    std::string path = module->currentDir->getPath();
    if(path.length() > 14) {
      path = ".." + path.substr(path.length() - 12);
    }
    std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
    nvgFontSize(args.vg, box.size.y - 2);
    nvgFontFaceId(args.vg, font->handle);
    NVGcolor textColor = nvgRGB(0xff, 0xff, 0xaa);
    nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(args.vg, textColor);
    nvgText(args.vg, 2, box.size.y / 2.f + 1.5f, path.c_str(), nullptr);
  }

  void onDragHover(const event::DragHover &e) override {
    if(e.button == GLFW_MOUSE_BUTTON_LEFT) {
      e.consume(this);
    }
    Widget::onDragHover(e);
  }

  void onDragEnter(const event::DragEnter &e) override {
    createTooltip();
    Widget::onDragEnter(e);
  };

  void createTooltip() {
    if(!module) return;
    if(!module->currentDir) return;
    std::string path = module->currentDir->getPath();
    if(tooltip == nullptr) {
      tooltip = new Tooltip;
      tooltip->text = path;
      APP->scene->addChild(tooltip);
    }
  }


  void destroyTooltip() {
    if(!tooltip)
      return;
    APP->scene->removeChild(tooltip);
    delete tooltip;
    tooltip = nullptr;
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

struct Presets : OpaqueWidget {
  Preset *module = nullptr;
  DirWidget *dirWidget;
  PresetItemWidget *presetWidget;

  Presets(Preset *m, Vec pos, Vec size) : OpaqueWidget(), module(m) {
    box.size = size;
    box.pos = pos;
    dirWidget = new DirWidget(module);
    presetWidget = new PresetItemWidget(module);

    auto *scrollDirWidget = new ScrollWidget();
    scrollDirWidget->box.size = Vec(size.x, size.y * 0.3);
    scrollDirWidget->box.pos = Vec(0, 0);
    addChild(scrollDirWidget);
    scrollDirWidget->container->addChild(dirWidget);
    auto *scrollPresetWidget = new ScrollWidget();
    scrollPresetWidget->box.size = Vec(size.x, size.y * 0.7);
    scrollPresetWidget->box.pos = Vec(size.x * 0, size.y * 0.3);
    addChild(scrollPresetWidget);
    scrollPresetWidget->container->addChild(presetWidget);
  }

  void update() {
    if(module->currentDir) {
      dirWidget->init(module->currentDir->getDirNames());
      presetWidget->init(module->currentDir->presetNames);
    } else {
      dirWidget->clear();
      presetWidget->clear();
    }
  }

  void draw(const DrawArgs &args) override {
    if(module) {
      if(module->expanderChanged) {
        module->updatePresets();
        update();
        module->expanderChanged = false;
      }
      if(module->naviChanged) {
        update();
        module->naviChanged = false;
      }
      if(module->presetChanged) {
        module->presetChanged = false;
        module->applyPreset(module->currentCV);
      }
    }
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFillColor(args.vg, nvgRGB(0, 0, 60));
    nvgFill(args.vg);

    OpaqueWidget::draw(args);
  }
};

struct RefreshButton : MLEDM {
  Preset *module = nullptr;

  void onChange(const ChangeEvent &e) override {
    SvgSwitch::onChange(e);
    if(module) {
      if(module->params[Preset::REFRESH_PARAM].getValue() > 0.f)
        module->expanderChanged = true;
    }
  }
};

struct PresetWidget : ModuleWidget {
  PresetWidget(Preset *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Preset.svg")));
    auto pathWidget = new PathWidget(module, mm2px(Vec(1, 6)), mm2px(Vec(29, 4)));
    addChild(pathWidget);
    auto presetsWidget = new Presets(module, mm2px(Vec(1, 14)), mm2px(Vec(29, 94)));
    addChild(presetsWidget);
    addInput(createInput<SmallPort>(mm2px(Vec(2.5, 116)), module, Preset::CV_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(11.5, 116)), module, Preset::TRIG_SAVE_INPUT));
    auto refreshParam = createParam<RefreshButton>(mm2px(Vec(21.5, 116)), module, Preset::REFRESH_PARAM);
    refreshParam->module = module;
    addParam(refreshParam);
  }
  void appendContextMenu(Menu* menu) override {
    auto *module=dynamic_cast<Preset *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    menu->addChild(createBoolPtrMenuItem("Overwrite Presets","",&module->overwrite));
  }
};


Model *modelPreset = createModel<Preset, PresetWidget>("Preset");
