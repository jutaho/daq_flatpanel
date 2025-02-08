#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QThread>
#include <QImage>
#include <QPixmap>
#include <QRandomGenerator>
#include <QFrame>
#include <QDebug>

// ------------------------------------------------------------------
// AcquisitionWorker
// This worker simulates an acquisition process by generating a new
// grayscale image (with random pixel values) every second.
// In your real application, replace this simulated code with your actual
// image-acquisition API calls.
// ------------------------------------------------------------------
class AcquisitionWorker : public QObject {
    Q_OBJECT
public:
    explicit AcquisitionWorker(QObject *parent = nullptr)
        : QObject(parent), m_abort(false) {}

public slots:
    void startAcquisition(const QString &fileName, int frameCount) {
        m_abort = false;
        emit logMessage("Initializing detector...");
        QThread::sleep(1);  // simulate initialization delay
        emit logMessage("Detector initialized.");
        emit logMessage(QString("Starting acquisition for %1 frame(s)...").arg(frameCount));

        const int width = 320, height = 240;
        for (int i = 1; i <= frameCount; ++i) {
            if (m_abort) {
                emit logMessage("Acquisition aborted by user.");
                break;
            }
            // Create a simulated frame: a grayscale image with random pixel values.
            QImage frame(width, height, QImage::Format_Grayscale8);
            for (int y = 0; y < height; ++y) {
                uchar *line = frame.scanLine(y);
                for (int x = 0; x < width; ++x) {
                    line[x] = static_cast<uchar>(QRandomGenerator::global()->bounded(256));
                }
            }
            emit frameReady(frame);
            emit logMessage(QString("Acquired frame %1 of %2.").arg(i).arg(frameCount));
            emit frameCaptured(i, frameCount);
            QThread::sleep(1);  // simulate delay between frames
        }
        if (!m_abort) {
            emit logMessage("Acquisition complete. Saving frames...");
            QThread::sleep(1);
            emit logMessage(QString("Frames successfully saved to %1.his").arg(fileName));
        }
        emit acquisitionFinished();
    }

    void abortAcquisition() {
        m_abort = true;
    }

signals:
    void logMessage(const QString &msg);
    void frameCaptured(int currentFrame, int totalFrames);
    void acquisitionFinished();
    void frameReady(const QImage &frame);

private:
    bool m_abort;
};

// ------------------------------------------------------------------
// MainWindow
// This MainWindow provides a simple GUI with input fields for a file name
// and frame count, Start/Stop buttons, a progress bar, a live view area,
// and a log area.
// ------------------------------------------------------------------
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
    {
        setupUI();

        // Create the acquisition worker and move it to its own thread.
        worker = new AcquisitionWorker();
        workerThread = new QThread(this);
        worker->moveToThread(workerThread);
        connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
        connect(worker, &AcquisitionWorker::logMessage, this, &MainWindow::appendLog);
        connect(worker, &AcquisitionWorker::frameCaptured, this, &MainWindow::updateProgress);
        connect(worker, &AcquisitionWorker::acquisitionFinished, this, &MainWindow::onAcquisitionFinished);
        connect(worker, &AcquisitionWorker::frameReady, this, &MainWindow::updateLiveView);
        workerThread->start();
    }

    ~MainWindow() override {
        workerThread->quit();
        workerThread->wait();
    }

private slots:
    void onStartClicked() {
        startButton->setEnabled(false);
        stopButton->setEnabled(true);
        logTextEdit->clear();
        progressBar->setValue(0);
        QString fileName = fileNameEdit->text().trimmed();
        if (fileName.isEmpty())
            fileName = "capture";
        int frameCount = frameSpinBox->value();
        appendLog("Starting acquisition...");
        QMetaObject::invokeMethod(worker, "startAcquisition",
                                  Q_ARG(QString, fileName),
                                  Q_ARG(int, frameCount));
    }

    void onStopClicked() {
        appendLog("Stopping acquisition...");
        QMetaObject::invokeMethod(worker, "abortAcquisition");
        stopButton->setEnabled(false);
    }

    void appendLog(const QString &msg) {
        logTextEdit->append(msg);
    }

    void updateProgress(int currentFrame, int totalFrames) {
        int progress = (currentFrame * 100) / totalFrames;
        progressBar->setValue(progress);
    }

    void onAcquisitionFinished() {
        appendLog("Acquisition finished.");
        startButton->setEnabled(true);
        stopButton->setEnabled(false);
    }

    void updateLiveView(const QImage &frame) {
        liveViewLabel->setPixmap(QPixmap::fromImage(frame).scaled(
            liveViewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

private:
    void setupUI() {
        QWidget *central = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(central);

        // File name input
        QHBoxLayout *fileLayout = new QHBoxLayout();
        QLabel *fileLabel = new QLabel("File Name:");
        fileNameEdit = new QLineEdit();
        fileLayout->addWidget(fileLabel);
        fileLayout->addWidget(fileNameEdit);
        mainLayout->addLayout(fileLayout);

        // Number of frames input
        QHBoxLayout *frameLayout = new QHBoxLayout();
        QLabel *frameLabel = new QLabel("Number of Frames:");
        frameSpinBox = new QSpinBox();
        frameSpinBox->setRange(1, 600);
        frameSpinBox->setValue(5);
        frameLayout->addWidget(frameLabel);
        frameLayout->addWidget(frameSpinBox);
        mainLayout->addLayout(frameLayout);

        // Start and Stop buttons
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        startButton = new QPushButton("Start Acquisition");
        stopButton = new QPushButton("Stop Acquisition");
        stopButton->setEnabled(false);
        buttonLayout->addWidget(startButton);
        buttonLayout->addWidget(stopButton);
        mainLayout->addLayout(buttonLayout);

        // Progress bar
        progressBar = new QProgressBar();
        progressBar->setRange(0, 100);
        mainLayout->addWidget(progressBar);

        // Live view area
        QLabel *liveViewTitle = new QLabel("Live View:");
        mainLayout->addWidget(liveViewTitle);
        liveViewLabel = new QLabel();
        liveViewLabel->setFixedSize(320, 240);
        liveViewLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);
        liveViewLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(liveViewLabel);

        // Log output
        QLabel *logTitle = new QLabel("Log:");
        mainLayout->addWidget(logTitle);
        logTextEdit = new QTextEdit();
        logTextEdit->setReadOnly(true);
        mainLayout->addWidget(logTextEdit);

        setCentralWidget(central);
        setWindowTitle("Acquisition Live View GUI");

        // Connect button signals
        connect(startButton, &QPushButton::clicked, this, &MainWindow::onStartClicked);
        connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    }

    // UI elements
    QLineEdit    *fileNameEdit;
    QSpinBox     *frameSpinBox;
    QPushButton  *startButton;
    QPushButton  *stopButton;
    QTextEdit    *logTextEdit;
    QProgressBar *progressBar;
    QLabel       *liveViewLabel;

    // Worker and thread
    AcquisitionWorker *worker;
    QThread           *workerThread;
};

#include "main.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.resize(600, 600);
    window.show();
    return app.exec();
}
