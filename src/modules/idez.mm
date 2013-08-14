module idez(qt,io)
  qt:  memetalk/qt/1.0();
  io:  memetalk/io/1.0();
  [QWidget, QMainWindow, QsciScintilla, QLineEdit, QComboBox, QTableWidget] <= qt;
{

  evalWithVars: fun(text, vars) {
    var fn = evalWithVarsFn(text, vars);
    var res = fn();
    return {"result": res, "env": fn.getEnv()};
  }

  evalWithVarsFn: fun(text, vars) {
    var cmod = get_compiled_module(thisModule);
    var code = "fun() {" + text + "}";
    var cfn = CompiledFunction.newClosure(code, thisContext.compiledFunction(), false);
    return cfn.asContextWithVars(thisModule, vars);
  }

  evalWithFrame: fun(text, frame) {
    var cmod = get_compiled_module(thisModule);
    var code = "fun() {" + text + "}";
    var cfn = CompiledFunction.newClosure(code, thisContext.compiledFunction(), false);
    var fn = cfn.asContextWithFrame(thisModule, frame);
    var res = fn();
    return {"result": res, "env": fn.getEnv()};
  }

  class LineEditor < QLineEdit {
    fields: receiver;
    init new: fun(parent, receiver) {
      super.new(parent);
      if (parent == null) {
        this.initActions();
      }
      @receiver = receiver;
    }

    instance_method initActions: fun() {
      this.connect("returnPressed", fun() {
        this.selectAllAndDoit(null);
      });

      var action = qt.QAction.new("&Do it", this);
      action.setShortcut("ctrl+d");
      action.connect("triggered", fun() {
          this.doIt();
      });
      action.setShortcutContext(0); //widget context
      this.addAction(action);

      action = qt.QAction.new("&Print it", this);
      action.setShortcut("ctrl+p");
      action.connect("triggered", fun() {
          this.printIt();
      });
      action.setShortcutContext(0); //widget context
      this.addAction(action);

      action = qt.QAction.new("&Inspect it", this);
      action.setShortcut("ctrl+i");
      action.connect("triggered", fun() {
          this.inspectIt();
      });
      action.setShortcutContext(0); //widget context
      this.addAction(action);

      action = qt.QAction.new("De&bug it", this);
      action.setShortcut("ctrl+b");
      action.connect("triggered", fun() {
          this.debugIt();
      });
      action.setShortcutContext(0); //widget context
      this.addAction(action);
    }
    instance_method evalSelection: fun() {
      var r = evalWithVars(this.selectedText(), {"this" : @receiver});
      return r["result"];
    }
    instance_method insertSelectedText: fun(text) {
      var len = this.text().size();
      this.setText(this.text() + text);
      this.setSelection(len, text.size());
    }
    instance_method selectAllAndDoit: fun(receiver) {
        this.selectAll();
        this.doIt();
    }
    instance_method doIt: fun() {
      try {
        this.evalSelection();
      } catch(e) {
        this.insertSelectedText(e.value());
      }
    }
    instance_method printIt: fun() {
      try {
        var res = this.evalSelection();
        this.insertSelectedText(res.toString());
      } catch(e) {
        this.insertSelectedText(e.value());
      }
    }
    instance_method inspectIt: fun() {
      try {
        var res = this.evalSelection();
        Inspector.new(res).show();
      } catch(e) {
        this.insertSelectedText(e.value());
      }
    }
    instance_method debugIt: fun() {
      try {
        var fn = evalWithVarsFn(this.selectedText(), thisModule, {"this" : @receiver});
        VMProcess.debug(fn,[]);
      } catch(e) {
        this.insertSelectedText(e.value());
      }
    }
  }

  class Editor < QsciScintilla {
    fields: getContext, getFrame, afterEval;
    init new: fun(parent, getContext, getFrame, afterEval) {
      super.new(parent);
      if (parent == null) {
        this.initActions();
      }
      @getContext = getContext;
      @getFrame = getFrame;
      @afterEval = afterEval;
    }

    instance_method initActions: fun() {
      var action = qt.QAction.new("&Do it", this);
      action.setShortcut("ctrl+d");
      action.connect("triggered", fun() {
          this.doIt();
      });
      action.setShortcutContext(0); //widget context
      this.addAction(action);

      action = qt.QAction.new("&Print it", this);
      action.setShortcut("ctrl+p");
      action.connect("triggered", fun() {
          this.printIt();
      });
      action.setShortcutContext(0); //widget context
      this.addAction(action);

      action = qt.QAction.new("&Inspect it", this);
      action.setShortcut("ctrl+i");
      action.connect("triggered", fun() {
          this.inspectIt();
      });
      action.setShortcutContext(0); //widget context
      this.addAction(action);

      action = qt.QAction.new("De&bug it", this);
      action.setShortcut("ctrl+b");
      action.connect("triggered", fun() {
          this.debugIt();
      });
      action.setShortcutContext(0); //widget context
      this.addAction(action);
    }

    instance_method evalSelection: fun() {
      var r = null;
      if (@getContext) {
        r = evalWithVars(this.selectedText(), @getContext());
      } else {
        r = evalWithFrame(this.selectedText(), @getFrame());
      }
      if (@afterEval) {
        @afterEval(r["env"]);
      }
      return r["result"];
    }

    instance_method doIt: fun() {
      try {
        this.evalSelection();
      } catch(e) {
        this.insertSelectedText(e.value());
      }
    }

    instance_method printIt: fun() {
      try {
        var res = this.evalSelection();
        this.insertSelectedText(res.toString());
      } catch(e) {
        this.insertSelectedText(e.value());
      }
    }

    instance_method inspectIt: fun() {
      try {
        var res = this.evalSelection();
        Inspector.new(res).show();
      } catch(e) {
        this.insertSelectedText(e.value());
      }
    }

    instance_method debugIt: fun() {
      try {
        var fn = evalWithVarsFn(this.selectedText(), @getContext());
        VMProcess.debug(fn,[]);
        if (@afterEval) {
          @afterEval(fn.getEnv());
        }
      } catch(e) {
        this.insertSelectedText(e.value());
      }
    }
    instance_method insertSelectedText: fun(text) {
      var pos = this.getSelection();
      this.insertAt(text, pos["end_line"], pos["end_index"]);
      //text.size(): this is rude
      this.setSelection(pos["end_line"], pos["end_index"], pos["end_line"] + text.count("\n"), pos["end_index"] + text.size());
    }
  }

  class Workspace < QMainWindow {
    fields: editor, variables;
    init new: fun() {
      super.new();
      @variables = {};

      this.setWindowTitle("Workspace");

      @editor = Editor.new(this, fun() { @variables }, null,
                           fun(env) { @variables = env + @variables; });

      @editor.initActions();
      this.setCentralWidget(@editor);

      var execMenu = this.menuBar().addMenu("&Exploring");
      @editor.actions().each(fun(action) {
        execMenu.addAction(action)
      });
    }
  }

  class Inspector  < QMainWindow {
    fields: inspectee, variables, mirror, fieldList, textArea, lineEdit;
    init new: fun(inspectee) {
      super.new();

      @variables = {"this":@inspectee};
      @inspectee = inspectee;
      @mirror = Mirror.new(@inspectee);

      this.resize(300,250);
      this.setWindowTitle("Inspector");
      var centralWidget = QWidget.new(this);
      var mainLayout = qt.QVBoxLayout.new(centralWidget);

      var hbox = qt.QHBoxLayout.new(null);

      @fieldList = qt.QListWidget.new(centralWidget);
      @fieldList.setMaximumWidth(200);
      hbox.addWidget(@fieldList);

      @textArea = Editor.new(centralWidget, fun() { {"this" : @inspectee} }, null, null);

      hbox.addWidget(@textArea);

      mainLayout.addLayout(hbox);

      @lineEdit = LineEditor.new(centralWidget, @inspectee);
      mainLayout.addWidget(@lineEdit);

      @lineEdit.connect("returnPressed", fun() {
        @lineEdit.selectAllAndDoit(@inspectee);
      });

      this.setCentralWidget(centralWidget);

      this.loadValues();
      @fieldList.connect("currentItemChanged", fun(item, prev) {
          this.itemSelected(item);
      });
      @fieldList.connect("itemActivated", fun(item) {
          this.itemActivated(item);
      });

      var execMenu = this.menuBar().addMenu("&Exploring");

      var action = qt.QAction.new("&Do it", this);
      action.setShortcut("ctrl+d");
      action.connect("triggered", fun() {
          this.doIt();
      });
      execMenu.addAction(action);

      action = qt.QAction.new("&Print it", this);
      action.setShortcut("ctrl+p");
      action.connect("triggered", fun() {
          this.printIt();
      });
      execMenu.addAction(action);

      action = qt.QAction.new("&Inspect it", this);
      action.setShortcut("ctrl+i");
      action.connect("triggered", fun() {
          this.inspectIt();
      });
      execMenu.addAction(action);

      action = qt.QAction.new("De&bug it", this);
      action.setShortcut("ctrl+b");
      action.connect("triggered", fun() {
          this.debugIt();
      });
      execMenu.addAction(action);

      action = qt.QAction.new("Accept it", execMenu);
      action.setShortcut("ctrl+s");
      action.connect("triggered", fun() {
          this.acceptIt();
      });
      execMenu.addAction(action);
    }

    instance_method loadValues: fun() {
      qt.QListWidgetItem.new('*self', @fieldList);
      @mirror.fields().each(fun(name) {
          qt.QListWidgetItem.new(name, @fieldList);
      });
    }

    instance_method itemSelected: fun(item) { //QListWidgetItem, curr from the signal
      if (item.text() == '*self') {
        @textArea.setText(@inspectee.toString());
      } else {
        var value = @mirror.valueFor(item.text());
        @textArea.setText(value.toSource());
      }
    }

    instance_method itemActivated: fun(item) {
      if (item.text() != '*self') {
        var value = @mirror.valueFor(item.text());
        Inspector.new(value).show();
      }
    }

    instance_method doIt: fun() {
      if (@lineEdit.hasFocus()) {
        @lineEdit.doIt();
      } else {
        @textArea.doIt();
      }
    }

    instance_method printIt: fun() {
      if (@lineEdit.hasFocus()) {
        @lineEdit.printIt();
      } else {
        @textArea.printIt();
      }
    }
    instance_method inspectIt: fun() {
      if (@lineEdit.hasFocus()) {
        @lineEdit.inspectIt();
      } else {
        @textArea.inspectIt();
      }
    }
    instance_method debugIt: fun() {
      if (@lineEdit.hasFocus()) {
        @lineEdit.debugIt();
      } else {
        @textArea.debugIt();
      }
    }
    instance_method acceptIt: fun() {
      var fn = evalWithVarsFn(@textArea.text(), {"this":@inspectee});
      var new_value = fn.apply([]);
      var slot = @fieldList.currentItem().text();
      @mirror.setValueFor(slot, new_value);
    }
  }


  class StackCombo < QComboBox {
    fields: frames;
    init new: fun(parent, execframes) {
      super.new(parent);
      @frames = execframes;
    }
    instance_method updateInfo: fun() {
      this.clear();
      @frames.names().each(fun(name) {
        this.addItem(name);
      });
      this.setCurrentIndex(@frames.size() - 1);
    }
  }

  class ExecutionFrames {
    fields: vmproc;
    init new: fun(vmproc) {
      @vmproc = vmproc;
    }
    instance_method names: fun() {
      return @vmproc.stackFrames().map(fun(frame) {
        frame.contextPointer().compiledFunction().fullName() + ":" + frame.instructionPointer()["start_line"].toString()
      });
    }
    instance_method codeFor: fun(i) {
      var cp = @vmproc.stackFrames().get(i).contextPointer().compiledFunction();
      if (cp.isTopLevel()) {
        return cp.text();
      } else {
        if (cp.isEmbedded()) {
          return cp.topLevelCompiledFunction().text();
        } else {
          return cp.text();
        }
      }
    }
    instance_method localsFor: fun(i) { // this is used for the local variable list widet
      return @vmproc.stackFrames().get(i).localVars();
    }
    instance_method frame: fun(i) { // this is used for doIt/printIt/etc.
      return @vmproc.stackFrames().get(i);
    }
    instance_method moduleVarsFor: fun(i) { // this is used for the module variables list wdiget
      // var pnames = null;
      // if (i < @vmproc.stackFrames().size()) {
      //   pnames = @vmproc.stackFrames().get(i).modulePointer()._compiledModule().params();
      // } else {
      //   pnames = @vmproc.modulePointer()._compiledModule().params();
      // }
      // var ret = {};
      // pnames.each(fun(name) {
      //   ret[name] = @vmproc.modulePointer.entry(name);
      // });
      return {};
    }
    instance_method locationInfoFor: fun(i) {
      return @vmproc.stackFrames().get(i).instructionPointer();
    }
    instance_method size: fun() {
      return @vmproc.stackFrames().size();
    }
  }

  class VariableListWidget < QTableWidget {
    fields: variables;
    init new: fun(parent) {
      super.new(2, 2, parent);
      this.verticalHeader().hide();
      this.setSelectionMode(1);
      var header = this.horizontalHeader();
      header.setStretchLastSection(true);
      this.setSortingEnabled(false);
    }
    instance_method setVariables: fun(vars) {
      @variables = vars;
      this.clear();
      this.setHorizontalHeaderLabels(['Name', 'Value']);
      var i = 0;
      vars.each(fun(name,value) {
        this.setItem(i, 0, qt.QTableWidgetItem.new(name));
        this.setItem(i, 1, qt.QTableWidgetItem.new(value.toString()));
        i = i + 1;
      });
    }
  }

  class DebuggerUI < QMainWindow {
    fields: cont_on_exit, frame_index, process, execFrames, stackCombo, editor, localVarList, moduleVarList;
    init new: fun(process) {
      super.new();
      @process = process;

      @cont_on_exit = true;
      @frame_index = 0;

      this.resize(700,250);
      this.setWindowTitle("Debugger");
      var centralWidget = QWidget.new(this);
      var mainLayout = qt.QVBoxLayout.new(centralWidget);

      @execFrames = ExecutionFrames.new(process);

      @stackCombo = StackCombo.new(centralWidget, @execFrames);
      mainLayout.addWidget(@stackCombo);

      @editor = Editor.new(centralWidget, null, fun() { @execFrames.frame(@frame_index) }, null);
      mainLayout.addWidget(@editor);

      @stackCombo.connect("currentIndexChanged",fun(i) {
        if (0 <= i) {
          @frame_index = i;
          @editor.setText(@execFrames.codeFor(i));
          var locInfo = @execFrames.locationInfoFor(i);
          @editor.pausedAtLine(locInfo["start_line"]-1, locInfo["start_col"], locInfo["end_line"]-1, locInfo["end_col"]);
          @localVarList.setVariables(@execFrames.localsFor(i));
          @moduleVarList.setVariables(@execFrames.moduleVarsFor(i));
        }
      });

      var hbox = qt.QHBoxLayout.new(null);
      @localVarList = VariableListWidget.new(centralWidget);
      hbox.addWidget(@localVarList);

      @moduleVarList = VariableListWidget.new(centralWidget);
      hbox.addWidget(@moduleVarList);

      mainLayout.addLayout(hbox);
      this.setCentralWidget(centralWidget);

      var execMenu = this.menuBar().addMenu("&Debugging");
      var action = qt.QAction.new("Step &Into", execMenu);
      action.setShortcut("F6");
      action.connect("triggered", fun() {
        this.stepInto();
      });
      execMenu.addAction(action);

      action = qt.QAction.new("Step &Over", execMenu);
      action.setShortcut("F7");
      action.connect("triggered", fun() {
        this.stepOver()
      });
      execMenu.addAction(action);

      action = qt.QAction.new("&Continue", execMenu);
      action.setShortcut("F5");
      action.connect("triggered", fun() {
        this.continue()
      });
      execMenu.addAction(action);

      action = qt.QAction.new("Compile and &Rewind", execMenu);
      action.setShortcut("ctrl+s");
      action.connect("triggered", fun() {
        this.compileAndRewind()
      });
      execMenu.addAction(action);

      action = qt.QAction.new("Leave context", execMenu);
      action.setShortcut("ctrl+o");
      action.connect("triggered", fun() {
        //this.leaveContext(); //drop stack frame
      });
      execMenu.addAction(action);

      execMenu = this.menuBar().addMenu("&Exploring");
      action = qt.QAction.new("&Do it", this);
      action.setShortcut("ctrl+d");
      action.connect("triggered", fun() {
        @editor.doIt();
      });
      execMenu.addAction(action);

      action = qt.QAction.new("&Print it", this);
      action.setShortcut("ctrl+p");
      action.connect("triggered", fun() {
          @editor.printIt();
      });
      execMenu.addAction(action);

      action = qt.QAction.new("&Inspect it", this);
      action.setShortcut("ctrl+i");
      action.connect("triggered", fun() {
          @editor.inspectIt();
      });
      execMenu.addAction(action);

      action = qt.QAction.new("De&bug it", this);
      action.setShortcut("ctrl+b");
      action.connect("triggered", fun() {
          @editor.debugIt();
      });
      execMenu.addAction(action);

      action = qt.QAction.new("Accept it", execMenu);
      action.setShortcut("ctrl+s");
      action.connect("triggered", fun() {
          this.acceptIt();
      });
      execMenu.addAction(action);

      @stackCombo.updateInfo();
    }
    instance_method closeEvent: fun() {
      if (@cont_on_exit) {
        @process.continue();
      }
    }
    instance_method stepInto: fun() {
      if(@process.stepInto()) {
        @stackCombo.updateInfo();
      } else {
        @cont_on_exit = false;
        this.close();
      }
    }
    instance_method stepOver: fun() {
      if(@process.stepOver()) {
        @stackCombo.updateInfo();
      } else {
        @cont_on_exit = false;
        this.close();
      }
    }
    instance_method continue: fun() {
      this.close();
    }
    instance_method compileAndRewind: fun() {
      var text = @editor.text();
      @process.contextPointer().compiledFunction().setCode(text);
      @process.rewind();
      @stackCombo.updateInfo();
    }
  }

  class MiniBuffer < QWidget {
    fields: label, lineEdit, callback;
    init new: fun(parent) {
      super.new(parent);

      this.hide();
      this.setMaximumHeight(30);
      @callback = null;
      @lineEdit = QLineEdit.new(this);
      @lineEdit.setMinimumSize(200,30);
      @label = qt.QLabel.new(this);
      var l = qt.QHBoxLayout.new(this);
      l.addWidget(@label);
      l.addWidget(@lineEdit);
      l.setContentsMargins(10,2,10,2);

      @lineEdit.connect("returnPressed", fun() {
        if (@callback) {
          @callback(@lineEdit.text());
        }
        @callback = null;
        this.hide();
      });
    }

    instance_method prompt: fun(labelText, defaultValue, callback) {
      @callback = callback;
      @label.setText(labelText);
      @lineEdit.setText(defaultValue.toString());
      this.show();
      @lineEdit.setFocus();
    }
  }

  class CommandHistory {
    fields: undo, redo, next;
    init new: fun() {
      @next = null;
      @undo = null;
      @redo = null;
    }
    instance_method add: fun(undo, redo) {
      @undo = undo;
      @redo = redo;
      @next = "undo";
    }
    instance_method undo: fun() {
      if (@next == "undo") {
        @undo();
        @next = "redo";
      }
    }
    instance_method redo: fun() {
      if (@next == "redo") {
        @redo();
        @next = "undo";
      }
    }
  }

  class ExplorerEditor < Editor {
    fields: cfun;
    init new: fun(cfun, parent, getContext, getFrame, afterEval) {
      super.new(parent, getContext, getFrame, afterEval);
      @cfun = cfun;
    }

    instance_method accept: fun() {
        try {
          @cfun.setCode(this.text());
        } catch(ex) {
          this.insertSelectedText(ex.value());
        }
    }

    instance_method cfun: fun() {
      return @cfun;
    }
  }

  class ModuleExplorer < QMainWindow {
    fields: webview, miniBuffer, current_cmodule, chistory, statusLabel;
    init new: fun() {
      super.new();

      @chistory = CommandHistory.new();

      this.setWindowTitle("Memetalk");
      this.resize(800,600);

      var centralWidget = QWidget.new(this);
      this.setCentralWidget(centralWidget);

      var l = qt.QVBoxLayout.new(centralWidget);
      @webview = qt.QWebView.new(centralWidget);
      l.addWidget(@webview);

      @miniBuffer = MiniBuffer.new(centralWidget);
      l.addWidget(@miniBuffer);

      @statusLabel = qt.QLabel.new(this.statusBar());
      @statusLabel.setMinimumWidth(300);

      @webview.page().setLinkDelegationPolicy(2);

      @webview.page().enablePluginsWith("editor", fun(params) {
        var variables = {};

        var e = null;
        if (params.has("module_function")) {
          var cfn = get_module(params["module_name"]).compiled_functions()[params["function_name"]];
          e = ExplorerEditor.new(cfn, null, fun() { variables }, null,
                                 fun(env) { variables = env + variables;});
        } else {
          if (params.has("code")) {
            e = ExplorerEditor.new(null, null, fun() { variables }, null,
                                   fun(env) { variables = env + variables;});
          }
        }
        e.setText(params["code"]);
        e.setStyleSheet("border-style: outset;");
        return e;
      });

      @webview.connect('linkClicked', fun(url) {
        io.print("URL selected: " + url);

        if (url == "/") {
          this.show_home();
          return null;
        }

        if (url == "/tutorial") {
          this.show_tutorial();
          return null;
        }
        if (url == "/modules-index") {
          this.show_modules();
          return null;
        }
        var modules = available_modules();
        var name = url.from(1);
        if (modules.has(name)) {
          this.show_module(name);
          return null;
        }
      });

      this.initActions();
      this.show_home();
    }

    instance_method initActions: fun() {
      var execMenu = this.menuBar().addMenu("&System");
      var action = qt.QAction.new("&Save", execMenu);
      action.setShortcut("alt+x,s");
      action.connect("triggered", fun() {
        io.print("save to filesystem");
      });
      action.setShortcutContext(1);
      execMenu.addAction(action);

      action = qt.QAction.new("&Accept code", execMenu);
      action.setShortcut("alt+x,a");
      action.connect("triggered", fun() {
        this.action_acceptCode();
      });
      action.setShortcutContext(1);
      execMenu.addAction(action);

      action = qt.QAction.new("Dismiss Mini Buffer", execMenu);
      action.setShortcut("ctrl+g");
      action.connect("triggered", fun() {
        @miniBuffer.hide();
      });
      action.setShortcutContext(1);
      execMenu.addAction(action);

      action = qt.QAction.new("Undo", execMenu);
      action.setShortcut("alt+u");
      action.connect("triggered", fun() {
        @chistory.undo();
      });
      action.setShortcutContext(1);
      execMenu.addAction(action);

      action = qt.QAction.new("Redo", execMenu);
      action.setShortcut("alt+r");
      action.connect("triggered", fun() {
        @chistory.redo();
      });
      action.setShortcutContext(1);
      execMenu.addAction(action);

      execMenu = this.menuBar().addMenu("&Edit Module");
      action = qt.QAction.new("&Rename module", execMenu);
      action.setShortcut("alt+m,r");
      action.connect("triggered", fun() {
        this.action_renameModule()
      });
      action.setShortcutContext(1);
      execMenu.addAction(action);

      action = qt.QAction.new("Edit Module &Parameters", execMenu);
      action.setShortcut("alt+m,p");
      action.connect("triggered", fun() {
        io.print("Edit module parameters");
      });
      action.setShortcutContext(1);
      execMenu.addAction(action);

      action = qt.QAction.new("Edit Module &Aliases", execMenu);
      action.setShortcut("alt+m,a");
      action.connect("triggered", fun() {
        io.print("Edit module aliases");
      });
      action.setShortcutContext(1);
      execMenu.addAction(action);

      action = qt.QAction.new("Add Module &Function", execMenu);
      action.setShortcut("alt+m,f");
      action.connect("triggered", fun() {
        this.action_addFunction()
      });
      action.setShortcutContext(1);
      execMenu.addAction(action);

      action = qt.QAction.new("&Delete Module Function", execMenu);
      action.setShortcut("alt+m,d");
      action.connect("triggered", fun() {
        this.action_deleteFunction()
      });
      action.setShortcutContext(1);
      execMenu.addAction(action);

      execMenu = this.menuBar().addMenu("&Edit Class");
      action = qt.QAction.new("&Add Class", execMenu);
      action.setShortcut("alt+c,a");
      action.connect("triggered", fun() {
        io.print("Add class");
      });
      action.setShortcutContext(1);
      execMenu.addAction(action);

      action = qt.QAction.new("Rename Class", execMenu);
      action.setShortcut("alt+c,r");
      action.connect("triggered", fun() {
        io.print("Rename class/superclass");
      });
      action.setShortcutContext(1);
      execMenu.addAction(action);

      action = qt.QAction.new("Edit &Fields", execMenu);
      action.setShortcut("alt+c,f");
      action.connect("triggered", fun() {
        io.print("Edit fields");
      });
      action.setShortcutContext(1);
      execMenu.addAction(action);

      action = qt.QAction.new("Add &Method", execMenu);
      action.setShortcut("alt+c,m");
      action.connect("triggered", fun() {
        io.print("Add method");
      });
      action.setShortcutContext(1);
      execMenu.addAction(action);

      action = qt.QAction.new("&Delete Method", execMenu);
      action.setShortcut("alt+c,d");
      action.connect("triggered", fun() {
        io.print("Delete Method");
      });
      action.setShortcutContext(1);
      execMenu.addAction(action);
    }

    instance_method command: fun(redo, undo) {
      redo();
      @chistory.add(undo,redo);
    }

    instance_method action_acceptCode: fun() {
      if (@current_cmodule == null) {
        @statusLabel.setText("No current module");
        return null;
      }

      var e = qt.QApplication.focusWidget();
      if (Mirror.vtFor(e) == ExplorerEditor) {
        e.accept();
        @statusLabel.setText("Code changed");
      } else {
        @statusLabel.setText("No function selected");
      }
    }

    instance_method action_addFunction: fun() {
      if (@current_cmodule == null) {
        @statusLabel.setText("No current module");
        return null;
      }

      @miniBuffer.prompt("Function name: ", "", fun(name) {
        this.command(fun() {
          var cfun = CompiledFunction.newTopLevel(name, "fun() { return null; }", @current_cmodule);
          @current_cmodule.addFunction(cfun);
          this.showEditorForFunction(cfun);
          @statusLabel.setText("Function added: " + name);
        }, fun() {
          @current_cmodule.removeFunction(name);
          @webview.page().mainFrame().documentElement().findFirst("#"+name).takeFromDocument();
          @statusLabel.setText("Function removed: " + name);
        });
      });
    }

    instance_method action_deleteFunction: fun() {
      if (@current_cmodule == null) {
        @statusLabel.setText("No current module");
        return null;
      }

      var e = qt.QApplication.focusWidget();
      if (Mirror.vtFor(e) == ExplorerEditor) {
        var cfun = e.cfun();
        this.command(fun() {
          cfun.owner.removeFunction(cfun.name);
          @webview.page().mainFrame().documentElement().findFirst("#"+cfun.name).takeFromDocument();
          @statusLabel.setText("Function deleted: " + cfun.name);
        }, fun() {
          cfun.owner.addFunction(cfun);
          this.showEditorForFunction(cfun);
          @statusLabel.setText("Function added: " + cfun.name);
        });
      } else {
        @statusLabel.setText("No function selected");
      }
    }

    instance_method action_renameModule: fun() {
      if (@current_cmodule == null) {
        @statusLabel.setText("No current module");
        return null;
      }

      @miniBuffer.prompt("Module name: ", @current_cmodule.name(), fun(value) {
        var old_name = @current_cmodule.name();
        this.command(fun() {
          @current_cmodule.setName(value);
          @webview.page().mainFrame().documentElement().findFirst(".module_name").setPlainText(@current_cmodule.name());
          @statusLabel.setText("Renamed module to: " + value);
        }, fun() {
          @current_cmodule.setName(old_name);
          @webview.page().mainFrame().documentElement().findFirst(".module_name").setPlainText(@current_cmodule.name());
          @statusLabel.setText("Renamed module to: " + old_name);
        });
      });
    }

    instance_method show_home: fun() {
      @current_cmodule = null;
      @webview.setUrl(modules_path() + "/module-explorer/index.html");
    }

    instance_method show_tutorial: fun() {
      @current_cmodule = null;
      @webview.setUrl(modules_path() + "/module-explorer/tutorial.html");
    }

    instance_method show_modules: fun() {
      @current_cmodule = null;
      @webview.loadUrl(modules_path() + "/module-explorer/modules-index.html");
      var modules = available_modules();
      var ul = @webview.page().mainFrame().documentElement().findFirst("ul.modules");
      modules.each(fun(name) {
        ul.appendInside("<li><a href='/" + name + "'>" + name + "</a></li>")
      });
    }

    instance_method show_module: fun(name) {
      @current_cmodule = get_module(name);
      @webview.loadUrl(modules_path() + "/module-explorer/module-view.html");
      var doc = @webview.page().mainFrame().documentElement();
      doc.findFirst(".module_name").setPlainText(@current_cmodule.name());

      var ul = doc.findFirst(".module_parameters");
      @current_cmodule.params().each(fun(p) {
        ul.appendInside("<li>" + p + "</li>");
      });

      var fns = @current_cmodule.compiled_functions();
      fns.each(fun(name,cfn) {
        this.showEditorForFunction(cfn)
      });
    }

    instance_method showEditorForFunction: fun(cfn) {
      var doc = @webview.page().mainFrame().documentElement();
      var div = doc.findFirst(".fun_tpl").clone();
      div.setAttribute("id", cfn.name);
      div.setStyleProperty("display","block");
      div.findFirst(".function_name").setPlainText(cfn.name);
      div.findFirst(".fun_body param[name=module_name]").setAttribute("value",@current_cmodule.name());
      div.findFirst(".fun_body param[name=function_name]").setAttribute("value",cfn.name);
      div.findFirst(".fun_body param[name=code]").setAttribute("value",cfn.text());
      doc.findFirst(".functions").appendInside(div);
    }
  }

  main: fun() {
    var app = qt.QApplication.new();
    var main = ModuleExplorer.new();
    main.show();
    return app.exec();
  }

  debug: fun(process) {
    var eventloop = null;
    if (!qt.qapp_running()) {
      eventloop = qt.QApplication.new();
    } else {
      eventloop = qt.QEventLoop.new();
    }

    var dbg = DebuggerUI.new(process);
    dbg.show();
    eventloop.exec();
    io.print("debug:main left loop");
  }
}
