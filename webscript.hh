#ifndef WEBSCRIPT_HH
#define WEBSCRIPT_HH

#include <QJsonDocument>
#include <QJsonObject>
// #include <QWebEngineView>
// #include <QWebChannel>
#include <QJsonArray>

#include <QFileInfo>

#include <iostream>

#include "execution.hh"

#include <QWebFrame>
#include <QWebPage>
#include <QWebView>
#include <QWebInspector>

class WebPage : public QWebPage {
    Q_OBJECT
public:
    explicit WebPage(QObject* parent = 0)
        : QWebPage(parent) {}
protected:
    void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID) {
        //        qDebug() << sourceID << lineNumber << message;
        std::cerr << message.toStdString() << "\n";
    }
};

// class WebMessenger : public QObject {
//     Q_OBJECT
// public:
//     WebMessenger(Execution* _execution, std::string _dataString)
//         : execution(_execution),
//           // treeCanvas(_treeCanvas),
//           dataString(_dataString)
//     {}
    
//     Q_INVOKABLE void message(int gid) {
//         announceSelectNode(gid);
//     }

//     Q_INVOKABLE void messageMany(QList<QVariant> gidsV) {
//         announceSelectManyNodes(gidsV);
//     }

//     Q_INVOKABLE QString getCSV(void) {
//         return QString::fromStdString(dataString);
//     }

// Q_SIGNALS:
//     void announceSelectNode(int);
//     void announceSelectManyNodes(QList<QVariant>);

// private:
//     Execution* execution;
//     std::string dataString;
// };

// class WebscriptView : public QWebEngineView {
class WebscriptView : public QWebView {
    Q_OBJECT
public:
    WebscriptView(QWidget* parent, QString htmlPath, Execution* _execution, std::string _dataString)
        //        : QWebEngineView(parent),
        : QWebView(parent),
          execution(_execution),
          dataString(_dataString)
          // messenger(_execution, _dataString)
    {
        connect(this, &WebscriptView::loadFinished, this, &WebscriptView::onload);

        WebPage* p = new WebPage(this);
        setPage(p);
        connect(page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(jsCleared()));

        p->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

        // Uncomment this to get JavaScript/HTML debugging
        // QWebInspector* inspector = new QWebInspector;
        // inspector->setPage(p);
        // inspector->show();

        // connect(&messenger, &WebMessenger::announceSelectNode, this, &WebscriptView::announceSelectNode);
        // connect(&messenger, &WebMessenger::announceSelectManyNodes,
        //         this, &WebscriptView::announceSelectManyNodes);

        // We want the web engine view to expand to fill the dialog
        // window it inhabits.  When the dialog window is resized, so
        // is the web engine view.
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // In "development mode", we want to load the HTML/JavaScript
        // directly from the file system, not from an embedded
        // resource.  This way we can modify it and reload while the
        // profiler is running.
#ifdef CP_PROFILER_DEVELOPMENT
        QFileInfo fileInfo("../" + htmlPath);
        QUrl url = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
#else
        QUrl url("qrc:///" + htmlPath);
#endif

        load(url);

        // channel = new QWebChannel;
        // channel->registerObject("profiler", &messenger);
    }

    void select(int nodeid) {
        QString js;
        QTextStream(&js) << "select(" << nodeid << ")";
        // page()->runJavaScript(js);
        page()->mainFrame()->evaluateJavaScript(js);
    }
    void selectMany(QList<QVariant> nodeids) {
        QString js;
        QTextStream(&js) << "selectMany(" << QJsonDocument(QJsonArray::fromVariantList(nodeids)).toJson() << ")";
        // page()->runJavaScript(js);
        page()->mainFrame()->evaluateJavaScript(js);
    }
    Q_INVOKABLE QString getCSV(void) {
        return QString::fromStdString(dataString);
    }
    Q_INVOKABLE QString getVariableListString(void) {
        return QString::fromStdString(execution->getVariableListString());
    }
    Q_INVOKABLE void message(int gid) {
        announceSelectNode(gid);
    }
    Q_INVOKABLE void messageMany(QList<QVariant> gidsV) {
        announceSelectManyNodes(gidsV);
    }
Q_SIGNALS:
    void announceSelectNode(int);
    void announceSelectManyNodes(QList<QVariant>);
private Q_SLOTS:
    void jsCleared(void) {
        page()->mainFrame()->addToJavaScriptWindowObject("profiler", this);
    }
private:
    Execution* execution;
    std::string dataString;
    // QWebChannel* channel;
    // WebMessenger messenger;
    
    void onload(void) {
        page()->mainFrame()->evaluateJavaScript("window.onprofilerload()");
        // Load qwebchannel javascript
        // QFile qwebchanneljs(":/qtwebchannel/qwebchannel.js");
        // if (!qwebchanneljs.open(QIODevice::ReadOnly | QIODevice::Text)) {
        //     qDebug() << "can't open qrc:///qtwebchannel/qwebchannel.js";
        //     return;
        // }
        // QTextStream qwebchanneljs_in(&qwebchanneljs);
        // QString qwebchanneljs_text = qwebchanneljs_in.readAll();
        // QWebEnginePage* p = page();
        // p->runJavaScript(qwebchanneljs_text);
        // p->setWebChannel(channel);

        // QString setup_object("new QWebChannel(qt.webChannelTransport, function (channel) {"
        //                      "window.profiler = channel.objects.profiler;"
        //                      "});"
        //                      );
        // p->runJavaScript(setup_object, [p](const QVariant&) { p->runJavaScript("window.onprofilerload()"); });

        // // We send the CSV file as a big JSON string.  Since a string
        // // alone is not a valid JSON document, we wrap it in an object
        // // and send that.
        // QJsonValue val = QJsonValue(QString::fromStdString(dataString));
        // QJsonObject obj({{"s", val}});
        // QJsonDocument doc(obj);

        // QString msg;
        // msg.append("initialise(");
        // msg.append(doc.toJson());
        // msg.append(")");

        // p->runJavaScript(msg);



    }

};

#endif
