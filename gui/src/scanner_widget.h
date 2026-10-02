#pragma once

#include <QPointF>
#include <QString>
#include <QTimer>
#include <QVector>
#include <QWidget>

namespace face_unlock_gui {

class ScannerWidget final : public QWidget {
 public:
  enum class State {
    Idle,
    Scanning,
    Validating,
    Complete,
    Error,
  };

  explicit ScannerWidget(QWidget* parent = nullptr);

  void setScannerState(State state);
  void setProgress(int progress);
  void setGuidance(const QString& guidance);
  void setAnimationPhaseForTest(double phase);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

 protected:
  void paintEvent(QPaintEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;

 private:
  struct FacePoint {
    double x = 0.0;
    double y = 0.0;
    double depth = 0.0;
  };

  void buildPointCloud();
  void updateTimer();
  bool shouldAnimate() const;
  QString stateLabel() const;

  QVector<FacePoint> points_;
  QTimer timer_;
  State state_ = State::Idle;
  QString guidance_ = QStringLiteral("READY");
  int progress_ = 0;
  double phase_ = 0.0;
  bool fixedPhase_ = false;
};

bool renderScannerPreview(const QString& path);
int scannerWidgetSelfTest();
int scannerWidgetBenchmark(int frames);

}  // namespace face_unlock_gui
