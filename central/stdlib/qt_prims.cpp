#include <QMetaType>
#include "process.hpp"
Q_DECLARE_METATYPE (Process*);

#include "vm.hpp"
#include "log.hpp"
#include "mmobj.hpp"
#include "utils.hpp"
#include "core_image.hpp"
#include <QScriptEngine>
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
//#include <QListWidgetitem>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QPlainTextEdit>
#include "qsc.hpp"
#include <QShortcut>
#include <QTableWidget>
#include <QUrl>
#include <QWebElement>
#include <QWebFrame>
#include <QWebSettings>
#include <QWebPage>
#include <QWebPluginFactory>
#include <QWebView>

#include <QWidget>
#include <QMetaType>
#include <QPushButton>
#include <QTcpServer>
#include <QTcpSocket>


#define DBG() _log << _log.yellow + _log.bold + "[prim|" << __FUNCTION__ << "] " << _log.normal
#define WARNING() MMLog::warning() << "[prim|" << __FUNCTION__ << "] " << _log.normal
#define ERROR() MMLog::error() << "[prim|" << __FUNCTION__ << "] " << _log.normal

static MMLog _log(LOG_QTPRIMS);


Q_DECLARE_METATYPE (QListWidgetItem*);

static
QScriptEngine *engine;


static
oop lookup_bottom_qt_instance(MMObj* mmobj, oop obj) {
  if (obj == MM_NULL) {
    return NULL;
  } else if (mmobj->mm_object_vt(mmobj->mm_object_delegate(obj)) == mmobj->core()->get_prime("Object")) {
    return obj;
  } else {
    return lookup_bottom_qt_instance(mmobj, mmobj->mm_object_delegate(obj));
  }
}

static
void* get_qt_field(oop instance) {
  return ((oop*)instance)[2]; //@self
}

static
void set_qt_field(oop bottom_instance, void* qtobj) {
  ((oop*)bottom_instance)[2] = (oop) qtobj;
}


static
void* get_qt_instance(MMObj* mmobj, oop obj) {
  oop bottom = lookup_bottom_qt_instance(mmobj, obj);
  if (bottom) {
    return get_qt_field(bottom);
  } else {
    return NULL;
  }
}

static
void set_qt_instance(MMObj* mmobj, oop obj, void* qtobj) {
  oop bottom = lookup_bottom_qt_instance(mmobj, obj);
  set_qt_field(bottom, qtobj);
}

static
void set_meme_instance(QObject* obj, oop instance) {
  obj->setProperty("meme_instance", QVariant::fromValue((void*)instance));
}

static
oop meme_instance(Process* proc, QObject* obj, int* exc) {
  *exc = 0;
  QVariant prop = obj->property("meme_instance");
  if (prop.isValid()) {
    return (oop) prop.value<void*>();
  } else {
    oop qt_imod = proc->mp();
    const char* name = obj->metaObject()->className();
    oop qt_class = proc->send_0(qt_imod, proc->vm()->new_symbol(name), exc);
    if (*exc != 0) {
      return qt_class;
    }
    oop instance = proc->mmobj()->alloc_instance(proc, qt_class);
    set_qt_instance(proc->mmobj(), instance, obj);
    set_meme_instance(obj, instance);
    return instance;
  }
}

static std::map<void*, oop> meme_mapping;

static
void set_meme_instance(QListWidgetItem* obj, oop instance) {
  meme_mapping[obj] = instance;
}

static
oop meme_instance(Process* proc, QListWidgetItem* obj, int* exc) {
  *exc = 0;
  if (meme_mapping.find(obj) == meme_mapping.end()) {
    oop qt_imod = proc->mp();
    oop qt_class = proc->send_0(qt_imod, proc->vm()->new_symbol("QListWidgetItem"), exc);
    if (*exc != 0) {
      return qt_class;
    }
    oop instance = proc->mmobj()->alloc_instance(proc, qt_class);
    set_qt_instance(proc->mmobj(), instance, obj);
    meme_mapping[obj] = instance;
    return instance;
  } else {
    return meme_mapping[obj];
  }
}

static
void set_meme_instance(QTableWidgetItem* obj, oop instance) {
  meme_mapping[obj] = instance;
}

// static
// oop meme_instance(Process* proc, QTableWidgetItem* obj) {
//   if (meme_mapping.find(obj) == meme_mapping.end()) {
//     oop qt_imod = proc->mp();
//     int exc;
//     oop qt_class = proc->send_0(qt_imod, proc->mmobj()->mm_string_new("QTableWidgetItem"), &exc);
//     if (!(exc == 0)) {
//       raise(something here)
//     }
//     oop instance = proc->mmobj()->alloc_instance(qt_class);
//     set_qt_instance(proc->mmobj(), instance, obj);
//     meme_mapping[obj] = instance;
//     return instance;
//   } else {
//     return meme_mapping[obj];
//   }
// }

static
oop meme_instance(Process* proc, QTextCursor* obj, int* exc) {
  *exc = 0;
  if (meme_mapping.find(obj) == meme_mapping.end()) {
    oop qt_imod = proc->mp();
    oop qt_class = proc->send_0(qt_imod, proc->vm()->new_symbol("QTextCursor"), exc);
    if (*exc != 0) {
      return qt_class;
    }
    oop instance = proc->mmobj()->alloc_instance(proc, qt_class);
    set_qt_instance(proc->mmobj(), instance, obj);
    meme_mapping[obj] = instance;
    return instance;
  } else {
    return meme_mapping[obj];
  }
}

static
oop meme_instance(Process* proc, QWebElement* obj, int* exc) {
  *exc = 0;
  if (meme_mapping.find(obj) == meme_mapping.end()) {
    oop qt_imod = proc->mp();
    oop qt_class = proc->send_0(qt_imod, proc->vm()->new_symbol("QWebElement"), exc);
    if (*exc != 0) {
      return qt_class;
    }
    oop instance = proc->mmobj()->alloc_instance(proc, qt_class);
    set_qt_instance(proc->mmobj(), instance, obj);
    meme_mapping[obj] = instance;
    return instance;
  } else {
    return meme_mapping[obj];
  }
}

static
oop meme_instance(Process* proc, QUrl* obj, int* exc) {
  *exc = 0;
  if (meme_mapping.find(obj) == meme_mapping.end()) {
    oop qt_imod = proc->mp();
    oop qt_class = proc->send_0(qt_imod, proc->vm()->new_symbol("QUrl"), exc);
    if (*exc != 0) {
      return qt_class;
    }
    oop instance = proc->mmobj()->alloc_instance(proc, qt_class);
    set_qt_instance(proc->mmobj(), instance, obj);
    meme_mapping[obj] = instance;
    return instance;
  } else {
    return meme_mapping[obj];
  }
}



/////////// bridge //////////////


static const char* js_bridge =
  "function qt_bind_connect(qtobj, signal, mm_closure, process) {"
  "  var global = this;"
  "  qtobj[signal].connect(qtobj, function() {"
  // "    print('SIGNAL!...' + arguments.length);"
  "    for (var i = 0; i < arguments.length; i++) { print(arguments[i]); }"
  "    return mm_handle(process, mm_closure, arguments);"
  "  });"
  "}";

QScriptValue js_print(QScriptContext *ctx, QScriptEngine *engine) {
  QString str = ctx->argument(0).toString();
  // std::cerr << "print: " << qPrintable(str) << endl;
  return engine->undefinedValue();
}

static
QScriptValue mm_handle(QScriptContext *ctx, QScriptEngine *engine) {
  Process* proc = ctx->argument(0).toVariant().value<Process*>();
  oop mm_closure = (oop) ctx->argument(1).toVariant().value<void*>();
  QScriptValue args = ctx->argument(2);

  oop mm_args = proc->mmobj()->mm_list_new();
  for (int i = args.property("length").toInt32() -1; 0 <= i; i--) {
    QScriptValue arg = args.property(i);
    if (arg.isBool()) {
      proc->mmobj()->mm_list_append(proc, mm_args, proc->mmobj()->mm_boolean_new(arg.toBool()));
    } else if (arg.isNull()) {
      proc->mmobj()->mm_list_append(proc, mm_args, MM_NULL);
    } else if (arg.isNumber()) {
      proc->mmobj()->mm_list_append(proc, mm_args, tag_small_int(arg.toVariant().toInt()));
    } else if (arg.isQObject()) {
      proc->bail("TODO: arg.toQObject");
    } else {
      QVariant v = arg.toVariant();
      if (!v.isValid()) {
        _log << "mm_handle: unknown arg" << arg.scriptClass() << endl;
        proc->bail("TODO: unknown arg");
      } else {
        if (QString(v.typeName()) == "QListWidgetItem*") {
          QListWidgetItem* p = v.value<QListWidgetItem*>();
          // std::cerr << "from variant: " << p << endl;
          int exc;
          proc->mmobj()->mm_list_append(proc, mm_args, meme_instance(proc, p, &exc));
          if (!(exc == 0)) {
            proc->raise("InternalError", "Unable to get memeinstance from QListWidgetItem");
          }
        } else if (QString(v.typeName()) == "QUrl") {
          QUrl *p = new QUrl(v.value<QUrl>());
          int exc;
          proc->mmobj()->mm_list_append(proc, mm_args, meme_instance(proc, p, &exc));
          if (!(exc == 0)) {
            proc->raise("InternalError", "Unable to get memeinstance from QListWidgetItem");
          }
        }
      }
    }
  }
  //if proc is halted, we will screw it bt calling
  //(this is while we don't have proper multi-process support + debugging)
  if (!proc->is_running()) {
    MMLog::error() << "** woah! don't trigger a slot while its process is not in running state!!! **" << endl;
  } else {
    int exc;
    oop res = proc->call(mm_closure, mm_args, &exc);
    check_and_print_exception(proc, exc, res);
  }
  return engine->undefinedValue();
}

static
void init_qt_stuff() { //stuff that needs QApplication to exist
  engine = new QScriptEngine;
  engine->evaluate(js_bridge, "<qt_prims>");
  QScriptValue globalObject = engine->globalObject();
  globalObject.setProperty("mm_handle", engine->newFunction(mm_handle), QScriptValue::SkipInEnumeration);
  globalObject.setProperty("print", engine->newFunction(js_print), QScriptValue::SkipInEnumeration);
}


////////// bindings /////////


static int prim_qt_server_create_server(Process* proc) {
  oop data_self =  proc->dp();
  QTcpServer* server = new QTcpServer(NULL);
  set_meme_instance(server, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, server);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_server_listen(Process* proc) {
  oop data_self =  proc->dp();
  oop port = proc->get_arg(0);

  QTcpServer* server = (QTcpServer*) get_qt_instance(proc->mmobj(), data_self);
  number _port = untag_small_int(port);

  server->listen(QHostAddress("127.0.0.1"), _port);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_server_next_pending_connection(Process* proc) {
  oop data_self =  proc->dp();
  QTcpServer* server = (QTcpServer*) get_qt_instance(proc->mmobj(), data_self);

  QTcpSocket* socket = server->nextPendingConnection();

  int exc;
  proc->stack_push(meme_instance(proc, socket, &exc));
  return exc;
}

#include <sstream>

static int prim_qt_socket_read_string(Process* proc) {
  oop data_self =  proc->dp();
  QTcpSocket* socket = (QTcpSocket*) get_qt_instance(proc->mmobj(), data_self);
  char c[1];
  std::stringstream s;
  while (socket->getChar(c) && *c) {
    s << *c;
  }
  s << '\0';
  DBG() << "read_string: [" << s.str() << "]" << endl;

  oop text = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(text);
  return 0;
}

static int prim_qt_socket_write_line(Process* proc) {
  oop data_self =  proc->dp();
  oop line = proc->get_arg(0);
  char* str = proc->mmobj()->mm_string_cstr(proc, line);
  std::stringstream s;
  s << str << "\n";
  QTcpSocket* socket = (QTcpSocket*) get_qt_instance(proc->mmobj(), data_self);
  socket->write(s.str().c_str(), s.str().size());
  socket->flush();
  DBG() << "write line: [" << str << "]" << endl;
  proc->stack_push(proc->rp());
  return 0;
}

/** QApplication **/

static QApplication* _app = NULL;

static int prim_qapplication_new(Process* proc) {
  oop data_self =  proc->dp();
  if (!_app) {
    _app = new QApplication(proc->vm()->argc(), proc->vm()->argv());
  }
  set_meme_instance(_app, proc->rp());
  // _log << "QT: QApplication " << app << endl;
  init_qt_stuff();
  set_qt_instance(proc->mmobj(), data_self, _app);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qapplication_exec(Process* proc) {
  oop data_self =  proc->dp();
  QApplication* app = (QApplication*) get_qt_instance(proc->mmobj(), data_self);
  app->exec();
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qapplication_exit(Process* proc) {
  oop data_self =  proc->dp();
  oop code = proc->get_arg(0);
  QApplication* app = (QApplication*) get_qt_instance(proc->mmobj(), data_self);
  app->exit(untag_small_int(code));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qapplication_focus_widget(Process* proc) {
  oop data_self =  proc->dp();
  QApplication* app = (QApplication*) get_qt_instance(proc->mmobj(), data_self);
  int exc;
  oop instance = meme_instance(proc, app->focusWidget(), &exc);
  proc->stack_push(instance);
  return exc;
}

/** QAction **/

static int prim_qaction_new(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_label = proc->get_arg(0);
  oop oop_parent = proc->get_arg(1);

  QWidget* parent = (QWidget*) get_qt_instance(proc->mmobj(), oop_parent);

  QAction* qtobj = new QAction(proc->mmobj()->mm_string_cstr(proc, oop_label), parent);
  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qaction_connect(Process* proc) {
  oop data_self =  proc->dp();
  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);

  oop signal = proc->get_arg(0);
  oop fn = proc->get_arg(1);

  QScriptValueList args;
  args << engine->newQObject(w)
       << proc->mmobj()->mm_string_cstr(proc, signal)
       << engine->newVariant(QVariant::fromValue((void*)fn))
       << engine->newVariant(QVariant::fromValue(proc));

  QScriptValue globalObject = engine->globalObject();
  QScriptValue bridge = globalObject.property("qt_bind_connect");
  bridge.call(globalObject, args);
  if (engine->hasUncaughtException()) {
    _log << "QT: exception: " << qPrintable(engine->uncaughtException().toString()) << endl;
    proc->bail();
  }
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qaction_set_enabled(Process* proc) {
  oop data_self =  proc->dp();
  oop val = proc->get_arg(0);
  QAction* qtobj = (QAction*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setEnabled(proc->mmobj()->mm_boolean_cbool(proc, val));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qaction_set_shortcut(Process* proc) {
  oop data_self =  proc->dp();
  oop shortcut = proc->get_arg(0);
  QAction* qtobj = (QAction*) get_qt_instance(proc->mmobj(), data_self);

  qtobj->setShortcut(QString(proc->mmobj()->mm_string_cstr(proc, shortcut)));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qaction_set_shortcut_context(Process* proc) {
  oop data_self =  proc->dp();
  oop ctx = proc->get_arg(0);
  QAction* qtobj = (QAction*) get_qt_instance(proc->mmobj(), data_self);

  qtobj->setShortcutContext((Qt::ShortcutContext) untag_small_int(ctx));
  proc->stack_push(proc->rp());
  return 0;
}



/** QComboBox **/

static int prim_qt_qcombobox_new(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_parent = proc->get_arg(0);

  QWidget* parent = (QWidget*) get_qt_instance(proc->mmobj(), oop_parent);
  QComboBox* qtobj = new QComboBox(parent);

  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qcombobox_add_item(Process* proc) {
  oop data_self =  proc->dp();
  oop item = proc->get_arg(0);

  // std::cerr << "add ITEM " << proc->mmobj()->mm_string_cstr(item) << endl;

  QComboBox* qtobj = (QComboBox*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->addItem(proc->mmobj()->mm_string_cstr(proc, item));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qcombobox_clear(Process* proc) {
  oop data_self =  proc->dp();

  QComboBox* qtobj = (QComboBox*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->clear();
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qcombobox_count(Process* proc) {
  oop data_self =  proc->dp();

  QComboBox* qtobj = (QComboBox*) get_qt_instance(proc->mmobj(), data_self);
  proc->stack_push((oop) tag_small_int(qtobj->count()));
  return 0;
}

static int prim_qt_qcombobox_set_current_index(Process* proc) {
  oop data_self =  proc->dp();
  oop idx = proc->get_arg(0);

  QComboBox* qtobj = (QComboBox*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setCurrentIndex(untag_small_int(idx));
  proc->stack_push(proc->rp());
  return 0;
}


/** QEventLoop **/

static int prim_qt_qeventloop_new(Process* proc) {
  oop data_self =  proc->dp();

  QEventLoop* qtobj = new QEventLoop();

  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qeventloop_exec(Process* proc) {
  oop data_self =  proc->dp();

  QEventLoop* qtobj = (QEventLoop*) get_qt_instance(proc->mmobj(), data_self);
  int val = qtobj->exec();
  proc->stack_push(tag_small_int(val));
  return 0;
}

static int prim_qt_qeventloop_exit(Process* proc) {
  oop data_self =  proc->dp();
  oop code = proc->get_arg(0);

  QEventLoop* qtobj = (QEventLoop*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->exit(untag_small_int(code));
  proc->stack_push(proc->rp());
  return 0;
}

/** QHBoxLayout **/

static int prim_qt_qhboxlayout_new(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_parent = proc->get_arg(0);

  QWidget* parent = (QWidget*) get_qt_instance(proc->mmobj(), oop_parent);

  QHBoxLayout* qtobj = new QHBoxLayout(parent);

  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qhboxlayout_add_layout(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_layout = proc->get_arg(0);

  QLayout* layout = (QLayout*) get_qt_instance(proc->mmobj(), oop_layout);
  QHBoxLayout* qtobj = (QHBoxLayout*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->addLayout(layout);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qhboxlayout_add_widget(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_widget = proc->get_arg(0);

  QWidget* widget = (QWidget*) get_qt_instance(proc->mmobj(), oop_widget);
  QHBoxLayout* qtobj = (QHBoxLayout*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->addWidget(widget);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qhboxlayout_set_contents_margins(Process* proc) {
  oop data_self =  proc->dp();

  oop l = proc->get_arg(0);
  oop t = proc->get_arg(1);
  oop r = proc->get_arg(2);
  oop b = proc->get_arg(3);

  QHBoxLayout* qtobj = (QHBoxLayout*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setContentsMargins(untag_small_int(l),
                            untag_small_int(t),
                            untag_small_int(r),
                            untag_small_int(b));
  proc->stack_push(proc->rp());
  return 0;
}

/** QHeaderView **/

static int prim_qt_qheaderview_hide(Process* proc) {
  oop data_self =  proc->dp();

  QHeaderView* qtobj = (QHeaderView*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->hide();
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_set_stretch_last_section(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_bool = proc->get_arg(0);

  QHeaderView* qtobj = (QHeaderView*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setStretchLastSection(proc->mmobj()->mm_boolean_cbool(proc, oop_bool));
  proc->stack_push(proc->rp());
  return 0;
}


/** QLabel **/

static int prim_qt_qlabel_new(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_parent = proc->get_arg(0);

  QWidget* parent = (QWidget*) get_qt_instance(proc->mmobj(), oop_parent);

  QLabel* qtobj = new QLabel(parent);

  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qlabel_set_text(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_text = proc->get_arg(0);

  QLabel* qtobj = (QLabel*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setText(proc->mmobj()->mm_string_cstr(proc, oop_text));
  proc->stack_push(proc->rp());
  return 0;
}


/** QLayout **/

static int prim_qt_qlayout_add_widget(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_widget = proc->get_arg(0);

  QWidget* widget = (QWidget*) get_qt_instance(proc->mmobj(), oop_widget);
  QLayout* qtobj = (QLayout*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->addWidget(widget);
  proc->stack_push(proc->rp());
  return 0;
}


/** QLineEdit **/

static int prim_qt_qlineedit_new(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_parent = proc->get_arg(0);

  QWidget* parent = (QWidget*) get_qt_instance(proc->mmobj(), oop_parent);

  QLineEdit* qtobj = new QLineEdit(parent);

  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qlineedit_select_all(Process* proc) {
  oop data_self =  proc->dp();

  QLineEdit* qtobj = (QLineEdit*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->selectAll();
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qlineedit_selected_text(Process* proc) {
  oop data_self =  proc->dp();

  QLineEdit* qtobj = (QLineEdit*) get_qt_instance(proc->mmobj(), data_self);
  proc->stack_push(proc->mmobj()->mm_string_new(qtobj->selectedText().toLocal8Bit().data()));
  return 0;
}

static int prim_qt_qlineedit_set_text(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_text = proc->get_arg(0);

  QLineEdit* qtobj = (QLineEdit*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setText(proc->mmobj()->mm_string_cstr(proc, oop_text));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qlineedit_text(Process* proc) {
  oop data_self =  proc->dp();

  QLineEdit* qtobj = (QLineEdit*) get_qt_instance(proc->mmobj(), data_self);
  proc->stack_push(proc->mmobj()->mm_string_new(qtobj->text().toLocal8Bit().data()));
  return 0;
}

static int prim_qt_qlineedit_set_selection(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_start = proc->get_arg(0);
  oop oop_length = proc->get_arg(1);

  QLineEdit* qtobj = (QLineEdit*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setSelection(untag_small_int(oop_start), untag_small_int(oop_length));
  proc->stack_push(proc->rp());
  return 0;
}


/** QListWidget **/

static int prim_qt_qlistwidget_new(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_parent = proc->get_arg(0);

  QWidget* parent = (QWidget*) get_qt_instance(proc->mmobj(), oop_parent);

  QListWidget* qtobj = new QListWidget(parent);

  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qlistwidget_current_item(Process* proc) {
  oop data_self =  proc->dp();

  QListWidget* qtobj = (QListWidget*) get_qt_instance(proc->mmobj(), data_self);
  int exc;
  proc->stack_push(meme_instance(proc, qtobj->currentItem(), &exc));
  return exc;
}

/** QListWidgetItem **/

static int prim_qt_qlistwidgetitem_new(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_text = proc->get_arg(0);
  oop oop_parent = proc->get_arg(1);

  QListWidget* parent = (QListWidget*) get_qt_instance(proc->mmobj(), oop_parent);

  QListWidgetItem* qtobj = new QListWidgetItem(proc->mmobj()->mm_string_cstr(proc, oop_text), parent);

  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qlistwidgetitem_text(Process* proc) {
  oop data_self =  proc->dp();

  QListWidgetItem* qtobj = (QListWidgetItem*) get_qt_instance(proc->mmobj(), data_self);
  proc->stack_push(proc->mmobj()->mm_string_new(qtobj->text().toLocal8Bit().data()));
  return 0;
}


/** QMainWindow **/

class _QMainWindow : public QMainWindow {
public:
  _QMainWindow(QWidget* parent, oop self, Process* proc) : QMainWindow(parent), _self(self), _proc(proc) {};
  void closeEvent(QCloseEvent* ev) {
    std::pair<oop, oop> r = _proc->lookup(_self, _proc->mmobj()->mm_object_vt(_self), _proc->vm()->new_symbol("closeEvent"));
    if (!r.first && !r.second) {
      QMainWindow::closeEvent(ev);
    } else {
      int exc;
      oop res = _proc->send_0(_self, _proc->vm()->new_symbol("closeEvent"), &exc);
      check_and_print_exception(_proc, exc, res);
    }
  };
private:
  oop _self;
  Process* _proc;
};

static int prim_qt_qmainwindow_new(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_parent = proc->get_arg(0);

  QWidget* parent = (QWidget*) get_qt_instance(proc->mmobj(), oop_parent);

  QMainWindow* qtobj = new _QMainWindow(parent, proc->rp(), proc);

  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}


static int prim_qt_qmainwindow_menu_bar(Process* proc) {
  oop data_self =  proc->dp();

  _QMainWindow* qtobj = (_QMainWindow*) get_qt_instance(proc->mmobj(), data_self);
  int exc;
  proc->stack_push(meme_instance(proc, qtobj->menuBar(), &exc));
  return exc;
}

static int prim_qt_qmainwindow_set_central_widget(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_widget = proc->get_arg(0);

  QWidget* widget = (QWidget*) get_qt_instance(proc->mmobj(), oop_widget);
  _QMainWindow* qtobj = (_QMainWindow*) get_qt_instance(proc->mmobj(), data_self);

  qtobj->setCentralWidget(widget);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qmainwindow_status_bar(Process* proc) {
  oop data_self =  proc->dp();

  _QMainWindow* qtobj = (_QMainWindow*) get_qt_instance(proc->mmobj(), data_self);
  int exc;
  QStatusBar* bar = qtobj->statusBar();
  bar->showMessage("default");
  _log << "statusBar: " << bar << endl;
  proc->stack_push(meme_instance(proc, qtobj->statusBar(), &exc));
  return exc;
}


/** QMenuBar **/

static int prim_qt_qmenubar_add_menu(Process* proc) {
  oop data_self =  proc->dp();
  oop text = proc->get_arg(0);

  QMenuBar* qtobj = (QMenuBar*) get_qt_instance(proc->mmobj(), data_self);
  int exc;
  proc->stack_push(meme_instance(proc, qtobj->addMenu(proc->mmobj()->mm_string_cstr(proc, text)), &exc));
  return exc;
}


/** QPlainTextEdit **/

static int prim_qt_qplaintextedit_new(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_parent = proc->get_arg(0);

  QWidget* parent = (QWidget*) get_qt_instance(proc->mmobj(), oop_parent);

  QPlainTextEdit* qtobj = new QPlainTextEdit(parent);

  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qplaintextedit_set_plain_text(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_text = proc->get_arg(0);

  QPlainTextEdit* qtobj = (QPlainTextEdit*) get_qt_instance(proc->mmobj(), data_self);

  qtobj->setPlainText(proc->mmobj()->mm_string_cstr(proc, oop_text));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qplaintextedit_set_tabstop_width(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_width = proc->get_arg(0);

  QPlainTextEdit* qtobj = (QPlainTextEdit*) get_qt_instance(proc->mmobj(), data_self);

  qtobj->setTabStopWidth(untag_small_int(oop_width));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qplaintextedit_set_text_cursor(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_cursor = proc->get_arg(0);

  QTextCursor* cursor = (QTextCursor*) get_qt_instance(proc->mmobj(), oop_cursor);

  QPlainTextEdit* qtobj = (QPlainTextEdit*) get_qt_instance(proc->mmobj(), data_self);

  qtobj->setTextCursor(*cursor);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qplaintextedit_text_cursor(Process* proc) {
  oop data_self =  proc->dp();

  QPlainTextEdit* qtobj = (QPlainTextEdit*) get_qt_instance(proc->mmobj(), data_self);
  QTextCursor* c = new QTextCursor(qtobj->textCursor());
  int exc;
  proc->stack_push(meme_instance(proc, c, &exc));
  return exc;
}

static int prim_qt_qplaintextedit_to_plain_text(Process* proc) {
  oop data_self =  proc->dp();

  QPlainTextEdit* qtobj = (QPlainTextEdit*) get_qt_instance(proc->mmobj(), data_self);
  proc->stack_push(proc->mmobj()->mm_string_new(qtobj->toPlainText().toLocal8Bit().data()));
  return 0;
}


/** QScintilla **/

static int prim_qt_scintilla_editor_new(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_parent = proc->get_arg(0);

  QWidget* parent = (QWidget*) get_qt_instance(proc->mmobj(), oop_parent);

  _QScintilla* qtobj = new _QScintilla(parent);

  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_scintilla_append(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_text = proc->get_arg(0);

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->append(proc->mmobj()->mm_string_cstr(proc, oop_text));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_scintilla_copy(Process* proc) {
  oop data_self =  proc->dp();

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->copy();
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_scintilla_cut(Process* proc) {
  oop data_self =  proc->dp();

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->cut();
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_scintilla_get_cursor_position(Process* proc) {
  oop data_self =  proc->dp();

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  int line, index;
  qtobj->getCursorPosition(&line, &index);

  oop dict = proc->mmobj()->mm_dictionary_new();
  oop oop_line = proc->mmobj()->mm_string_new("line");
  oop oop_index = proc->mmobj()->mm_string_new("index");
  proc->mmobj()->mm_dictionary_set(proc, dict, oop_line, tag_small_int(line));
  proc->mmobj()->mm_dictionary_set(proc, dict, oop_index, tag_small_int(index));
  proc->stack_push(dict);
  return 0;
}

static int prim_qt_scintilla_get_selection(Process* proc) {
  oop data_self =  proc->dp();

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  int lineFrom, indexFrom, lineTo, indexTo;
  qtobj->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

  oop dict = proc->mmobj()->mm_dictionary_new();
  oop oop_start_line = proc->vm()->new_symbol("start_line");
  oop oop_end_line = proc->vm()->new_symbol("end_line");
  oop oop_start_index = proc->vm()->new_symbol("start_index");
  oop oop_end_index = proc->vm()->new_symbol("end_index");

  proc->mmobj()->mm_dictionary_set(proc, dict, oop_start_line, tag_small_int(lineFrom));
  proc->mmobj()->mm_dictionary_set(proc, dict, oop_end_line, tag_small_int(lineTo));
  proc->mmobj()->mm_dictionary_set(proc, dict, oop_start_index, tag_small_int(indexFrom));
  proc->mmobj()->mm_dictionary_set(proc, dict, oop_end_index, tag_small_int(indexTo));
  proc->stack_push(dict);
  return 0;
}

static int prim_qt_scintilla_insert_at(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_text = proc->get_arg(0);
  oop oop_line = proc->get_arg(1);
  oop oop_index = proc->get_arg(2);

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->insertAt(proc->mmobj()->mm_string_cstr(proc, oop_text),
                  untag_small_int(oop_line), untag_small_int(oop_index));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_scintilla_lines(Process* proc) {
  oop data_self =  proc->dp();

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  proc->stack_push(tag_small_int(qtobj->lines()));
  return 0;
}

static int prim_qt_scintilla_paste(Process* proc) {
  oop data_self =  proc->dp();

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->paste();
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_scintilla_paused_at_line(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_start_line = proc->get_arg(0);
  oop oop_start_col = proc->get_arg(1);
  oop oop_end_line = proc->get_arg(2);
  oop oop_end_col = proc->get_arg(3);

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->paused_at_line(untag_small_int(oop_start_line),
                        untag_small_int(oop_start_col),
                        untag_small_int(oop_end_line),
                        untag_small_int(oop_end_col));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_scintilla_redo(Process* proc) {
  oop data_self =  proc->dp();

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->redo();
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_scintilla_saved(Process* proc) {
  oop data_self =  proc->dp();

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->saved();
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_scintilla_selected_text(Process* proc) {
  oop data_self =  proc->dp();

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  oop text = proc->mmobj()->mm_string_new(qtobj->selectedText().toLocal8Bit().data());

  proc->stack_push(text);
  return 0;
}

static int prim_qt_scintilla_set_selection(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_start_line = proc->get_arg(0);
  oop oop_start_col = proc->get_arg(1);
  oop oop_end_line = proc->get_arg(2);
  oop oop_end_col = proc->get_arg(3);

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setSelection(untag_small_int(oop_start_line),
                      untag_small_int(oop_start_col),
                      untag_small_int(oop_end_line),
                      untag_small_int(oop_end_col));

  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_scintilla_set_text(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_text = proc->get_arg(0);

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setText(proc->mmobj()->mm_string_cstr(proc, oop_text));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_scintilla_text(Process* proc) {
  oop data_self =  proc->dp();

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  oop text = proc->mmobj()->mm_string_new(qtobj->text().toLocal8Bit().data());

  proc->stack_push(text);
  return 0;
}

static int prim_qt_scintilla_undo(Process* proc) {
  oop data_self =  proc->dp();

  _QScintilla* qtobj = (_QScintilla*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->undo();
  proc->stack_push(proc->rp());
  return 0;
}

/** QShortcut **/

static int prim_qt_qshortcut_new(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_parent = proc->get_arg(0);

  QWidget* parent = (QWidget*) get_qt_instance(proc->mmobj(), oop_parent);

  QShortcut* qtobj = new QShortcut(parent);
  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qshortcut_set_context(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_ctx = proc->get_arg(0);

  QShortcut* qtobj = (QShortcut*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setContext((Qt::ShortcutContext)untag_small_int(oop_ctx));
  proc->stack_push(proc->rp());
  return 0;
}

/** QTableWidget **/

static int prim_qt_qtablewidget_new(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_parent = proc->get_arg(0);

  QWidget* parent = (QWidget*) get_qt_instance(proc->mmobj(), oop_parent);

  QTableWidget* qtobj = new QTableWidget(parent);
  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qtablewidget_clear(Process* proc) {
  oop data_self =  proc->dp();

  QTableWidget* qtobj = (QTableWidget*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->clear();
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qtablewidget_horizontal_header(Process* proc) {
  oop data_self =  proc->dp();

  QTableWidget* qtobj = (QTableWidget*) get_qt_instance(proc->mmobj(), data_self);
  QHeaderView* header = qtobj->horizontalHeader();

  int exc;
  oop instance = meme_instance(proc, header, &exc);
  if (exc != 0) {
    proc->stack_push(instance);
    return exc;
  }
  // oop qt_imod = proc->mp();
  // oop qt_class = proc->send_0(qt_imod, proc->mmobj()->mm_string_new("QHeaderView"));
  // oop instance = proc->mmobj()->alloc_instance(qt_class);

  set_qt_instance(proc->mmobj(), instance, header);
  set_meme_instance(header, instance);

  proc->stack_push(instance);
  return 0;
}

static int prim_qt_qtablewidget_set_column_count(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_count = proc->get_arg(0);

  QTableWidget* qtobj = (QTableWidget*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setColumnCount(untag_small_int(oop_count));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qtablewidget_set_horizontal_header_labels(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_labels = proc->get_arg(0);

  QTableWidget* qtobj = (QTableWidget*) get_qt_instance(proc->mmobj(), data_self);
  QStringList lst;
  for (int i = 0; i < proc->mmobj()->mm_list_size(proc, oop_labels); i++) {
    lst << proc->mmobj()->mm_string_cstr(proc, proc->mmobj()->mm_list_entry(proc, oop_labels, i));
  }
  qtobj->setHorizontalHeaderLabels(lst);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qtablewidget_set_item(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_line = proc->get_arg(0);
  oop oop_col = proc->get_arg(1);
  oop oop_item = proc->get_arg(2);

  QTableWidgetItem* item = (QTableWidgetItem*) get_qt_instance(proc->mmobj(), oop_item);

  QTableWidget* qtobj = (QTableWidget*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setItem(untag_small_int(oop_line), untag_small_int(oop_col), item);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qtablewidget_set_row_count(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_count = proc->get_arg(0);

  QTableWidget* qtobj = (QTableWidget*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setRowCount(untag_small_int(oop_count));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qtablewidget_set_selection_mode(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_mode = proc->get_arg(0);

  QTableWidget* qtobj = (QTableWidget*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setSelectionMode((QAbstractItemView::SelectionMode) untag_small_int(oop_mode));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qtablewidget_set_sorting_enabled(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_val = proc->get_arg(0);

  QTableWidget* qtobj = (QTableWidget*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setSortingEnabled(proc->mmobj()->mm_boolean_cbool(proc, oop_val));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qtablewidget_vertical_header(Process* proc) {
  oop data_self =  proc->dp();

  QTableWidget* qtobj = (QTableWidget*) get_qt_instance(proc->mmobj(), data_self);
  QHeaderView* header = qtobj->verticalHeader();

  int exc;
  oop instance = meme_instance(proc, header, &exc);
  if (exc != 0) {
    proc->stack_push(instance);
    return exc;
  }
  // oop qt_imod = proc->mp();
  // oop qt_class = proc->send_0(qt_imod, proc->mmobj()->mm_string_new("QHeaderView"));
  // oop instance = proc->mmobj()->alloc_instance(qt_class);

  set_qt_instance(proc->mmobj(), instance, header);
  set_meme_instance(header, instance);

  proc->stack_push(instance);
  return 0;
}

/** QTableWidgetItem **/

static int prim_qt_qtablewidgetitem_new(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_label = proc->get_arg(0);

  QTableWidgetItem* qtobj = new QTableWidgetItem(proc->mmobj()->mm_string_cstr(proc, oop_label));
  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qtablewidgetitem_set_flags(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_flags = proc->get_arg(0);

  QTableWidgetItem* qtobj = (QTableWidgetItem*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setFlags((Qt::ItemFlags)untag_small_int(oop_flags));
  proc->stack_push(proc->rp());
  return 0;
}


/** QTextCursor **/

static int prim_qt_qtextcursor_drag_right(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_len = proc->get_arg(0);

  QTextCursor* qtobj = (QTextCursor*) get_qt_instance(proc->mmobj(), data_self);
  bool res = qtobj->movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, untag_small_int(oop_len));
  proc->stack_push(proc->mmobj()->mm_boolean_new(res));
  return 0;
}

static int prim_qt_qtextcursor_insert_text(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_text = proc->get_arg(0);

  QTextCursor* qtobj = (QTextCursor*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->insertText(proc->mmobj()->mm_string_cstr(proc, oop_text));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qtextcursor_selected_text(Process* proc) {
  oop data_self =  proc->dp();

  QTextCursor* qtobj = (QTextCursor*) get_qt_instance(proc->mmobj(), data_self);
  oop oop_text = proc->mmobj()->mm_string_new(qtobj->selectedText().toLocal8Bit().data());
  proc->stack_push(oop_text);
  return 0;
}

static int prim_qt_qtextcursor_selection_end(Process* proc) {
  oop data_self =  proc->dp();

  QTextCursor* qtobj = (QTextCursor*) get_qt_instance(proc->mmobj(), data_self);
  proc->stack_push(tag_small_int(qtobj->selectionEnd()));
  return 0;
}

static int prim_qt_qtextcursor_set_position(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_pos = proc->get_arg(0);

  QTextCursor* qtobj = (QTextCursor*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setPosition(untag_small_int(oop_pos));
  proc->stack_push(proc->rp());
  return 0;
}


/** QUrl **/

static int prim_qt_qurl_fragment(Process* proc) {
  oop data_self =  proc->dp();

  QUrl* qtobj = (QUrl*) get_qt_instance(proc->mmobj(), data_self);
  proc->stack_push(proc->mmobj()->mm_string_new(qtobj->fragment().toLocal8Bit().data()));
  return 0;
}

static int prim_qt_qurl_has_fragment(Process* proc) {
  oop data_self =  proc->dp();

  QUrl* qtobj = (QUrl*) get_qt_instance(proc->mmobj(), data_self);
  proc->stack_push(proc->mmobj()->mm_boolean_new(qtobj->hasFragment()));
  return 0;
}

static int prim_qt_qurl_has_query_item(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_name = proc->get_arg(0);

  QUrl* qtobj = (QUrl*) get_qt_instance(proc->mmobj(), data_self);
  proc->stack_push(proc->mmobj()->mm_boolean_new(qtobj->hasQueryItem(proc->mmobj()->mm_string_cstr(proc, oop_name))));
  return 0;
}

static int prim_qt_qurl_path(Process* proc) {
  oop data_self =  proc->dp();

  QUrl* qtobj = (QUrl*) get_qt_instance(proc->mmobj(), data_self);
  proc->stack_push(proc->mmobj()->mm_string_new(qtobj->path().toLocal8Bit().data()));
  return 0;
}

static int prim_qt_qurl_query_item_value(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_name = proc->get_arg(0);

  QUrl* qtobj = (QUrl*) get_qt_instance(proc->mmobj(), data_self);
  QString val = qtobj->queryItemValue(proc->mmobj()->mm_string_cstr(proc, oop_name));
  proc->stack_push(proc->mmobj()->mm_string_new(val.toLocal8Bit().data()));
  return 0;
}

static int prim_qt_qurl_to_string(Process* proc) {
  oop data_self =  proc->dp();

  QUrl* qtobj = (QUrl*) get_qt_instance(proc->mmobj(), data_self);
  QString val = qtobj->toString();
  proc->stack_push(proc->mmobj()->mm_string_new(val.toLocal8Bit().data()));
  return 0;
}


/** QVBoxLayout **/

static int prim_qt_qvboxlayout_new(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_parent = proc->get_arg(0);

  QWidget* parent = (QWidget*) get_qt_instance(proc->mmobj(), oop_parent);

  QVBoxLayout* qtobj = new QVBoxLayout(parent);

  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qvboxlayout_add_layout(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_layout = proc->get_arg(0);

  QLayout* layout = (QLayout*) get_qt_instance(proc->mmobj(), oop_layout);
  QVBoxLayout* qtobj = (QVBoxLayout*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->addLayout(layout);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qvboxlayout_add_widget(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_widget = proc->get_arg(0);

  QWidget* widget = (QWidget*) get_qt_instance(proc->mmobj(), oop_widget);
  QVBoxLayout* qtobj = (QVBoxLayout*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->addWidget(widget);
  proc->stack_push(proc->rp());
  return 0;
}

/** QWebElement **/

static int prim_qt_qwebelement_append_inside(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_str_or_webel = proc->get_arg(0);

  QWebElement* qtobj = (QWebElement*) get_qt_instance(proc->mmobj(), data_self);

  if (proc->mmobj()->mm_is_string(oop_str_or_webel)) {
    qtobj->appendInside(proc->mmobj()->mm_string_cstr(proc, oop_str_or_webel));
  } else {
    QWebElement* el = (QWebElement*) get_qt_instance(proc->mmobj(), oop_str_or_webel);
    qtobj->appendInside(*el);
  }
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwebelement_append_outside(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_str_or_webel = proc->get_arg(0);

  QWebElement* qtobj = (QWebElement*) get_qt_instance(proc->mmobj(), data_self);

  if (proc->mmobj()->mm_is_string(oop_str_or_webel)) {
    qtobj->appendOutside(proc->mmobj()->mm_string_cstr(proc, oop_str_or_webel));
  } else {
    QWebElement* el = (QWebElement*) get_qt_instance(proc->mmobj(), oop_str_or_webel);
    qtobj->appendOutside(*el);
  }
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwebelement_clone(Process* proc) {
  oop data_self =  proc->dp();

  QWebElement* qtobj = (QWebElement*) get_qt_instance(proc->mmobj(), data_self);
  QWebElement* el = new QWebElement(qtobj->clone());

  int exc;
  oop instance = meme_instance(proc, el, &exc);
  proc->stack_push(instance);
  return exc;
}

static int prim_qt_qwebelement_find_first(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_str = proc->get_arg(0);

  QWebElement* qtobj = (QWebElement*) get_qt_instance(proc->mmobj(), data_self);
  QWebElement* el = new QWebElement(qtobj->findFirst(proc->mmobj()->mm_string_cstr(proc, oop_str)));

  int exc;
  oop instance = meme_instance(proc, el, &exc);
  proc->stack_push(instance);
  return exc;
}

static int prim_qt_qwebelement_set_attribute(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_name = proc->get_arg(0);
  oop oop_val = proc->get_arg(1);

  QWebElement* qtobj = (QWebElement*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setAttribute(proc->mmobj()->mm_string_cstr(proc, oop_name), proc->mmobj()->mm_string_cstr(proc, oop_val));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwebelement_set_inner_xml(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_xml = proc->get_arg(0);

  QWebElement* qtobj = (QWebElement*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setInnerXml(proc->mmobj()->mm_string_cstr(proc, oop_xml));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwebelement_set_plain_text(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_text = proc->get_arg(0);

  QWebElement* qtobj = (QWebElement*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setPlainText(proc->mmobj()->mm_string_cstr(proc, oop_text));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwebelement_set_style_property(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_name = proc->get_arg(0);
  oop oop_val = proc->get_arg(1);

  QWebElement* qtobj = (QWebElement*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setStyleProperty(proc->mmobj()->mm_string_cstr(proc, oop_name), proc->mmobj()->mm_string_cstr(proc, oop_val));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwebelement_take_from_document(Process* proc) {
  oop data_self =  proc->dp();

  QWebElement* qtobj = (QWebElement*) get_qt_instance(proc->mmobj(), data_self);
  int exc;
  oop instance = meme_instance(proc, &(qtobj->takeFromDocument()), &exc);
  proc->stack_push(instance);
  return exc;
}

static int prim_qt_qwebelement_to_outer_xml(Process* proc) {
  oop data_self =  proc->dp();

  QWebElement* qtobj = (QWebElement*) get_qt_instance(proc->mmobj(), data_self);
  proc->stack_push(proc->mmobj()->mm_string_new(qtobj->toOuterXml().toLocal8Bit().data()));
  return 0;
}

/** QWebFrame **/

static int prim_qt_qwebframe_document_element(Process* proc) {
  oop data_self =  proc->dp();

  QWebFrame* qtobj = (QWebFrame*) get_qt_instance(proc->mmobj(), data_self);
  QWebElement* el = new QWebElement(qtobj->documentElement());
  int exc;
  proc->stack_push(meme_instance(proc, el, &exc));
  return exc;
}

static int prim_qt_qwebframe_scroll_to_anchor(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_anchor = proc->get_arg(0);

  QWebFrame* qtobj = (QWebFrame*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->scrollToAnchor(proc->mmobj()->mm_string_cstr(proc, oop_anchor));
  proc->stack_push(proc->rp());
  return 0;
}

/** QWebPage **/

class WebPluginFactory : public QWebPluginFactory {
private:
  oop _fun;
  Process* _proc;
  QString _name;
public:
  WebPluginFactory(oop fun, Process* proc, QString name) : QWebPluginFactory(), _fun(fun), _proc(proc), _name(name)  {};
  virtual QObject* create(const QString & mimeType, const QUrl &,
              const QStringList & argumentNames, const QStringList & argumentValues) const {
    if (mimeType == "x-pyqt/" + _name) {
      int exc;
      oop obj = _proc->call(_fun, create_args(argumentNames, argumentValues), &exc);
      if (!check_and_print_exception(_proc, exc, obj)) {
        return (QObject*) get_qt_instance(_proc->mmobj(), obj);
      }
    }
    return NULL;
  }
  oop create_args(const QStringList& names, const QStringList& vals) const { //dict(zip(names,vals))
    if (!(names.size() == vals.size())) {
      _proc->raise("InternalError", "WebPluginFactory got different sizes for name and vals");
    }
    oop dict = _proc->mmobj()->mm_dictionary_new();
    for (int i = 0; i < names.length(); i++) {
      oop key = _proc->vm()->new_symbol(names[i].toLocal8Bit().data());
      _log << "FACTORY args: " << names[i].toLocal8Bit().data() << endl;
      oop val = _proc->mmobj()->mm_string_new(vals[i].toLocal8Bit().data());
      _proc->mmobj()->mm_dictionary_set(_proc, dict, key, val);
    }
    oop lst = _proc->mmobj()->mm_list_new();
    _proc->mmobj()->mm_list_append(_proc, lst, dict);
    return lst;
  }

  virtual QList<Plugin> plugins() const {
    QWebPluginFactory::Plugin plugin;
    plugin.name = "PyQt Widget";
    plugin.description = "An example Web plugin written with PyQt.";
    QWebPluginFactory::MimeType mimeType;
    mimeType.name = "x-pyqt/widget";
    mimeType.description = "PyQt widget";
    // mimeType.fileExtensions = []
    QList<QWebPluginFactory::MimeType> lst;
    lst << mimeType;
    plugin.mimeTypes = lst;
    QList<QWebPluginFactory::Plugin> plugins;
    plugins << plugin;
    return plugins;
  }
};

static int prim_qt_extra_qwebpage_enable_plugins(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_name = proc->get_arg(0);
  oop oop_fun = proc->get_arg(1);

  WebPluginFactory* factory = new WebPluginFactory(oop_fun, proc, proc->mmobj()->mm_string_cstr(proc, oop_name));
  QWebPage* qtobj = (QWebPage*) get_qt_instance(proc->mmobj(), data_self);
  QWebSettings::globalSettings()->setAttribute(QWebSettings::PluginsEnabled, true);
  qtobj->setPluginFactory(factory);
  proc->stack_push(proc->rp());
  return 0;
}

/** QWebPage **/

static int prim_qt_qwebpage_main_frame(Process* proc) {
  oop data_self =  proc->dp();

  QWebPage* qtobj = (QWebPage*) get_qt_instance(proc->mmobj(), data_self);
  QWebFrame* frame = qtobj->mainFrame();
  int exc;
  proc->stack_push(meme_instance(proc, frame, &exc));
  return exc;
}

static int prim_qt_qwebpage_set_link_delegation_policy(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_policy = proc->get_arg(0);

  QWebPage* qtobj = (QWebPage*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setLinkDelegationPolicy((QWebPage::LinkDelegationPolicy) untag_small_int(oop_policy));
  proc->stack_push(proc->rp());
  return 0;
}


/** QWebView **/

static int prim_qt_qwebview_new(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_parent = proc->get_arg(0);

  QWidget* parent = (QWidget*) get_qt_instance(proc->mmobj(), oop_parent);

  QWebView* qtobj = new QWebView(parent);

  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwebview_page(Process* proc) {
  oop data_self =  proc->dp();

  QWebView* qtobj = (QWebView*) get_qt_instance(proc->mmobj(), data_self);
  QWebPage* page = qtobj->page();
  int exc;
  proc->stack_push(meme_instance(proc, page, &exc));
  return exc;
}

static int prim_qt_qwebview_set_html(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_html = proc->get_arg(0);

  QWebView* qtobj = (QWebView*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setHtml(proc->mmobj()->mm_string_cstr(proc, oop_html));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwebview_set_url(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_url = proc->get_arg(0);

  QWebView* qtobj = (QWebView*) get_qt_instance(proc->mmobj(), data_self);
  qtobj->setUrl(QUrl(proc->mmobj()->mm_string_cstr(proc, oop_url)));
  proc->stack_push(proc->rp());
  return 0;
}


/** QWidget **/

static int prim_qt_qwidget_new(Process* proc) {
  oop data_self =  proc->dp();
  QWidget* qtobj = new QWidget;
  set_meme_instance(qtobj, proc->rp());
  set_qt_instance(proc->mmobj(), data_self, qtobj);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwidget_actions(Process* proc) {
  oop data_self =  proc->dp();
  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);

  QList<QAction*> lst = w->actions();
  oop oop_lst = proc->mmobj()->mm_list_new();
  for (int i = 0; i < lst.size(); i++) {
    int exc;
    oop obj = meme_instance(proc, lst[i], &exc);
    if (exc != 0) {
      proc->stack_push(obj);
      return exc;
    }
    proc->mmobj()->mm_list_append(proc, oop_lst, obj);
  }
  proc->stack_push(oop_lst);
  return 0;
}

static int prim_qt_qwidget_add_action(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_action = proc->get_arg(0);

  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);
  QAction* ac = (QAction*)get_qt_instance(proc->mmobj(), oop_action);
  _log << "prim_qt_qwidget_add_action -- " << ac << endl;
  w->addAction(ac);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwidget_close(Process* proc) {
  oop data_self =  proc->dp();

  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);
  proc->stack_push(proc->mmobj()->mm_boolean_new(w->close()));
  return 0;
}

static int prim_qt_qwidget_connect(Process* proc) {
  oop data_self =  proc->dp();
  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);

  oop signal = proc->get_arg(0);
  oop fn = proc->get_arg(1);

  QScriptValueList args;
  args << engine->newQObject(w)
       << proc->mmobj()->mm_string_cstr(proc, signal)
       << engine->newVariant(QVariant::fromValue((void*)fn))
       << engine->newVariant(QVariant::fromValue(proc));

  QScriptValue globalObject = engine->globalObject();
  QScriptValue bridge = globalObject.property("qt_bind_connect");
  bridge.call(globalObject, args);
  if (engine->hasUncaughtException()) {
    _log << "QT: exception: " << qPrintable(engine->uncaughtException().toString()) << endl;
    proc->bail();
  }
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwidget_has_focus(Process* proc) {
  oop data_self =  proc->dp();

  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);
  proc->stack_push(proc->mmobj()->mm_boolean_new(w->hasFocus()));
  return 0;
}

static int prim_qt_qwidget_hide(Process* proc) {
  oop data_self =  proc->dp();

  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);
  w->hide();
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwidget_is_visible(Process* proc) {
  oop data_self =  proc->dp();

  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);
  proc->stack_push(proc->mmobj()->mm_boolean_new(w->isVisible()));
  return 0;
}

static int prim_qt_qwidget_resize(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_w = proc->get_arg(0);
  oop oop_h = proc->get_arg(1);

  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);
  w->resize(untag_small_int(oop_w), untag_small_int(oop_h));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwidget_set_focus(Process* proc) {
  oop data_self =  proc->dp();

  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);
  w->setFocus();
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwidget_set_maximum_height(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_h = proc->get_arg(0);

  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);
  w->setMaximumHeight(untag_small_int(oop_h));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwidget_set_maximum_width(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_w = proc->get_arg(0);

  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);
  w->setMaximumWidth(untag_small_int(oop_w));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwidget_set_minimum_size(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_w = proc->get_arg(0);
  oop oop_h = proc->get_arg(1);

  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);
  w->setMinimumSize(untag_small_int(oop_w), untag_small_int(oop_h));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwidget_set_minimum_width(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_w = proc->get_arg(0);

  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);
  w->setMinimumWidth(untag_small_int(oop_w));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwidget_set_stylesheet(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_str = proc->get_arg(0);

  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);
  w->setStyleSheet(proc->mmobj()->mm_string_cstr(proc, oop_str));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwidget_set_window_title(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_str = proc->get_arg(0);

  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);
  w->setWindowTitle(proc->mmobj()->mm_string_cstr(proc, oop_str));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_qwidget_show(Process* proc) {
  oop data_self =  proc->dp();
  QWidget* w = (QWidget*) get_qt_instance(proc->mmobj(), data_self);
  w->show();
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_status_bar_show_message(Process* proc) {
  oop data_self =  proc->dp();
  oop oop_str = proc->get_arg(0);
  QStatusBar* w = (QStatusBar*) get_qt_instance(proc->mmobj(), data_self);
  w->showMessage(proc->mmobj()->mm_string_cstr(proc, oop_str));
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_qt_maybe_construct_qapp(Process* proc) {
  if (!_app) {
    _app = new QApplication(proc->vm()->argc(), proc->vm()->argv());
    init_qt_stuff();
  }
  proc->stack_push(proc->rp());
  return 0;
}

// static int prim_qpushbutton_new(Process* proc) {
//   oop data_self =  proc->dp();
//   oop oop_parent = proc->get_arg(0);
//   oop oop_label = *((oop*) proc->fp() - 2);
//   QWidget* parent = (QWidget*) get_qt_instance(proc->mmobj(), oop_parent);
//   QPushButton* button = new QPushButton(proc->mmobj()->mm_string_cstr(proc, oop_label), parent);
//   set_qt_instance(proc->mmobj(), data_self, button);
//   proc->stack_push(proc->rp());
//   return 0;
// }

extern "C" void init_primitives(VM* vm) {
  vm->register_primitive("qt_qapplication_new", prim_qapplication_new);
  vm->register_primitive("qt_qapplication_exec", prim_qapplication_exec);
  vm->register_primitive("qt_qapplication_exit", prim_qapplication_exit);
  vm->register_primitive("qt_qapplication_focus_widget", prim_qapplication_focus_widget);

  vm->register_primitive("qt_qaction_new", prim_qaction_new);
  vm->register_primitive("qt_qaction_connect", prim_qaction_connect);
  vm->register_primitive("qt_qaction_set_enabled", prim_qaction_set_enabled);
  vm->register_primitive("qt_qaction_set_shortcut", prim_qaction_set_shortcut);
  vm->register_primitive("qt_qaction_set_shortcut_context", prim_qaction_set_shortcut_context);

  vm->register_primitive("qt_qcombobox_new", prim_qt_qcombobox_new);
  vm->register_primitive("qt_qcombobox_add_item", prim_qt_qcombobox_add_item);
  vm->register_primitive("qt_qcombobox_clear", prim_qt_qcombobox_clear);
  vm->register_primitive("qt_qcombobox_count", prim_qt_qcombobox_count);
  vm->register_primitive("qt_qcombobox_set_current_index", prim_qt_qcombobox_set_current_index);

  vm->register_primitive("qt_qeventloop_new", prim_qt_qeventloop_new);
  vm->register_primitive("qt_qeventloop_exec", prim_qt_qeventloop_exec);
  vm->register_primitive("qt_qeventloop_exit", prim_qt_qeventloop_exit);

  vm->register_primitive("qt_qhboxlayout_new", prim_qt_qhboxlayout_new);
  vm->register_primitive("qt_qhboxlayout_add_layout", prim_qt_qhboxlayout_add_layout);
  vm->register_primitive("qt_qhboxlayout_add_widget", prim_qt_qhboxlayout_add_widget);
  vm->register_primitive("qt_qhboxlayout_set_contents_margins", prim_qt_qhboxlayout_set_contents_margins);

  vm->register_primitive("qt_qheaderview_hide", prim_qt_qheaderview_hide);
  vm->register_primitive("qt_qheaderview_set_stretch_last_section", prim_qt_set_stretch_last_section);

  vm->register_primitive("qt_qlabel_new", prim_qt_qlabel_new);
  vm->register_primitive("qt_qlabel_set_text", prim_qt_qlabel_set_text);

  vm->register_primitive("qt_qlayout_add_widget", prim_qt_qlayout_add_widget);

  vm->register_primitive("qt_qlineedit_new", prim_qt_qlineedit_new);
  vm->register_primitive("qt_qlineedit_select_all", prim_qt_qlineedit_select_all);
  vm->register_primitive("qt_qlineedit_selected_text", prim_qt_qlineedit_selected_text);
  vm->register_primitive("qt_qlineedit_set_selection", prim_qt_qlineedit_set_selection);
  vm->register_primitive("qt_qlineedit_set_text", prim_qt_qlineedit_set_text);
  vm->register_primitive("qt_qlineedit_text", prim_qt_qlineedit_text);

  vm->register_primitive("qt_qlistwidget_new", prim_qt_qlistwidget_new);
  vm->register_primitive("qt_qlistwidget_current_item", prim_qt_qlistwidget_current_item);

  vm->register_primitive("qt_qlistwidgetitem_new", prim_qt_qlistwidgetitem_new);
  vm->register_primitive("qt_qlistwidgetitem_text", prim_qt_qlistwidgetitem_text);

  vm->register_primitive("qt_qmainwindow_new", prim_qt_qmainwindow_new);
  vm->register_primitive("qt_qmainwindow_menu_bar", prim_qt_qmainwindow_menu_bar);
  vm->register_primitive("qt_qmainwindow_set_central_widget", prim_qt_qmainwindow_set_central_widget);
  vm->register_primitive("qt_qmainwindow_status_bar", prim_qt_qmainwindow_status_bar);

  vm->register_primitive("qt_qmenubar_add_menu", prim_qt_qmenubar_add_menu);

  vm->register_primitive("qt_qplaintextedit_new", prim_qt_qplaintextedit_new);
  vm->register_primitive("qt_qplaintextedit_set_plain_text", prim_qt_qplaintextedit_set_plain_text);
  vm->register_primitive("qt_qplaintextedit_set_tabstop_width", prim_qt_qplaintextedit_set_tabstop_width);
  vm->register_primitive("qt_qplaintextedit_set_text_cursor", prim_qt_qplaintextedit_set_text_cursor);
  vm->register_primitive("qt_qplaintextedit_text_cursor", prim_qt_qplaintextedit_text_cursor);
  vm->register_primitive("qt_qplaintextedit_to_plain_text", prim_qt_qplaintextedit_to_plain_text);


  vm->register_primitive("qt_scintilla_editor_new", prim_qt_scintilla_editor_new);
  vm->register_primitive("qt_scintilla_append", prim_qt_scintilla_append);
  vm->register_primitive("qt_scintilla_copy", prim_qt_scintilla_copy);
  vm->register_primitive("qt_scintilla_cut", prim_qt_scintilla_cut);
  vm->register_primitive("qt_scintilla_get_cursor_position", prim_qt_scintilla_get_cursor_position);
  vm->register_primitive("qt_scintilla_get_selection", prim_qt_scintilla_get_selection);
  vm->register_primitive("qt_scintilla_insert_at", prim_qt_scintilla_insert_at);
  vm->register_primitive("qt_scintilla_lines", prim_qt_scintilla_lines);
  vm->register_primitive("qt_scintilla_paste", prim_qt_scintilla_paste);
  vm->register_primitive("qt_scintilla_paused_at_line", prim_qt_scintilla_paused_at_line);
  vm->register_primitive("qt_scintilla_redo", prim_qt_scintilla_redo);
  vm->register_primitive("qt_scintilla_saved", prim_qt_scintilla_saved);
  vm->register_primitive("qt_scintilla_selected_text", prim_qt_scintilla_selected_text);
  vm->register_primitive("qt_scintilla_set_selection", prim_qt_scintilla_set_selection);
  vm->register_primitive("qt_scintilla_set_text", prim_qt_scintilla_set_text);
  vm->register_primitive("qt_scintilla_text", prim_qt_scintilla_text);
  vm->register_primitive("qt_scintilla_undo", prim_qt_scintilla_undo);

  vm->register_primitive("qt_qshortcut_new", prim_qt_qshortcut_new);
  vm->register_primitive("qt_qshortcut_set_context", prim_qt_qshortcut_set_context);

  vm->register_primitive("qt_qtablewidget_new", prim_qt_qtablewidget_new);
  vm->register_primitive("qt_qtablewidget_clear", prim_qt_qtablewidget_clear);
  vm->register_primitive("qt_qtablewidget_horizontal_header", prim_qt_qtablewidget_horizontal_header);
  vm->register_primitive("qt_qtablewidget_set_column_count", prim_qt_qtablewidget_set_column_count);
  vm->register_primitive("qt_qtablewidget_set_horizontal_header_labels", prim_qt_qtablewidget_set_horizontal_header_labels);
  vm->register_primitive("qt_qtablewidget_set_item", prim_qt_qtablewidget_set_item);
  vm->register_primitive("qt_qtablewidget_set_row_count", prim_qt_qtablewidget_set_row_count);
  vm->register_primitive("qt_qtablewidget_set_selection_mode", prim_qt_qtablewidget_set_selection_mode);
  vm->register_primitive("qt_qtablewidget_set_sorting_enabled", prim_qt_qtablewidget_set_sorting_enabled);
  vm->register_primitive("qt_qtablewidget_vertical_header", prim_qt_qtablewidget_vertical_header);

  vm->register_primitive("qt_qtablewidgetitem_new", prim_qt_qtablewidgetitem_new);
  vm->register_primitive("qt_qtablewidgetitem_set_flags", prim_qt_qtablewidgetitem_set_flags);

  vm->register_primitive("qt_qtextcursor_drag_right", prim_qt_qtextcursor_drag_right);
  vm->register_primitive("qt_qtextcursor_insert_text", prim_qt_qtextcursor_insert_text);
  vm->register_primitive("qt_qtextcursor_selected_text", prim_qt_qtextcursor_selected_text);
  vm->register_primitive("qt_qtextcursor_selection_end", prim_qt_qtextcursor_selection_end);
  vm->register_primitive("qt_qtextcursor_set_position", prim_qt_qtextcursor_set_position);

  vm->register_primitive("qt_qurl_fragment", prim_qt_qurl_fragment);
  vm->register_primitive("qt_qurl_has_fragment", prim_qt_qurl_has_fragment);
  vm->register_primitive("qt_qurl_has_query_item", prim_qt_qurl_has_query_item);
  vm->register_primitive("qt_qurl_path", prim_qt_qurl_path);
  vm->register_primitive("qt_qurl_query_item_value", prim_qt_qurl_query_item_value);
  vm->register_primitive("qt_qurl_to_string", prim_qt_qurl_to_string);

  vm->register_primitive("qt_qvboxlayout_new", prim_qt_qvboxlayout_new);
  vm->register_primitive("qt_qvboxlayout_add_layout", prim_qt_qvboxlayout_add_layout);
  vm->register_primitive("qt_qvboxlayout_add_widget", prim_qt_qvboxlayout_add_widget);

  vm->register_primitive("qt_qwebelement_append_inside", prim_qt_qwebelement_append_inside);
  vm->register_primitive("qt_qwebelement_append_outside", prim_qt_qwebelement_append_outside);
  vm->register_primitive("qt_qwebelement_clone", prim_qt_qwebelement_clone);
  vm->register_primitive("qt_qwebelement_find_first", prim_qt_qwebelement_find_first);
  vm->register_primitive("qt_qwebelement_set_attribute", prim_qt_qwebelement_set_attribute);
  vm->register_primitive("qt_qwebelement_set_inner_xml", prim_qt_qwebelement_set_inner_xml);
  vm->register_primitive("qt_qwebelement_set_plain_text", prim_qt_qwebelement_set_plain_text);
  vm->register_primitive("qt_qwebelement_set_style_property", prim_qt_qwebelement_set_style_property);
  vm->register_primitive("qt_qwebelement_take_from_document", prim_qt_qwebelement_take_from_document);
  vm->register_primitive("qt_qwebelement_to_outer_xml", prim_qt_qwebelement_to_outer_xml);

  vm->register_primitive("qt_qwebframe_document_element", prim_qt_qwebframe_document_element);
  vm->register_primitive("qt_qwebframe_scroll_to_anchor", prim_qt_qwebframe_scroll_to_anchor);

  vm->register_primitive("qt_extra_qwebpage_enable_plugins", prim_qt_extra_qwebpage_enable_plugins);

  vm->register_primitive("qt_qwebpage_main_frame", prim_qt_qwebpage_main_frame);
  vm->register_primitive("qt_qwebpage_set_link_delegation_policy", prim_qt_qwebpage_set_link_delegation_policy);

  vm->register_primitive("qt_qwebview_new", prim_qt_qwebview_new);
  vm->register_primitive("qt_qwebview_page", prim_qt_qwebview_page);
  vm->register_primitive("qt_qwebview_set_html", prim_qt_qwebview_set_html);
  vm->register_primitive("qt_qwebview_set_url", prim_qt_qwebview_set_url);

  vm->register_primitive("qt_qwidget_new", prim_qt_qwidget_new);
  vm->register_primitive("qt_qwidget_actions", prim_qt_qwidget_actions);
  vm->register_primitive("qt_qwidget_add_action", prim_qt_qwidget_add_action);
  vm->register_primitive("qt_qwidget_close", prim_qt_qwidget_close);
  vm->register_primitive("qt_qwidget_connect", prim_qt_qwidget_connect);
  vm->register_primitive("qt_qwidget_has_focus", prim_qt_qwidget_has_focus);
  vm->register_primitive("qt_qwidget_hide", prim_qt_qwidget_hide);
  vm->register_primitive("qt_qwidget_is_visible", prim_qt_qwidget_is_visible);
  vm->register_primitive("qt_qwidget_resize", prim_qt_qwidget_resize);
  vm->register_primitive("qt_qwidget_set_focus", prim_qt_qwidget_set_focus);
  vm->register_primitive("qt_qwidget_set_maximum_height", prim_qt_qwidget_set_maximum_height);
  vm->register_primitive("qt_qwidget_set_maximum_width", prim_qt_qwidget_set_maximum_width);
  vm->register_primitive("qt_qwidget_set_minimum_size", prim_qt_qwidget_set_minimum_size);
  vm->register_primitive("qt_qwidget_set_minimum_width", prim_qt_qwidget_set_minimum_width);
  vm->register_primitive("qt_qwidget_set_stylesheet", prim_qt_qwidget_set_stylesheet);
  vm->register_primitive("qt_qwidget_set_window_title", prim_qt_qwidget_set_window_title);
  vm->register_primitive("qt_qwidget_show", prim_qt_qwidget_show);

  vm->register_primitive("qt_status_bar_show_message", prim_qt_status_bar_show_message);

  vm->register_primitive("qt_maybe_construct_qapp", prim_qt_maybe_construct_qapp);

  vm->register_primitive("qt_server_create_server", prim_qt_server_create_server);
  vm->register_primitive("qt_server_listen", prim_qt_server_listen);
  vm->register_primitive("qt_socket_write_line", prim_qt_socket_write_line);
  vm->register_primitive("qt_socket_read_string", prim_qt_socket_read_string);
  vm->register_primitive("qt_server_next_pending_connection", prim_qt_server_next_pending_connection);


  qRegisterMetaType<QListWidgetItem*>("QListWidgetItem");//Do this if you need signal/slots

  // vm->register_primitive("qt_qpushbutton_new", prim_qpushbutton_new);
}
