#include <qgenericplugin_qpa.h>

#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QOpenGLBuffer>
#include <QQuickView>

#include <QtConcurrentRun>
#include <QtDebug>

class Recorder : public QObject
{
    Q_OBJECT

public:
    Recorder(const QString &key, const QString &spec);
    ~Recorder();

protected:
    bool eventFilter(QObject *object, QEvent *event);

private slots:
    void grabFrame();

private:
    QString m_prefix;
    QQuickView *m_view;
    int m_frame;

    QSize m_size;

    QOpenGLBuffer *m_pbos[2];
};

Recorder::Recorder(const QString &, const QString &)
    : m_prefix("/tmp/" + qApp->applicationName() + "-" + QString::number(quint64(getpid())) + "/")
    , m_view(0)
    , m_frame(0)
{
    qApp->installEventFilter(this);
    qDebug() << "Recorder initialized:" << QDir().mkpath(m_prefix);

    m_pbos[0] = m_pbos[1] = 0;
}

Recorder::~Recorder()
{
    qDebug() << "Recorded" << m_frame << "frames to" << m_prefix;
}

bool Recorder::eventFilter(QObject *, QEvent *event)
{
    if (m_view)
        return false;

    if (event->type() == QEvent::ApplicationActivate) {
        QWindow *window = qApp->focusWindow();

        QQuickView *view = qobject_cast<QQuickView *>(window);

        if (!view)
            return false;

        connect(view, SIGNAL(afterRendering()), this, SLOT(grabFrame()), Qt::DirectConnection);

        m_view = view;
    }

    return false;
}

void storeFrame(QImage *image, int frame, const QString &prefix)
{
    QString num = QString::number(frame);
    num = QString(qMax(0, 8 - num.size()), QChar('0')) + num;

    image->save(prefix + num + ".ppm");

    delete image;
}

void Recorder::grabFrame()
{
    QImage *image = new QImage(m_size, QImage::Format_ARGB32_Premultiplied);

    if (m_view->size() != m_size) {
        for (int i = 0; i < 2; ++i) {
            delete m_pbos[i];
            m_pbos[i] = new QOpenGLBuffer(QOpenGLBuffer::PixelPackBuffer);
            m_pbos[i]->create();
            m_pbos[i]->bind();
            m_pbos[i]->setUsagePattern(QOpenGLBuffer::StreamRead);
            m_pbos[i]->allocate(m_view->width() * m_view->height() * 4);
        }
        m_size = m_view->size();
    }

    // read pixels into first pbo
    m_pbos[0]->bind();
    glReadPixels(0, 0, m_size.width(), m_size.height(), GL_BGRA, GL_UNSIGNED_BYTE, 0);

    // read second pbo
    if (image->size() == m_size) {
        m_pbos[1]->bind();
        const uchar *data = static_cast<uchar *>(m_pbos[1]->map(QOpenGLBuffer::ReadOnly)) + m_pbos[1]->size();
        int stride = m_size.width() * 4;
        for (int y = 0; y < m_size.height(); ++y)
            memcpy(image->scanLine(y), data -= stride, stride);
        QtConcurrent::run(storeFrame, image, m_frame - 1, m_prefix);
        m_pbos[1]->unmap();
        m_pbos[1]->release();
    } else {
        m_pbos[0]->release();
        delete image;
    }

    qSwap(m_pbos[0], m_pbos[1]);

    ++m_frame;
}

class RecorderPlugin : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QGenericPluginFactoryInterface" FILE "recorder.json")

public:
    RecorderPlugin();

    QStringList keys() const;
    QObject* create(const QString &key, const QString &specification);
};

RecorderPlugin::RecorderPlugin()
    : QGenericPlugin()
{
}

QStringList RecorderPlugin::keys() const
{
    return (QStringList()
            << QLatin1String("recorder"));
}

QObject* RecorderPlugin::create(const QString &key,
                                const QString &specification)
{
    if (!key.compare(QLatin1String("recorder"), Qt::CaseInsensitive))
        return new Recorder(key, specification);
    return 0;
}

#include "main.moc"
